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

#ifndef VECTOR_H
#define VECTOR_H

template <typename T>
class Vector
{
public:
    enum { ChunkSize = 64 };
    Vector(void) {
        buff = 0;
        curcnt = 0;
        capacity = 0;
        resize(ChunkSize);
    }

    Vector(int n) {
        resize(n);
    }

    ~Vector(void) {
        delete []buff;
    }

    void append(const T& item) {
        if (curcnt == capacity) {
            int new_capacity = capacity+ChunkSize;
            resize(new_capacity);
        }
        buff[curcnt] = item;
        ++curcnt;
    }

    T* data(void) { return buff; }
    const T* data(void) const { return buff; }
    int size(void) const { return curcnt; }

    bool isEmpty(void) const { return (curcnt == 0); }

    T& at(int n) { return buff[n]; }
    const T& at(int n) const { return buff[n]; }

    void push_back(const T& item) { append(item); }
    T pop_back(const T& defaultVal) {
        if (curcnt == 0) {
            return defaultVal;
        }
        return buff[--curcnt];
    }

    void resize(int n) {
        if (n <= 0) {
            return;
        }

        T* p = new T[n];
        int copy = (n < curcnt) ? n : curcnt;
        for (int i = 0; i < copy; ++i) {
            p[i] = buff[i];
        }
        if (buff) {
            delete []buff;
        }
        buff = p;
        capacity = n;
    }

    void clear(void) {
        if (buff) {
            delete []buff;
        }
        buff = 0;
        curcnt = 0;
        capacity = 0;
        resize(ChunkSize);
    }

private:
    T* buff;
    int curcnt;
    int capacity;
};

#endif
