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

#include <time.h>

#include "redis-proxy-config.h"
#include "redis-servant-select.h"

#ifdef WIN32
#define strcasecmp stricmp
#define strncasecmp strnicmp
#endif


COperateXml::COperateXml() {
    m_docPointer = new TiXmlDocument;
}

COperateXml::~COperateXml() {
    delete m_docPointer;
}

bool COperateXml::xml_open(const char* xml_path) {
    if (!m_docPointer->LoadFile(xml_path)) {
        return false;
    }
    m_rootElement = m_docPointer->RootElement();
    return true;
}

void COperateXml::xml_print() {
    m_docPointer->Print();
}

const TiXmlElement* COperateXml::get_rootElement() {
    return m_rootElement;
}

// host info
CHostInfo::CHostInfo() {
    ip = "";
    host_name = "";
    port = 0;
    master = false;
    priority = 0;
    policy = 0;
    connection_num = 50;
    memset(password, '\0', sizeof(password));
}

CHostInfo::~CHostInfo(){}

string CHostInfo::get_ip()const
{
    return ip;
}

string CHostInfo::get_hostName()const
{
    return host_name;
}
int CHostInfo::get_port()const          { return port;}
bool CHostInfo::get_master()const       { return master;}
int CHostInfo::get_policy()const        { return policy;}
int CHostInfo::get_priority()const      { return priority;}
int CHostInfo::get_connectionNum()const { return connection_num;}
const string CHostInfo::passWord()const  {return string(password, strlen(password));}

void CHostInfo::set_ip(string& s)        { ip = s;}
void CHostInfo::set_hostName(string& s)  { host_name = s;}
void CHostInfo::set_port(int p)          { port = p;}
void CHostInfo::set_master(bool m)       { master = m;}
void CHostInfo::set_policy(int p)        { policy = p;}
void CHostInfo::set_priority(int p)      { priority = p;}
void CHostInfo::set_connectionNum(int p) { connection_num = p;}
void CHostInfo::set_passWord(const char* p) { strcpy(password, p);}


CGroupInfo::CGroupInfo()
{
    memset(m_groupName, '\0', sizeof(m_groupName));
    memset(m_groupPolicy, '\0', sizeof(m_groupPolicy));
    m_weight = 1;
    m_hashMin = 0;
    m_hashMax = 0;
}

CGroupInfo::~CGroupInfo() {}

CRedisProxyCfg::CRedisProxyCfg() {
    m_operateXmlPointer = new COperateXml;
    m_hashInfo.hash_value_max = 0;
    m_threadNum = 0;
    m_port = 0;
    memset(m_vip.if_alias_name, '\0', sizeof(m_vip.if_alias_name));
    memset(m_vip.vip_address, '\0', sizeof(m_vip.vip_address));
    m_vip.enable = false;
    memset(m_logFile, '\0', sizeof(m_logFile));
    memset(m_pidFile, '\0', sizeof(m_pidFile));
    m_daemonize = false;
    m_debug = false;
    m_guard = false;
    m_topKeyEnable = false;
    m_isTwemproxyMode = false;
    m_hashMappingList = new HashMappingList;
    m_keyMappingList = new KeyMappingList;
    m_groupInfo = new GroupInfoList;
}

CRedisProxyCfg::~CRedisProxyCfg() {
    if(NULL != m_operateXmlPointer) delete m_operateXmlPointer;
    delete m_hashMappingList;
    delete m_keyMappingList;
    delete m_groupInfo;
}

CRedisProxyCfg *CRedisProxyCfg::instance()
{
    static CRedisProxyCfg* cfg = NULL;
    if (cfg == NULL) {
        cfg = new CRedisProxyCfg;
    }
    return cfg;
}

void CRedisProxyCfg::set_groupName(CGroupInfo& group, const char* name) {
    int size = strlen(name);
    memcpy(group.m_groupName, name, size+1);
}

void CRedisProxyCfg::set_hashMin(CGroupInfo& group, int num) {
    group.m_hashMin = num;
}

void CRedisProxyCfg::set_weight(CGroupInfo& group, unsigned int weight) {
    group.m_weight= weight;
}


void CRedisProxyCfg::set_hashMax(CGroupInfo& group, int num) {
    group.m_hashMax = num;
}

