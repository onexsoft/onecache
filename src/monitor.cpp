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

#include "monitor.h"
#include <string.h>

#define STATUS          "STATUS"
#define OUTPUTSTATUS    "OUTPUTSTATUS"
#define TOPKEY          "TOPKEY"
#define TOPVALUE          "TOPVALUE"

CCommandRecorder::CCommandRecorder() {
    for (int i = 0; i < RedisCommand::CMD_COUNT; i++) {
        m_cmdCnt[i] = 0LL;
    }
}

CCommandRecorder::~CCommandRecorder(){}

void CCommandRecorder::addCnt(int commandType) {
    if (commandType >= 0 && commandType <= RedisCommand::CMD_COUNT)
        ++m_cmdCnt[commandType];
}

void CTimming::timmingBegin() {
    begin_time = time(NULL);
}

void CTimming::timmingEnd() {
    end_time = time(NULL);
}

char* CTimming::toString(time_t t)
{
    static char buf[50];
    memset(buf, 0, 50);
    struct tm* current_time = localtime(&t);
    sprintf(buf,"%d-%d-%d %02d:%02d:%02d",
        current_time->tm_year + 1900,
        current_time->tm_mon + 1,
        current_time->tm_mday,
        current_time->tm_hour,
        current_time->tm_min,
        current_time->tm_sec);
    return buf;
}

char* CTimming::secondToString(int sec)
{
    static char buf[50];
    memset(buf, 0, 50);
    int hour,min;
    hour = sec / 3600;
    min = (sec % 3600) / 60;
    sec = (sec % 3600) % 60;
    sprintf(buf, "%02d:%02d:%02d\n", hour, min, sec);
    return buf;
}

time_t CTimming::beginTime() {
    return begin_time;
}

time_t CTimming::endTime() {
    return end_time;
}


CByteCounter::CByteCounter() {
        for (int i = 0; i < 1024; i++) {
            gb_array[i] = 0;
            mb_array[i] = 0;
            kb_array[i] = 0;
        }
}

CByteCounter::~CByteCounter() {}

void CByteCounter::addBytes(int bytes) {
    if (bytes >= GBSIZE) {
        ++gb_array[bytes / GBSIZE];
    } else if (bytes >= MBSIZE) {
        ++mb_array[bytes / MBSIZE];
    } else if (bytes >= KBSIZE) {
        ++kb_array[bytes / KBSIZE];
    }
}

int CByteCounter::queryKBRange(int left, int right)
{
    int cnt = 0;
    for (int i = left; i <= right; ++i) {
        cnt += kb_array[i];
    }
    return cnt;
}

int CByteCounter::queryMBRange(int left, int right)
{
    int cnt = 0;
    for (int i = left; i <= right; ++i) {
        cnt += mb_array[i];
    }
    return cnt;
}

int CByteCounter::queryGBRange(int left, int right)
{
    int cnt = 0;
    for (int i = left; i <= right; ++i) {
        cnt += gb_array[i];
    }
    return cnt;
}


SClientRecorder::SClientRecorder() {
    request_num = 0;
    connect_num = 0;
}

SClientRecorder::~SClientRecorder() {}


CRedisRecorder::~CRedisRecorder() {}

CRedisRecorder::CRedisRecorder() {
    m_active = false;
    m_AllRequestTimes = 0LL;
    m_requestSuccTimes = 0LL;
    m_requestFailTimes = 0LL;
    m_allReplySize = 0LL;
    m_allRequestSize = 0LL;
}

CProxyMonitor::CProxyMonitor(){
    m_topKeyEnable = false;
}

CProxyMonitor::~CProxyMonitor(){}


void statusProc(ClientPacket* packet, void* arg) {
    IOBuffer& iobuffer = packet->sendBuff;
    CProxyMonitor* monitor = (CProxyMonitor*)arg;
    iobuffer.append("+");
    CFormatMonitorToIoBuf c(&iobuffer);
    CShowMonitor::showMonitorToIobuf(c, *monitor);

    iobuffer.append("\r\n");
    packet->setFinishedState(ClientPacket::RequestFinished);
}

