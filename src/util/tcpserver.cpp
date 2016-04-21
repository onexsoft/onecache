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

#include "logger.h"

#include "tcpserver.h"

void onReadClientHandler(socket_t, short, void* arg)
{
    Context* c = (Context*)arg;
    IOBuffer* buf = &c->recvBuff;
    IOBuffer::DirectCopy cp = buf->beginCopy();
    int n = c->clientSocket.asyncRecv(cp.address, cp.maxsize);
    switch (n) {
    case 0:
        LOG(Logger::Debug, "Client (%s:%d) closed the connection",
            c->clientAddress.ip(), c->clientAddress.port());
        c->server->closeConnection(c);
        break;
    case TcpSocket::IOAgain:
        c->server->waitRequest(c);
        break;
    case TcpSocket::IOError:
        LOG(Logger::Debug, "Read client request: %s", strerror(errno));
        c->server->closeConnection(c);
        break;
    default:
        buf->endCopy(n);
        c->recvBytes += n;
        switch (c->server->readingRequest(c)) {
        case TcpServer::ReadFinished:
            c->server->readRequestFinished(c);
            break;
        case TcpServer::ReadIncomplete:
            c->server->waitRequest(c);
            break;
        case TcpServer::ReadError:
            c->server->closeConnection(c);
            break;
        }
        break;
    }
}

void onWriteClientHandler(socket_t, short, void* arg)
{
    Context* c = (Context*)arg;
    char* data = c->sendBuff.data() + c->sendBytes;
    int size = c->sendBuff.size() - c->sendBytes;
    int ret = c->clientSocket.asyncSend(data, size);
    switch (ret) {
    case TcpSocket::IOAgain:
        c->_event.set(c->eventLoop, c->clientSocket.socket(), EV_WRITE, onWriteClientHandler, c);
        c->_event.active();
        break;
    case TcpSocket::IOError:
        LOG(Logger::Debug, "Write response to client: %s", strerror(errno));
        c->server->closeConnection(c);
        break;
    default:
        c->sendBytes += ret;
        if (c->sendBytes != c->sendBuff.size()) {
            onWriteClientHandler(0, 0, c);
        } else {
            c->server->writeReplyFinished(c);
        }
        break;
    }
}


void TcpServer::onAcceptHandler(evutil_socket_t sock, short, void* arg)
{
    sockaddr_in clientAddr;
    socketlen_t len = sizeof(sockaddr_in);
    socket_t clisock = accept(sock, (sockaddr*)&clientAddr, &len);
    TcpSocket socket(clisock);
    if (socket.isNull()) {
        return;
    }

    socket.setKeepAlive();
    socket.setNonBlocking();
    socket.setNoDelay();

    TcpServer* srv = (TcpServer*)arg;
    Context* c = srv->createContextObject();
    if (c != NULL) {
        c->clientSocket = socket;
        c->clientAddress = HostAddress(clientAddr);
        c->server = srv;
        if (c->eventLoop == NULL) {
            c->eventLoop = srv->eventLoop();
        }
        srv->clientConnected(c);
        srv->waitRequest(c);
    } else {
        socket.close();
    }
}



TcpServer::TcpServer(void)
{
    m_loop = NULL;
}

TcpServer::~TcpServer(void)
{
    stop();
}

bool TcpServer::run(const HostAddress& addr)
{
    if (!m_loop) {
        LOG(Logger::Error, "event loop not set");
        return false;
    }

    if (isRunning()) {
        LOG(Logger::Error, "TcpServer::run: server is already running");
        return false;
    }

    TcpSocket tcpSocket = TcpSocket::createTcpSocket();
    if (tcpSocket.isNull()) {
        LOG(Logger::Error, "TcpServer::run: %s", strerror(errno));
        return false;
    }

    tcpSocket.setReuseaddr();
    tcpSocket.setNoDelay();
    tcpSocket.setNonBlocking();

    if (!tcpSocket.bind(addr)) {
        LOG(Logger::Error, "TcpServer::run: bind failed at port %d: %s",
                    addr.port(), strerror(errno));
        tcpSocket.close();
        return false;
    }

    if (!tcpSocket.listen(128)) {
        LOG(Logger::Error, "TcpServer::run: listen failed at port %d: %s",
                    addr.port(), strerror(errno));
        tcpSocket.close();
        return false;
    }

    m_listener.set(m_loop, tcpSocket.socket(), EV_READ | EV_PERSIST, onAcceptHandler, this);
    m_listener.active();

    m_socket = tcpSocket;
    m_addr = addr;
    return true;
}

bool TcpServer::isRunning(void) const
{
    return !m_socket.isNull();
}

void TcpServer::stop(void)
{
    if (isRunning()) {
        m_listener.remove();
        m_socket.close();
    }
}

Context *TcpServer::createContextObject(void)
{
    return new Context;
}

void TcpServer::destroyContextObject(Context *c)
{
    delete c;
}

void TcpServer::closeConnection(Context *c)
{
    c->clientSocket.close();
    destroyContextObject(c);
}

void TcpServer::clientConnected(Context*)
{
}

void TcpServer::waitRequest(Context *c)
{
    c->_event.set(c->eventLoop, c->clientSocket.socket(), EV_READ, onReadClientHandler, c);
    c->_event.active();
}

TcpServer::ReadStatus TcpServer::readingRequest(Context*)
{
    return ReadFinished;
}

void TcpServer::readRequestFinished(Context*)
{
}

void TcpServer::writeReply(Context* c)
{
    onWriteClientHandler(0, 0, c);
}

void TcpServer::writeReplyFinished(Context*)
{
}
