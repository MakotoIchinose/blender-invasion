/*
 * Copyright 2011-2013 Blender Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __UTIL_HASH_H__
#define __UTIL_HASH_H__

#include "util/util_types.h"

CCL_NAMESPACE_BEGIN

#ifdef __KERNEL_OPENCL__

#  define float_as_uint(f) as_uint((f))

#else
#  ifdef __KERNEL_CUDA__

#    define float_as_uint(f) __float_as_uint((f))

#  else

#    define float_as_uint(f) *(uint *)&(f)

#  endif
#endif

/* Jenkins Lookup3 Hash Functions.
 * http://burtleburtle.net/bob/c/lookup3.c
 */

#define rot(x, k) (((x) << (k)) | ((x) >> (32 - (k))))

#define mix(a, b, c) \
  { \
    a -= c; \
    a ^= rot(c, 4); \
    c += b; \
    b -= a; \
    b ^= rot(a, 6); \
    a += c; \
    c -= b; \
    c ^= rot(b, 8); \
    b += a; \
    a -= c; \
    a ^= rot(c, 16); \
    c += b; \
    b -= a; \
    b ^= rot(a, 19); \
    a += c; \
    c -= b; \
    c ^= rot(b, 4); \
    b += a; \
  }

#define final(a, b, c) \
  { \
    c ^= b; \
    c -= rot(b, 14); \
    a ^= c; \
    a -= rot(c, 11); \
    b ^= a; \
    b -= rot(a, 25); \
    c ^= b; \
    c -= rot(b, 16); \
    a ^= c; \
    a -= rot(c, 4); \
    b ^= a; \
    b -= rot(a, 14); \
    c ^= b; \
    c -= rot(b, 24); \
  }

ccl_device_inline uint hash_uint(uint kx)
{
  uint a, b, c;
  a = b = c = 0xdeadbeef + (1 << 2) + 13;

  a += kx;
  final(a, b, c);

  return c;
}

ccl_device_inline uint hash_uint2(uint kx, uint ky)
{
  uint a, b, c;
  a = b = c = 0xdeadbeef + (2 << 2) + 13;

  b += ky;
  a += kx;
  final(a, b, c);

  return c;
}

ccl_device_inline uint hash_uint3(uint kx, uint ky, uint kz)
{
  uint a, b, c;
  a = b = c = 0xdeadbeef + (3 << 2) + 13;

  c += kz;
  b += ky;
  a += kx;
  final(a, b, c);

  return c;
}

ccl_device_inline uint hash_uint4(uint kx, uint ky, uint kz, uint kw)
{
  uint a, b, c;
  a = b = c = 0xdeadbeef + (4 << 2) + 13;

  a += kx;
  b += ky;
  c += kz;
  mix(a, b, c);

  a += kw;
  final(a, b, c);

  return c;
}

#undef rot
#undef final
#undef mix

/* **** Hashing uints into the [0, 1] range. **** */

ccl_device_inline float hash_uint_01(uint kx)
{
  return (float)hash_uint(kx) * (1.0f / (float)0xFFFFFFFF);
}

ccl_device_inline float hash_uint2_01(uint kx, uint ky)
{
  return (float)hash_uint2(kx, ky) * (1.0f / (float)0xFFFFFFFF);
}

ccl_device_inline float hash_uint3_01(uint kx, uint ky, uint kz)
{
  return (float)hash_uint3(kx, ky, kz) * (1.0f / (float)0xFFFFFFFF);
}

ccl_device_inline float hash_uint4_01(uint kx, uint ky, uint kz, uint kw)
{
  return (float)hash_uint4(kx, ky, kz, kw) * (1.0f / (float)0xFFFFFFFF);
}

/* **** Hashing floats into the [0, 1] range. **** */

ccl_device_inline float hash_float_01(float kx)
{
  return hash_uint_01(float_as_uint(kx));
}

ccl_device_inline float hash_float2_01(float kx, float ky)
{
  return hash_uint2_01(float_as_uint(kx), float_as_uint(ky));
}

ccl_device_inline float hash_float3_01(float kx, float ky, float kz)
{
  return hash_uint3_01(float_as_uint(kx), float_as_uint(ky), float_as_uint(kz));
}

ccl_device_inline float hash_float4_01(float kx, float ky, float kz, float kw)
{
  return hash_uint4_01(float_as_uint(kx), float_as_uint(ky), float_as_uint(kz), float_as_uint(kw));
}

#ifndef __KERNEL_GPU__
static inline uint hash_string(const char *str)
{
  uint i = 0, c;

  while ((c = *str++))
    i = i * 37 + c;

  return i;
}
#endif

CCL_NAMESPACE_END

#endif /* __UTIL_HASH_H__ */
