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

#include <signal.h>

#include "util/logger.h"
#include "redisproxy.h"
#include "non-portable.h"
#include "redis-proxy-config.h"

#include "monitor.h"

#ifdef WIN32
#define getpid ::GetCurrentProcessId
#endif

EventLoop* loop = NULL;
void handler(int val)
{
    LOG(Logger::Message, "Signal %d received, exit the program", val);
#ifdef WIN32
    switch (val) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        break;
    default:
        break;
    }
#else
    if (val == SIGINT) {
        exit(APP_EXIT_KEY);
    }
#endif
    exit(0);
}

void setupSignal(void)
{
#ifdef WIN32
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)handler, true);
#else
    struct sigaction sig;

    sig.sa_handler = handler;
    sigemptyset(&sig.sa_mask);
    sig.sa_flags = 0;

    sigaction(SIGTERM, &sig, NULL);
    sigaction(SIGINT, &sig, NULL);

    signal(SIGPIPE, SIG_IGN);
#endif
}

unsigned int ketama_hash(const char* s, int len, int alignment)
{
    unsigned char results[16];

    md5_signature((unsigned char*)s, len, results);

    return ((unsigned int) (results[3 + alignment * 4] & 0xFF) << 24)
            | ((unsigned int) (results[2 + alignment * 4] & 0xFF) << 16)
            | ((unsigned int) (results[1 + alignment * 4] & 0xFF) << 8)
            | (results[0 + alignment * 4] & 0xFF);
}

static HashFunc hashfuncFromName(const char* name)
{
    struct FuncInfo {
        const char* name;
        HashFunc func;
    } temp[] = {
    {"md5", hash_md5},
    {"crc16", hash_crc16},
    {"crc32", hash_crc32},
    {"crc32a", hash_crc32a},
    {"hsieh", hash_hsieh},
    {"jenkins", hash_jenkins},
    {"fnv1_64", hash_fnv1_64},
    {"fnv1a_64", hash_fnv1a_64},
    {"fnv1_32", hash_fnv1_32},
    {"fnv1a_32", hash_fnv1a_32},
    {"murmur", hash_murmur}
};

    for (unsigned int i = 0; i < sizeof(temp)/sizeof(FuncInfo); ++i) {
        if (strcmp(temp[i].name, name) == 0) {
            LOG(Logger::Debug, "Use the '%s' hash function", name);
            return temp[i].func;
        }
    }
    LOG(Logger::Debug, "hash name '%s' is invalid. use default hash function", name);
    return hashForBytes;
}

static int slot_item_cmp(const void* l, const void* r)
{
    Slot* a = (Slot*)l;
    Slot* b = (Slot*)r;
    if (a->value == b->value) {
        return 0;
    } else if (a->value > b->value) {
        return 1;
    } else {
        return -1;
    }
}

