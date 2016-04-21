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

#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

class Logger
{
public:
    enum MsgType {
        Message = 0,
        Warning = 1,
        Error = 2,
        Debug = 3
    };

    Logger(void);
    virtual ~Logger(void);

    virtual void output(const char* msg);

    static void log(MsgType type, const char* format, ...);
    static void log(const char* file, int line, MsgType type, const char* format, ...);

    static Logger* defaultLogger(void);
    static void setDefaultLogger(Logger* logger);
};


class FileLogger : public Logger
{
public:
    FileLogger(void);
    FileLogger(const char* fileName);
    ~FileLogger(void);

    bool setFileName(const char* fileName);
    const char* fileName(void) const { return m_fileName; }

    virtual void output(const char *msg);

private:
    FILE* m_fp;
    char m_fileName[512];
};

class GlobalLogOption
{
public:
    static bool __debug;
};

#define LOG(x, ...) do { \
    if (x == Logger::Debug) { \
        if (GlobalLogOption::__debug) Logger::log(__FILE__, __LINE__, x, __VA_ARGS__); \
    } else Logger::log(x, __VA_ARGS__); \
    } while(0);

#endif
