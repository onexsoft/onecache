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
#include "command.h"
#include "cmdhandler.h"
#include "non-portable.h"
#include "redisservant.h"
#include "redisproxy.h"
#include "redis-proxy-config.h"

ClientPacket::ClientPacket(void)
{
    commandType = -1;
    recvBufferOffset = 0;
    sendBufferOffset = 0;
    finishedState = 0;
    sendToRedisBytes = 0;
    requestServant = NULL;
    redisSocket = NULL;
    finished_func = defaultFinishedHandler;
}

ClientPacket::~ClientPacket(void)
{
}

void ClientPacket::setFinishedState(ClientPacket::State state)
{
    finishedState = state;
    finished_func(this, finished_arg);
}

RedisProto::ParseState ClientPacket::parseRecvBuffer(void)
{
    recvParseResult.reset();
    RedisProto::ParseState state = RedisProto::parse(recvBuff.data() + recvBufferOffset,
                                                     recvBuff.size() - recvBufferOffset,
                                                     &recvParseResult);
    if (state == RedisProto::ProtoOK) {
        recvBufferOffset += recvParseResult.protoBuffLen;
    }
    return state;
}

RedisProto::ParseState ClientPacket::parseSendBuffer(void)
{
    sendParseResult.reset();
    RedisProto::ParseState state = RedisProto::parse(sendBuff.data() + sendBufferOffset,
                                                     sendBuff.size() - sendBufferOffset,
                                                     &sendParseResult);
    if (state == RedisProto::ProtoOK) {
        sendBufferOffset += sendParseResult.protoBuffLen;
    }
    return state;
}

void ClientPacket::defaultFinishedHandler(ClientPacket *packet, void *)
{
    switch (packet->finishedState) {
    case ClientPacket::Unknown:
        packet->sendBuff.append("-Unknown state\r\n");
        break;
    case ClientPacket::ProtoError:
        packet->sendBuff.append("-Proto error\r\n");
        break;
    case ClientPacket::ProtoNotSupport:
        packet->sendBuff.append("-Proto not support\r\n");
        break;
    case ClientPacket::WrongNumberOfArguments:
        packet->sendBuff.append("-Wrong number of arguments\r\n");
        break;
    case ClientPacket::RequestError:
        packet->sendBuff.append("-Request error\r\n");
        break;
    case ClientPacket::RequestFinished:
        break;
    default:
        break;
    }
    packet->server->writeReply(packet);
}



static Monitor dummy;
RedisProxy::RedisProxy(void)
{
    m_monitor = &dummy;
    m_hashFunc = hashForBytes;
    m_maxHashValue = DefaultMaxHashValue;
    for (int i = 0; i < MaxHashValue; ++i) {
        m_hashMapping[i] = NULL;
    }
    m_vipAddress[0] = 0;
    m_vipName[0] = 0;
    m_vipEnabled = false;
    m_groupRetryTime = 30;
    m_autoEjectGroup = false;
    m_ejectAfterRestoreEnabled = false;
    m_threadPoolRefCount = 0;
    m_proxyManager.setProxy(this);
}

RedisProxy::~RedisProxy(void)
{
    for (int i = 0; i < m_groups.size(); ++i) {
        delete m_groups.at(i);
    }
}

