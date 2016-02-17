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

#ifndef SELECTSERVANT_H
#define SELECTSERVANT_H

#include "redisservantgroup.h"

#define POLICY_READ_BALANCE "read_balance"
#define POLICY_MASTER_ONLY  "master_only"

class ServantSelect
{
public:
    ServantSelect(void);
    ~ServantSelect(void);
    RedisServant* selectMaster(RedisServantGroup* g);
    RedisServant* selectSlave(RedisServantGroup* g);
private:
    RedisServant* oneMaster(RedisServantGroup* g);
    RedisServant* oneSlave(RedisServantGroup* g);
private:
    unsigned int m_masterCallNum;
    unsigned int m_slaveCallNum;
};


class ReadBalancePolicy : public RedisServantGroupPolicy
{
public:
    ReadBalancePolicy(void);
    ~ReadBalancePolicy(void);
    virtual RedisServant* selectServant(RedisServantGroup* g, ClientPacket* p);
private:
    ServantSelect m_servantSelect;
    unsigned int  m_readCnt;
};


class MasterOnlyPolicy : public RedisServantGroupPolicy
{
public:
    MasterOnlyPolicy(void){}
    ~MasterOnlyPolicy(void){}
    virtual RedisServant* selectServant(RedisServantGroup* g, ClientPacket* p);
private:
    ServantSelect m_servantSelect;
};



#endif


