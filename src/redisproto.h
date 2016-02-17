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

#ifndef REDISPROTO_H
#define REDISPROTO_H

struct Token {
    char* s;
    int len;
};

class RedisProtoParseResult
{
public:
    enum { MaxToken = 1024 };
    enum Type {
        Unknown = 0,    //?????
        Status,         //"+"
        Error,          //"-"
        Integer,        //":"
        Bulk,           //"$"
        MultiBulk       //"*"
    };
    RedisProtoParseResult(void) { reset(); }
    ~RedisProtoParseResult(void) {}

    void reset(void) {
        protoBuff = 0;
        protoBuffLen = 0;
        type = Unknown;
        integer = 0;
        tokenCount = 0;
    }

    char* protoBuff;
    int protoBuffLen;
    int type;
    int integer;
    Token tokens[MaxToken];
    int tokenCount;
};

class RedisProto
{
public:
    enum ParseState {
        ProtoOK = 0,
        ProtoError = -1,
        ProtoIncomplete = -2
    };

    RedisProto(void);
    ~RedisProto(void);

    static void outputProtoString(const char* s, int len);
    static ParseState parse(char* s, int len, RedisProtoParseResult* result);
};

#endif
