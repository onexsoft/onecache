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

#include <memory.h>

#ifdef WIN32
#include <Windows.h>
#else
#include <unistd.h>
#include <pthread.h>
#endif

#include "thread.h"

class ThreadPrivate
{
public:
    ThreadPrivate(Thread* thread) :
        m_isRunning(false),
        m_thread(thread)
    {}
    virtual ~ThreadPrivate(void) {
        if (m_isRunning) {
            terminate();
        }
    }

    virtual void start(void) {}
    virtual void terminate(void) {}
    bool isRunning(void) const { return m_isRunning; }

    bool m_isRunning;
    Thread* m_thread;
};

#ifdef WIN32
class WinThread : public ThreadPrivate
{
public:
    WinThread(Thread* thread) :
        ThreadPrivate(thread),
        m_tHandle(INVALID_HANDLE_VALUE)
    {}

    ~WinThread(void) {}

    virtual void start(void)
    {
        m_tHandle = ::CreateThread(NULL, 0,
                                   (LPTHREAD_START_ROUTINE)WinThreadEntry,
                                   (void*)m_thread,
                                   NULL,
                                   NULL);
    }

    virtual void terminate(void)
    {
        ::TerminateThread(m_tHandle, 0);
        m_isRunning = false;
        m_tHandle = INVALID_HANDLE_VALUE;
    }

    static DWORD WINAPI WinThreadEntry(LPVOID lp)
    {
        Thread* thread = (Thread*)lp;
        thread->m_priv->m_isRunning = true;
        thread->run();
        thread->m_priv->m_isRunning = false;
        return 0;
    }
private:
    HANDLE m_tHandle;
};

#else

class UnixThread : public ThreadPrivate
{
public:
    UnixThread(Thread* thread) :
        ThreadPrivate(thread)
    {}
    ~UnixThread(void) {}

    virtual void start(void) {
        pthread_create(&m_thread_id, NULL, UnixThreadEntry, m_thread);
    }

    virtual void terminate(void) {
        pthread_cancel(m_thread_id);
        m_isRunning = false;
    }

    static void* UnixThreadEntry(void* lp)
    {
        Thread* thread = (Thread*)lp;
        thread->m_priv->m_isRunning = true;
        thread->run();
        thread->m_priv->m_isRunning = false;
        return NULL;
    }

private:
    pthread_t m_thread_id;
};

#endif

Thread::Thread(void) : m_priv(NULL)
{
#ifdef WIN32
    m_priv = new WinThread(this);
#else
    m_priv = new UnixThread(this);
#endif
}

Thread::~Thread(void)
{
    delete m_priv;
}

void Thread::start(void)
{
    if (!isRunning()) {
        m_priv->start();
    }
}

void Thread::terminate(void)
{
    if (isRunning()) {
        m_priv->terminate();
    }
}

bool Thread::isRunning(void) const
{
    return m_priv->isRunning();
}

void Thread::sleep(int msec)
{
#ifdef WIN32
    Sleep(msec);
#else
    usleep(msec * 1000);
#endif
}

Thread::tid_t Thread::currentThreadId(void)
{
#ifdef WIN32
    return (tid_t)::GetCurrentThreadId();
#else
    pthread_t tid = pthread_self();
    tid_t thread_id = 0;
    memcpy(&thread_id, &tid, sizeof(tid_t));
    return thread_id;
#endif
}