bool RedisProxy::run(const HostAddress& addr)
{
    if (isRunning()) {
        return false;
    }

    if (m_vipEnabled) {
        TcpSocket sock = TcpSocket::createTcpSocket();
        Logger::log(Logger::Message, "connect to vip address(%s:%d)...", m_vipAddress, addr.port());
        if (!sock.connect(HostAddress(m_vipAddress, addr.port()))) {
            Logger::log(Logger::Message, "set VIP [%s,%s]...", m_vipName, m_vipAddress);
            int ret = NonPortable::setVipAddress(m_vipName, m_vipAddress, 0);
            Logger::log(Logger::Message, "set_vip_address return %d", ret);
        } else {
            m_vipSocket = sock;
            m_vipEvent.set(eventLoop(), sock.socket(), EV_READ, vipHandler, this);
            m_vipEvent.active();
        }
    }


    m_monitor->proxyStarted(this);
    Logger::log(Logger::Message, "Start the %s on port %d", APP_NAME, addr.port());

    RedisCommand cmds[] = {
        {"HASHMAPPING", 11, -1, onHashMapping, NULL},
        {"ADDKEYMAPPING", 13, -1, onAddKeyMapping, NULL},
        {"DELKEYMAPPING", 13, -1, onDelKeyMapping, NULL},
        {"SHOWMAPPING", 11, -1, onShowMapping, NULL},
        {"POOLINFO", 8, -1, onPoolInfo, NULL},
        {"SHUTDOWN", 8, -1, onShutDown, this}
    };
    RedisCommandTable::instance()->registerCommand(cmds, sizeof(cmds)/sizeof(RedisCommand));

    return TcpServer::run(addr);
}

void RedisProxy::stop(void)
{
    TcpServer::stop();
    if (m_vipEnabled) {
        if (!m_vipSocket.isNull()) {
            Logger::log(Logger::Message, "delete vip...");
            int ret = NonPortable::setVipAddress(m_vipName, m_vipAddress, 1);
            Logger::log(Logger::Message, "delete vip return %d", ret);

            m_vipEvent.remove();
            m_vipSocket.close();
        }
    }
    Logger::log(Logger::Message, "%s has stopped", APP_NAME);
}

void RedisProxy::addRedisGroup(RedisServantGroup *group)
{
    if (group) {
        group->setGroupId(m_groups.size());
        m_groups.append(group);
    }
}

bool RedisProxy::setGroupMappingValue(int hashValue, RedisServantGroup *group)
{
    if (hashValue >= 0 && hashValue < MaxHashValue) {
        m_hashMapping[hashValue] = group;
        return true;
    }
    return false;
}

RedisServantGroup *RedisProxy::hashForGroup(int hashValue) const
{
    if (hashValue >= 0 && hashValue < m_maxHashValue) {
        return m_hashMapping[hashValue];
    }
    return NULL;
}

RedisServantGroup *RedisProxy::group(const char *name) const
{
    for (int i = 0; i < groupCount(); ++i) {
        RedisServantGroup* p = group(i);
        if (strcmp(name, p->groupName()) == 0) {
            return p;
        }
    }
    return NULL;
}

RedisServantGroup *RedisProxy::mapToGroup(const char* key, int len)
{
    if (!m_keyMapping.empty()) {
        String _key(key, len, false);
        StringMap<RedisServantGroup*>::iterator it = m_keyMapping.find(_key);
        if (it != m_keyMapping.end()) {
            return it->second;
        }
    }

    unsigned int hash_val = m_hashFunc(key, len);
    unsigned int idx = hash_val % m_maxHashValue;
    return m_hashMapping[idx];
}

void RedisProxy::handleClientPacket(const char *key, int len, ClientPacket *packet)
{
    RedisServantGroup* group = mapToGroup(key, len);
    if (!group) {
        packet->setFinishedState(ClientPacket::RequestError);
        return;
    }
    RedisServant* servant = group->findUsableServant(packet);
    if (servant) {
        servant->handle(packet);
    } else {
        if (m_autoEjectGroup) {
            m_groupMutex.lock();
            m_proxyManager.setGroupTTL(group, m_groupRetryTime, m_ejectAfterRestoreEnabled);
            m_groupMutex.unlock();
        }
        packet->setFinishedState(ClientPacket::RequestError);
    }
}

bool RedisProxy::addGroupKeyMapping(const char *key, int len, RedisServantGroup *group)
{
    if (key && len > 0 && group) {
        m_keyMapping.insert(StringMap<RedisServantGroup*>::value_type(String(key, len, true), group));
        return true;
    }
    return false;
}