void CRedisProxyCfg::addHost(CGroupInfo& group, CHostInfo& p) {
    group.m_hosts.push_back(p);
}

void CRedisProxyCfg::set_groupAttribute(TiXmlAttribute *groupAttr, CGroupInfo& pGroup) {
    for (; groupAttr != NULL; groupAttr = groupAttr->Next()) {
        const char* name = groupAttr->Name();
        const char* value = groupAttr->Value();
        if (value == NULL) value = "";
        if (0 == strcasecmp(name, "hash_min")) {
            set_hashMin(pGroup, atoi(value));
            continue;
        }
        if (0 == strcasecmp(name, "hash_max")) {
            set_hashMax(pGroup, atoi(value));
            continue;
        }
        if (0 == strcasecmp(name, "weight")) {
            set_weight(pGroup, (unsigned int)atoi(value));
            continue;
        }

        if (0 == strcasecmp(name, "name")) {
            set_groupName(pGroup, value);
            continue;
        }
        if (0 == strcasecmp(name, "policy")) {
            memcpy(pGroup.m_groupPolicy, value, strlen(value)+1);
            continue;
        }
    }
}

void CRedisProxyCfg::set_hostAttribute(TiXmlAttribute *addrAttr, CHostInfo& pHostInfo) {
     for (; addrAttr != NULL; addrAttr = addrAttr->Next()) {
        const char* name = addrAttr->Name();
        const char* value = addrAttr->Value();
        if (value == NULL) value = "";
        if (0 == strcasecmp(name, "host_name")) {
            string s = value;
            pHostInfo.set_hostName(s);
            continue;
        }
        if (0 == strcasecmp(name, "ip")) {
            string s = value;
            pHostInfo.set_ip(s);
            continue;
        }
        if (0 == strcasecmp(name, "connection_num")) {
            pHostInfo.set_connectionNum(atoi(value));
            continue;
        }
        if (0 == strcasecmp(name, "port")) {
            pHostInfo.set_port(atoi(value));
            continue;
        }
        if (0 == strcasecmp(name, "policy")) {
            pHostInfo.set_policy(atoi(value));
            continue;
        }
        if (0 == strcasecmp(name, "priority")) {
            pHostInfo.set_priority(atoi(value));
            continue;
        }
        if (0 == strcasecmp(name, "master")) {
            pHostInfo.set_master((atoi(value) != 0));
            continue;
        }
        if (0 == strcasecmp(name, "password")) {
            pHostInfo.set_passWord(value);
            continue;
        }
    }
}

void CRedisProxyCfg::set_hostEle(TiXmlElement* hostContactEle, CHostInfo& hostInfo) {
    for (; hostContactEle != NULL; hostContactEle = hostContactEle->NextSiblingElement()) {
        const char* strValue;
        strValue = hostContactEle->Value();
        if (strValue == NULL) strValue = "";

        const char* strText;
        strText = hostContactEle->GetText();
        if (strText == 0) strText = "";

        if (0 == strcasecmp(strValue, "ip")) {
            string s = strText;
            hostInfo.set_ip(s);
            continue;
        }
        if (0 == strcasecmp(strValue, "connection_num")) {
            if (hostContactEle->GetText() != NULL)
                hostInfo.set_connectionNum(atoi(strText));
            continue;
        }
        if (0 == strcasecmp(strValue, "port")) {
            hostInfo.set_port(atoi(strText));
            continue;
        }
        if (0 == strcasecmp(strValue, "policy")) {
            hostInfo.set_policy(atoi(strText));
            continue;
        }
        if (0 == strcasecmp(strValue, "host_name")) {
            string s = strText;
            hostInfo.set_hostName(s);
            continue;
        }
        if (0 == strcasecmp(strValue, "priority")) {
            hostInfo.set_priority(atoi(strText));
            continue;
        }
        if (0 == strcasecmp(strValue, "master")) {
            hostInfo.set_master((atoi(strText) != 0));
            continue;
        }
        if (0 == strcasecmp(strValue, "password")) {
            hostInfo.set_passWord(strText);
            continue;
        }
    }
}

