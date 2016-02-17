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

#include "tcpsocket.h"

#ifdef WIN32
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")
#define SOCK_EAGAIN WSAEWOULDBLOCK
#define SOCKET_ERRNO WSAGetLastError()
#else
#define closesocket close
#define SOCK_EAGAIN EAGAIN
#define SOCKET_ERRNO (errno)
#endif


class EnvInitializer
{
public:
    EnvInitializer(void)
    {
#ifdef WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    }

    ~EnvInitializer(void)
    {
#ifdef WIN32
        WSACleanup();
#endif
    }
};

static EnvInitializer envinit;



HostAddress::HostAddress(int port)
{
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons(port);
    m_addr.sin_addr.s_addr = 0;
}

HostAddress::HostAddress(const char *ip, int port)
{
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons(port);
    m_addr.sin_addr.s_addr = inet_addr(ip);
}

HostAddress::HostAddress(const sockaddr_in &addr)
{
    m_addr = addr;
}

HostAddress::~HostAddress(void)
{
}

const char *HostAddress::ip(void) const
{
    char* s = inet_ntoa(m_addr.sin_addr);
    strcpy(m_ipBuff, s);
    return m_ipBuff;
}

int HostAddress::port(void) const
{
    return ntohs(m_addr.sin_port);
}




TcpSocket::TcpSocket(socket_t sock)
{
    m_socket = sock;
}

TcpSocket::~TcpSocket(void)
{
}

TcpSocket TcpSocket::createTcpSocket(void)
{
    socket_t sock = ::socket(AF_INET, SOCK_STREAM, 0);
    return TcpSocket(sock);
}

bool TcpSocket::bind(const HostAddress& addr)
{
    if (::bind(m_socket, (sockaddr*)addr._sockaddr(), sizeof(sockaddr_in)) != 0) {
        return false;
    }
    return true;
}

bool TcpSocket::listen(int backlog)
{
    if (::listen(m_socket, backlog) != 0) {
        return false;
    }
    return true;
}

int TcpSocket::option(int level, int name, char *val, socketlen_t* vallen)
{
    return ::getsockopt(m_socket, level, name, val, vallen);
}

int TcpSocket::setOption(int level, int name, char *val, socketlen_t vallen)
{
    return ::setsockopt(m_socket, level, name, val, vallen);
}

bool TcpSocket::setNonBlocking(void)
{
#ifdef WIN32
    u_long nonblocking = 1;
    if (ioctlsocket(m_socket, FIONBIO, &nonblocking) == SOCKET_ERROR) {
        return false;
    }
#else
    int flags;
    if ((flags = fcntl(m_socket, F_GETFL, NULL)) < 0) {
        return false;
    }
    if (fcntl(m_socket, F_SETFL, flags | O_NONBLOCK) == -1) {
        return false;
    }
#endif
    return true;
}

bool TcpSocket::setReuseaddr(void)
{
    int reuse;
    socketlen_t len;

    reuse = 1;
    len = sizeof(reuse);

    return (setOption(SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, len) == 0);
}

bool TcpSocket::setNoDelay(void)
{
    int nodelay;
    socketlen_t len;

    nodelay = 1;
    len = sizeof(nodelay);

    return (setOption(IPPROTO_TCP, TCP_NODELAY, (char*)&nodelay, len) == 0);
}

bool TcpSocket::setKeepAlive(void)
{
    int val = 1;
    return (setOption(SOL_SOCKET, SO_KEEPALIVE, (char*)&val, sizeof(val)) == 0);
}

bool TcpSocket::setSendBufferSize(int size)
{
    socketlen_t len;
    len = sizeof(size);
    return (setOption(SOL_SOCKET, SO_SNDBUF, (char*)&size, len) == 0);
}

bool TcpSocket::setRecvBufferSize(int size)
{
    socketlen_t len;
    len = sizeof(size);
    return (setOption(SOL_SOCKET, SO_RCVBUF, (char*)&size, len) == 0);
}

bool TcpSocket::setSendTimeout(int msec)
{
    timeval val;
    val.tv_sec = (msec / 1000);
    val.tv_usec = (msec - val.tv_sec * 1000) * 1000;

    return (setOption(SOL_SOCKET, SO_SNDTIMEO,(char*)&val,sizeof(val)) == 0);
}

bool TcpSocket::setRecvTimeout(int msec)
{
    timeval val;
    val.tv_sec = (msec / 1000);
    val.tv_usec = (msec - val.tv_sec * 1000) * 1000;

    return (setOption(SOL_SOCKET, SO_RCVTIMEO,(char*)&val,sizeof(val)) == 0);
}

int TcpSocket::sendBufferSize(void)
{
    int status, size;
    socketlen_t len;

    size = 0;
    len = sizeof(size);

    status = option(SOL_SOCKET, SO_SNDBUF, (char*)&size, &len);
    if (status < 0) {
        return -1;
    }

    return size;
}

int TcpSocket::recvBufferSize(void)
{
    int status, size;
    socketlen_t len;

    size = 0;
    len = sizeof(size);

    status = option(SOL_SOCKET, SO_RCVBUF, (char*)&size, &len);
    if (status < 0) {
        return -1;
    }

    return size;
}

bool TcpSocket::connect(const HostAddress &addr)
{
    if (::connect(m_socket, (sockaddr*)addr._sockaddr(), sizeof(sockaddr_in)) != 0) {
        return false;
    }

    return true;
}

int TcpSocket::nonblocking_send(const char *buff, int size, int flag)
{
    int ret = ::send(m_socket, buff, size, flag);
    if (ret > 0) {
        return ret;
    } else if (ret == -1) {
        switch(SOCKET_ERRNO) {
        case SOCK_EAGAIN:
            return IOAgain;
        default:
            return IOError;
        }
    } else {
        return IOError;
    }
}

int TcpSocket::nonblocking_recv(char *buff, int size, int flag)
{
    int ret = ::recv(m_socket, buff, size, flag);
    if (ret > 0) {
        return ret;
    } else if (ret == 0) {
        return IOError;
    } else if (ret == -1) {
        switch(SOCKET_ERRNO) {
        case SOCK_EAGAIN:
            return IOAgain;
        default:
            return IOError;
        }
    } else {
        return IOError;
    }
}

void TcpSocket::close(void)
{
    if (m_socket != -1) {
        TcpSocket::close(m_socket);
        m_socket = -1;
    }
}

bool TcpSocket::isNull(void) const
{
    return (m_socket < 0);
}

void TcpSocket::close(socket_t sock)
{
#ifdef WIN32
    ::closesocket(sock);
#else
    ::close(sock);
#endif
}

