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

#ifndef CMDHANDLER_H
#define CMDHANDLER_H

#include "command.h"

void onAuthCommand(ClientPacket*, void*);

void onStandardKeyCommand(ClientPacket*, void*);

void onDelCommand(ClientPacket*, void*);

void onPingCommand(ClientPacket*, void*);

void onShowCommand(ClientPacket*, void*);

void onMGetCommand(ClientPacket*, void*);

void onMSetCommand(ClientPacket*, void*);

void onHashMapping(ClientPacket* packet, void*);

void onAddKeyMapping(ClientPacket* packet, void*);

void onDelKeyMapping(ClientPacket* packet, void*);

void onShowMapping(ClientPacket* packet, void*);

void onPoolInfo(ClientPacket* packet, void*);

void onShutDown(ClientPacket* packet, void*);

#endif
