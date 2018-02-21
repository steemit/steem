/*
 * CyoEncode.c - part of the CyoEncode library
 *
 * Copyright (c) 2009-2012, Graham Bull.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "CyoEncode.h"

#include <assert.h>

/********************************** Shared ***********************************/

static size_t cyoBaseXXEncodeGetLength( size_t size, size_t inputBytes, size_t outputBytes )
{
    return (((size + inputBytes - 1) / inputBytes) * outputBytes) + 1; /*plus terminator*/
}

/****************************** Base16 Encoding ******************************/

static const size_t BASE16_INPUT = 1;
static const size_t BASE16_OUTPUT = 2;
static const char* const BASE16_TABLE = "0123456789ABCDEF";

size_t cyoBase16EncodeGetLength( size_t size )
{
    return cyoBaseXXEncodeGetLength( size, BASE16_INPUT, BASE16_OUTPUT );
}

size_t cyoBase16Encode( char* dest, const void* src, size_t size )
{
    /*
     * output 2 bytes for every 1 input:
     *
     *                 inputs: 1
     * outputs: 1 = ----1111 = 1111----
     *          2 = ----2222 = ----2222
     */

    if (dest && src)
    {
        unsigned char* pSrc = (unsigned char*)src;
        size_t dwSrcSize = size;
        size_t dwDestSize = 0;
        unsigned char ch;

        while (dwSrcSize >= 1)
        {
            /* 1 input */
            ch = *pSrc++;
            dwSrcSize -= BASE16_INPUT;

            /* 2 outputs */
            *dest++ = BASE16_TABLE[ (ch & 0xf0) >> 4 ];
            *dest++ = BASE16_TABLE[ (ch & 0x0f)      ];
            dwDestSize += BASE16_OUTPUT;
        }
        *dest++ = '\x0'; /*append terminator*/

        return dwDestSize;
    }
    else
        return 0; /*ERROR - null pointer*/
}

/****************************** Base32 Encoding ******************************/

static const size_t BASE32_INPUT = 5;
static const size_t BASE32_OUTPUT = 8;
static const char* const BASE32_TABLE = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567=";

size_t cyoBase32EncodeGetLength( size_t size )
{
    return cyoBaseXXEncodeGetLength( size, BASE32_INPUT, BASE32_OUTPUT );
}

size_t cyoBase32Encode( char* dest, const void* src, size_t size )
{
    /*
     * output 8 bytes for every 5 input:
     *
     *                 inputs: 1        2        3        4        5
     * outputs: 1 = ---11111 = 11111---
     *          2 = ---222XX = -----222 XX------
     *          3 = ---33333 =          --33333-
     *          4 = ---4XXXX =          -------4 XXXX----
     *          5 = ---5555X =                   ----5555 X-------
     *          6 = ---66666 =                            -66666--
     *          7 = ---77XXX =                            ------77 XXX-----
     *          8 = ---88888 =                                     ---88888
     */

    if (dest && src)
    {
        unsigned char* pSrc = (unsigned char*)src;
        size_t dwSrcSize = size;
        size_t dwDestSize = 0;
        size_t dwBlockSize;
        unsigned char n1, n2, n3, n4, n5, n6, n7, n8;

        while (dwSrcSize >= 1)
        {
            /* Encode inputs */
            dwBlockSize = (dwSrcSize < BASE32_INPUT ? dwSrcSize : BASE32_INPUT);
            n1 = n2 = n3 = n4 = n5 = n6 = n7 = n8 = 0;
            switch (dwBlockSize)
            {
            case 5:
                n8  =  (pSrc[ 4 ] & 0x1f);
                n7  = ((pSrc[ 4 ] & 0xe0) >> 5);
            case 4:
                n7 |= ((pSrc[ 3 ] & 0x03) << 3);
                n6  = ((pSrc[ 3 ] & 0x7c) >> 2);
                n5  = ((pSrc[ 3 ] & 0x80) >> 7);
            case 3:
                n5 |= ((pSrc[ 2 ] & 0x0f) << 1);
                n4  = ((pSrc[ 2 ] & 0xf0) >> 4);
            case 2:
                n4 |= ((pSrc[ 1 ] & 0x01) << 4);
                n3  = ((pSrc[ 1 ] & 0x3e) >> 1);
                n2  = ((pSrc[ 1 ] & 0xc0) >> 6);
            case 1:
                n2 |= ((pSrc[ 0 ] & 0x07) << 2);
                n1  = ((pSrc[ 0 ] & 0xf8) >> 3);
                break;

            default:
                assert( 0 );
            }
            pSrc += dwBlockSize;
            dwSrcSize -= dwBlockSize;

            /* Validate */
            assert( n1 <= 31 );
            assert( n2 <= 31 );
            assert( n3 <= 31 );
            assert( n4 <= 31 );
            assert( n5 <= 31 );
            assert( n6 <= 31 );
            assert( n7 <= 31 );
            assert( n8 <= 31 );

            /* Padding */
            switch (dwBlockSize)
            {
            case 1: n3 = n4 = 32;
            case 2: n5 = 32;
            case 3: n6 = n7 = 32;
            case 4: n8 = 32;
            case 5:
                break;

            default:
                assert( 0 );
            }

            /* 8 outputs */
            *dest++ = BASE32_TABLE[ n1 ];
            *dest++ = BASE32_TABLE[ n2 ];
            *dest++ = BASE32_TABLE[ n3 ];
            *dest++ = BASE32_TABLE[ n4 ];
            *dest++ = BASE32_TABLE[ n5 ];
            *dest++ = BASE32_TABLE[ n6 ];
            *dest++ = BASE32_TABLE[ n7 ];
            *dest++ = BASE32_TABLE[ n8 ];
            dwDestSize += BASE32_OUTPUT;
        }
        *dest++ = '\x0'; /*append terminator*/

        return dwDestSize;
    }
    else
        return 0; /*ERROR - null pointer*/
}

