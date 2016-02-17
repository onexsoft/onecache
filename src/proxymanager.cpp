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
#include "proxymanager.h"
#include "redisproxy.h"

ProxyManager::ProxyManager(void)
{
    m_proxy = NULL;
}

ProxyManager::~ProxyManager(void)
{
    GroupInfoMap::iterator it = m_groupInfoMap.begin();
    for (; it != m_groupInfoMap.end(); ++it) {
        delete it->second;
    }
}

void ProxyManager::setGroupTTL(RedisServantGroup* group, int seconds, bool restore)
{
    GroupInfo* info = NULL;
    GroupInfoMap::iterator it = m_groupInfoMap.find(group);
    if (it == m_groupInfoMap.end()) {
        info = new GroupInfo;
        info->group = group;
        info->restore = restore;
        info->manager = this;
        m_groupInfoMap[group] = info;
    } else {
        info = it->second;
    }

    if (!info->can_ttl) {
        return;
    }

    //Update group old hash values
    info->oldHashValues.clear();
    for (int i = 0; i < m_proxy->maxHashValue(); ++i) {
        if (m_proxy->hashForGroup(i) == group) {
            info->oldHashValues.push_back(i);
        }
    }

    Logger::log(Logger::Message, "set group '%s' TTL=%d second(s)", group->groupName(), seconds);
    info->ev.setTimer(m_proxy->eventLoop(), onSetGroupTTL, info);
    info->ev.active(seconds * 1000);
    info->can_ttl = false;
}

void ProxyManager::removeGroup(RedisServantGroup* group)
{
    std::vector<int> groupOldHashValue;
    std::set<RedisServantGroup*> otherGroups;
    int maxHashValue = m_proxy->maxHashValue();
    for (int i = 0; i < maxHashValue; ++i) {
        RedisServantGroup* mp = m_proxy->hashForGroup(i);
        if (mp == group) {
            groupOldHashValue.push_back(i);
            m_proxy->setGroupMappingValue(i, NULL);
        } else {
            otherGroups.insert(mp);
        }
    }

    if (!groupOldHashValue.empty()) {
        Logger::log(Logger::Message, "Group '%s' removed", group->groupName());
    }

    if (otherGroups.empty() || groupOldHashValue.empty()) {
        return;
    }

    std::vector<RedisServantGroup*> vec(otherGroups.size());
    std::set<RedisServantGroup*>::const_iterator itGroup = otherGroups.cbegin();
    for (int i = 0; itGroup != otherGroups.cend(); ++itGroup, ++i) {
        vec[i] = *itGroup;
    }

    std::vector<int>::const_iterator itHash = groupOldHashValue.cbegin();
    for (int i = 0; itHash != groupOldHashValue.cend(); ++itHash, ++i) {
        int hashval = *itHash;
        RedisServantGroup* group = vec[i % vec.size()];
        m_proxy->setGroupMappingValue(hashval, group);
    }
}

void ProxyManager::onSetGroupTTL(int, short, void* arg)
{
    GroupInfo* info = (GroupInfo*)arg;
    Logger::log(Logger::Message, "TTL: Remove group '%s'...", info->group->groupName());
    RedisServantGroup* group = info->group;
    if (group->isEnabled()) {
        Logger::log(Logger::Message, "TTL: Group '%s' is actived. Has been cancelled to remove",
                    info->group->groupName());
        info->can_ttl = true;
        return;
    }

    info->manager->removeGroup(group);
    if (info->restore) {
        Logger::log(Logger::Message, "When the group '%s' becomes active state before will restore",
                    info->group->groupName());
        info->ev.setTimer(info->manager->proxy()->eventLoop(), onRestoreGroup, info);
        info->ev.active(500);
    }
}

void ProxyManager::onRestoreGroup(int, short, void* arg)
{
    GroupInfo* info = (GroupInfo*)arg;
    RedisServantGroup* group = info->group;
    if (group->isEnabled()) {
        Logger::log(Logger::Message, "Group '%s' has been restored", info->group->groupName());
        std::vector<int>::iterator it = info->oldHashValues.begin();
        for (; it != info->oldHashValues.end(); ++it) {
            info->manager->proxy()->setGroupMappingValue(*it, group);
        }
        info->can_ttl = true;
    } else {
        info->ev.active(500);
    }
}