void RedisProxy::removeGroupKeyMapping(const char *key, int len)
{
    if (key && len > 0) {
        m_keyMapping.erase(String(key, len));
    }
}


Context *RedisProxy::createContextObject(void)
{
    ClientPacket* packet = new ClientPacket;

    EventLoop* loop;
    if (m_eventLoopThreadPool) {
        int threadCount = m_eventLoopThreadPool->size();
        EventLoopThread* loopThread = m_eventLoopThreadPool->thread(m_threadPoolRefCount % threadCount);
        ++m_threadPoolRefCount;
        loop = loopThread->eventLoop();
    } else {
        loop = eventLoop();
    }

    packet->eventLoop = loop;
    return packet;
}

void RedisProxy::destroyContextObject(Context *c)
{
    delete c;
}

void RedisProxy::closeConnection(Context* c)
{
    ClientPacket* packet = (ClientPacket*)c;
    m_monitor->clientDisconnected(packet);
    TcpServer::closeConnection(c);
}

void RedisProxy::clientConnected(Context* c)
{
    ClientPacket* packet = (ClientPacket*)c;
    m_monitor->clientConnected(packet);
}

TcpServer::ReadStatus RedisProxy::readingRequest(Context *c)
{
    ClientPacket* packet = (ClientPacket*)c;
    switch (packet->parseRecvBuffer()) {
    case RedisProto::ProtoError:
        return ReadError;
    case RedisProto::ProtoIncomplete:
        return ReadIncomplete;
    case RedisProto::ProtoOK:
        return ReadFinished;
    default:
        return ReadError;
    }
}

void RedisProxy::readRequestFinished(Context *c)
{
    ClientPacket* packet = (ClientPacket*)c;
    RedisProtoParseResult& r = packet->recvParseResult;
    char* cmd = r.tokens[0].s;
    int len = r.tokens[0].len;

    RedisCommandTable* cmdtable = RedisCommandTable::instance();
    cmdtable->execCommand(cmd, len, packet);
}

void RedisProxy::writeReply(Context *c)
{
    ClientPacket* packet = (ClientPacket*)c;
    if (!packet->isRecvParseEnd()) {
        switch (packet->parseRecvBuffer()) {
        case RedisProto::ProtoError:
            closeConnection(c);
            break;
        case RedisProto::ProtoIncomplete:
            waitRequest(c);
            break;
        case RedisProto::ProtoOK:
            readRequestFinished(c);
            break;
        default:
            break;
        }
    } else {
        TcpServer::writeReply(c);
    }
}

void RedisProxy::writeReplyFinished(Context *c)
{
    ClientPacket* packet = (ClientPacket*)c;
    m_monitor->replyClientFinished(packet);
    packet->finishedState = ClientPacket::Unknown;
    packet->commandType = -1;
    packet->sendBuff.clear();
    packet->recvBuff.clear();
    packet->sendBytes = 0;
    packet->recvBytes = 0;
    packet->sendToRedisBytes = 0;
    packet->requestServant = NULL;
    packet->redisSocket = NULL;
    packet->recvBufferOffset = 0;
    packet->sendBufferOffset = 0;
    packet->sendParseResult.reset();
    packet->recvParseResult.reset();
    waitRequest(c);
}

void RedisProxy::vipHandler(socket_t sock, short, void* arg)
{
    char buff[64];
    RedisProxy* proxy = (RedisProxy*)arg;
    int ret = ::recv(sock, buff, 64, 0);
    if (ret == 0) {
        Logger::log(Logger::Message, "disconnected from VIP. change vip address...");
        int ret = NonPortable::setVipAddress(proxy->m_vipName, proxy->m_vipAddress, 0);
        Logger::log(Logger::Message, "set_vip_address return %d", ret);
        proxy->m_vipSocket.close();
    }
}
