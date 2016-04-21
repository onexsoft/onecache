/*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*/

#include <time.h>
#include <stdarg.h>
#include <memory.h>

#include "util/string.h"
#include "logger.h"

static Logger _stdoutput;
Logger* _defaultLogger = &_stdoutput;
bool GlobalLogOption::__debug = false;

static const char* msg_type_text[] =
{
    "Message",
    "Warning",
    "Error",
    "Debug"
};

Logger::Logger(void)
{
}

Logger::~Logger(void)
{
}

void Logger::output(const char *msg)
{
    printf("%s\n", msg);
}

void Logger::log(Logger::MsgType type, const char *format, ...)
{
    if (!format || !_defaultLogger) {
        return;
    }

    char buffer[10240];
    time_t t = time(NULL);
    tm* lt = localtime(&t);

    int n = sprintf(buffer, "[%d-%02d-%02d %02d:%02d:%02d] %s: ",
                    lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday,
                    lt->tm_hour, lt->tm_min, lt->tm_sec, msg_type_text[type]);

    va_list marker;
    va_start(marker, format);
    vsprintf(buffer + n, format, marker);
    va_end(marker);

    _defaultLogger->output(buffer);
}

void Logger::log(const char *file, int line, Logger::MsgType type, const char *format, ...)
{
    if (!format || !_defaultLogger) {
        return;
    }

    char buffer[10240];
    time_t t = time(NULL);
    tm* lt = localtime(&t);

    int n = sprintf(buffer, "[%d-%02d-%02d %02d:%02d:%02d] %s: %s:%d ",
                    lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday,
                    lt->tm_hour, lt->tm_min, lt->tm_sec, msg_type_text[type], file, line);

    va_list marker;
    va_start(marker, format);
    vsprintf(buffer + n, format, marker);
    va_end(marker);

    _defaultLogger->output(buffer);
}

Logger *Logger::defaultLogger(void)
{
    return _defaultLogger;
}

void Logger::setDefaultLogger(Logger *logger)
{
    _defaultLogger = logger;
}




FileLogger::FileLogger(void)
{
    m_fp = NULL;
    memset(m_fileName, 0, sizeof(m_fileName));
}

FileLogger::FileLogger(const char *fileName)
{
    setFileName(fileName);
}

FileLogger::~FileLogger(void)
{
    if (m_fp) {
        fclose(m_fp);
    }
}

bool FileLogger::setFileName(const char *fileName)
{
    FILE* fp = fopen(fileName, "a+");
    if (!fp) {
        return false;
    }

    strcpy(m_fileName, fileName);
    if (m_fp) {
        fclose(m_fp);
    }
    m_fp = fp;
    return true;
}

void FileLogger::output(const char *msg)
{
    /*
    time_t t = time(NULL);
    tm* lt = localtime(&t);

    fprintf(m_fp, "[%d-%02d-%02d %02d:%02d:%02d] %s: %s\n",
            lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday,
            lt->tm_hour, lt->tm_min, lt->tm_sec, msg_type_text[type], msg);
    fflush(m_fp);
    */
    fprintf(m_fp, "%s\n", msg);
    fflush(m_fp);
}
