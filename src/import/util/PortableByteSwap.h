//  :copyright: (c) 2014-2016 Mathias Panzenböck
//  :license: Public Domain/MIT, see licenses/MIT.txt for more details.
//  :modified: Alex Huszagh, 2016. Added Windows-specific pre-processor definitions and byte-swap algorithms.
/** Endian utilties for determining and swapping byte order in
 *  applications.
 */

#include <algorithm>

#pragma once

// MACROS
// ------

// INTEGERS
// --------

#if (defined(_WIN16) || defined(_WIN32) || defined(_WIN64)) && !defined(__WINDOWS__)

#   define __WINDOWS__

#endif

#if defined(__linux__) || defined(__CYGWIN__)

#   include <endian.h>

#elif defined(__APPLE__)

#   include <libkern/OSByteOrder.h>

#   define htobe16(x) OSSwapHostToBigInt16(x)
#   define htole16(x) OSSwapHostToLittleInt16(x)
#   define be16toh(x) OSSwapBigToHostInt16(x)
#   define le16toh(x) OSSwapLittleToHostInt16(x)

#   define htobe32(x) OSSwapHostToBigInt32(x)
#   define htole32(x) OSSwapHostToLittleInt32(x)
#   define be32toh(x) OSSwapBigToHostInt32(x)
#   define le32toh(x) OSSwapLittleToHostInt32(x)

#   define htobe64(x) OSSwapHostToBigInt64(x)
#   define htole64(x) OSSwapHostToLittleInt64(x)
#   define be64toh(x) OSSwapBigToHostInt64(x)
#   define le64toh(x) OSSwapLittleToHostInt64(x)

#   define __BYTE_ORDER    BYTE_ORDER
#   define __BIG_ENDIAN    BIG_ENDIAN
#   define __LITTLE_ENDIAN LITTLE_ENDIAN
#   define __PDP_ENDIAN    PDP_ENDIAN

#elif defined(__OpenBSD__)

#   include <sys/endian.h>

#elif defined(__NetBSD__) || defined(__FreeBSD__) || defined(__DragonFly__)

#   include <sys/endian.h>

#   define be16toh(x) betoh16(x)
#   define le16toh(x) letoh16(x)

#   define be32toh(x) betoh32(x)
#   define le32toh(x) letoh32(x)

#   define be64toh(x) betoh64(x)
#   define le64toh(x) letoh64(x)

#elif defined(__WINDOWS__)

#   include <winsock2.h>
#   include <Windows.h>
#   pragma comment(lib, "Ws2_32.lib")

#   ifndef BIG_ENDIAN
        /* Windows does not always set byte order, such as MSVC */
#       define LITTLE_ENDIAN REG_DWORD_LITTLE_ENDIAN
#       define BIG_ENDIAN REG_DWORD_BIG_ENDIAN
#       if REG_DWORD == REG_DWORD_BIG_ENDIAN
#           define BYTE_ORDER BIG_ENDIAN
#       else
#           define BYTE_ORDER LITTLE_ENDIAN
#       endif

#   endif

#   if BYTE_ORDER == LITTLE_ENDIAN

#       define htobe16(x) htons(x)
#       define htole16(x) (x)
#       define be16toh(x) ntohs(x)
#       define le16toh(x) (x)

#       define htobe32(x) htonl(x)
#       define htole32(x) (x)
#       define be32toh(x) ntohl(x)
#       define le32toh(x) (x)

#       define htobe64(x) htonll(x)
#       define htole64(x) (x)
#       define be64toh(x) ntohll(x)
#       define le64toh(x) (x)

#   elif BYTE_ORDER == BIG_ENDIAN

        /* that would be xbox 360 */
#       define htobe16(x) (x)
#       define htole16(x) __builtin_bswap16(x)
#       define be16toh(x) (x)
#       define le16toh(x) __builtin_bswap16(x)

#       define htobe32(x) (x)
#       define htole32(x) __builtin_bswap32(x)
#       define be32toh(x) (x)
#       define le32toh(x) __builtin_bswap32(x)

#       define htobe64(x) (x)
#       define htole64(x) __builtin_bswap64(x)
#       define be64toh(x) (x)
#       define le64toh(x) __builtin_bswap64(x)

#   else

#       error byte order not supported

#   endif

#   define __BYTE_ORDER    BYTE_ORDER
#   define __BIG_ENDIAN    BIG_ENDIAN
#   define __LITTLE_ENDIAN LITTLE_ENDIAN
#   define __PDP_ENDIAN    PDP_ENDIAN

#else

#   error platform not supported

#endif

// FLOATS
// ------

#ifndef __FLOAT_WORD_ORDER
    #define __FLOAT_WORD_ORDER  LITTLE_ENDIAN
#endif

#ifndef FLOAT_WORD_ORDER
    #define FLOAT_WORD_ORDER    __FLOAT_WORD_ORDER
#endif


// DEFINITIONS
// -----------


/** Reverse byte order for a single C primitive.
 *
 *  @param value        Value to swap byte order (inplace) for
 */
template <typename T>
void byteswap(T *value)
{
    const int size = sizeof(*value);

    char *cast = (char *) value;

    char *lo = cast;
    char *hi = cast + size - 1;
    char swap;

    while (lo < hi) {
        swap = *lo;
        *lo++ = *hi;
        *hi-- = swap;
    }
}


/** Reverse byte order for an array of C primitives.
 *
 *  @param value        Array of values to swap byteorder for
 *  @param size         Size of array
 */
template <typename T>
void byteswap(T *value, const unsigned int &size)
{
    #pragma omp parallel for
    for (int i = 0; i < size; i++) {
        byteswap(&value[i]);
    }
}

