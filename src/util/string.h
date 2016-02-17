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

#ifndef STRING_H
#define STRING_H

#include <string.h>

#ifdef WIN32
#pragma warning(disable: 4996)

#define strcasecmp stricmp
#define strncasecmp strnicmp
#endif

class String
{
public:
    String(void) : m_str(NULL), m_len(0), m_dup(false) {}
    String(const String& rhs) : m_str(NULL), m_len(0), m_dup(false) { *this = rhs; }
    String(const char* s, int len = -1, bool dup = false) {
        if (len == -1) {
            len = strlen(s);
        }
        m_str = (char*)s;
        m_len = len;
        m_dup = dup;
        if (m_dup) {
            char* p = new char[m_len+1];
            strncpy(p, m_str, m_len);
            p[len] = 0;
            m_str = p;
        }
    }
    String& operator=(const String& rhs) {
        if (this != &rhs) {
            if (m_dup) {
                delete []m_str;
            }
            m_str = rhs.m_str;
            m_len = rhs.m_len;
            m_dup = rhs.m_dup;
            if (m_dup) {
                char* p = new char[m_len+1];
                strncpy(p, m_str, m_len);
                p[m_len] = 0;
                m_str = p;
            }
        }
        return *this;
    }

    ~String(void) {
        if (m_str && m_dup) {
            delete []m_str;
        }
    }

    friend bool operator ==(const String& lhs, const String& rhs) {
        if (lhs.m_len != rhs.m_len) {
            return false;
        }
        return (strncmp(lhs.m_str, rhs.m_str, lhs.m_len) == 0);
    }
    friend bool operator !=(const String& lhs, const String& rhs) {
        return !(lhs == rhs);
    }

    inline const char* data(void) const { return m_str; }
    inline int length(void) const { return m_len; }

private:
    char* m_str;
    unsigned int m_len : 31;
    unsigned int m_dup : 1;
};

#endif
