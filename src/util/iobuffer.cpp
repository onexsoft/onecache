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
#include <stdarg.h>
#include <stdio.h>

#include "util/string.h"
#include "iobuffer.h"

IOBuffer::IOBuffer(void)
{
    m_capacity = sizeof(m_data);
    m_offset = 0;
    m_ptr = m_data;
}

IOBuffer::IOBuffer(const IOBuffer &rhs)
{
    *this = rhs;
}

IOBuffer::~IOBuffer(void)
{
    clear();
}

IOBuffer &IOBuffer::operator=(const IOBuffer &rhs)
{
    if (this != &rhs) {
        clear();

        m_capacity = rhs.m_capacity;
        m_offset = rhs.m_offset;
        if (m_offset > ChunkSize) {
            m_ptr = new char[m_capacity];
        }
        memcpy(m_ptr, rhs.m_ptr, m_offset);
    }
    return *this;
}

void IOBuffer::reserve(int size)
{
    if (size <= m_capacity) {
        return;
    }

    int append_chunk_cnt = size / ChunkSize + 1;
    appendChunk(append_chunk_cnt);
}

void IOBuffer::appendFormatString(const char *format, ...)
{
    if (!format) {
        return;
    }

    char buffer[10240];
    va_list marker;
    va_start(marker, format);
    int len = vsprintf(buffer, format, marker);
    va_end(marker);

    append(buffer, len);
}

void IOBuffer::append(const char *data, int size)
{
    if (size == -1) {
        size = strlen(data);
    }

    int need_size = m_offset + size;
    if (need_size > m_capacity) {
        int append_chunk_cnt = (need_size - m_capacity) / ChunkSize + 1;
        appendChunk(append_chunk_cnt);
        append(data, size);
    } else {
        memcpy(m_ptr + m_offset, data, size);
        m_offset += size;
    }
}

void IOBuffer::append(const IOBuffer &rhs)
{
    append(rhs.data(), rhs.size());
}

void IOBuffer::clear(void)
{
    if (m_capacity > ChunkSize) {
        delete []m_ptr;
    }
    m_capacity = sizeof(m_data);
    m_offset = 0;
    m_ptr = m_data;
}

IOBuffer::DirectCopy IOBuffer::beginCopy(void)
{
    int freeSize = m_capacity - m_offset;
    if (freeSize < (ChunkSize * 0.01)) {
        appendChunk(1);
        freeSize = m_capacity - m_offset;
    }
    DirectCopy cp;
    cp.address = m_ptr + m_offset;
    cp.maxsize = freeSize;
    return cp;
}

void IOBuffer::endCopy(int cpsize)
{
    if (cpsize > 0 && cpsize <= (m_capacity - m_offset)) {
        m_offset += cpsize;
    }
}

void IOBuffer::appendChunk(int nums)
{
    if (nums <= 0) {
        return;
    }
    int new_size = m_capacity + nums * ChunkSize;
    char* tmp = new char[new_size];
    memcpy(tmp, m_ptr, m_offset);
    if (m_capacity > ChunkSize) {
        delete []m_ptr;
    }
    m_ptr = tmp;
    m_capacity = new_size;
}

