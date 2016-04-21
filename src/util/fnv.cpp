/*
 * twemproxy - A fast and lightweight proxy for memcached protocol.
 * Copyright (C) 2011 Twitter, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "hash.h"

static unsigned long long FNV_64_INIT = (unsigned long long)(0xcbf29ce484222325);
static unsigned long long FNV_64_PRIME = (unsigned long long)(0x100000001b3);
static unsigned int FNV_32_INIT = 2166136261UL;
static unsigned int FNV_32_PRIME = 16777619;

unsigned int hash_fnv1_64(const char *key, int key_length)
{
    unsigned long long hash = FNV_64_INIT;
    int x;

    for (x = 0; x < key_length; x++) {
        hash *= FNV_64_PRIME;
        hash ^= (unsigned long long)key[x];
    }

    return (unsigned int)hash;
}

unsigned int hash_fnv1a_64(const char *key, int key_length)
{
    unsigned int hash = (unsigned int) FNV_64_INIT;
    int x;

    for (x = 0; x < key_length; x++) {
        unsigned int val = (unsigned int)key[x];
        hash ^= val;
        hash *= (unsigned int) FNV_64_PRIME;
    }

    return hash;
}

unsigned int hash_fnv1_32(const char *key, int key_length)
{
    unsigned int hash = FNV_32_INIT;
    int x;

    for (x = 0; x < key_length; x++) {
        unsigned int val = (unsigned int)key[x];
        hash *= FNV_32_PRIME;
        hash ^= val;
    }

    return hash;
}

unsigned int hash_fnv1a_32(const char *key, int key_length)
{
    unsigned int hash = FNV_32_INIT;
    int x;

    for (x= 0; x < key_length; x++) {
        unsigned int val = (unsigned int)key[x];
        hash ^= val;
        hash *= FNV_32_PRIME;
    }

    return hash;
}
