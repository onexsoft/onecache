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

#include "hash.h"

#define hashsize(n) ((unsigned int)1<<(n))
#define hashmask(n) (hashsize(n)-1)
#define rot(x,k) (((x)<<(k)) | ((x)>>(32-(k))))

#define mix(a,b,c) \
{ \
  a -= c;  a ^= rot(c, 4);  c += b; \
  b -= a;  b ^= rot(a, 6);  a += c; \
  c -= b;  c ^= rot(b, 8);  b += a; \
  a -= c;  a ^= rot(c,16);  c += b; \
  b -= a;  b ^= rot(a,19);  a += c; \
  c -= b;  c ^= rot(b, 4);  b += a; \
}

#define final(a,b,c) \
{ \
  c ^= b; c -= rot(b,14); \
  a ^= c; a -= rot(c,11); \
  b ^= a; b -= rot(a,25); \
  c ^= b; c -= rot(b,16); \
  a ^= c; a -= rot(c,4);  \
  b ^= a; b -= rot(a,14); \
  c ^= b; c -= rot(b,24); \
}

#define JENKINS_INITVAL 13

/*
 * jenkins_hash() -- hash a variable-length key into a 32-bit value
 *  k       : the key (the unaligned variable-length array of bytes)
 *  length  : the length of the key, counting by bytes
 *  initval : can be any 4-byte value
 * Returns a 32-bit value.  Every bit of the key affects every bit of
 * the return value.  Two keys differing by one or two bits will have
 * totally different hash values.

 * The best hash table sizes are powers of 2.  There is no need to do
 * mod a prime (mod is sooo slow!).  If you need less than 32 bits,
 * use a bitmask.  For example, if you need only 10 bits, do
 *   h = (h & hashmask(10));
 * In which case, the hash table should have hashsize(10) elements.
 */

unsigned int hash_jenkins(const char *key, int length)
{
  unsigned int a,b,c;                                          /* internal state */
  union { const void *ptr; int i; } u;     /* needed for Mac Powerbook G4 */

  /* Set up the internal state */
  a = b = c = 0xdeadbeef + ((unsigned int)length) + JENKINS_INITVAL;

  u.ptr = key;
#ifndef WORDS_BIGENDIAN
  if ((u.i & 0x3) == 0)
  {
    const unsigned int *k = (const unsigned int *)key;         /* read 32-bit chunks */

    /*------ all but last block: aligned reads and affect 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      b += k[1];
      c += k[2];
      mix(a,b,c);
      length -= 12;
      k += 3;
    }

    /*----------------------------- handle the last (probably partial) block */
    /*
     * "k[2]&0xffffff" actually reads beyond the end of the string, but
     * then masks off the part it's not allowed to read.  Because the
     * string is aligned, the masked-off tail is in the same word as the
     * rest of the string.  Every machine with memory protection I've seen
     * does it on word boundaries, so is OK with this.  But VALGRIND will
     * still catch it and complain.  The masking trick does make the hash
     * noticeably faster for short strings (like English words).
     */
    switch(length)
    {
    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
    case 11: c+=k[2]&0xffffff; b+=k[1]; a+=k[0]; break;
    case 10: c+=k[2]&0xffff; b+=k[1]; a+=k[0]; break;
    case 9 : c+=k[2]&0xff; b+=k[1]; a+=k[0]; break;
    case 8 : b+=k[1]; a+=k[0]; break;
    case 7 : b+=k[1]&0xffffff; a+=k[0]; break;
    case 6 : b+=k[1]&0xffff; a+=k[0]; break;
    case 5 : b+=k[1]&0xff; a+=k[0]; break;
    case 4 : a+=k[0]; break;
    case 3 : a+=k[0]&0xffffff; break;
    case 2 : a+=k[0]&0xffff; break;
    case 1 : a+=k[0]&0xff; break;
    case 0 : return c;              /* zero length strings require no mixing */
    default: return c;
    }

  }
  else if ((u.i & 0x1) == 0)
  {
    const unsigned short *k = (const unsigned short *)key;         /* read 16-bit chunks */
    const unsigned char  *k8;

    /*--------------- all but last block: aligned reads and different mixing */
    while (length > 12)
    {
      a += k[0] + (((unsigned int)k[1])<<16);
      b += k[2] + (((unsigned int)k[3])<<16);
      c += k[4] + (((unsigned int)k[5])<<16);
      mix(a,b,c);
      length -= 12;
      k += 6;
    }

    /*----------------------------- handle the last (probably partial) block */
    k8 = (const unsigned char *)k;
    switch(length)
    {
    case 12: c+=k[4]+(((unsigned int)k[5])<<16);
             b+=k[2]+(((unsigned int)k[3])<<16);
             a+=k[0]+(((unsigned int)k[1])<<16);
             break;
    case 11: c+=((unsigned int)k8[10])<<16;     /* fall through */
    case 10: c+=k[4];
             b+=k[2]+(((unsigned int)k[3])<<16);
             a+=k[0]+(((unsigned int)k[1])<<16);
             break;
    case 9 : c+=k8[8];                      /* fall through */
    case 8 : b+=k[2]+(((unsigned int)k[3])<<16);
             a+=k[0]+(((unsigned int)k[1])<<16);
             break;
    case 7 : b+=((unsigned int)k8[6])<<16;      /* fall through */
    case 6 : b+=k[2];
             a+=k[0]+(((unsigned int)k[1])<<16);
             break;
    case 5 : b+=k8[4];                      /* fall through */
    case 4 : a+=k[0]+(((unsigned int)k[1])<<16);
             break;
    case 3 : a+=((unsigned int)k8[2])<<16;      /* fall through */
    case 2 : a+=k[0];
             break;
    case 1 : a+=k8[0];
             break;
    case 0 : return c;                     /* zero length requires no mixing */
    default: return c;
    }

  }
  else
  {                        /* need to read the key one byte at a time */
#endif /* little endian */
    const unsigned char *k = (const unsigned char *)key;

    /*--------------- all but the last block: affect some 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      a += ((unsigned int)k[1])<<8;
      a += ((unsigned int)k[2])<<16;
      a += ((unsigned int)k[3])<<24;
      b += k[4];
      b += ((unsigned int)k[5])<<8;
      b += ((unsigned int)k[6])<<16;
      b += ((unsigned int)k[7])<<24;
      c += k[8];
      c += ((unsigned int)k[9])<<8;
      c += ((unsigned int)k[10])<<16;
      c += ((unsigned int)k[11])<<24;
      mix(a,b,c);
      length -= 12;
      k += 12;
    }

    /*-------------------------------- last block: affect all 32 bits of (c) */
    switch(length)                   /* all the case statements fall through */
    {
    case 12: c+=((unsigned int)k[11])<<24;
    case 11: c+=((unsigned int)k[10])<<16;
    case 10: c+=((unsigned int)k[9])<<8;
    case 9 : c+=k[8];
    case 8 : b+=((unsigned int)k[7])<<24;
    case 7 : b+=((unsigned int)k[6])<<16;
    case 6 : b+=((unsigned int)k[5])<<8;
    case 5 : b+=k[4];
    case 4 : a+=((unsigned int)k[3])<<24;
    case 3 : a+=((unsigned int)k[2])<<16;
    case 2 : a+=((unsigned int)k[1])<<8;
    case 1 : a+=k[0];
             break;
    case 0 : return c;
    default : return c;
    }
#ifndef WORDS_BIGENDIAN
  }
#endif

  final(a,b,c);
  return c;
}

