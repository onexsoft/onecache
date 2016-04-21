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


#ifndef THREAD_H
#define THREAD_H

class ThreadPrivate;
class Thread
{
public:
    typedef unsigned long long tid_t;

    Thread(void);
    virtual ~Thread(void);

    void start(void);
    void terminate(void);
    bool isRunning(void) const;

    static void sleep(int msec);
    static tid_t currentThreadId(void);

protected:
    virtual void run(void) = 0;

private:
    ThreadPrivate* m_priv;

    friend class WinThread;
    friend class UnixThread;
    Thread(const Thread& rhs);
    Thread& operator=(const Thread& rhs);
};

#endif
