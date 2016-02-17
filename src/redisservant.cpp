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
#include "redisproxy.h"
#include "redisservant.h"

RedisConnection::RedisConnection(void)
{
}

RedisConnection::~RedisConnection(void)
{
    disconnect();
}

bool RedisConnection::connect(const HostAddress& addr)
{
    timeval defaultVal;
    socketlen_t len = sizeof(timeval);
    TcpSocket sock = TcpSocket::createTcpSocket();
    if (sock.isNull()) {
        Logger::log(Logger::Error, "RedisConnection::connect: %s", strerror(errno));
        return false;
    }

    sock.option(SOL_SOCKET, SO_SNDTIMEO, (char*)&defaultVal, &len);

    timeval sndTimeout = {1, 0};
    sock.setOption(SOL_SOCKET, SO_SNDTIMEO, (char*)&sndTimeout, sizeof(timeval));
    if (!sock.connect(addr)) {
        Logger::log(Logger::Error, "RedisConnection::connect: %s", strerror(errno));
        sock.close();
        return false;
    }

    sock.setOption(SOL_SOCKET, SO_SNDTIMEO, (char*)&defaultVal, sizeof(timeval));
    sock.setNonBlocking();
    sock.setNoDelay();
    sock.setKeepAlive();
    m_socket = sock;
    return true;
}

void RedisConnection::disconnect(void)
{
    m_socket.close();
}




RedisConnectionPool::RedisConnectionPool(void)
{
    m_activeConnNums = 0;
    m_capacity = 0;
}

RedisConnectionPool::~RedisConnectionPool(void)
{
    close();
}

bool RedisConnectionPool::open(const HostAddress& addr, int capacity)
{
    close();
    Logger::log(Logger::Message, "Create connection pool (%s:%d)...",
                addr.ip(), addr.port());

    if (capacity <= 0) {
        Logger::log(Logger::Error, "Create failed: capacity parameter error");
        return false;
    }

    m_redisAddress = addr;
    m_capacity = capacity;

    for (int i = 0; i < m_capacity; ++i) {
        RedisConnection* sock = new RedisConnection;
        if (!sock->connect(addr)) {
            delete sock;
            close();
            return false;
        } else {
            m_pool.push_back(sock);
        }
    }
    Logger::log(Logger::Message, "Creating successful. nums: %d", m_pool.size());
    return true;
}

RedisConnection *RedisConnectionPool::select(void)
{
    m_locker.lock();
    RedisConnection* sock = m_pool.pop_back(NULL);
    if (sock) {
        ++m_activeConnNums;
    } else {
        if ((m_pool.size() + m_activeConnNums) < m_capacity) {
            sock = new RedisConnection;
            if (!sock->connect(m_redisAddress)) {
                delete sock;
                sock = NULL;
            } else {
                ++m_activeConnNums;
            }
        }
    }
    m_locker.unlock();
    return sock;
}

void RedisConnectionPool::unSelect(RedisConnection *sock)
{
    m_locker.lock();
    m_pool.push_back(sock);
    --m_activeConnNums;
    m_locker.unlock();
}

bool RedisConnectionPool::repairSocket(RedisConnection *sock)
{
    sock->disconnect();
    return sock->connect(m_redisAddress);
}

void RedisConnectionPool::free(RedisConnection *sock)
{
    delete sock;
    m_locker.lock();
    --m_activeConnNums;
    m_locker.unlock();
}

void RedisConnectionPool::close(void)
{
    m_locker.lock();
    while (1) {
        RedisConnection* sock = m_pool.pop_back(NULL);
        if (sock != NULL) {
            delete sock;
        } else {
            break;
        }
    }
    m_locker.unlock();
}



RedisServant::RedisServant(void)
{
    m_loop = NULL;
    m_reconnCount = 0;
    m_actived = false;
    m_reconnectEnabled = true;
}

RedisServant::~RedisServant(void)
{
    stop();

    if (m_connListener.isActived()) {
        m_connListener.disconnect();
        m_connEvent.remove();
    }
}

bool RedisServant::start(void)
{
    if (m_actived) {
        return true;
    }

    if (!m_connPool.open(m_redisAddress, m_option.poolSize)) {
        return false;
    }

    if (m_connListener.connect(m_redisAddress)) {
        m_connEvent.set(m_loop, m_connListener.m_socket.socket(), EV_READ, onDisconnected, this);
        m_connEvent.active();
    }
    m_actived = true;
    return true;
}

void RedisServant::stop(void)
{
    m_locker.lock();
    m_connPool.close();
    while (1) {
        ClientPacket* packet = m_requests.take(NULL);
        if (packet != NULL) {
            packet->setFinishedState(ClientPacket::RequestError);
        } else {
            break;
        }
    }
    m_actived = false;
    m_locker.unlock();
}


void RedisServant::handle(ClientPacket* packet)
{
    packet->requestServant = this;
    RedisConnection* sock = m_connPool.select();
    if (sock == NULL) {
        m_locker.lock();
        if (m_actived) {
            m_requests.append(packet);
            m_locker.unlock();
        } else {
            m_locker.unlock();
            packet->setFinishedState(ClientPacket::RequestError);
        }
    } else {
        packet->redisSocket = sock;
        onSendRequest(sock->m_socket.socket(), 0, packet);
    }
}