void CRedisProxyCfg::getRootAttr(const TiXmlElement* pRootNode) {
    TiXmlAttribute *addrAttr = (TiXmlAttribute *)pRootNode->FirstAttribute();
    for (; addrAttr != NULL; addrAttr = addrAttr->Next()) {
        const char* name = addrAttr->Name();
        const char* value = addrAttr->Value();
        if (value == NULL) value = "";
        if (0 == strcasecmp(name, "thread_num")) {
            m_threadNum = atoi(value);
            continue;
        }
        if (0 == strcasecmp(name, "port")) {
            m_port = atoi(value);
            continue;
        }
        if (0 == strcasecmp(name, "hash_value_max")) {
            m_hashInfo.hash_value_max = atoi(value);
            continue;
        }
        if (0 == strcasecmp(name, "log_file")) {
            strcpy(m_logFile, value);
            continue;
        }
        if (0 == strcasecmp(name, "pid_file")) {
            strcpy(m_pidFile, value);
            continue;
        }
        if (0 == strcasecmp(name, "password")) {
            m_password = value;
            continue;
        }
        if (0 == strcasecmp(name, "hash")) {
            m_hashFunction = value;
            continue;
        }
        if (0 == strcasecmp(name, "daemonize")) {
            if(strcasecmp(value, "0") != 0 && strcasecmp(value, "") != 0 ) {
                m_daemonize = true;
            }
            continue;
        }
        if (0 == strcasecmp(name, "debug")) {
            if(strcasecmp(value, "0") != 0 && strcasecmp(value, "") != 0 ) {
                m_debug = true;
            }
            continue;
        }
        if (0 == strcasecmp(name, "twemproxy_mode")) {
            if(strcasecmp(value, "0") != 0 && strcasecmp(value, "") != 0 ) {
                m_isTwemproxyMode = true;
            }
            continue;
        }
        if (0 == strcasecmp(name, "guard")) {
            if(strcasecmp(value, "0") != 0 && strcasecmp(value, "") != 0) {
                m_guard = true;
            }
            continue;
        }
    }
}

void CRedisProxyCfg::getVipAttr(const TiXmlElement* vidNode) {
    TiXmlAttribute *addrAttr = (TiXmlAttribute *)vidNode->FirstAttribute();
    for (; addrAttr != NULL; addrAttr = addrAttr->Next()) {
        const char* name = addrAttr->Name();
        const char* value = addrAttr->Value();
        if (value == NULL) value = "";
        if (0 == strcasecmp(name, "if_alias_name")) {
            strcpy(m_vip.if_alias_name, value);
            continue;
        }
        if (0 == strcasecmp(name, "vip_address")) {
            strcpy(m_vip.vip_address, value);
            continue;
        }
        if (0 == strcasecmp(name, "enable")) {
            if(strcasecmp(value, "0") != 0 && strcasecmp(value, "") != 0 ) {
                m_vip.enable = true;
            }
        }
    }
}

void CRedisProxyCfg::setHashMappingNode(TiXmlElement* pNode) {
    TiXmlElement* pNext = pNode->FirstChildElement();
    for (; pNext != NULL; pNext = pNext->NextSiblingElement()) {
        CHashMapping hashMap;
        if (0 == strcasecmp(pNext->Value(), "hash")) {
            TiXmlAttribute *addrAttr = pNext->FirstAttribute();
            for (; addrAttr != NULL; addrAttr = addrAttr->Next()) {
                const char* name = addrAttr->Name();
                const char* value = addrAttr->Value();
                if (value == NULL) value = "";
                if (0 == strcasecmp(name, "value")) {
                    hashMap.hash_value = atoi(value);
                    continue;
                }
                if (0 == strcasecmp(name, "group_name")) {
                    strcpy(hashMap.group_name, value);
                }
            }
            m_hashMappingList->push_back(hashMap);
        }
    }
}


void CRedisProxyCfg::setGroupOption(const TiXmlElement* vidNode) {
    TiXmlAttribute *addrAttr = (TiXmlAttribute *)vidNode->FirstAttribute();
    for (; addrAttr != NULL; addrAttr = addrAttr->Next()) {
        const char* name = addrAttr->Name();
        const char* value = addrAttr->Value();
        if (value == NULL) value = "";
        if (0 == strcasecmp(name, "backend_retry_interval")) {
            m_groupOption.backend_retry_interval = atoi(value);
            continue;
        }
        if (0 == strcasecmp(name, "backend_retry_limit")) {
            m_groupOption.backend_retry_limit = atoi(value);
            continue;
        }
        if (0 == strcasecmp(name, "group_retry_time")) {
            m_groupOption.group_retry_time = atoi(value);
            continue;
        }

        if (0 == strcasecmp(name, "auto_eject_group")) {
            if(strcasecmp(value, "0") != 0 && strcasecmp(value, "") != 0 ) {
                m_groupOption.auto_eject_group = true;
                continue;
            }
        }

        if (0 == strcasecmp(name, "eject_after_restore")) {
            if(strcasecmp(value, "0") != 0 && strcasecmp(value, "") != 0 ) {
                m_groupOption.eject_after_restore = true;
            }
        }
    }
}



