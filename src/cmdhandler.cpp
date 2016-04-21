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

#include "cmdhandler.h"
#include "redisproxy.h"
#include "redisservant.h"
#include "redis-proxy-config.h"

struct MGetCommandContext
{
    int keyCount;
    int returnCount;
    ClientPacket* subs[1024];
    ClientPacket* packet;
};

struct MSetCommandContext
{
    int keyvalCount;
    int succeedCount;
    ClientPacket* packet;
};

struct DelCommandContext
{
    int keyCount;
    int returnCount;
    int integer;
    ClientPacket* subs[1024];
    ClientPacket* packet;
};

void onGetPacketFinished(ClientPacket*, void* arg)
{
    MGetCommandContext* mgetcontext = (MGetCommandContext*)arg;
    ++mgetcontext->returnCount;
    if (mgetcontext->returnCount == mgetcontext->keyCount) {
        for (int i = 0; i < mgetcontext->keyCount; ++i) {
            mgetcontext->packet->sendBuff.append(mgetcontext->subs[i]->sendBuff);
            delete mgetcontext->subs[i];
        }
        mgetcontext->packet->setFinishedState(ClientPacket::RequestFinished);
        delete mgetcontext;
    }
}

void onSetPacketFinished(ClientPacket* packet, void* arg)
{
    MSetCommandContext* msetcontext = (MSetCommandContext*)arg;
    ++msetcontext->succeedCount;
    if (msetcontext->succeedCount == msetcontext->keyvalCount) {
        msetcontext->packet->sendBuff.append("+OK\r\n");
        msetcontext->packet->setFinishedState(ClientPacket::RequestFinished);
        delete msetcontext;
    }
    delete packet;
}

void onDelPacketFinished(ClientPacket* packet, void* arg)
{
    DelCommandContext* delcontext = (DelCommandContext*)arg;
    ++delcontext->returnCount;
    delcontext->integer += packet->sendParseResult.integer;
    if (delcontext->returnCount == delcontext->keyCount) {
        delcontext->packet->sendBuff.appendFormatString(":%d\r\n", delcontext->integer);
        delcontext->packet->setFinishedState(ClientPacket::RequestFinished);
        delete delcontext;
    }
    delete packet;
}



void onStandardKeyCommand(ClientPacket* packet, void*)
{
    char* key = packet->recvParseResult.tokens[1].s;
    int len = packet->recvParseResult.tokens[1].len;
    packet->proxy()->handleClientPacket(key, len, packet);
}

void onMGetCommand(ClientPacket* packet, void*)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    int keyCount = r.tokenCount - 1;
    if (keyCount <= 0) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }

    if (keyCount == 1) {
        char* key = r.tokens[1].s;
        int len = r.tokens[1].len;
        packet->proxy()->handleClientPacket(key, len, packet);
    } else {
        MGetCommandContext* mgetcontext = new MGetCommandContext;
        mgetcontext->keyCount = keyCount;
        mgetcontext->returnCount = 0;
        mgetcontext->packet = packet;
        packet->sendBuff.appendFormatString("*%d\r\n", keyCount);
        for (int i = 1; i < r.tokenCount; ++i) {
            char* key = r.tokens[i].s;
            int len = r.tokens[i].len;

            ClientPacket* get = new ClientPacket;
            get->eventLoop = packet->eventLoop;
            get->commandType = RedisCommand::GET;
            get->finished_func = onGetPacketFinished;
            get->finished_arg = mgetcontext;
            get->recvBuff.appendFormatString("*2\r\n$3\r\nGET\r\n$%d\r\n", len);
            get->recvBuff.append(key, len);
            get->recvBuff.append("\r\n");
            mgetcontext->subs[i-1] = get;
            get->continueToParseRecvBuffer();
            packet->proxy()->handleClientPacket(key, len, get);
        }
    }
}

void onMSetCommand(ClientPacket* packet, void*)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    int keyvalCount = (r.tokenCount - 1) / 2;
    if (keyvalCount <= 0) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }

    if (keyvalCount == 1) {
        char* key = r.tokens[1].s;
        int len = r.tokens[1].len;
        packet->proxy()->handleClientPacket(key, len, packet);
    } else {
        MSetCommandContext* msetcontext = new MSetCommandContext;
        msetcontext->keyvalCount = keyvalCount;
        msetcontext->succeedCount = 0;
        msetcontext->packet = packet;

        for (int i = 1; i < r.tokenCount; i += 2) {
            char* key = r.tokens[i].s;
            int len = r.tokens[i].len;

            char* val = r.tokens[i+1].s;
            int vallen = r.tokens[i+1].len;

            ClientPacket* set = new ClientPacket;
            set->finished_func = onSetPacketFinished;
            set->finished_arg = msetcontext;
            set->eventLoop = packet->eventLoop;
            set->commandType = RedisCommand::SET;
            set->recvBuff.appendFormatString("*3\r\n$3\r\nSET\r\n$%d\r\n", len);
            set->recvBuff.append(key, len);
            set->recvBuff.append("\r\n");
            set->recvBuff.appendFormatString("$%d\r\n", vallen);
            set->recvBuff.append(val, vallen);
            set->recvBuff.append("\r\n");
            set->continueToParseRecvBuffer();
            packet->proxy()->handleClientPacket(key, len, set);
        }
    }
}

