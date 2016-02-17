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

#ifdef WIN32
#include <Windows.h>
#endif

#include "locker.h"

Mutex::Mutex(void)
{
#ifdef WIN32
    m_hMutex = ::CreateMutexA(NULL, FALSE, NULL);
#else
    pthread_mutex_init(&m_mutex, NULL);
#endif
}

Mutex::~Mutex(void)
{
#ifdef WIN32
    ::CloseHandle(m_hMutex);
#else
    pthread_mutex_destroy(&m_mutex);
#endif
}

void Mutex::lock(void)
{
#ifdef WIN32
    ::WaitForSingleObject(m_hMutex, INFINITE);
#else
    pthread_mutex_lock(&m_mutex);
#endif
}

void Mutex::unlock(void)
{
#ifdef WIN32
    ::ReleaseMutex(m_hMutex);
#else
    pthread_mutex_unlock(&m_mutex);
#endif
}


SpinLocker::SpinLocker(void)
{
#ifndef WIN32
    pthread_spin_init(&m_spinlock, PTHREAD_PROCESS_SHARED);
#endif
}

SpinLocker::~SpinLocker(void)
{
#ifndef WIN32
    pthread_spin_destroy(&m_spinlock);
#endif
}

void SpinLocker::lock(void)
{
#ifdef WIN32
    m_mutex.lock();
#else
    pthread_spin_lock(&m_spinlock);
#endif
}

void SpinLocker::unlock(void)
{
#ifdef WIN32
    m_mutex.unlock();
#else
    pthread_spin_unlock(&m_spinlock);
#endif
}

