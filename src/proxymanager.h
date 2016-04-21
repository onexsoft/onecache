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

#ifndef PROXYMANAGER_H
#define PROXYMANAGER_H

#include <map>
#include <vector>
#include <set>

#include "eventloop.h"

class RedisProxy;
class RedisServantGroup;
class ProxyManager;

struct GroupInfo
{
    GroupInfo(void) {
        manager = NULL;
        group = NULL;
        can_ttl = true;
        restore = false;
    }

    ProxyManager* manager;
    RedisServantGroup* group;
    bool can_ttl;
    Event ev;
    bool restore;
    std::vector<int> oldHashValues;
};


class ProxyManager
{
public:
    ProxyManager(void);
    ~ProxyManager(void);

    void setProxy(RedisProxy* p) { m_proxy = p; }
    RedisProxy* proxy(void) const { return m_proxy; }

    void setGroupTTL(RedisServantGroup* group, int seconds, bool restore);

private:
    void removeGroup(RedisServantGroup* group);
    static void onSetGroupTTL(int, short, void*);
    static void onRestoreGroup(int, short, void*);

private:
    typedef std::map<RedisServantGroup*, GroupInfo*> GroupInfoMap;
    GroupInfoMap m_groupInfoMap;
    RedisProxy* m_proxy;
};

#endif