void onDelCommand(ClientPacket* packet, void*)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    int keyCount = r.tokenCount - 1;
    if (keyCount <= 0) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }
    if (keyCount == 1) {
        char* key = r.tokens[1].s;
        int len = r.tokens[1].len;
        packet->proxy()->handleClientPacket(key, len, packet);
    } else {
        DelCommandContext* delcontext = new DelCommandContext;
        delcontext->keyCount = keyCount;
        delcontext->returnCount = 0;
        delcontext->integer = 0;
        delcontext->packet = packet;
        for (int i = 1; i < r.tokenCount; ++i) {
            char* key = r.tokens[i].s;
            int len = r.tokens[i].len;

            ClientPacket* del = new ClientPacket;
            del->eventLoop = packet->eventLoop;
            del->finished_func = onDelPacketFinished;
            del->finished_arg = delcontext;
            del->commandType = RedisCommand::DEL;
            del->eventLoop = packet->eventLoop;
            del->recvBuff.appendFormatString("*2\r\n$3\r\nDEL\r\n$%d\r\n", len);
            del->recvBuff.append(key, len);
            del->recvBuff.append("\r\n");
            del->continueToParseRecvBuffer();
            packet->proxy()->handleClientPacket(key, len, del);
        }
    }
}

void onPingCommand(ClientPacket* packet, void*)
{
    packet->sendBuff.append("+PONG\r\n");
    packet->setFinishedState(ClientPacket::RequestFinished);
}

void onShowCommand(ClientPacket* packet, void*)
{
    IOBuffer& reply = packet->sendBuff;
    reply.append("+ *** Support the command ***\n");
    reply.append("[Redis Command]\n");

    IOBuffer redis;
    IOBuffer oneCache;

    const std::list<RedisCommand*>& seq = RedisCommandTable::instance()->commands();
    std::list<RedisCommand*>::const_iterator it = seq.cbegin();
    for (; it != seq.cend(); ++it) {
        RedisCommand* cmd = *it;
        if (cmd->type >= 0 && cmd->type < RedisCommand::CMD_COUNT) {
            redis.appendFormatString("%s ", cmd->name);
        } else {
            oneCache.appendFormatString("%s ", cmd->name);
        }
    }

    reply.append(redis);
    reply.append("\n\n");
    reply.append("[OneCache Command]\n");
    reply.append(oneCache);
    reply.append("\n\r\n");
    packet->setFinishedState(ClientPacket::RequestFinished);
}

