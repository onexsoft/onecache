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

#include "util/logger.h"
#include "cmdhandler.h"
#include "redisproxy.h"
#include "command.h"

static RedisCommand _redisCommand[] = {
    {"AUTH", 4, RedisCommand::AUTH, onAuthCommand, NULL},
    {"APPEND", 6, RedisCommand::APPEND, onStandardKeyCommand, NULL},
    {"BITCOUNT", 8, RedisCommand::BITCOUNT, onStandardKeyCommand, NULL},
    {"BITPOS", 6, RedisCommand::BITPOS, onStandardKeyCommand, NULL},
    {"DUMP", 4, RedisCommand::DUMP, onStandardKeyCommand, NULL},
    {"DEL", 3, RedisCommand::DEL, onDelCommand, NULL},
    {"DECR", 4, RedisCommand::DECR, onStandardKeyCommand, NULL},
    {"DECRBY", 6, RedisCommand::DECRBY, onStandardKeyCommand, NULL},
    {"EXPIREAT", 8, RedisCommand::EXPIREAT, onStandardKeyCommand, NULL},
    {"EXISTS", 6, RedisCommand::EXISTS, onStandardKeyCommand, NULL},
    {"EXPIRE", 6, RedisCommand::EXPIRE, onStandardKeyCommand, NULL},
    {"GET", 3, RedisCommand::GET, onStandardKeyCommand, NULL},
    {"GETBIT", 6, RedisCommand::GETBIT, onStandardKeyCommand, NULL},
    {"GETRANGE", 8, RedisCommand::GETRANGE, onStandardKeyCommand, NULL},
    {"GETSET", 6, RedisCommand::GETSET, onStandardKeyCommand, NULL},
    {"HSET", 4, RedisCommand::HSET, onStandardKeyCommand, NULL},
    {"HSETNX", 6, RedisCommand::HSETNX, onStandardKeyCommand, NULL},
    {"HMSET", 5, RedisCommand::HMSET, onStandardKeyCommand, NULL},
    {"HGET", 4, RedisCommand::HGET, onStandardKeyCommand, NULL},
    {"HMGET", 5, RedisCommand::HMGET, onStandardKeyCommand, NULL},
    {"HINCRBY", 7, RedisCommand::HINCRBY, onStandardKeyCommand, NULL},
    {"HEXISTS", 7, RedisCommand::HEXISTS, onStandardKeyCommand, NULL},
    {"HLEN", 4, RedisCommand::HLEN, onStandardKeyCommand, NULL},
    {"HDEL", 4, RedisCommand::HDEL, onStandardKeyCommand, NULL},
    {"HKEYS", 5, RedisCommand::HKEYS, onStandardKeyCommand, NULL},
    {"HVALS", 5, RedisCommand::HVALS, onStandardKeyCommand, NULL},
    {"HGETALL", 7, RedisCommand::HGETALL, onStandardKeyCommand, NULL},
    {"HINCRBYFLOAT", 12, RedisCommand::HINCRBYFLOAT, onStandardKeyCommand, NULL},
    {"INCR", 4, RedisCommand::INCR, onStandardKeyCommand, NULL},
    {"INCRBY", 6, RedisCommand::INCRBY, onStandardKeyCommand, NULL},
    {"INCRBYFLOAT", 11, RedisCommand::INCRBYFLOAT, onStandardKeyCommand,  NULL},

    {"LPUSH", 5, RedisCommand::LPUSH, onStandardKeyCommand, NULL},
    {"LPUSHX", 6, RedisCommand::LPUSHX, onStandardKeyCommand, NULL},
    {"LPOP", 4, RedisCommand::LPOP, onStandardKeyCommand, NULL},
    {"LRANGE", 6, RedisCommand::LRANGE, onStandardKeyCommand, NULL},
    {"LREM", 4, RedisCommand::LREM, onStandardKeyCommand, NULL},
    {"LINDEX", 5, RedisCommand::LINDEX, onStandardKeyCommand, NULL},
    {"LINSERT", 7, RedisCommand::LINSERT, onStandardKeyCommand, NULL},
    {"LLEN", 4, RedisCommand::LLEN, onStandardKeyCommand, NULL},
    {"LSET", 4, RedisCommand::LSET, onStandardKeyCommand, NULL},
    {"LTRIM", 5, RedisCommand::LTRIM, onStandardKeyCommand, NULL},

    {"MGET", 4, RedisCommand::MGET, onMGetCommand, NULL},
    {"MSET", 4, RedisCommand::MSET, onMSetCommand, NULL},

    {"PSETEX", 6, RedisCommand::PSETEX, onStandardKeyCommand, NULL},
    {"PERSIST", 7, RedisCommand::PERSIST, onStandardKeyCommand, NULL},
    {"PEXPIRE", 7, RedisCommand::PEXPIRE, onStandardKeyCommand, NULL},
    {"PEXPIREAT", 9, RedisCommand::PEXPIREAT, onStandardKeyCommand, NULL},
    {"PTTL", 4, RedisCommand::PTTL, onStandardKeyCommand, NULL},
    {"PING", 4, RedisCommand::PING, onPingCommand, NULL},

    {"PFADD", 5, RedisCommand::PFADD, onStandardKeyCommand, NULL},
    {"PFCOUNT", 7, RedisCommand::PFCOUNT, onStandardKeyCommand, NULL},
    {"PFMERGE", 7, RedisCommand::PFMERGE, onStandardKeyCommand, NULL},

    {"RESTORE", 7, RedisCommand::RESTORE, onStandardKeyCommand, NULL},
    {"RPOP", 4, RedisCommand::RPOP, onStandardKeyCommand, NULL},
    {"RPUSH", 5, RedisCommand::RPUSH, onStandardKeyCommand, NULL},
    {"RPUSHX", 6, RedisCommand::RPUSHX, onStandardKeyCommand, NULL},

    {"SADD", 4, RedisCommand::SADD, onStandardKeyCommand, NULL},
    {"SMEMBERS", 8, RedisCommand::SMEMBERS, onStandardKeyCommand, NULL},
    {"SREM", 4, RedisCommand::SREM, onStandardKeyCommand, NULL},
    {"SPOP", 4, RedisCommand::SPOP, onStandardKeyCommand, NULL},
    {"SCARD", 5, RedisCommand::SCARD, onStandardKeyCommand, NULL},
    {"SISMEMBER", 9, RedisCommand::SISMEMBER, onStandardKeyCommand, NULL},
    {"SRANDMEMBER", 11, RedisCommand::SRANDMEMBER, onStandardKeyCommand, NULL},

    {"SETBIT", 6, RedisCommand::SETBIT, onStandardKeyCommand, NULL},
    {"SETRANGE", 8, RedisCommand::SETRANGE, onStandardKeyCommand, NULL},
    {"STRLEN", 6, RedisCommand::STRLEN, onStandardKeyCommand, NULL},
    {"SET", 3, RedisCommand::SET, onStandardKeyCommand, NULL},
    {"SETEX", 5, RedisCommand::SETEX, onStandardKeyCommand, NULL},
    {"SETNX", 5, RedisCommand::SETNX, onStandardKeyCommand, NULL},

    {"TTL", 3, RedisCommand::TTL, onStandardKeyCommand, NULL},
    {"TYPE", 4, RedisCommand::TYPE, onStandardKeyCommand, NULL},

    {"ZADD", 4, RedisCommand::ZADD, onStandardKeyCommand, NULL},
    {"ZRANGE", 6, RedisCommand::ZRANGE, onStandardKeyCommand, NULL},
    {"ZREM", 4, RedisCommand::ZREM, onStandardKeyCommand, NULL},
    {"ZINCRBY", 7, RedisCommand::ZINCRBY, onStandardKeyCommand, NULL},
    {"ZRANK", 5, RedisCommand::ZRANK, onStandardKeyCommand, NULL},
    {"ZREVRANK", 8, RedisCommand::ZREVRANK, onStandardKeyCommand, NULL},
    {"ZREVRANGE", 9, RedisCommand::ZREVRANGE, onStandardKeyCommand, NULL},
    {"ZRANGEBYSCORE", 13, RedisCommand::ZRANGEBYSCORE, onStandardKeyCommand, NULL},
    {"ZCOUNT", 6, RedisCommand::ZCOUNT, onStandardKeyCommand, NULL},
    {"ZCARD", 5, RedisCommand::ZCARD, onStandardKeyCommand, NULL},
    {"ZREMRANGEBYRANK", 15, RedisCommand::ZREMRANGEBYRANK, onStandardKeyCommand, NULL},
    {"ZREMRANGEBYSCORE", 16, RedisCommand::ZREMRANGEBYSCORE, onStandardKeyCommand, NULL},

    {"SHOWCMD", 7, -1, onShowCommand, NULL}
};

