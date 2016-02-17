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

#ifndef ONE_PROXY_CACHE_MONITOR
#define ONE_PROXY_CACHE_MONITOR

#include <map>
#include <string>
#include <time.h>

#include "redisservant.h"
#include "redisproxy.h"
#include "top-key.h"
#include "redis-proxy-config.h"

class CCommandRecorder {
public:
    CCommandRecorder();
    ~CCommandRecorder();
    inline void addCnt(int commandType);
    inline unsigned long long commandCount(int cmdType) const
    { return m_cmdCnt[cmdType]; }

    void reset(){  }
private:
     unsigned long long m_cmdCnt[RedisCommand::CMD_COUNT];
};


class CByteCounter
{
public:
    enum {
        KBSIZE = 1024,
        MBSIZE = 1048576,
        GBSIZE = 1073741824
    };
    CByteCounter();
    ~CByteCounter();

    void addBytes(int bytes);
    int queryKBRange(int left, int right);
    int queryMBRange(int left, int right);
    int queryGBRange(int left, int right);
private:
    int gb_array[1024];
    int mb_array[1024];
    int kb_array[1024];
};


class CTimming {
public:
    void timmingBegin();
    void timmingEnd();
    static char* toString(time_t t);
    static char* secondToString(int sec);
    time_t beginTime();
    time_t endTime();
private:
    time_t begin_time;
    time_t end_time;
};


class CIpUtil {
public:
    static char* int2ipstr (int ip);
};

class SClientRecorder
{
public:
    SClientRecorder();
    ~SClientRecorder();
    void reset(){}

    inline int clientIp() {return m_clientIp;}
    inline long requestNum() {return request_num;}
    inline long connectNum() {return connect_num;}
public:
    int m_clientIp;
    long request_num;
    long connect_num;
    CCommandRecorder commandRecorder;
    CByteCounter valueInfo;
    CTimming connectInfo;
    friend class CProxyMonitor;
};

class CRedisRecorder
{
public:
    CRedisRecorder();
    ~CRedisRecorder();
    void reset(){}
    inline unsigned long long allRequestTimes(){ return m_AllRequestTimes; }
    inline unsigned long long requestSuccTimes(){ return m_requestSuccTimes; }
    inline unsigned long long requestFailTimes(){ return m_requestFailTimes; }
    inline unsigned long long allRequestSize(){ return m_allRequestSize; }
    inline unsigned long long allReplySize(){ return m_allReplySize; }

    inline void addAllReplySize(unsigned long size) { m_allReplySize += size;}
    inline void addAllRequestSize(unsigned long size) { m_allRequestSize += size;}

public:
    bool m_active;
    unsigned long long m_AllRequestTimes;
    unsigned long long m_requestSuccTimes;
    unsigned long long m_requestFailTimes;
    unsigned long long m_allRequestSize;
    unsigned long long m_allReplySize;
    CCommandRecorder commandRecorder;
    CByteCounter     valueInfo;
};

class CProxyMonitor : public Monitor
{
public:
    CProxyMonitor();
    ~CProxyMonitor();
    typedef std::map<RedisServant*, CRedisRecorder> RedisRecorderMap;
    typedef std::map<int, SClientRecorder> ClientRecorderMap;

    virtual void proxyStarted(RedisProxy*);
    virtual void clientConnected(ClientPacket*);
    virtual void clientDisconnected(ClientPacket*);
    virtual void replyClientFinished(ClientPacket*);
public:
    time_t proxyBeginTime()              { return m_proxyBeginTime.beginTime(); }
    RedisRecorderMap* redisRecortMap()   { return &m_redisRecMap; }
    ClientRecorderMap* clientRecordMap() { return &m_clientRecMap; }
    RedisProxy* redisProxy()             { return m_redisProxy; }
    CTopKeyRecorderThread* topKeyRecorder()    { return &m_topKeyRecorderThread; }
public:
    bool m_topKeyEnable;
private:
    // for client
    ClientRecorderMap      m_clientRecMap;
    // for redis  backend
    RedisRecorderMap       m_redisRecMap;
    // for proxy self
    CTimming               m_proxyBeginTime;
    RedisProxy*            m_redisProxy;
    SpinLocker                 m_clientLock;
    SpinLocker                 m_backendLock;
    SpinLocker                 m_topKeyLock;

    CTopKeyRecorderThread  m_topKeyRecorderThread;
};


class CFileOperate {
public:
    static FILE* openFile(const char* path, const char* mode) {
        return fopen(path, mode);
    }
    static void formatString2File(const char* string, FILE* f){ fprintf(f,"%s",string); }
};

class CFormatMonitorToIoBuf {
public:
    CFormatMonitorToIoBuf(IOBuffer* pIobuf, int topKeyCnt = 20);
    ~CFormatMonitorToIoBuf(){}
    void formatProxyToIoBuf(CProxyMonitor& proxyMonirot);
    void formatClientsToIoBuf(CProxyMonitor& proxyMonirot);

    void formatTopKeyToIoBuf(CProxyMonitor& proxyMonirot);
    void formatTopValueToIoBuf(CProxyMonitor& proxyMonirot);

    IOBuffer*      m_iobuf;
    FILE*          m_pfile;
    CTopKeySorter  m_topKeySorter;
    int            m_topKeyCnt;
private:
    void formatServants(CProxyMonitor& m, RedisServantGroup* p);
    void formatOneServant(const char* p, const char* str, CProxyMonitor::RedisRecorderMap* mapRedisRec, RedisServant* servant);
};

class CShowMonitor {
public:
    static void showMonitorToIobuf(CFormatMonitorToIoBuf& formatMonitor, CProxyMonitor& monitor);
    static bool showMonitorToFile(const char* p, const char* mode, CFormatMonitorToIoBuf& f, CProxyMonitor& m);
};


#endif