void outPutStatusProc(ClientPacket* packet, void* arg) {
    IOBuffer& iobuffer = packet->sendBuff;
    CProxyMonitor* monitor = (CProxyMonitor*)arg;
    CFormatMonitorToIoBuf c(&iobuffer);
    if (!CShowMonitor::showMonitorToFile("onecache_monitor.log", "w", c, *monitor)) {
        iobuffer.clear();
        iobuffer.append("+load file failed\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return ;
    }
    iobuffer.clear();
    iobuffer.append("+output file ok\r\n");
    packet->setFinishedState(ClientPacket::RequestFinished);
}


void topValueProc(ClientPacket* packet, void* arg) {
    IOBuffer& iobuffer = packet->sendBuff;
    CProxyMonitor* monitor = (CProxyMonitor*)arg;
    iobuffer.append("+");
    RedisProtoParseResult& requestParseResult = packet->recvParseResult;
    if (requestParseResult.tokenCount < 2) {
        // default
        CFormatMonitorToIoBuf c(&iobuffer ,20);
        c.formatTopValueToIoBuf(*monitor);
        iobuffer.append("\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }
    Token& token = requestParseResult.tokens[1];
    char buf[20];
    strncpy(buf, token.s, token.len);
    int topNum = atoi(buf);
    CFormatMonitorToIoBuf c(&iobuffer ,topNum);
    c.formatTopValueToIoBuf(*monitor);
    iobuffer.append("\r\n");
    packet->setFinishedState(ClientPacket::RequestFinished);
}


void topKeyProc(ClientPacket* packet, void* arg) {
    IOBuffer& iobuffer = packet->sendBuff;
    CProxyMonitor* monitor = (CProxyMonitor*)arg;
    iobuffer.append("+");
    RedisProtoParseResult& requestParseResult = packet->recvParseResult;
    if (requestParseResult.tokenCount < 2) {
        // default
        CFormatMonitorToIoBuf c(&iobuffer ,20);
        c.formatTopKeyToIoBuf(*monitor);
        iobuffer.append("\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }
    Token& token = requestParseResult.tokens[1];
    char buf[20];
    strncpy(buf, token.s, token.len);
    int topNum = atoi(buf);
    CFormatMonitorToIoBuf c(&iobuffer ,topNum);
    c.formatTopKeyToIoBuf(*monitor);
    iobuffer.append("\r\n");
    packet->setFinishedState(ClientPacket::RequestFinished);
}


void CProxyMonitor::proxyStarted(RedisProxy* proxy) {
    m_proxyBeginTime.timmingBegin();
    m_redisProxy = proxy;

    // Register command
    RedisCommand status;
    strcpy(status.name, STATUS);
    status.len = strlen(STATUS);
    status.type = -1;
    status.proc = statusProc;
    status.arg = this;
    RedisCommandTable::instance()->registerCommand(&status, 1);

    RedisCommand outPutStatus;
    strcpy(outPutStatus.name, OUTPUTSTATUS);
    outPutStatus.len = strlen(OUTPUTSTATUS);
    outPutStatus.type = -1;
    outPutStatus.proc = outPutStatusProc;
    outPutStatus.arg = this;
    RedisCommandTable::instance()->registerCommand(&outPutStatus, 1);

    // top key
    RedisCommand topKey;
    strcpy(topKey.name, TOPKEY);
    topKey.len = strlen(TOPKEY);
    topKey.type = -1;
    topKey.proc = topKeyProc;
    topKey.arg = this;
    RedisCommandTable::instance()->registerCommand(&topKey, 1);

    // top value
    RedisCommand topValue;
    strcpy(topValue.name, TOPVALUE);
    topValue.len = strlen(TOPVALUE);
    topValue.type = -1;
    topValue.proc = topValueProc;
    topValue.arg = this;
    RedisCommandTable::instance()->registerCommand(&topValue, 1);

    m_topKeyEnable = CRedisProxyCfg::instance()->topKeyEnable();
    if (m_topKeyEnable) {
        m_topKeyRecorderThread.start();
    }
}

char* CIpUtil::int2ipstr (int ip) {
    static char buf[50];
    sprintf (buf, "%u.%u.%u.%u",
        (unsigned char) * ((char *) &ip + 0),
        (unsigned char )* ((char *) &ip + 1),
        (unsigned char )* ((char *) &ip + 2),
        (unsigned char) * ((char *) &ip + 3));
    return buf;
}

void CProxyMonitor::clientConnected(ClientPacket* packet) {
    int ip = packet->clientAddress._sockaddr()->sin_addr.s_addr;
    SClientRecorder& recorder = m_clientRecMap[ip];
    recorder.m_clientIp = ip;
    ++recorder.connect_num;
    recorder.connectInfo.timmingBegin();
}

void CProxyMonitor::clientDisconnected(ClientPacket* ) {
}


void CProxyMonitor::replyClientFinished(ClientPacket* packet) {
    int replySize = packet->sendBuff.size();
    if (m_topKeyEnable) {
        KeyStrValueSize keyInfo;
        char* key = packet->recvParseResult.tokens[1].s;
        int len = packet->recvParseResult.tokens[1].len;
        if (len > 0) {
            keyInfo.key = new char[len + 1];
            strncpy(keyInfo.key, key, len);
            keyInfo.key[len] = '\0';
            keyInfo.valueSize = replySize;
            m_topKeyLock.lock();
            m_topKeyRecorderThread.keyToList(keyInfo); // 2015/9/18 10:28:46
            m_topKeyLock.unlock();
        }
    }

    int ip = packet->clientAddress._sockaddr()->sin_addr.s_addr;
    // add client info
    m_clientLock.lock();
    SClientRecorder& clientRecorder = m_clientRecMap[ip];
    ++clientRecorder.request_num;
    clientRecorder.m_clientIp = ip;
    clientRecorder.commandRecorder.addCnt(packet->commandType);
    clientRecorder.valueInfo.addBytes(replySize);
    m_clientLock.unlock();

    RedisServant* pServant = packet->requestServant;
    if (pServant == NULL) {
        return;
    }

    // add backend info
    m_backendLock.lock();
    CRedisRecorder& redisRecorder = m_redisRecMap[pServant];
    redisRecorder.valueInfo.addBytes(replySize);
    ++redisRecorder.m_AllRequestTimes;
    redisRecorder.addAllReplySize(replySize);
    redisRecorder.addAllRequestSize(packet->recvBuff.size());
    redisRecorder.commandRecorder.addCnt(packet->commandType);
    m_backendLock.unlock();
}



CFormatMonitorToIoBuf::CFormatMonitorToIoBuf(IOBuffer* pIobuf, int topKeyCnt) {
    m_iobuf = pIobuf;
    m_topKeyCnt = topKeyCnt;
}

void CFormatMonitorToIoBuf::formatOneServant(
    const char* groupName,
    const char* str,
    CProxyMonitor::RedisRecorderMap* mapRedisRec,
    RedisServant* servant)
{
    m_iobuf->appendFormatString("%-10s%17s",groupName, servant->redisAddress().ip());
    m_iobuf->appendFormatString("%10d%11d%6s%8s",
                                servant->redisAddress().port(),
                                servant->option().poolSize,
                                str, servant->isActived()?"Y":"N");

    CProxyMonitor::RedisRecorderMap::iterator itMap = mapRedisRec->find(servant);
    if (itMap != mapRedisRec->end()) {
        m_iobuf->appendFormatString("%20lld", itMap->second.allRequestTimes());
        if (itMap->second.allRequestSize() > (1024*1024*1024)) {
            m_iobuf->appendFormatString("%17.2fGB", itMap->second.allRequestSize()/(1024*1024*(1024.0)));
        }
        else if (itMap->second.allRequestSize() > (1024*1024)) {
            m_iobuf->appendFormatString("%17.2fMB", itMap->second.allRequestSize()/(1024*(1024.0)));
        }
        else {
            m_iobuf->appendFormatString("%17.2fKB", itMap->second.allRequestSize()/(1024.0));
        }
        if (itMap->second.allReplySize() > (1024*1024*1024)) {
            m_iobuf->appendFormatString("%16.2fGB", itMap->second.allReplySize()/(1024*1024*(1024.0)));
        }
        else if (itMap->second.allReplySize() > (1024*1024)) {
            m_iobuf->appendFormatString("%16.2fMB", itMap->second.allReplySize()/(1024*(1024.0)));
        }
        else {
            m_iobuf->appendFormatString("%16.2fKB", itMap->second.allReplySize()/(1024.0));
        }
        CByteCounter& valueInfo = itMap->second.valueInfo;
        m_iobuf->appendFormatString("%14d", valueInfo.queryKBRange(1, 1023));
        m_iobuf->appendFormatString("%12d", valueInfo.queryMBRange(1, 1023));
        bool bSet = false;
        for (int i = 0; i < RedisCommand::CMD_COUNT; i++) {
            unsigned long long cnt = itMap->second.commandRecorder.commandCount(i);
            if (cnt > 0) {
                m_iobuf->appendFormatString("   [%s]:%ld ", RedisCommand::commandName(i), cnt);
                bSet = true;
            }
        }
        if (!bSet) {
            m_iobuf->appendFormatString("%7s","NULL\n");
        }
        m_iobuf->append("\n");
        return;
    } else {
        m_iobuf->appendFormatString("%20d%17dKB%16dKB%14d%12d%7s\n",0, 0, 0, 0, 0,"NULL");
        return;
    }
}

void CFormatMonitorToIoBuf::formatTopValueToIoBuf(CProxyMonitor& proxyMonirot) {
    CTopKeyRecorderThread* topKeyRec = proxyMonirot.topKeyRecorder();
    m_topKeySorter.sortTopKeyByValueSize(*topKeyRec);
    m_iobuf->appendFormatString("%-120s%s\n","[KEY]", "[VALUESIZE]");

    vector<pair <const char*, CTopKeyRecorderThread::KeyCntList::iterator> >::iterator it = m_topKeySorter.m_sortVectorKeyCnt->begin();
    int i = 0;
    for (; it != m_topKeySorter.m_sortVectorKeyCnt->end(); ++it) {
        if (i++ < m_topKeyCnt) {
            unsigned long valueSize = (it->second)->valueSize;
            if (valueSize > (1024*1024*1024)) {
                m_iobuf->appendFormatString("%-120s%-0.2fGB\n", it->first, valueSize/(1024*1024*(1024.0)));
            }
            else if (valueSize > (1024*1024)) {
                m_iobuf->appendFormatString("%-120s%-0.3fMB\n", it->first, valueSize/(1024*(1024.0)));
            }
            else {
                m_iobuf->appendFormatString("%-120s%-0.3fKB\n", it->first, valueSize/(1024.0));
            }
        }
    }
}

void CFormatMonitorToIoBuf::formatTopKeyToIoBuf(CProxyMonitor& proxyMonirot) {
    CTopKeyRecorderThread* topKeyRec = proxyMonirot.topKeyRecorder();
    m_topKeySorter.sortTopKeyByCnt(*topKeyRec);
    m_iobuf->appendFormatString("%-120s%s\n","[KEY]", "[COUNT]");

    vector<pair <const char*, CTopKeyRecorderThread::KeyCntList::iterator> >::iterator it = m_topKeySorter.m_sortVectorKeyCnt->begin();
    int i = 0;
    for (; it != m_topKeySorter.m_sortVectorKeyCnt->end(); ++it) {
        if (i++ < m_topKeyCnt) {
            m_iobuf->appendFormatString("%-120s%-17d\n", it->first, (it->second)->keyCnt);
        }
    }
}


void CFormatMonitorToIoBuf::formatServants(CProxyMonitor& proxyMonirot, RedisServantGroup* group) {
    CProxyMonitor::RedisRecorderMap* mapRedisRec = proxyMonirot.redisRecortMap();
    const char* groupName = group->groupName();
    for (int i = 0; i < group->masterCount(); i++) {
        formatOneServant(groupName, "Y", mapRedisRec, group->master(i));
    }
    for (int i = 0; i < group->slaveCount(); i++) {
        formatOneServant(groupName, "N", mapRedisRec, group->slave(i));
    }
}


void CFormatMonitorToIoBuf::formatProxyToIoBuf(CProxyMonitor& proxyMonirot) {
    m_iobuf->append("[OneCache]\n");
    m_iobuf->appendFormatString("Version=%s\n", APP_VERSION);
    m_iobuf->appendFormatString("Port=%d\n",proxyMonirot.redisProxy()->address().port());
    m_iobuf->appendFormatString("StartTime=%s\n", CTimming::toString(proxyMonirot.proxyBeginTime()));
    time_t now = time(NULL);
    time_t upTime = now - proxyMonirot.proxyBeginTime();
    m_iobuf->appendFormatString("UpTime=%s", CTimming::secondToString((int)upTime));
    RedisProxy* proxy = proxyMonirot.redisProxy();
    m_iobuf->appendFormatString("GroupCount=%d\n", proxy->groupCount());
    m_iobuf->appendFormatString("VipEnabled=%s\n", proxy->vipEnabled()?"Yes":"No");
    m_iobuf->appendFormatString("TopKeyEnabled=%s\n", CRedisProxyCfg::instance()->topKeyEnable()?"Yes":"No");
    if (proxyMonirot.redisProxy()->vipEnabled()) {
        m_iobuf->appendFormatString("VipName=%s\n", proxy->vipName());
        m_iobuf->appendFormatString("VipAddress=%s\n", proxy->vipAddress());
    }
    m_iobuf->append("\n[Backends]\n");
    int groupCnt = proxy->groupCount();
    m_iobuf->append("[GROUP]                [IP]    [PORT] [CONNPOOL] [MASTER] [ACTIVE]      [REQUESTS]        [RECV SIZE]       [SEND SIZE]    [SEND>1KB]  [SEND>1MB]   [COMMANDS]\n");
    for (int  i = 0; i < groupCnt; i++) {
        RedisServantGroup* group = proxy->group(i);
        formatServants(proxyMonirot, group);
    }
    m_iobuf->append("\n");
}

void CFormatMonitorToIoBuf::formatClientsToIoBuf(CProxyMonitor& proxyMonirot) {
    CProxyMonitor::ClientRecorderMap* cliRecMap = proxyMonirot.clientRecordMap();
    CProxyMonitor::ClientRecorderMap::iterator itCliMap = cliRecMap->begin();
    m_iobuf->append("[Clients]\n[NUM]               [IP]   [CONNECTS]  [REQUESTS]  [RECV>1KB]  [RECV>1MB]      [LAST CONNECT]   [COMMANDS]\n");
    for (long i = 1; itCliMap != cliRecMap->end(); ++itCliMap, ++i) {
        SClientRecorder& client = (itCliMap)->second;
        m_iobuf->appendFormatString("%-8ld%16s%13ld%12ld %11d %11d %19s",
                                    i,
                                    CIpUtil::int2ipstr(client.clientIp()),
                                    client.connectNum(),
                                    client.requestNum(),
                                    client.valueInfo.queryKBRange(1, 1023),
                                    client.valueInfo.queryMBRange(1, 1023),
                                    CTimming::toString(client.connectInfo.beginTime()));
        bool bSet = false;
        for (int i = 0; i < RedisCommand::CMD_COUNT; i++) {
            unsigned long long cnt = client.commandRecorder.commandCount(i);
            if (cnt > 0) {
                m_iobuf->appendFormatString("   [%s]:%ld ", RedisCommand::commandName(i), cnt);
                bSet = true;
            }
        }
        if (!bSet) {
            m_iobuf->appendFormatString("%8s","NULL");
        }
        m_iobuf->append("\n");
    }
}

void CShowMonitor::showMonitorToIobuf(CFormatMonitorToIoBuf& formatMonitor,CProxyMonitor& monitor) {
    formatMonitor.formatProxyToIoBuf(monitor);
    formatMonitor.formatClientsToIoBuf(monitor);
}

bool CShowMonitor::showMonitorToFile(
    const char* path,
    const char* mode,
    CFormatMonitorToIoBuf& formatMonitor,
    CProxyMonitor& monitor) {
    if ((formatMonitor.m_pfile = CFileOperate::openFile(path, mode))==NULL) {
        return false;
    }
    formatMonitor.formatProxyToIoBuf(monitor);
    formatMonitor.formatClientsToIoBuf(monitor);
    formatMonitor.m_iobuf->append("\0", 1);
    CFileOperate::formatString2File(formatMonitor.m_iobuf->data(), formatMonitor.m_pfile);
    fclose(formatMonitor.m_pfile);
    return true;
}