void CRedisProxyCfg::setKeyMappingNode(TiXmlElement* pNode) {
    TiXmlElement* pNext = pNode->FirstChildElement();
    for (; pNext != NULL; pNext = pNext->NextSiblingElement()) {
        CKeyMapping hashMap;
        if (0 == strcasecmp(pNext->Value(), "key")) {
            TiXmlAttribute *addrAttr = pNext->FirstAttribute();
            for (; addrAttr != NULL; addrAttr = addrAttr->Next()) {
                const char* name = addrAttr->Name();
                const char* value = addrAttr->Value();
                if (value == NULL) value = "";
                if (0 == strcasecmp(name, "key_name")) {
                    strcpy(hashMap.key, value);
                    continue;
                }
                if (0 == strcasecmp(name, "group_name")) {
                    strcpy(hashMap.group_name, value);
                }
            }
            m_keyMappingList->push_back(hashMap);
        }
    }
}

void CRedisProxyCfg::getGroupNode(TiXmlElement* pNode) {
    CGroupInfo groupTmp;
    set_groupAttribute(pNode->FirstAttribute(), groupTmp);
    TiXmlElement* pNext = pNode->FirstChildElement();
    for (; pNext != NULL; pNext = pNext->NextSiblingElement()) {
        if (0 == strcasecmp(pNext->Value(), "hash_min")) {
            if (NULL == pNext->GetText()) {
                set_hashMin(groupTmp, 0);
                continue;
            }
            set_hashMin(groupTmp, atoi(pNext->GetText()));
            continue;
        }
        if (0 == strcasecmp(pNext->Value(), "hash_max")) {
            if (NULL == pNext->GetText()) {
                set_hashMax(groupTmp, 0);
                continue;
            }
            set_hashMax(groupTmp, atoi(pNext->GetText()));
            continue;
        }

        if (0 == strcasecmp(pNext->Value(), "weight")) {
            if (NULL == pNext->GetText()) {
                set_weight(groupTmp, 0);
                continue;
            }
            set_weight(groupTmp, (unsigned int)atoi(pNext->GetText()));
            continue;
        }

        if (0 == strcasecmp(pNext->Value(), "name")) {
            set_groupName(groupTmp, pNext->GetText());
            continue;
        }

        if (0 == strcasecmp(pNext->Value(), "host")) {
            CHostInfo hostInfo;
            set_hostAttribute(pNext->FirstAttribute(), hostInfo);
            TiXmlElement* hostContactEle = (TiXmlElement* )pNext->FirstChildElement();
            set_hostEle(hostContactEle, hostInfo);
            addHost(groupTmp, hostInfo);
        }
    }
    m_groupInfo->push_back(groupTmp);
}

bool GetNodePointerByName(
    TiXmlElement* pRootEle,
    const std::string &strNodeName,
    TiXmlElement* &Node)
{
    if (strNodeName==pRootEle->Value()) {
        Node = pRootEle;
        return true;
    }
    TiXmlElement* pEle = pRootEle;
    for (pEle = pRootEle->FirstChildElement(); pEle; pEle = pEle->NextSiblingElement()) {
        if(GetNodePointerByName(pEle,strNodeName,Node))
            return true;
    }

    return false;
}


