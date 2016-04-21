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

#ifndef REDISSERVANTGROUP_H
#define REDISSERVANTGROUP_H

class ClientPacket;
class RedisServant;
class RedisServantGroup;

class RedisServantGroupPolicy
{
public:
    RedisServantGroupPolicy(void);
    virtual ~RedisServantGroupPolicy(void);

    virtual RedisServant* selectServant(RedisServantGroup* group, ClientPacket* packet);

    static RedisServantGroupPolicy* createPolicy(const char* name);
};


class RedisServantGroup
{
public:
    enum {
        MaxServantCount = 1024,
    };

    RedisServantGroup(void);
    ~RedisServantGroup(void);

    void setGroupId(int id) { m_groupId = id; }
    int groupId(void) const { return m_groupId; }

    void setGroupName(const char* name);
    const char* groupName(void) const { return m_name; }

    void setPolicy(RedisServantGroupPolicy* policy);
    RedisServantGroupPolicy* policy(void) const { return m_policy; }

    void addMasterRedisServant(RedisServant* servant);
    void addSlaveRedisServant(RedisServant* servant);

    RedisServant* master(int index) const { return m_master[index]; }
    RedisServant* slave(int index) const { return m_slaver[index]; }

    int masterCount(void) const { return m_masterCount; }
    int slaveCount(void) const { return m_slaveCount; }

    void setEnabled(bool b);
    bool isEnabled(void) const;

    RedisServant* findUsableServant(ClientPacket* packet)
    { return m_policy->selectServant(this, packet); }

private:
    int m_groupId;
    char m_name[256];
    int m_masterCount;
    int m_slaveCount;
    RedisServant* m_master[MaxServantCount];
    RedisServant* m_slaver[MaxServantCount];
    RedisServantGroupPolicy* m_policy;

private:
    RedisServantGroup(const RedisServantGroup&);
    RedisServantGroup& operator =(const RedisServantGroup&);
};


#endif
