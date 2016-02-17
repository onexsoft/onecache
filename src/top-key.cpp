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

#include "top-key.h"


// group by key count
static bool cmp(const pair<const char*, CTopKeyRecorderThread::KeyCntList::iterator>& x, \
                const pair<const char*,CTopKeyRecorderThread::KeyCntList::iterator>& y) {
    return (x.second)->keyCnt > (y.second)->keyCnt;
}

// group by value size
static bool cmpValueSize(const pair<const char*, CTopKeyRecorderThread::KeyCntList::iterator>& x, \
                const pair<const char*,CTopKeyRecorderThread::KeyCntList::iterator>& y) {
    return (x.second)->valueSize > (y.second)->valueSize;
}

CTopKeySorter::CTopKeySorter() {
    m_sortVectorKeyCnt = new vector<pair <const char*, CTopKeyRecorderThread::KeyCntList::iterator> >;
}

CTopKeySorter::~CTopKeySorter() {
    if (m_sortVectorKeyCnt != NULL) {
        delete m_sortVectorKeyCnt;
        m_sortVectorKeyCnt = NULL;
    }
}

void CTopKeySorter::sortTopKeyByValueSize(CTopKeyRecorderThread& keyRecorder) {
    keyRecorder.m_lock.lock();
    m_sortVectorKeyCnt->insert(m_sortVectorKeyCnt->begin(),
                              keyRecorder.m_keyCntMap.begin(),
                              keyRecorder.m_keyCntMap.end());
    keyRecorder.m_lock.unlock();
    sort(m_sortVectorKeyCnt->begin(), m_sortVectorKeyCnt->end(), cmpValueSize);
}

void CTopKeySorter::sortTopKeyByCnt(CTopKeyRecorderThread& keyRecorder) {
    keyRecorder.m_lock.lock();
    m_sortVectorKeyCnt->insert(m_sortVectorKeyCnt->begin(),
                              keyRecorder.m_keyCntMap.begin(),
                              keyRecorder.m_keyCntMap.end());
    keyRecorder.m_lock.unlock();
    sort(m_sortVectorKeyCnt->begin(), m_sortVectorKeyCnt->end(), cmp);
}


void CTopKeyRecorderThread::delListMap() {
    int midSize = m_maxKeyCnt - 500;
    KeyCntList::iterator it = m_keyCntList.begin();
    KeyCntList::iterator itDel = m_keyCntList.end();
    const unsigned int endCnt = (--m_keyCntList.end())->keyCnt;
    for (int i = 0; i < midSize; ++i) {
        ++it;
    }
    bool del = false;
    while (it != m_keyCntList.end()) {
        if (it->keyCnt < endCnt) {
            KeyCntMap::iterator itMap = m_keyCntMap.end();
            itMap = minCntMap(it);
            if (itMap != m_keyCntMap.end()) {
                delete[] (itMap->first);
                m_keyCntMap.erase(itMap);
                it = m_keyCntList.erase(it);
            }
            del = true;
        } else {
            ++it;
        }
    }

    if (!del) {
        it = --m_keyCntList.end();
        KeyCntMap::iterator itMap = m_keyCntMap.end();
        itMap = minCntMap(it);
        if (itMap != m_keyCntMap.end()) {
            delete[] (itMap->first);
            m_keyCntMap.erase(itMap);
            m_keyCntList.erase(it);
        }
    }
}

CTopKeyRecorderThread::KeyCntMap::iterator CTopKeyRecorderThread::minCntMap(KeyCntList::iterator itListDel){
    KeyCntMap::iterator itMap = m_keyCntMap.begin();
    for (; itMap != m_keyCntMap.end(); ++itMap) {
        if (itListDel == itMap->second)
            return itMap;
    }
    return itMap;
}

void CTopKeyRecorderThread::keyToList(KeyStrValueSize& keyInfo) {
    if (!m_processTopKey) {
        m_allKeys.push(keyInfo);
        return;
    }
    delete[] keyInfo.key;
}

void CTopKeyRecorderThread::keyRecorder(KeyStrValueSize& keyInfo) {
    KeyCntMap::iterator itMap = m_keyCntMap.find(keyInfo.key);
    KeyCntValueSize node;
    if (itMap == m_keyCntMap.end()) {
        m_lock.lock();
        // new key
        if (m_keyCntList.size() >= m_maxKeyCnt) {
            delListMap();
        }
        node.keyCnt = 1;
        node.valueSize = keyInfo.valueSize;

        m_keyCntList.push_front(node);
        m_keyCntMap[keyInfo.key] = m_keyCntList.begin();
        m_lock.unlock();
    } else {
        m_lock.lock();
        // already exist
        KeyCntList::iterator listIt = itMap->second;
        node = *listIt;
        m_keyCntList.erase(listIt);
        node.keyCnt++;
        node.valueSize += keyInfo.valueSize;

        m_keyCntList.push_front(node);
        itMap->second = m_keyCntList.begin();
        delete[] keyInfo.key;
        m_lock.unlock();
    }
}



void CTopKeyRecorderThread::run() {
    while(true) {
        Thread::sleep(50);
        m_processTopKey = true;
        while (!this->m_allKeys.empty()) {
            KeyStrValueSize node = this->m_allKeys.front();
            this->m_allKeys.pop();
            this->keyRecorder(node);
        }
        this->m_processTopKey = false;
    }
}

void CTopKeyRecorderThread::onExit()
{

}
