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

#include <stdio.h>

#include "redisproto.h"

#define READ_AGAIN -2
#define READ_ERROR -1

static int readTextLine(char* s, int len, int stringlen)
{
    if (len <= stringlen) {
        return READ_AGAIN;
    }

    if (len - stringlen == 1) {
        if (s[stringlen] == '\r') {
            return READ_AGAIN;
        } else {
            return READ_ERROR;
        }
    }

    if (len - stringlen >= 2) {
        if (s[stringlen] == '\r' && s[stringlen+1] == '\n') {
            return stringlen + 2;
        } else {
            return READ_ERROR;
        }
    }
    return READ_ERROR;
}

static int readTextEndByCRLF(char* s, int len, int* stringlen)
{
    int pos = 0;
    *stringlen = 0;
    while (pos < len && s[pos] != '\r') {
        ++(*stringlen);
        ++pos;
    }
    if (*stringlen == 0) {
        return READ_ERROR;
    }

    if (s[pos] != '\r') {
        return READ_AGAIN;
    } else {
        if (pos < len) {
            ++pos;
            return (s[pos] == '\n') ? pos + 1 : READ_ERROR;
        }
    }
    return READ_AGAIN;
}


static int readNumberLine(char* s, int len, int* num)
{
    int pos = 0;
    *num = 0;
    int signed_num = 1;
    while (pos < len && s[pos] != '\r') {
        if (s[pos] == '-') {
            signed_num = -1;
        } else if (s[pos] == '+') {
            signed_num = 1;
        } else if (s[pos] >= '0' && s[pos] <= '9') {
            *num = (*num * 10) + (s[pos] - '0');
        } else {
            return READ_ERROR;
        }
        ++pos;
    }
    *num *= signed_num;

    if (s[pos] != '\r') {
        return READ_AGAIN;
    } else {
        if (pos < len) {
            ++pos;
            return (s[pos] == '\n') ? pos + 1 : READ_ERROR;
        }
    }
    return READ_AGAIN;
}

static int readBulk(char* s, int len, Token* tok)
{
    int pos = 0;
    if (s[pos++] != '$') {
        return READ_ERROR;
    }

    if (pos < len) {
        char* str = NULL;
        int stringlen = 0;
        int ret = readNumberLine(s + pos, len - pos, &stringlen);
        if (ret < 0) {
            return ret;
        }

        pos += ret;
        if (pos == len) {
            if (stringlen <= 0) {
                return pos;
            } else {
                return READ_AGAIN;
            }
        }

        str = s + pos;
        ret = readTextLine(s + pos, len - pos, stringlen);
        if (ret < 0) {
            return ret;
        }
        pos += ret;
        tok->s = str;
        tok->len = stringlen;
        return pos;
    } else {
        return READ_AGAIN;
    }
}

static int readMultiBulk(char* s, int len, Token* toks, int* cnt)
{
    int pos = 0;
    if (s[pos++] != '*') {
        return READ_ERROR;
    }
    int lines = 0;
    int argc = 0;
    int ret = 0;
    if (pos < len) {
        ret = readNumberLine(s + pos, len - pos, &argc);
        if (ret < 0) {
            return ret;
        }
        pos += ret;
        while (pos < len && (lines != argc)) {
            Token* tok = toks + lines;
            ret = readBulk(s + pos, len - pos, tok);
            if (ret < 0) {
                return ret;
            }
            ++lines;
            pos += ret;
        }
        if (lines != argc) {
            return READ_AGAIN;
        }
        *cnt = lines;
        return pos;
    } else {
        return READ_AGAIN;
    }
}

static int readStatus(char* s, int len, Token* tok)
{
    int pos = 0;
    if (s[pos++] != '+') {
        return READ_ERROR;
    }

    if (pos < len) {
        char* str = s + pos;
        int stringlen = 0;
        int ret = readTextEndByCRLF(s + pos, len - pos, &stringlen);
        if (ret < 0) {
            return ret;
        }
        tok->s = str;
        tok->len = stringlen;
        pos += ret;
        return pos;
    } else {
        return READ_AGAIN;
    }
}

static int readError(char* s, int len, Token* tok)
{
    int pos = 0;
    if (s[pos++] != '-') {
        return READ_ERROR;
    }

    if (pos < len) {
        int stringlen = 0;
        char* str = s + pos;
        int ret = readTextEndByCRLF(s + pos, len - pos, &stringlen);
        if (ret < 0) {
            return ret;
        }
        tok->s = str;
        tok->len = stringlen;
        pos += ret;
        return pos;
    } else {
        return READ_AGAIN;
    }
}

static int readInteger(char* s, int len, int* num)
{
    int pos = 0;
    if (s[pos++] != ':') {
        return READ_ERROR;
    }
    if (pos < len) {
        int ret = readNumberLine(s + pos, len - pos, num);
        if (ret < 0) {
            return ret;
        }
        pos += ret;
        return pos;
    } else {
        return READ_AGAIN;
    }
}



RedisProto::RedisProto(void)
{
}

RedisProto::~RedisProto(void)
{
}

void RedisProto::outputProtoString(const char* s, int len)
{
    char buff[10240];
    int offs = 0;
    for (int i = 0; i < len; ++i) {
        switch (s[i]) {
        case '\r':
            buff[offs++]='\\';
            buff[offs++] = 'r';
            break;
        case '\n':
            buff[offs++]='\\';
            buff[offs++] = 'n';
            break;
        default:
            buff[offs++] = s[i];
            break;
        }
        buff[offs] = '\0';
    }
    printf("%s\n", buff);
}

RedisProto::ParseState RedisProto::parse(char *s, int len, RedisProtoParseResult *result)
{
    int ret = 0;
    switch (s[0]) {
    case '+':
        result->type = RedisProtoParseResult::Status;
        ret = readStatus(s, len, &(result->tokens[0]));
        break;
    case '-':
        result->type = RedisProtoParseResult::Error;
        ret = readError(s, len, &(result->tokens[0]));
        break;
    case ':':
        result->type = RedisProtoParseResult::Integer;
        ret = readInteger(s, len, &result->integer);
        break;
    case '$':
        result->type = RedisProtoParseResult::Bulk;
        ret = readBulk(s, len, &(result->tokens[0]));
        break;
    case '*':
        result->type = RedisProtoParseResult::MultiBulk;
        ret = readMultiBulk(s, len, result->tokens, &result->tokenCount);
        break;
    default:
        result->type = RedisProtoParseResult::Unknown;
        ret = READ_ERROR;
        break;
    }

    switch (ret) {
    case READ_AGAIN:
        return ProtoIncomplete;
    case READ_ERROR:
        return ProtoError;
    default:
        result->protoBuff = s;
        result->protoBuffLen = ret;
        return ProtoOK;
    }
}