void onHashMapping(ClientPacket* packet, void*)
{
    RedisProtoParseResult& request = packet->recvParseResult;
    if (request.tokenCount != 3) {
        packet->sendBuff.append("+Usage:\nHASHMAPPING [hash value] [group name]\n\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }

    RedisProxy* proxy = packet->proxy();
    char hashValueStr[1024] = {0};
    char groupName[1024] = {0};
    strncpy(hashValueStr, request.tokens[1].s, request.tokens[1].len);
    strncpy(groupName, request.tokens[2].s, request.tokens[2].len);

    int hashValue = atoi(hashValueStr);
    RedisServantGroup* group = proxy->group(groupName);
    if (!group) {
        packet->sendBuff.append("-Group is not exists\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }

    if (proxy->setSlot(hashValue, group)) {
        packet->sendBuff.append("+OK\r\n");
        CRedisProxyCfg::instance()->saveProxyLastState(proxy);
    } else {
        packet->sendBuff.append("-Invalid hash value\r\n");
    }
    packet->setFinishedState(ClientPacket::RequestFinished);
}

void onAddKeyMapping(ClientPacket* packet, void*)
{
    RedisProtoParseResult& request = packet->recvParseResult;
    if (request.tokenCount <= 2) {
        packet->sendBuff.append("+Usage:\nADDKEYMAPPING [group name] [key1] [key2]...\n\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }

    RedisProxy* proxy = packet->proxy();

    char groupName[1024] = {0};
    strncpy(groupName, request.tokens[1].s, request.tokens[1].len);
    RedisServantGroup* group = proxy->group(groupName);
    if (!group) {
        packet->sendBuff.append("-Group is not exists\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }

    for (int i = 2; i < request.tokenCount; ++i) {
        const char* key = request.tokens[i].s;
        int keyLen = request.tokens[i].len;
        proxy->addGroupKeyMapping(key, keyLen, group);
    }

    packet->sendBuff.append("+OK\r\n");
    packet->setFinishedState(ClientPacket::RequestFinished);
    CRedisProxyCfg::instance()->saveProxyLastState(proxy);
}

void onDelKeyMapping(ClientPacket* packet, void*)
{
    RedisProtoParseResult& request = packet->recvParseResult;
    if (request.tokenCount <= 1) {
        packet->sendBuff.append("+Usage:\nDELKEYMAPPING [key1] [key2]...\n\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }

    RedisProxy* proxy = packet->proxy();
    for (int i = 1; i < request.tokenCount; ++i) {
        const char* key = request.tokens[i].s;
        int keyLen = request.tokens[i].len;
        proxy->removeGroupKeyMapping(key, keyLen);
    }

    packet->sendBuff.append("+OK\r\n");
    packet->setFinishedState(ClientPacket::RequestFinished);
    CRedisProxyCfg::instance()->saveProxyLastState(proxy);
}

void onShowMapping(ClientPacket* packet, void*)
{
    RedisProxy* proxy = packet->proxy();
    packet->sendBuff.append("+\n");

    packet->sendBuff.append("[HASH MAPPING]\n");
    packet->sendBuff.appendFormatString("%-15s %-15s\n", "HASH_VALUE", "GROUP_NAME");
    for (int i = 0; i < proxy->slotCount(); ++i) {
        packet->sendBuff.appendFormatString("%-15d %-15s\n",
                                            i, proxy->groupBySlot(i)->groupName());
    }
    packet->sendBuff.append("\n");
    packet->sendBuff.append("[KEY MAPPING]\n");
    packet->sendBuff.appendFormatString("%-4s %-15s KEYS\n", "ID", "NAME");

    StringMap<RedisServantGroup*>& keyMapping = proxy->keyMapping();
    for (int j = 0; j < proxy->groupCount(); ++j) {
        RedisServantGroup* group = proxy->group(j);
        packet->sendBuff.appendFormatString("%-4d %-15s ", group->groupId(), group->groupName());

        StringMap<RedisServantGroup*>::iterator it = keyMapping.begin();
        for (; it != keyMapping.end(); ++it) {
            if (it->second == group) {
                String key = it->first;
                packet->sendBuff.append(key.data(), key.length());
                packet->sendBuff.append(" ");
            }
        }
        packet->sendBuff.append("\n");
    }

    packet->sendBuff.append("\n\r\n");
    packet->setFinishedState(ClientPacket::RequestFinished);
}

void onPoolInfo(ClientPacket* packet, void*)
{
    RedisProxy* proxy = packet->proxy();
    IOBuffer& sendbuf = packet->sendBuff;
    sendbuf.append("+", 1);
    sendbuf.appendFormatString("%-10s %-20s %-8s %-10s %-12s\n",
                               "GROUP", "HOST", "ACTIVE", "UNACTIVE", "POOLSIZE");
    for (int i = 0; i < proxy->groupCount(); ++i) {
        RedisServantGroup* group = proxy->group(i);
        for (int m = 0; m < group->masterCount(); ++m) {
            RedisServant* servant = group->master(m);
            RedisConnectionPool* pool = servant->connectionPool();
            char buf[64];
            sprintf(buf, "%s:%d", servant->redisAddress().ip(), servant->redisAddress().port());
            sendbuf.appendFormatString("%-10s %-20s %-8d %-10d %-12d\n",
                                       group->groupName(),
                                       buf,
                                       pool->activeConnectionNums(),
                                       pool->unActiveConnectionNums(),
                                       pool->capacity());
        }
        for (int s = 0; s < group->slaveCount(); ++s) {
            RedisServant* servant = group->slave(s);
            RedisConnectionPool* pool = servant->connectionPool();
            char buf[64];
            sprintf(buf, "%s:%d", servant->redisAddress().ip(), servant->redisAddress().port());
            sendbuf.appendFormatString("%-10s %-20s %-8d %-10d %-12d\n",
                                       group->groupName(),
                                       buf,
                                       pool->activeConnectionNums(),
                                       pool->unActiveConnectionNums(),
                                       pool->capacity());
        }
    }
    sendbuf.append("\r\n", 2);
    packet->setFinishedState(ClientPacket::RequestFinished);
}

void onShutDown(ClientPacket* packet, void*)
{
    RedisProtoParseResult& request = packet->recvParseResult;
    if (request.tokenCount == 1) {
        exit(0);
    } else if (request.tokenCount == 2) {
        if (strncasecmp(request.tokens[1].s, "FORCE", request.tokens[1].len) == 0) {
            exit(APP_EXIT_KEY);
        } else {
            exit(0);
        }
    }
}

void onAuthCommand(ClientPacket* packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount != 2) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }
    const std::string onecachePassword = packet->proxy()->password();
    if (onecachePassword.empty()) {
        packet->sendBuff.append("-ERR Client sent AUTH, but no password is set\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }
    const std::string password(r.tokens[1].s, r.tokens[1].len);
    if (password == onecachePassword) {
        packet->auth = true;
        packet->sendBuff.append("+OK\r\n");
    } else {
        packet->sendBuff.append("-ERR invalid password\r\n");
    }
    packet->setFinishedState(ClientPacket::RequestFinished);
}


