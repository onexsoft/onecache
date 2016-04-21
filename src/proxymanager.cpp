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
    for (int i = 0; i < m_proxy->slotCount(); ++i) {
        if (m_proxy->groupBySlot(i) == group) {
            info->oldHashValues.push_back(i);
        }
    }

    LOG(Logger::Message, "Remove the group '%s' after %d seconds", group->groupName(), seconds);
    info->ev.setTimer(m_proxy->eventLoop(), onSetGroupTTL, info);
    info->ev.active(seconds * 1000);
    info->can_ttl = false;
}

void ProxyManager::removeGroup(RedisServantGroup* group)
{
    std::vector<int> groupOldHashValue;
    std::set<RedisServantGroup*> otherGroups;
    int maxHashValue = m_proxy->slotCount();
    for (int i = 0; i < maxHashValue; ++i) {
        RedisServantGroup* mp = m_proxy->groupBySlot(i);
        if (mp == group) {
            groupOldHashValue.push_back(i);
            m_proxy->setSlot(i, NULL);
        } else {
            otherGroups.insert(mp);
        }
    }

    if (!groupOldHashValue.empty()) {
        LOG(Logger::Message, "Group '%s' removed", group->groupName());
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
        m_proxy->setSlot(hashval, group);
    }
}

void ProxyManager::onSetGroupTTL(int, short, void* arg)
{
    GroupInfo* info = (GroupInfo*)arg;
    LOG(Logger::Message, "Remove group '%s'...", info->group->groupName());
    RedisServantGroup* group = info->group;
    if (group->isEnabled()) {
        LOG(Logger::Message, "Group '%s' is actived. cancelled to remove",
                    info->group->groupName());
        info->can_ttl = true;
        return;
    }

    info->manager->removeGroup(group);
    if (info->restore) {
        LOG(Logger::Message, "Restore group '%s'", info->group->groupName());
        info->ev.setTimer(info->manager->proxy()->eventLoop(), onRestoreGroup, info);
        info->ev.active(500);
    }
}

void ProxyManager::onRestoreGroup(int, short, void* arg)
{
    GroupInfo* info = (GroupInfo*)arg;
    RedisServantGroup* group = info->group;
    if (group->isEnabled()) {
        LOG(Logger::Message, "Group '%s' has been restored", info->group->groupName());
        std::vector<int>::iterator it = info->oldHashValues.begin();
        for (; it != info->oldHashValues.end(); ++it) {
            info->manager->proxy()->setSlot(*it, group);
        }
        info->can_ttl = true;
    } else {
        info->ev.active(500);
    }
}
