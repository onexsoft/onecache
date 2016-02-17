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
#include "redisproxy.h"
#include "redisservant.h"
#include "redisservantgroup.h"
#include "redis-servant-select.h"


RedisServantGroupPolicy::RedisServantGroupPolicy(void)
{
}

RedisServantGroupPolicy::~RedisServantGroupPolicy(void)
{
}

RedisServant *RedisServantGroupPolicy::selectServant(RedisServantGroup*, ClientPacket*)
{
    return NULL;
}

RedisServantGroupPolicy *RedisServantGroupPolicy::createPolicy(const char *name)
{
    if (name == NULL) {
        return new MasterOnlyPolicy;
    }
    if (strcmp(name, POLICY_MASTER_ONLY) == 0) {
        return new MasterOnlyPolicy;
    } else if (strcmp(name, POLICY_READ_BALANCE) == 0) {
        return new ReadBalancePolicy;
    } else {
        return new MasterOnlyPolicy;
    }
}




RedisServantGroup::RedisServantGroup(void)
{
    m_groupId = -1;
    m_masterCount = 0;
    m_slaveCount = 0;
    m_policy = NULL;
}

RedisServantGroup::~RedisServantGroup(void)
{
    for (int i = 0; i < masterCount(); ++i) {
        delete master(i);
    }

    for (int i = 0; i < slaveCount(); ++i) {
        delete slave(i);
    }

    if (m_policy) {
        delete m_policy;
    }
}

void RedisServantGroup::setGroupName(const char *name)
{
    if (name) {
        strcpy(m_name, name);
    }
}

void RedisServantGroup::setPolicy(RedisServantGroupPolicy *policy)
{
    if (policy) {
        if (m_policy) {
            delete m_policy;
        }
        m_policy = policy;
    }
}

void RedisServantGroup::addMasterRedisServant(RedisServant *servant)
{
    if (servant) {
        if (m_masterCount >= MaxServantCount) {
            return;
        }
        m_master[m_masterCount] = servant;
        ++m_masterCount;
    }
}

void RedisServantGroup::addSlaveRedisServant(RedisServant *servant)
{
    if (servant) {
        if (m_slaveCount >= MaxServantCount) {
            return;
        }
        m_slaver[m_slaveCount] = servant;
        ++m_slaveCount;
    }
}

void RedisServantGroup::setEnabled(bool b)
{
    for (int i = 0; i < m_masterCount; ++i) {
        if (b) {
            m_master[i]->start();
        } else {
            m_master[i]->stop();
        }
    }

    for (int i = 0; i < m_slaveCount; ++i) {
        if (b) {
            m_slaver[i]->start();
        } else {
            m_slaver[i]->stop();
        }
    }
}

bool RedisServantGroup::isEnabled(void) const
{
    for (int i = 0; i < m_masterCount; ++i) {
        if (m_master[i]->isActived()) {
            return true;
        }
    }

    for (int i = 0; i < m_slaveCount; ++i) {
        if (m_slaver[i]->isActived()) {
            return true;
        }
    }

    return false;
}

