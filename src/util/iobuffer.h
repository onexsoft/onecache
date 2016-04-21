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

#ifndef IOBUFFER_H
#define IOBUFFER_H

class IOBuffer
{
public:
    struct DirectCopy {
        char* address;
        int maxsize;
    };

    enum { ChunkSize = 1024 * 64 };
    IOBuffer(void);
    IOBuffer(const IOBuffer& rhs);
    ~IOBuffer(void);
    IOBuffer& operator=(const IOBuffer& rhs);

    void reserve(int size);

    void appendFormatString(const char* format, ...);
    void append(const char* data, int size = -1);
    void append(const IOBuffer& rhs);
    void clear(void);

    char* data(void) { return m_ptr; }
    const char* data(void) const { return m_ptr; }
    int size(void) const { return m_offset; }

    bool isEmpty(void) const { return (m_offset == 0); }

    //Fast copy
    DirectCopy beginCopy(void);
    void endCopy(int cpsize);

private:
    void appendChunk(int nums);

private:
    int m_capacity;
    int m_offset;
    char m_data[ChunkSize];
    char* m_ptr;
};


#endif
