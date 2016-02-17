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

RedisProxy* currentProxy = NULL;
void handler(int)
{
    if (currentProxy) {
        currentProxy->stop();
    }
}

void setupSignal(void)
{
#ifndef WIN32
    struct sigaction sig;

    sig.sa_handler = handler;
    sigemptyset(&sig.sa_mask);
    sig.sa_flags = 0;

    sigaction(SIGTERM, &sig, NULL);
    sigaction(SIGINT, &sig, NULL);

    signal(SIGPIPE, SIG_IGN);
#endif
}

void startOneCache(void)
{
    CRedisProxyCfg* cfg = CRedisProxyCfg::instance();

    setupSignal();

    RedisProxy proxy;
    currentProxy = &proxy;

    EventLoopThreadPool pool;
    pool.start(cfg->threadNum());
    proxy.setEventLoopThreadPool(&pool);

    FileLogger fileLogger;
    if (fileLogger.setFileName(cfg->logFile())) {
        Logger::log(Logger::Message, "Using the log file(%s) output", fileLogger.fileName());
        Logger::setDefaultLogger(&fileLogger);
    }

    const SVipInfo* vipInfo = cfg->vipInfo();
    proxy.setVipName(vipInfo->if_alias_name);
    proxy.setVipAddress(vipInfo->vip_address);
    proxy.setVipEnabled(vipInfo->enable);

    const SHashInfo* sHashInfo = cfg->hashInfo();
    proxy.setMaxHashValue(sHashInfo->hash_value_max);

    const GroupOption* groupOption = cfg->groupOption();
    proxy.setGroupRetryTime(groupOption->group_retry_time);
    proxy.setAutoEjectGroupEnabled(groupOption->auto_eject_group);
    proxy.setEjectAfterRestoreEnabled(groupOption->eject_after_restore);

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
        }
        group->setEnabled(true);
        proxy.addRedisGroup(group);
        for (int i = info->hashMin(); i <= info->hashMax(); ++i) {
            proxy.setGroupMappingValue(i, group);
        }

        for (int i = 0; i < cfg->hashMapCnt(); ++i) {
            const CHashMapping* mapping = cfg->hashMapping(i);
            RedisServantGroup* group = proxy.group(mapping->group_name);
            proxy.setGroupMappingValue(mapping->hash_value, group);
        }

        for (int i = 0; i < cfg->keyMapCnt(); ++i) {
            const CKeyMapping* mapping = cfg->keyMapping(i);
            RedisServantGroup* group = proxy.group(mapping->group_name);
            if (group != NULL) {
                proxy.addGroupKeyMapping(mapping->key, strlen(mapping->key), group);
            }
        }
    }

    CProxyMonitor monitor;
    proxy.setMonitor(&monitor);

    int port = cfg->port();
    if (port <= 0) {
        port = RedisProxy::DefaultPort;
    }
    proxy.run(HostAddress(port));
}

int main(int argc, char** argv)
{
    printf("%s %s, Copyright 2015 by OneXSoft\n\n", APP_NAME, APP_VERSION);

    const char* cfgFile = "onecache.xml";
    if (argc == 1) {
        Logger::log(Logger::Message, "No config file specified. using the default config(%s)", cfgFile);
    } if (argc >= 2) {
        cfgFile = argv[1];
    }

    CRedisProxyCfg* cfg = CRedisProxyCfg::instance();
    if (!cfg->loadCfg(cfgFile)) {
        Logger::log(Logger::Error, "Failed to read config file");
        return 1;
    }

    const char* errInfo;
    if (!CRedisProxyCfgChecker::isValid(cfg, errInfo)) {
        Logger::log(Logger::Error, "Invalid configuration: %s", errInfo);
        return 1;
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

    return 0;
}

