const char *RedisCommand::commandName(int type)
{
    if (type >= 0 && type < RedisCommand::CMD_COUNT) {
        return _redisCommand[type].name;
    }
    return "";
}

RedisCommandTable::RedisCommandTable(void)
{
    registerCommand(_redisCommand, sizeof(_redisCommand) / sizeof(RedisCommand));
}

RedisCommandTable::~RedisCommandTable(void)
{
    std::list<RedisCommand*>::iterator it = m_cmds.begin();
    for (; it != m_cmds.end(); ++it) {
        delete *it;
    }
}

RedisCommandTable* RedisCommandTable::instance(void)
{
    static RedisCommandTable* table = NULL;
    if (!table) {
        table = new RedisCommandTable;
    }
    return table;
}

int RedisCommandTable::registerCommand(RedisCommand *cmd, int cnt)
{
    int succeed = 0;
    for (int i = 0; i < cnt; ++i) {
        char* name = cmd[i].name;
        int len = cmd[i].len;

        for (int j = 0; j < len; ++j) {
            if (name[j] >= 'a' && name[j] <= 'z') {
                name[j] += ('A' - 'a');
            }
        }

        RedisCommand* val = new RedisCommand(cmd[i]);
        m_cmds.push_back(val);
        m_cmdMap.insert(StringMap<RedisCommand*>::value_type(String(name, len, true), val));
        ++succeed;
    }
    return succeed;
}

RedisCommand *RedisCommandTable::findCommand(const char *cmd, int len) const
{
    char* p = (char*)cmd;
    for (int i = 0; i < len; ++i) {
        if (p[i] >= 'a' && p[i] <= 'z') {
            p[i] += ('A' - 'a');
        }
    }

    StringMap<RedisCommand*>::const_iterator it = m_cmdMap.find(String(cmd, len));
    if (it != m_cmdMap.cend()) {
        return it->second;
    } else {
        return NULL;
    }
}