bool CRedisProxyCfg::saveProxyLastState(RedisProxy* proxy) {
    TiXmlElement* pRootNode = (TiXmlElement*)m_operateXmlPointer->get_rootElement();
    string nodeHash = "hash_mapping";
    TiXmlElement* pOldHashMap;
    if (GetNodePointerByName(pRootNode, nodeHash, pOldHashMap)) {
        pRootNode->RemoveChild(pOldHashMap);
    }
    TiXmlElement hashMappingNode("hash_mapping");
    for (int i = 0; i < proxy->slotCount(); ++i) {
        TiXmlElement hashNode("hash");
        hashNode.SetAttribute("value", i);
        hashNode.SetAttribute("group_name", proxy->groupBySlot(i)->groupName());
        hashMappingNode.InsertEndChild(hashNode);
    }
    pRootNode->InsertEndChild(hashMappingNode);

    // key mapping
    string nodeKey = "key_mapping";
    TiXmlElement* pKey;
    if (GetNodePointerByName(pRootNode, nodeKey, pKey)) {
        pRootNode->RemoveChild(pKey);
    }
    TiXmlElement keyMappingNode("key_mapping");

    StringMap<RedisServantGroup*>& keyMapping = proxy->keyMapping();
    StringMap<RedisServantGroup*>::iterator it = keyMapping.begin();
    for (; it != keyMapping.end(); ++it) {
        String key = it->first;
        TiXmlElement keyNode("key");
        keyNode.SetAttribute("key_name", key.data());
        RedisServantGroup* group = it->second;
        if (group != NULL) {
            keyNode.SetAttribute("group_name", group->groupName());
        }
        keyMappingNode.InsertEndChild(keyNode);
    }
    pRootNode->InsertEndChild(keyMappingNode);

    // add time
    time_t now = time(NULL);
    char fileName[512];
    struct tm* current_time = localtime(&now);
    sprintf(fileName,"onecache%d%02d%02d%02d.xml",
        current_time->tm_year + 1900,
        current_time->tm_mon + 1,
        current_time->tm_mday,
        current_time->tm_hour);

    bool ok = m_operateXmlPointer->m_docPointer->SaveFile(fileName);
    return ok;
}


bool CRedisProxyCfg::loadCfg(const char* file) {
    if (!m_operateXmlPointer->xml_open(file)) return false;

    const TiXmlElement* pRootNode = m_operateXmlPointer->get_rootElement();
    getRootAttr(pRootNode);
    TiXmlElement* pNode = (TiXmlElement*)pRootNode->FirstChildElement();
    for (; pNode != NULL; pNode = pNode->NextSiblingElement()) {
        if (0 == strcasecmp(pNode->Value(), "thread_num")) {
            if (pNode->GetText() != NULL) m_threadNum = atoi(pNode->GetText());
            continue;
        }
        if (0 == strcasecmp(pNode->Value(), "port")) {
            if (pNode->GetText() != NULL) m_port = atoi(pNode->GetText());
            continue;
        }
        if (0 == strcasecmp(pNode->Value(), "vip")) {
            getVipAttr(pNode);
            continue;
        }

        if (0 == strcasecmp(pNode->Value(), "top_key")) {
            TiXmlAttribute *addrAttr = (TiXmlAttribute *)pNode->FirstAttribute();
            for (; addrAttr != NULL; addrAttr = addrAttr->Next()) {
                const char* name = addrAttr->Name();
                const char* value = addrAttr->Value();
                if (0 == strcasecmp(name, "enable")) {
                    if(strcasecmp(value, "0") != 0 && strcasecmp(value, "") != 0 ) {
                        m_topKeyEnable = true;
                    }
                }
            }
            continue;
        }

        if (0 == strcasecmp(pNode->Value(), "hash")) {
            TiXmlElement* pNext = pNode->FirstChildElement();
            if (NULL == pNext) continue;
            for (; pNext != NULL; pNext = pNext->NextSiblingElement()) {
                if (0 == strcasecmp(pNext->Value(), "hash_value_max")) {
                    if (pNext->GetText() == NULL) {
                        m_hashInfo.hash_value_max = 0;
                        continue;
                    }
                    m_hashInfo.hash_value_max = atoi(pNext->GetText());
                }
            }
            continue;
        }
        if (0 == strcasecmp(pNode->Value(), "group")) {
            getGroupNode(pNode);
            continue;
        }
        if (0 == strcasecmp(pNode->Value(), "hash_mapping")) {
            setHashMappingNode(pNode);
            continue;
        }
        if (0 == strcasecmp(pNode->Value(), "group_option")) {
            setGroupOption(pNode);
            continue;
        }
        if (0 == strcasecmp(pNode->Value(), "key_mapping")) {
            setKeyMappingNode(pNode);
            continue;
        }
    }

    return true;
}


