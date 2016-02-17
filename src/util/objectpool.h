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

#ifndef OBJECTPOOL_H
#define OBJECTPOOL_H

#include <new>

#include "util/vector.h"

template <typename T>
class ObjectPool
{
public:
    enum { ObjectSize = sizeof(T) };

    ObjectPool(void) {}
    ~ObjectPool(void) { clear(); }

public:
    T* alloc(void) {
        char* ptr = m_pool.pop_back(0);
        if (ptr == 0) {
            ptr = new char[ObjectSize];
        }
        if (ptr) {
            return new (ptr) T;
        }
        return 0;
    }

    void free(T* object) {
        char* ptr = (char*)object;
        if (ptr) {
            object->~T();
            m_pool.push_back(ptr);
        }
    }

    void clear(void) {
        for (int i = 0; i < m_pool.size(); ++i) {
            delete []m_pool.at(i);
        }
        m_pool.clear();
    }

private:
    Vector<char*> m_pool;

private:
    ObjectPool(const ObjectPool&);
    ObjectPool& operator=(const ObjectPool&);
};

#endif
