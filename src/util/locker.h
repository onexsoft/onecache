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

#ifndef LOCKER_H
#define LOCKER_H

#ifdef WIN32
typedef void* HANDLE;
#else
#include <pthread.h>
#endif

class Mutex
{
public:
    Mutex(void);
    ~Mutex(void);

    void lock(void);
    void unlock(void);

#ifdef WIN32
    HANDLE m_hMutex;
#else
    pthread_mutex_t m_mutex;
#endif
};

class SpinLocker
{
public:
    SpinLocker(void);
    ~SpinLocker(void);

    void lock(void);
    void unlock(void);

private:
#ifndef WIN32
    pthread_spinlock_t m_spinlock;
#else
    Mutex m_mutex;
#endif
};

#endif
