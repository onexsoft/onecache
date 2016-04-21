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

#ifndef REDISPROXY_H
#define REDISPROXY_H

//Application version
#define APP_VERSION "3.3-160421"
#define APP_NAME "OneCache"
#define APP_EXIT_KEY 10

#include <string>

#include "util/tcpserver.h"
#include "util/locker.h"

#include "command.h"
#include "redisproto.h"
#include "redisservantgroup.h"
#include "proxymanager.h"

class RedisConnection;
class RedisServant;
class RedisProxy;
class ClientPacket : public Context
{
public:
    enum State {
        Unknown = 0,
        ProtoNotSupport = 2,
        WrongNumberOfArguments = 3,
        RequestFinished = 4
    };

    ClientPacket(void);
    ~ClientPacket(void);

    void setFinishedState(State state);
    RedisProxy* proxy(void) const { return (RedisProxy*)server; }
    RedisProto::ParseState continueToParseRecvBuffer(void);
    RedisProto::ParseState continueToParseSendBuffer(void);
    bool isContinueToParseRecvBuffer(void) const
    { return (recvBufferParsedOffset == recvBuff.size()); }

    static void defaultFinishedHandler(ClientPacket *packet, void*);

    int finishedState;                              //Finished state
    void* finished_arg;                             //Finished function arg
    void (*finished_func)(ClientPacket*, void*);    //Finished notify function
    int commandType;                                //Current command type
    int recvBufferParsedOffset;                     //Current request buffer offset
    int sendBufferParsedOffset;                     //Current reply buffer offset
    RedisProtoParseResult recvParseResult;          //Request parse result
    RedisProtoParseResult sendParseResult;          //Reply parse result
    int sendToRedisBytes;                           //Send to redis bytes
    RedisServant* requestServant;                   //Object of request
    RedisConnection* redisSocket;                   //Redis socket
    bool auth;
};

class Monitor
{
public:
    Monitor(void) {}
    virtual ~Monitor(void) {}

    virtual void proxyStarted(RedisProxy*) {}
    virtual void clientConnected(ClientPacket*) {}
    virtual void clientDisconnected(ClientPacket*) {}
    virtual void replyClientFinished(ClientPacket*) {}
};


struct Slot {
    RedisServantGroup* group;
    unsigned int value;
};

class RedisProxy : public TcpServer
{
public:
    RedisProxy(void);
    ~RedisProxy(void);

public:
    void setEventLoopThreadPool(EventLoopThreadPool* pool)
    { m_eventLoopThreadPool = pool; }

    EventLoopThreadPool* eventLoopThreadPool(void)
    { return m_eventLoopThreadPool; }

    void setTwemproxyModeEnabled(bool b) { m_twemproxyMode = b; }
    bool twemproxyEnabled(void) const { return m_twemproxyMode; }
    bool vipEnabled(void) const { return m_vipEnabled; }
    const char* vipName(void) const { return m_vipName; }
    const char* vipAddress(void) const { return m_vipAddress; }
    void setVipName(const char* name) { strcpy(m_vipName, name); }
    void setVipAddress(const char* address) { strcpy(m_vipAddress, address); }
    void setVipEnabled(bool b) { m_vipEnabled = b; }
    void setGroupRetryTime(int seconds) { m_groupRetryTime = seconds; }
    void setAutoEjectGroupEnabled(bool b) { m_autoEjectGroup = b; }
    void setEjectAfterRestoreEnabled(bool b)
    { m_ejectAfterRestoreEnabled = b; }

    void setMonitor(Monitor* monitor) { m_monitor = monitor; }
    Monitor* monitor(void) const { return m_monitor; }

    void setPassword(const std::string& pwd) { m_pwd = pwd; }
    const std::string password(void) const { return m_pwd; }

    bool run(const HostAddress &addr);
    void stop(void);

    void addRedisGroup(RedisServantGroup* group);
    bool setSlot(int n, RedisServantGroup* group);
    void setHashFunction(HashFunc func) { m_hashFunc = func; }
    void setSlotCount(int n);

    HashFunc hashFunction(void) const { return m_hashFunc; }
    Slot* slotData(void) { return m_slots; }
    int slotCount(void) const { return m_slotCount; }
    RedisServantGroup* groupBySlot(int n) const;

    int groupCount(void) const { return m_groups.size(); }
    RedisServantGroup* group(int index) const { return m_groups.at(index); }
    RedisServantGroup* group(const char* name) const;
    RedisServantGroup* mapToGroup(const char* key, int len);
    void handleClientPacket(const char* key, int len, ClientPacket* packet);

    bool addGroupKeyMapping(const char* key, int len, RedisServantGroup* group);
    void removeGroupKeyMapping(const char* key, int len);

    StringMap<RedisServantGroup*>& keyMapping(void) { return m_keyMapping; }

    virtual Context* createContextObject(void);
    virtual void destroyContextObject(Context* c);
    virtual void closeConnection(Context* c);
    virtual void clientConnected(Context* c);
    virtual ReadStatus readingRequest(Context* c);
    virtual void readRequestFinished(Context* c);
    virtual void writeReply(Context* c);
    virtual void writeReplyFinished(Context* c);

private:
    static void vipHandler(socket_t, short, void*);

private:
    bool m_twemproxyMode;
    Monitor* m_monitor;
    HashFunc m_hashFunc;
    int m_slotCount;
    Slot* m_slots;
    Vector<RedisServantGroup*> m_groups;
    TcpSocket m_vipSocket;
    char m_vipName[256];
    char m_vipAddress[256];
    Event m_vipEvent;
    bool m_vipEnabled;
    int  m_groupRetryTime;
    bool m_autoEjectGroup;
    bool m_ejectAfterRestoreEnabled;
    StringMap<RedisServantGroup*> m_keyMapping;
    unsigned int m_threadPoolRefCount;
    EventLoopThreadPool* m_eventLoopThreadPool;
    Mutex m_groupMutex;
    ProxyManager m_proxyManager;
    std::string m_pwd;

private:
    RedisProxy(const RedisProxy&);
    RedisProxy& operator =(const RedisProxy&);
};

#endif
