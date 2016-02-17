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

#ifndef SOCKET_H
#define SOCKET_H

#ifdef WIN32
#include <WinSock2.h>
#include <Windows.h>
typedef intptr_t socket_t;
typedef int socketlen_t;
#else
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/tcp.h>
typedef int socket_t;
typedef socklen_t socketlen_t;
#endif

#include "util/string.h"

class HostAddress
{
public:
    HostAddress(int port = 0);
    HostAddress(const char* ip, int port);
    HostAddress(const sockaddr_in& addr);
    ~HostAddress(void);

    const char* ip(void) const;
    int port(void) const;

    sockaddr_in* _sockaddr(void) const
    { return (sockaddr_in*)&m_addr; }

private:
    mutable char m_ipBuff[32];
    sockaddr_in m_addr;
};

class TcpSocket
{
public:
    enum {
        IOAgain = -1,
        IOError = -2
    };

    TcpSocket(socket_t sock = -1);
    ~TcpSocket(void);

    static TcpSocket createTcpSocket(void);

    socket_t socket(void) const { return m_socket; }

    bool bind(const HostAddress& addr);
    bool listen(int backlog);

    int option(int level, int name, char* val, socketlen_t* vallen);
    int setOption(int level, int name, char* val, socketlen_t vallen);

    bool setNonBlocking(void);
    bool setReuseaddr(void);
    bool setNoDelay(void);
    bool setKeepAlive(void);
    bool setSendBufferSize(int size);
    bool setRecvBufferSize(int size);
    bool setSendTimeout(int msec);
    bool setRecvTimeout(int msec);

    int sendBufferSize(void);
    int recvBufferSize(void);

    bool connect(const HostAddress& addr);

    //Blocking
    int send(const char *buf, int len, int flags = 0) {
        return ::send(m_socket, buf, len, flags);
    }
    int recv(char *buf, int len, int flags = 0) {
        return ::recv(m_socket, buf, len, flags);
    }

    //NonBlocking
    int nonblocking_send(const char* buff, int size, int flag = 0);
    int nonblocking_recv(char* buff, int size, int flag = 0);

    void close(void);
    bool isNull(void) const;

    static void close(socket_t sock);

private:
    socket_t m_socket;
};


#endif
