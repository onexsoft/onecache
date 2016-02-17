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

#ifndef HASH_H
#define HASH_H

#include <unordered_map>

#include "util/string.h"
#include "util/vector.h"

typedef unsigned int (*HashFunc)(const char* s, int len);
unsigned int hashForBytes(const char *key, int len);

struct StringHashFunc {
    unsigned int operator()(const String& str) const {
        return hashForBytes(str.data(), str.length());
    }
};

#define HashTable std::unordered_map

template <typename VAL>
class StringMap : public HashTable<String, VAL, StringHashFunc>
{
public:
};

#endif