void startOneCache(void)
{
    CRedisProxyCfg* cfg = CRedisProxyCfg::instance();
    setupSignal();

    EventLoop listenerLoop;
    bool twemproxyMode = cfg->isTwemproxyMode();
    const int points_per_server = 160;
    const int pointer_per_hash = 4;
    int slot_index = 0;

    RedisProxy proxy;
    proxy.setEventLoop(&listenerLoop);
    loop = &listenerLoop;
    CProxyMonitor monitor;
    proxy.setMonitor(&monitor);

    const int defaultPort = 8221;
    int port = cfg->port();
    if (port <= 0) {
        LOG(Logger::Debug, "Port %d invalid, use default port %d", defaultPort);
        port = defaultPort;
    }

    if (!proxy.run(HostAddress(port))) {
        exit(APP_EXIT_KEY);
    }

    proxy.setTwemproxyModeEnabled(twemproxyMode);

    EventLoopThreadPool pool;
    pool.start(cfg->threadNum());
    proxy.setEventLoopThreadPool(&pool);

    const SVipInfo* vipInfo = cfg->vipInfo();
    proxy.setVipName(vipInfo->if_alias_name);
    proxy.setVipAddress(vipInfo->vip_address);
    proxy.setVipEnabled(vipInfo->enable);

    const SHashInfo* sHashInfo = cfg->hashInfo();

    if (!twemproxyMode) {
        proxy.setSlotCount(sHashInfo->hash_value_max);
    } else {
        proxy.setSlotCount(cfg->groupCnt() * points_per_server);
    }

    const GroupOption* groupOption = cfg->groupOption();
    proxy.setGroupRetryTime(groupOption->group_retry_time);
    proxy.setAutoEjectGroupEnabled(groupOption->auto_eject_group);
    proxy.setEjectAfterRestoreEnabled(groupOption->eject_after_restore);
    proxy.setPassword(cfg->password());

    for (int i = 0; i < cfg->groupCnt(); ++i) {
        const CGroupInfo* info = cfg->group(i);

        RedisServantGroup* group = new RedisServantGroup;
        RedisServantGroupPolicy* policy = RedisServantGroupPolicy::createPolicy(info->groupPolicy());
        group->setGroupName(info->groupName());
        group->setPolicy(policy);

        const HostInfoList& hostList = info->hosts();
        HostInfoList::const_iterator itHost = hostList.begin();
        for (; itHost != hostList.end(); ++itHost) {
            const CHostInfo& hostInfo = (*itHost);
            RedisServant* servant = new RedisServant;
            RedisServant::Option opt;
            strcpy(opt.name, hostInfo.get_hostName().c_str());
            opt.poolSize = hostInfo.get_connectionNum();
            opt.reconnInterval = groupOption->backend_retry_interval;
            opt.maxReconnCount = groupOption->backend_retry_limit;
            servant->setOption(opt);
            servant->setRedisAddress(HostAddress(hostInfo.get_ip().c_str(), hostInfo.get_port()));
            servant->setEventLoop(proxy.eventLoop());
            if (hostInfo.get_master()) {
                group->addMasterRedisServant(servant);
            } else {
                group->addSlaveRedisServant(servant);
            }
            servant->connectionPool()->setPassword(itHost->passWord());
        }
        group->setEnabled(true);
        proxy.addRedisGroup(group);

        if (!twemproxyMode) {
            for (int i = info->hashMin(); i <= info->hashMax(); ++i) {
                proxy.setSlot(i, group);
            }

            for (int i = 0; i < cfg->hashMapCnt(); ++i) {
                const CHashMapping* mapping = cfg->hashMapping(i);
                RedisServantGroup* group = proxy.group(mapping->group_name);
                proxy.setSlot(mapping->hash_value, group);
            }
        } else {
            Slot* slot = proxy.slotData();
            for (int pointer_index = 1;
                 pointer_index <= points_per_server / pointer_per_hash;
                 pointer_index++) {

                char host[86];
                int hostlen = sprintf(host, "%s-%u", group->groupName(), pointer_index - 1);
                for (int x = 0; x < pointer_per_hash; x++) {
                    slot[slot_index].group = group;
                    slot[slot_index++].value = ketama_hash(host, hostlen, x);
                }
            }
        }
    }

    if (twemproxyMode) {
        LOG(Logger::Debug, "Twemproxy mode. dist=ketama slotcount=%d", proxy.slotCount());
        Slot* slot = proxy.slotData();
        int slotCount = proxy.slotCount();
        qsort(slot, slotCount, sizeof(Slot), slot_item_cmp);
    }

    HashFunc func = hashfuncFromName(cfg->hashFunctin().c_str());
    proxy.setHashFunction(func);

    for (int i = 0; i < cfg->keyMapCnt(); ++i) {
        const CKeyMapping* mapping = cfg->keyMapping(i);
        RedisServantGroup* group = proxy.group(mapping->group_name);
        if (group != NULL) {
            proxy.addGroupKeyMapping(mapping->key, strlen(mapping->key), group);
        }
    }

    LOG(Logger::Message, "Start the %s on port %d. PID: %d", APP_NAME, port, getpid());
    listenerLoop.exec();
}

void createPidFile(const char* pidFile)
{
    FILE* fp = fopen(pidFile, "w");
    if (fp) {
        fprintf(fp, "%d\n", getpid());
        fclose(fp);
    }
}

int main(int argc, char** argv)
{
    printf("%s %s, Copyright 2015 by OneXSoft\n\n", APP_NAME, APP_VERSION);

    const char* cfgFile = "onecache.xml";
    if (argc == 1) {
        LOG(Logger::Message, "No config file specified. using the default config(%s)", cfgFile);
    } if (argc >= 2) {
        cfgFile = argv[1];
    }

    CRedisProxyCfg* cfg = CRedisProxyCfg::instance();
    if (!cfg->loadCfg(cfgFile)) {
        LOG(Logger::Error, "Failed to read config file");
        return 1;
    }

    const char* errInfo;
    if (!CRedisProxyCfgChecker::isValid(cfg, errInfo)) {
        LOG(Logger::Error, "Invalid configuration: %s", errInfo);
        return 1;
    }

    FileLogger fileLogger;
    if (fileLogger.setFileName(cfg->logFile())) {
        LOG(Logger::Message, "Using the log file(%s) output", fileLogger.fileName());
        Logger::setDefaultLogger(&fileLogger);
    }

    GlobalLogOption::__debug = cfg->debug();

    const char* pidfile = cfg->pidFile();
    bool pidfile_enabled = (strlen(pidfile) > 0);
    if (pidfile_enabled) {
        createPidFile(pidfile);
    }

    //Background
    if (cfg->daemonize()) {
        NonPortable::daemonize();
    }

    //Guard
    if (cfg->guard()) {
        NonPortable::guard(startOneCache, APP_EXIT_KEY);
    } else {
        startOneCache();
    }

    if (pidfile_enabled) {
        remove(pidfile);
    }

    return 0;
}




