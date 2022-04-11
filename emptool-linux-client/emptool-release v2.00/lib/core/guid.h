/* guid.h
 *
 * (C) 2001 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.10 01-Apr-2003 18:52 rob:)
 */

#include "packing.h"

#if USE_PACK_HEADERS
#include <pshpack1.h>
#endif

#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef PACKED_PRE struct _GUID {
    UINT32 Data1;
    UINT16 Data2;
    UINT16 Data3;
    BYTE Data4[8];
} PACKED_POST GUID;
#endif // !defined(GUID_DEFINED)

#if USE_PACK_HEADERS
#include <poppack.h>
#endif

#if !defined(WIN32)
inline bool IsEqualGUID(const GUID &lhs, const GUID &rhs) { return memcmp(&lhs, &rhs, sizeof(GUID)) == 0; }
inline bool operator==(const GUID &lhs, const GUID &rhs) { return IsEqualGUID(lhs, rhs); }
inline bool operator!=(const GUID &lhs, const GUID &rhs) { return !IsEqualGUID(lhs, rhs); }

#if !defined(EXTERN_C)
 #ifdef __cplusplus
  #define EXTERN_C extern "C"
 #else
  #define EXTERN_C
 #endif // __cplusplus
#endif // EXTERN_C

#if defined(INITGUID) && !defined(WIN32)
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const GUID name = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#else
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const GUID name
#endif // INITGUID

#endif // WIN32
