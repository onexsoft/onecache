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
        Error = 2
    };

    Logger(void);
    virtual ~Logger(void);

    //Output handler
    virtual void output(MsgType type, const char* msg);

    //Output function
    static void log(MsgType type, const char* format, ...);

    //Default logger
    static Logger* defaultLogger(void);

    //Set Default logger
    static void setDefaultLogger(Logger* logger);
};



class FileLogger : public Logger
{
public:
    FileLogger(void);
    FileLogger(const char* fileName);
    ~FileLogger(void);

    bool setFileName(const char* fileName);
    const char* fileName(void) const
    { return m_fileName; }

    virtual void output(MsgType type, const char *msg);

private:
    FILE* m_fp;
    char m_fileName[512];
};



#endif
