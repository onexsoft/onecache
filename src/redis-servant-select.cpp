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

#include "command.h"
#include "redisproxy.h"
#include "redisservant.h"

#include "redis-servant-select.h"


ServantSelect::ServantSelect(void)
{   m_masterCallNum = 0;
    m_slaveCallNum = 0;
}

ServantSelect::~ServantSelect()
{

}


RedisServant* ServantSelect::selectMaster(RedisServantGroup* group) {
    RedisServant* servant = oneMaster(group);
    if (servant != NULL) return servant;
    return oneSlave(group);
}

RedisServant* ServantSelect::selectSlave(RedisServantGroup* group) {
    RedisServant* servant = oneSlave(group);
    if (servant != NULL) return servant;
    return oneMaster(group);
}


RedisServant* ServantSelect::oneMaster(RedisServantGroup* group) {
    int masterCount = group->masterCount();
    unsigned int callNum = m_masterCallNum;
    for (int i = 0; i < masterCount; ++i) {
        RedisServant* servant = group->master(callNum % masterCount);
        if (servant->isActived())
            return servant;
        callNum++;
    }
    m_masterCallNum = callNum;
    return NULL;
}

RedisServant* ServantSelect::oneSlave(RedisServantGroup* group) {
    int slaveCount = group->slaveCount();
    unsigned int callNum = m_slaveCallNum;
    for (int i = 0; i < slaveCount; ++i) {
        RedisServant* servant = group->slave(callNum % slaveCount);
        if (servant->isActived()) {
            ++m_slaveCallNum;
            return servant;
        }
        ++callNum;
    }
    m_slaveCallNum = callNum;
    return NULL;
}


static int CommandType[RedisCommand::CMD_COUNT] = {0};
void initCommandType()
{
    for (int i = 0; i < RedisCommand::CMD_COUNT; ++i) {
        // DUMP DEL DECR DECRBY EXPIREAT EXISTS EXPIRE GETSET HSET HSETNX HMSET HINCRBY
        // HDEL HINCRBYFLOAT INCR INCRBY INCRBYFLOAT LPUSH LPUSHX LPOP LREM
        // LINSERT   LSET LTRIM MSET PSETEX PERSIST PEXPIRE
        // PEXPIREAT PTTL  PING RESTORE RPOP RPUSH RPUSHX SADD SREM
        // SPOP SETBIT SETRANGE SET SETEX
        // SETNX TTL ZADD ZREM ZINCRBY ZREMRANGEBYRANK ZREMRANGEBYSCORE
        if (RedisCommand::APPEND == i || RedisCommand::BITPOS == i
            || RedisCommand::DUMP == i || RedisCommand::DEL == i
            || RedisCommand::DECR == i || RedisCommand::DECRBY == i
            || RedisCommand::EXPIREAT == i || RedisCommand::EXISTS == i
            || RedisCommand::EXPIRE == i || RedisCommand::GETSET == i
            || RedisCommand::HSET == i || RedisCommand::HSETNX == i
            || RedisCommand::HMSET == i || RedisCommand::HINCRBY == i
            || RedisCommand::HDEL == i || RedisCommand::HINCRBYFLOAT == i
            || RedisCommand::INCR == i || RedisCommand::INCRBY == i
            || RedisCommand::INCRBYFLOAT == i || RedisCommand::LPUSH == i
            || RedisCommand::LPUSHX == i || RedisCommand::LPOP == i
            || RedisCommand::LREM == i || RedisCommand::LINSERT == i
            || RedisCommand::LSET == i || RedisCommand::LTRIM == i
            || RedisCommand::MSET == i || RedisCommand::PSETEX == i
            || RedisCommand::PERSIST == i || RedisCommand::PEXPIRE == i
            || RedisCommand::PEXPIREAT == i || RedisCommand::PTTL == i
            || RedisCommand::PING == i || RedisCommand::RESTORE == i
            || RedisCommand::RPOP == i || RedisCommand::RPUSH == i
            || RedisCommand::RPUSHX == i || RedisCommand::SADD == i
            || RedisCommand::SREM == i || RedisCommand::SPOP == i
            || RedisCommand::SETBIT == i || RedisCommand::SETRANGE == i
            || RedisCommand::SET == i || RedisCommand::SETEX == i
            || RedisCommand::SETNX == i || RedisCommand::TTL == i
            || RedisCommand::ZADD == i || RedisCommand::ZREM == i
            || RedisCommand::ZINCRBY == i || RedisCommand::ZREMRANGEBYRANK == i
            || RedisCommand::ZREMRANGEBYSCORE == i)
        {
            CommandType[i] = 1;
        } else {
            CommandType[i] = 0;
        }
    }
}

class CommandTypeInit {
public:
    CommandTypeInit() {
        initCommandType();
    }
};

static CommandTypeInit init;

ReadBalancePolicy::ReadBalancePolicy(void) {
    m_readCnt = 0;
}

ReadBalancePolicy::~ReadBalancePolicy(void) {}


RedisServant* ReadBalancePolicy::selectServant(RedisServantGroup* group, ClientPacket* packet)
{
    if (packet->commandType < 0 || packet->commandType >= RedisCommand::CMD_COUNT) {
        return m_servantSelect.selectMaster(group);
    }
    if (1 == CommandType[packet->commandType]) {
        return m_servantSelect.selectMaster(group);
    }

    if (0 == (++m_readCnt % 2)){
        return m_servantSelect.selectSlave(group);
    }
    return m_servantSelect.selectMaster(group);
}

RedisServant* MasterOnlyPolicy::selectServant(RedisServantGroup* group, ClientPacket*)
{
    return m_servantSelect.selectMaster(group);
}



