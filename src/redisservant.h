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

#ifndef REDISSERVANT_H
#define REDISSERVANT_H

#include <string>

#include "util/vector.h"
#include "util/queue.h"
#include "util/locker.h"
#include "util/tcpsocket.h"

#include "eventloop.h"

class ClientPacket;
class RedisConnection
{
public:
    RedisConnection(void);
    ~RedisConnection(void);

    bool connect(const HostAddress& addr, const std::string& pwd);
    bool isActived(void) const { return !m_socket.isNull(); }
    void disconnect(void);

private:
    TcpSocket m_socket;
    friend class RedisConnectionPool;
    friend class RedisServant;
};


class RedisConnectionPool
{
public:
    RedisConnectionPool(void);
    ~RedisConnectionPool(void);

    void setPassword(const std::string& pwd) { m_password = pwd; }
    std::string password(void) const { return m_password; }

    const HostAddress& redisAddress(void) const { return m_redisAddress; }
    int capacity(void) const { return m_capacity; }
    int activeConnectionNums(void) const { return m_activeConnNums; }
    int unActiveConnectionNums(void) const { return m_pool.size(); }

    bool open(const HostAddress& addr, int capacity);
    RedisConnection* select(void);
    void unSelect(RedisConnection* sock);
    bool repairSocket(RedisConnection* sock);
    void free(RedisConnection* sock);
    void close(void);

private:
    HostAddress m_redisAddress;
    SpinLocker m_locker;
    int m_capacity;
    int m_activeConnNums;
    std::string m_password;
    //Vector<RedisConnection*> m_pool;
    Queue<RedisConnection*> m_pool;
};

class RedisServant
{
public:
    struct Option {
        Option(void) {
            name[0] = 0;
            maxReconnCount = 100;
            reconnInterval = 1;
            poolSize = 50;
        }
        ~Option(void) {}

        char name[64];
        int reconnInterval;
        int maxReconnCount;
        int poolSize;
    };

    RedisServant(void);
    ~RedisServant(void);

public:
    void setRedisAddress(const HostAddress& addr) { m_redisAddress = addr; }
    const HostAddress& redisAddress(void) const { return m_redisAddress; }

    void setOption(const Option& opt) { m_option = opt; }
    Option option(void) const { return m_option; }

    void setReconnectEnabled(bool b) { m_reconnectEnabled = b; }
    bool reconnectEnabled(void) const { return m_reconnectEnabled; }

    void setEventLoop(EventLoop* loop) { m_loop = loop; }
    EventLoop* eventLoop(void) const { return m_loop; }

    RedisConnectionPool* connectionPool(void) { return &m_connPool; }

    bool isActived(void) const { return m_actived; }
    bool start(void);
    void stop(void);

    void handle(ClientPacket* packet);

private:
    void onRedisSocketUseCompleted(RedisConnection* sock);
    static void onDisconnected(socket_t sock, short, void* arg);
    static void onReconnect(socket_t sock, short, void* arg);
    static void onSendRequest(socket_t sock, short, void* arg);
    static void onRecvReply(socket_t sock, short, void* arg);

private:
    HostAddress m_redisAddress;
    int m_reconnCount;
    Event m_connEvent;
    RedisConnection m_connListener;
    EventLoop* m_loop;
    Option m_option;
    Queue<ClientPacket*> m_requests;
    SpinLocker m_locker;
    bool m_actived;
    bool m_reconnectEnabled;
    RedisConnectionPool m_connPool;

private:
    RedisServant(const RedisServant&);
    RedisServant& operator =(const RedisServant&);
};

#endif
