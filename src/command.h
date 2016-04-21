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

#ifndef COMMAND_H
#define COMMAND_H

#include <list>

#include "util/hash.h"

class ClientPacket;
class RedisCommand
{
public:
    enum Type {
        AUTH,
        APPEND,
        BITCOUNT, BITPOS,
        DUMP,DEL, DECR, DECRBY,
        EXPIREAT, EXISTS, EXPIRE,
        GET, GETBIT, GETRANGE, GETSET,
        HSET, HSETNX, HMSET, HGET, HMGET, HINCRBY,
        HEXISTS, HLEN, HDEL, HKEYS, HVALS, HGETALL, HINCRBYFLOAT,
        INCR, INCRBY, INCRBYFLOAT,
        LPUSH, LPUSHX, LPOP, LRANGE, LREM, LINDEX, LINSERT, LLEN, LSET, LTRIM,
        MGET, MSET,
        PSETEX, PERSIST, PEXPIRE, PEXPIREAT, PTTL, PING,
        PFADD, PFCOUNT, PFMERGE,
        RESTORE, RPOP, RPUSH, RPUSHX,
        SADD, SMEMBERS, SREM, SPOP, SCARD, SISMEMBER, SRANDMEMBER,
        SETBIT, SETRANGE, STRLEN, SET, SETEX, SETNX,
        TTL, TYPE,
        ZADD, ZRANGE, ZREM, ZINCRBY, ZRANK, ZREVRANK, ZREVRANGE,
        ZRANGEBYSCORE, ZCOUNT, ZCARD, ZREMRANGEBYRANK, ZREMRANGEBYSCORE
    };
    enum {
        CMD_COUNT = ZREMRANGEBYSCORE+1
    };

public:
    static const char* commandName(int type);

public:
    char name[32];
    int len;
    int type;
    void (*proc)(ClientPacket*, void*);
    void* arg;
};

class RedisCommandTable
{
private:
    RedisCommandTable(void);
    ~RedisCommandTable(void);

public:
    static RedisCommandTable* instance(void);

    int registerCommand(RedisCommand* cmd, int cnt);
    RedisCommand* findCommand(const char* cmd, int len) const;

    const std::list<RedisCommand*>& commands(void) const { return m_cmds; }

private:
    StringMap<RedisCommand*> m_cmdMap;
    std::list<RedisCommand*> m_cmds;
};

#endif
