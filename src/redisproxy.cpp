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
    recvBufferParsedOffset = 0;
    sendBufferParsedOffset = 0;
    finishedState = 0;
    sendToRedisBytes = 0;
    requestServant = NULL;
    redisSocket = NULL;
    auth = false;
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

RedisProto::ParseState ClientPacket::continueToParseRecvBuffer(void)
{
    recvParseResult.reset();
    RedisProto::ParseState state;
    state = RedisProto::parse(recvBuff.data() + recvBufferParsedOffset,
                              recvBuff.size() - recvBufferParsedOffset,
                              &recvParseResult);
    if (state == RedisProto::ProtoOK) {
        recvBufferParsedOffset += recvParseResult.protoBuffLen;
    }
    return state;
}

RedisProto::ParseState ClientPacket::continueToParseSendBuffer(void)
{
    sendParseResult.reset();
    RedisProto::ParseState state;
    state = RedisProto::parse(sendBuff.data() + sendBufferParsedOffset,
                              sendBuff.size() - sendBufferParsedOffset,
                              &sendParseResult);
    if (state == RedisProto::ProtoOK) {
        sendBufferParsedOffset += sendParseResult.protoBuffLen;
    }
    return state;
}

void ClientPacket::defaultFinishedHandler(ClientPacket *packet, void *)
{
    switch (packet->finishedState) {
    case ClientPacket::Unknown:
        packet->sendBuff.append("-ERR unknown state\r\n");
        packet->server->writeReply(packet);
        break;
    case ClientPacket::ProtoNotSupport:
        packet->sendBuff.append("-ERR protocol not support\r\n");
        packet->server->writeReply(packet);
        break;
    case ClientPacket::WrongNumberOfArguments:
        packet->sendBuff.append("-ERR wrong number of arguments\r\n");
        packet->server->writeReply(packet);
        break;
    case ClientPacket::RequestFinished:
        packet->server->writeReply(packet);
        break;
    default:
        LOG(Logger::Debug, "Unknown state %d", packet->finishedState);
        packet->server->closeConnection(packet);
        break;
    }
}



static Monitor dummy;
RedisProxy::RedisProxy(void)
{
    m_monitor = &dummy;
    m_hashFunc = hashForBytes;
    m_slotCount = 0;
    m_slots = NULL;
    m_vipAddress[0] = 0;
    m_vipName[0] = 0;
    m_vipEnabled = false;
    m_groupRetryTime = 30;
    m_autoEjectGroup = false;
    m_ejectAfterRestoreEnabled = false;
    m_threadPoolRefCount = 0;
    m_proxyManager.setProxy(this);
    m_twemproxyMode = false;
}

RedisProxy::~RedisProxy(void)
{
    for (int i = 0; i < m_groups.size(); ++i) {
        RedisServantGroup* group = m_groups.at(i);
        delete group;
    }
}

bool RedisProxy::run(const HostAddress& addr)
{
    if (isRunning()) {
        return false;
    }

    if (m_vipEnabled) {
        TcpSocket sock = TcpSocket::createTcpSocket();
        LOG(Logger::Message, "Connect to VIP address(%s:%d)...", m_vipAddress, addr.port());
        if (!sock.connect(HostAddress(m_vipAddress, addr.port()))) {
            LOG(Logger::Message, "Set VIP address [%s,%s]...", m_vipName, m_vipAddress);
            int ret = NonPortable::setVipAddress(m_vipName, m_vipAddress, 0);
            LOG(Logger::Message, "Set VIP address return %d", ret);
        } else {
            m_vipSocket = sock;
            m_vipEvent.set(eventLoop(), sock.socket(), EV_READ, vipHandler, this);
            m_vipEvent.active();
        }
    }


    m_monitor->proxyStarted(this);

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
            LOG(Logger::Message, "Delete VIP address...");
            int ret = NonPortable::setVipAddress(m_vipName, m_vipAddress, 1);
            LOG(Logger::Message, "Delete VIP address return %d", ret);

            m_vipEvent.remove();
            m_vipSocket.close();
        }
    }
}

void RedisProxy::addRedisGroup(RedisServantGroup *group)
{
    if (group) {
        group->setGroupId(m_groups.size());
        m_groups.append(group);
    }
}

bool RedisProxy::setSlot(int n, RedisServantGroup *group)
{
    if (n >= 0 && n < m_slotCount) {
        m_slots[n].group = group;
        return true;
    }
    return false;
}

void RedisProxy::setSlotCount(int n)
{
    if (m_slots) {
        delete []m_slots;
    }
    m_slotCount = n;
    m_slots = new Slot[n];
    memset(m_slots, 0, sizeof(Slot) * n);
}

RedisServantGroup *RedisProxy::groupBySlot(int n) const
{
    if (n >= 0 && n < m_slotCount) {
        return m_slots[n].group;
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

    if (!m_twemproxyMode) {
        unsigned int hash = m_hashFunc(key, len);
        unsigned int idx = hash % m_slotCount;
        return m_slots[idx].group;
    } else {
        unsigned int hash = 0;
        if (len == 0 || m_groups.size() == 1) {
            hash = 0;
        } else {
            hash = m_hashFunc(key, len);
        }

        Slot *begin, *end, *left, *right, *middle;
        begin = left = m_slots;
        end = right = m_slots + m_slotCount;

        while (left < right) {
            middle = left + (right - left) / 2;
            if (middle->value < hash) {
                left = middle + 1;
            } else {
                right = middle;
            }
        }

        if (right == end) {
            right = begin;
        }

        return right->group;
    }
}

void RedisProxy::handleClientPacket(const char *key, int len, ClientPacket *packet)
{
    RedisServantGroup* group = mapToGroup(key, len);
    if (!group) {
        LOG(Logger::Debug, "Group is not available. mapToGroup return NULL");
        packet->sendBuff.append("-ERR group is not available\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
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
        LOG(Logger::Debug, "Redis backend is not available. findUsableServant return NULL");
        packet->sendBuff.append("-ERR backend is not available\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
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

    if (m_pwd.empty()) {
        packet->auth = true;
    } else {
        packet->auth = false;
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
    switch (packet->continueToParseRecvBuffer()) {
    case RedisProto::ProtoError:
        LOG(Logger::Debug, "Read client request: protocol error");
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
    RedisCommand* command = cmdtable->findCommand(cmd, len);
    if (command) {
        packet->commandType = command->type;
        if (command->type == RedisCommand::AUTH) {
            command->proc(packet, command->arg);
        } else {
            if (packet->auth) {
                command->proc(packet, command->arg);
            } else {
                packet->sendBuff.append("-NOAUTH Authentication required.\r\n");
                packet->setFinishedState(ClientPacket::RequestFinished);
            }
        }
    } else {
        LOG(Logger::Debug, "Command '%s' not support", String(cmd, len).data());
        packet->setFinishedState(ClientPacket::ProtoNotSupport);
    }
}

void RedisProxy::writeReply(Context *c)
{
    ClientPacket* packet = (ClientPacket*)c;
    if (!packet->isContinueToParseRecvBuffer()) {
        switch (packet->continueToParseRecvBuffer()) {
        case RedisProto::ProtoError:
            LOG(Logger::Debug, "Read client request: protocol error");
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
    packet->recvBufferParsedOffset = 0;
    packet->sendBufferParsedOffset = 0;
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
        LOG(Logger::Message, "Disconnected from VIP address. set VIP address...");
        int ret = NonPortable::setVipAddress(proxy->m_vipName, proxy->m_vipAddress, 0);
        LOG(Logger::Message, "Set VIP address return %d", ret);
        proxy->m_vipSocket.close();
    }
}