bool CRedisProxyCfgChecker::isValid(CRedisProxyCfg* pCfg, const char*& errMsg)
{
    const int hash_value_max = pCfg->hashInfo()->hash_value_max;
    if (!pCfg->isTwemproxyMode()) {
        if (hash_value_max > REDIS_PROXY_HASH_MAX) {
            errMsg = "hash_value_max is not greater than 1024";
            return false;
        }
    }

    int port = pCfg->port();
    if (port <= 0 || port > 65535) {
        errMsg = "onecache's port is invalid";
        return false;
    }

    int thread_num = pCfg->threadNum();
    if (thread_num <= 0) {
        errMsg = "onecache's thread_num is invalid";
        return false;
    }

    bool barray[REDIS_PROXY_HASH_MAX] = {0};
    string groupNameBuf[512];
    int groupCnt_ = pCfg->groupCnt();
    for (int i = 0; i < groupCnt_; ++i) {
        const CGroupInfo* group = pCfg->group(i);
        if (0 == strlen(group->groupName())) {
            errMsg = "group name cannot be empty";
            return false;
        }
        bool empty = false;
        if (0 == strlen(group->groupPolicy())) {
            empty = true;
        }
        if (!empty) {
            if (0 != strcasecmp(group->groupPolicy(), POLICY_READ_BALANCE) &&
                0 != strcasecmp(group->groupPolicy(), POLICY_MASTER_ONLY))
            {
                errMsg = "group's policy is wrong, it should be  read_balance or master_only";
                return false;
            }
        }

        groupNameBuf[i] = group->groupName();
        for (int j = 0; j < i; ++j) {
            if (groupNameBuf[i] == groupNameBuf[j]) {
                errMsg = "having the same group name";
                return false;
            }
        }
        if (!pCfg->isTwemproxyMode()) {
            if (group->hashMin() > group->hashMax()) {
                errMsg = "hash_min > hash_max";
                return false;
            }
            for (int j = group->hashMin(); j <= group->hashMax(); ++j) {
                if (j < 0 || j > REDIS_PROXY_HASH_MAX) {
                    errMsg = "hash value is invalid";
                    return false;
                }
                if (j >= hash_value_max) {
                    errMsg = "hash value is out of range";
                    return false;
                }

                if (barray[j]) {
                    errMsg = "hash range error";
                    return false;
                }
                barray[j] = true;
            }
        }
    }
    if (!pCfg->isTwemproxyMode()) {
        for (int i = 0; i < hash_value_max; ++i) {
            if (!barray[i]) {
                errMsg = "hash values are not complete";
                return false;
            }
        }
    }

    const GroupOption* groupOp = pCfg->groupOption();
    if (groupOp->backend_retry_interval <= 0) {
        errMsg = "backend_retry_interval invalid";
        return false;
    }

    if (groupOp->backend_retry_limit <= 0) {
        errMsg = "backend_retry_limit invalid";
        return false;
    }

    if (groupOp->auto_eject_group) {
        if (groupOp->group_retry_time <= 0) {
            errMsg = "group_retry_time invalid";
            return false;
        }
    }

    int hashMapCnt = pCfg->hashMapCnt();
    for (int i = 0; i < hashMapCnt; ++i) {
        const CHashMapping* p = pCfg->hashMapping(i);
        if (p->hash_value < 0) {
            errMsg = "hash_mapping's value < 0";
            return false;
        }
        bool exist = false;
        for (int j = 0; j < groupCnt_; ++j) {
            if (groupNameBuf[j] == p->group_name) {
                exist = true;
                break;
            }
        }
        if (!exist) {
            errMsg = "hash_mapping's group name doesn't exist";
            return false;
        }
    }

    int keyMapCnt = pCfg->keyMapCnt();
    for (int i = 0; i < keyMapCnt; ++i) {
        const CKeyMapping* p = pCfg->keyMapping(i);
        std::string groupName = p->group_name;
        bool exist = false;
        for (int j = 0; j < groupCnt_; ++j) {
            if (groupNameBuf[j] == groupName) {
                exist = true;
                break;
            }
        }
        if (!exist) {
            errMsg = "key_mapping's group name doesn't exist";
            return false;
        }
    }

    return true;
}