void RedisServant::onRedisSocketUseCompleted(RedisConnection* sock)
{
    m_locker.lock();
    ClientPacket* packet = m_requests.take(NULL);
    m_locker.unlock();
    if (!packet) {
        m_connPool.unSelect(sock);
    } else {
        packet->redisSocket = sock;
        onSendRequest(sock->m_socket.socket(), 0, packet);
    }
}

void RedisServant::onReconnect(socket_t, short, void* arg)
{
    RedisServant* servant = (RedisServant*)arg;
    if (!servant->m_reconnectEnabled) {
        return;
    }

    RedisServant::Option opt = servant->option();
    if (servant->m_reconnCount >= opt.maxReconnCount) {
        Logger::log(Logger::Message, "Stop the reconnection");
        return;
    }

    ++servant->m_reconnCount;
    Logger::log(Logger::Message, "(%d) Reconnect to redis...", servant->m_reconnCount);
    if (!servant->start()) {
        servant->m_connEvent.setTimer(servant->m_loop, onReconnect, servant);
        servant->m_connEvent.active(opt.reconnInterval * 1000);
        Logger::log(Logger::Message, "After %d second(s) reconnection...", opt.reconnInterval);
    } else {
        servant->m_reconnCount = 0;
    }
}

void RedisServant::onDisconnected(socket_t sock, short, void *arg)
{
    char buff[32];
    int len = recv(sock, buff, sizeof(buff), 0);
    if (len == 0) {
        RedisServant* servant = (RedisServant*)arg;
        servant->stop();

        Logger::log(Logger::Warning, "Redis (%s:%d) disconnected",
                    servant->redisAddress().ip(),
                    servant->redisAddress().port());

        servant->m_connListener.disconnect();
        onReconnect(0, 0, servant);
    }
}

void RedisServant::onSendRequest(socket_t sock, short, void *arg)
{
    ClientPacket* packet = (ClientPacket*)arg;
    RedisConnection* redisSocket = packet->redisSocket;
    RedisServant* redisServant = packet->requestServant;

    char* sendBuff = packet->recvParseResult.protoBuff + packet->sendToRedisBytes;
    int sendSize = packet->recvParseResult.protoBuffLen - packet->sendToRedisBytes;

    TcpSocket socket(sock);
    int ret = socket.nonblocking_send(sendBuff, sendSize);
    switch (ret) {
    default:
        packet->sendToRedisBytes += ret;
        if (packet->sendToRedisBytes != packet->recvParseResult.protoBuffLen) {
            onSendRequest(sock, 0, packet);
        } else {
            packet->_event.set(packet->eventLoop, sock, EV_READ, onRecvReply, packet);
            packet->_event.active();
        }
        break;
    case TcpSocket::IOAgain:
        packet->_event.set(packet->eventLoop, sock, EV_WRITE, onSendRequest, packet);
        packet->_event.active();
        break;
    case TcpSocket::IOError:
        if (!redisServant->m_connPool.repairSocket(redisSocket)) {
            redisServant->m_connPool.free(redisSocket);
        } else {
            redisServant->onRedisSocketUseCompleted(redisSocket);
        }
        packet->setFinishedState(ClientPacket::RequestError);
        break;
    }
}

void RedisServant::onRecvReply(socket_t sock, short, void *arg)
{
    ClientPacket* packet = (ClientPacket*)arg;
    RedisConnection* redisSocket = packet->redisSocket;
    RedisServant* redisServant = packet->requestServant;
    IOBuffer& sendbuf = packet->sendBuff;
    IOBuffer::DirectCopy cp = sendbuf.beginCopy();
    TcpSocket socket(sock);
    int ret = socket.nonblocking_recv(cp.address, cp.maxsize);
    switch (ret) {
    default:
        sendbuf.endCopy(ret);
        switch (packet->parseSendBuffer()) {
        case RedisProto::ProtoError:
            redisServant->onRedisSocketUseCompleted(redisSocket);
            packet->setFinishedState(ClientPacket::RequestError);
            break;
        case RedisProto::ProtoIncomplete:
            onRecvReply(sock, 0, packet);
            break;
        case RedisProto::ProtoOK:
            redisServant->onRedisSocketUseCompleted(redisSocket);
            packet->setFinishedState(ClientPacket::RequestFinished);
            break;
        default:
            break;
        }
        break;
    case TcpSocket::IOAgain:
        packet->_event.set(packet->eventLoop, sock, EV_READ, onRecvReply, packet);
        packet->_event.active();
        break;
    case TcpSocket::IOError:
        if (!redisServant->m_connPool.repairSocket(redisSocket)) {
            redisServant->m_connPool.free(redisSocket);
        } else {
            redisServant->onRedisSocketUseCompleted(redisSocket);
        }
        packet->setFinishedState(ClientPacket::RequestError);
        break;
    }
}