/****************************** Base64 Encoding ******************************/

static const size_t BASE64_INPUT = 3;
static const size_t BASE64_OUTPUT = 4;
static const char* const BASE64_TABLE = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

size_t cyoBase64EncodeGetLength( size_t size )
{
    return cyoBaseXXEncodeGetLength( size, BASE64_INPUT, BASE64_OUTPUT );
}

size_t cyoBase64Encode( char* dest, const void* src, size_t size )
{
    /*
     * output 4 bytes for every 3 input:
     *
     *                 inputs: 1        2        3
     * outputs: 1 = --111111 = 111111--
     *          2 = --22XXXX = ------22 XXXX----
     *          3 = --3333XX =          ----3333 XX------
     *          4 = --444444 =                   --444444
     */

    if (dest && src)
    {
        unsigned char* pSrc = (unsigned char*)src;
        size_t dwSrcSize = size;
        size_t dwDestSize = 0;
        size_t dwBlockSize = 0;
        unsigned char n1, n2, n3, n4;

        while (dwSrcSize >= 1)
        {
            /* Encode inputs */
            dwBlockSize = (dwSrcSize < BASE64_INPUT ? dwSrcSize : BASE64_INPUT);
            n1 = n2 = n3 = n4 = 0;
            switch (dwBlockSize)
            {
            case 3:
                n4  =  (pSrc[ 2 ] & 0x3f);
                n3  = ((pSrc[ 2 ] & 0xc0) >> 6);
            case 2:
                n3 |= ((pSrc[ 1 ] & 0x0f) << 2);
                n2  = ((pSrc[ 1 ] & 0xf0) >> 4);
            case 1:
                n2 |= ((pSrc[ 0 ] & 0x03) << 4);
                n1  = ((pSrc[ 0 ] & 0xfc) >> 2);
                break;

            default:
                assert( 0 );
            }
            pSrc += dwBlockSize;
            dwSrcSize -= dwBlockSize;

            /* Validate */
            assert( n1 <= 63 );
            assert( n2 <= 63 );
            assert( n3 <= 63 );
            assert( n4 <= 63 );

            /* Padding */
            switch (dwBlockSize)
            {
            case 1: n3 = 64;
            case 2: n4 = 64;
            case 3:
                break;

            default:
                assert( 0 );
            }

            /* 4 outputs */
            *dest++ = BASE64_TABLE[ n1 ];
            *dest++ = BASE64_TABLE[ n2 ];
            *dest++ = BASE64_TABLE[ n3 ];
            *dest++ = BASE64_TABLE[ n4 ];
            dwDestSize += BASE64_OUTPUT;
        }
        *dest++ = '\x0'; /*append terminator*/

        return dwDestSize;
    }
    else
        return 0; /*ERROR - null pointer*/
}
