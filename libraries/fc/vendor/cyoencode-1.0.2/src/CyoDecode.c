/*
 * CyoDecode.c - part of the CyoEncode library
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

#include "CyoDecode.h"

#include <assert.h>

#include <stdio.h> //TEMP

/********************************** Shared ***********************************/

static int cyoBaseXXValidate( const char* src, size_t size, size_t inputBytes, size_t maxPadding,
                              unsigned char maxValue, const unsigned char table[] )
{
    /*
     * returns 0 if the source is a valid baseXX encoding
     */

    if (!src)
        return -1; /*ERROR - NULL pointer*/

    if (size % inputBytes != 0)
        return -1; /*ERROR - extra characters*/

    /* check the bytes */
    for (; size >= 1; --size, ++src)
    {
        unsigned char ch = *src;
        if ((ch >= 0x80) || (table[ ch ] > maxValue))
            break;
    }

    /* check any padding */
    for (; 1 <= size && size <= maxPadding; --size, ++src)
    {
        unsigned char ch = *src;
        if ((ch >= 0x80) || (table[ ch ] != maxValue + 1))
            break;
    }

    /* if size isn't zero then the encoded string isn't valid */
    if (size != 0)
        return -2; /*ERROR - invalid baseXX character*/

    /* OK */
    return 0;
}

static size_t cyoBaseXXDecodeGetLength( size_t size, size_t inputBytes, size_t outputBytes )
{
    if (size % inputBytes != 0)
        return 0; /*ERROR - extra characters*/

    /* OK */
    return (((size + inputBytes - 1) / inputBytes) * outputBytes) + 1; /*plus terminator*/
}

/****************************** Base16 Decoding ******************************/

static const size_t BASE16_INPUT = 2;
static const size_t BASE16_OUTPUT = 1;
static const size_t BASE16_MAX_PADDING = 0;
static const unsigned char BASE16_MAX_VALUE = 15;
static const unsigned char BASE16_TABLE[ 0x80 ] = {
    /*00-07*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*08-0f*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*10-17*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*18-1f*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*20-27*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*28-2f*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*30-37*/ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, /*8 = '0'-'7'*/
    /*38-3f*/ 0x08, 0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /*2 = '8'-'9'*/
    /*40-47*/ 0xFF, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xFF, /*6 = 'A'-'F'*/
    /*48-4f*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*50-57*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*58-5f*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*60-67*/ 0xFF, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xFF, /*6 = 'a'-'f' (same as 'A'-'F')*/
    /*68-6f*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*70-77*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*78-7f*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

int cyoBase16Validate( const char* src, size_t size )
{
    return cyoBaseXXValidate( src, size, BASE16_INPUT, BASE16_MAX_PADDING, BASE16_MAX_VALUE, BASE16_TABLE );
}

size_t cyoBase16DecodeGetLength( size_t size )
{
    return cyoBaseXXDecodeGetLength( size, BASE16_INPUT, BASE16_OUTPUT );
}

size_t cyoBase16Decode( void* dest, const char* src, size_t size )
{
    /*
     * output 1 byte for every 2 input:
     *
     *               outputs: 1
     * inputs: 1 = ----1111 = 1111----
     *         2 = ----2222 = ----2222
     */

    if (dest && src && (size % BASE16_INPUT == 0))
    {
        unsigned char* pDest = (unsigned char*)dest;
        size_t dwSrcSize = size;
        size_t dwDestSize = 0;
        unsigned char in1, in2;

        while (dwSrcSize >= 1)
        {
            /* 2 inputs */
            in1 = *src++;
            in2 = *src++;
            dwSrcSize -= BASE16_INPUT;

            /* Validate ascii */
            if (in1 >= 0x80 || in2 >= 0x80)
                return 0; /*ERROR - invalid base16 character*/

            /* Convert ascii to base16 */
            in1 = BASE16_TABLE[ in1 ];
            in2 = BASE16_TABLE[ in2 ];

            /* Validate base16 */
            if (in1 > BASE16_MAX_VALUE || in2 > BASE16_MAX_VALUE)
                return 0; /*ERROR - invalid base16 character*/

            /* 1 output */
            *pDest++ = ((in1 << 4) | in2);
            dwDestSize += BASE16_OUTPUT;
        }
        *pDest++ = '\x0'; /*append terminator*/

        return dwDestSize;
    }
    else
        return 0; /*ERROR - null pointer, or size isn't a multiple of 2*/
}

/****************************** Base32 Decoding ******************************/

static const size_t BASE32_INPUT = 8;
static const size_t BASE32_OUTPUT = 5;
static const size_t BASE32_MAX_PADDING = 6;
static const unsigned char BASE32_MAX_VALUE = 31;
static const unsigned char BASE32_TABLE[ 0x80 ] = {
    /*00-07*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*08-0f*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*10-17*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*18-1f*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*20-27*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*28-2f*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*30-37*/ 0xFF, 0xFF, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, /*6 = '2'-'7'*/
    /*38-3f*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x20, 0xFF, 0xFF, /*1 = '='*/
    /*40-47*/ 0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, /*7 = 'A'-'G'*/
    /*48-4f*/ 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, /*8 = 'H'-'O'*/
    /*50-57*/ 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, /*8 = 'P'-'W'*/
    /*58-5f*/ 0x17, 0x18, 0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /*3 = 'X'-'Z'*/
    /*60-67*/ 0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, /*7 = 'a'-'g' (same as 'A'-'G')*/
    /*68-6f*/ 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, /*8 = 'h'-'o' (same as 'H'-'O')*/
    /*70-77*/ 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, /*8 = 'p'-'w' (same as 'P'-'W')*/
    /*78-7f*/ 0x17, 0x18, 0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF  /*3 = 'x'-'z' (same as 'X'-'Z')*/
};

int cyoBase32Validate( const char* src, size_t size )
{
    return cyoBaseXXValidate( src, size, BASE32_INPUT, BASE32_MAX_PADDING, BASE32_MAX_VALUE, BASE32_TABLE );
}

size_t cyoBase32DecodeGetLength( size_t size )
{
    return cyoBaseXXDecodeGetLength( size, BASE32_INPUT, BASE32_OUTPUT );
}

size_t cyoBase32Decode( void* dest, const char* src, size_t size )
{
    /*
     * output 5 bytes for every 8 input:
     *
     *               outputs: 1        2        3        4        5
     * inputs: 1 = ---11111 = 11111---
     *         2 = ---222XX = -----222 XX------
     *         3 = ---33333 =          --33333-
     *         4 = ---4XXXX =          -------4 XXXX----
     *         5 = ---5555X =                   ----5555 X-------
     *         6 = ---66666 =                            -66666--
     *         7 = ---77XXX =                            ------77 XXX-----
     *         8 = ---88888 =                                     ---88888
     */

    if (dest && src && (size % BASE32_INPUT == 0))
    {
        unsigned char* pDest = (unsigned char*)dest;
        size_t dwSrcSize = size;
        size_t dwDestSize = 0;
        unsigned char in1, in2, in3, in4, in5, in6, in7, in8;

        while (dwSrcSize >= 1)
        {
            /* 8 inputs */
            in1 = *src++;
            in2 = *src++;
            in3 = *src++;
            in4 = *src++;
            in5 = *src++;
            in6 = *src++;
            in7 = *src++;
            in8 = *src++;
            dwSrcSize -= BASE32_INPUT;

            /* Validate ascii */
            if (   in1 >= 0x80 || in2 >= 0x80 || in3 >= 0x80 || in4 >= 0x80
                || in5 >= 0x80 || in6 >= 0x80 || in7 >= 0x80 || in8 >= 0x80)
                return 0; /*ERROR - invalid base32 character*/

            /* Convert ascii to base16 */
            in1 = BASE32_TABLE[ in1 ];
            in2 = BASE32_TABLE[ in2 ];
            in3 = BASE32_TABLE[ in3 ];
            in4 = BASE32_TABLE[ in4 ];
            in5 = BASE32_TABLE[ in5 ];
            in6 = BASE32_TABLE[ in6 ];
            in7 = BASE32_TABLE[ in7 ];
            in8 = BASE32_TABLE[ in8 ];

            /* Validate base32 */
            if (in1 > BASE32_MAX_VALUE || in2 > BASE32_MAX_VALUE)
                return 0; /*ERROR - invalid base32 character*/
            /*the following can be padding*/
            if (   in3 > BASE32_MAX_VALUE + 1 || in4 > BASE32_MAX_VALUE + 1 || in5 > BASE32_MAX_VALUE + 1
                || in6 > BASE32_MAX_VALUE + 1 || in7 > BASE32_MAX_VALUE + 1 || in8 > BASE32_MAX_VALUE + 1)
                return 0; /*ERROR - invalid base32 character*/

            /* 5 outputs */
            *pDest++ = ((in1 & 0x1f) << 3) | ((in2 & 0x1c) >> 2);
            *pDest++ = ((in2 & 0x03) << 6) | ((in3 & 0x1f) << 1) | ((in4 & 0x10) >> 4);
            *pDest++ = ((in4 & 0x0f) << 4) | ((in5 & 0x1e) >> 1);
            *pDest++ = ((in5 & 0x01) << 7) | ((in6 & 0x1f) << 2) | ((in7 & 0x18) >> 3);
            *pDest++ = ((in7 & 0x07) << 5) | (in8 & 0x1f);
            dwDestSize += BASE32_OUTPUT;

            /* Padding */
            if (in8 == BASE32_MAX_VALUE + 1)
            {
                --dwDestSize;
                assert( (in7 == BASE32_MAX_VALUE + 1 && in6 == BASE32_MAX_VALUE + 1) || (in7 != BASE32_MAX_VALUE + 1) );
                if (in6 == BASE32_MAX_VALUE + 1)
                {
                    --dwDestSize;
                    if (in5 == BASE32_MAX_VALUE + 1)
                    {
                        --dwDestSize;
                        assert( (in4 == BASE32_MAX_VALUE + 1 && in3 == BASE32_MAX_VALUE + 1) || (in4 != BASE32_MAX_VALUE + 1) );
                        if (in3 == BASE32_MAX_VALUE + 1)
                        {
                            --dwDestSize;
                        }
                    }
                }
            }
        }
        *pDest++ = '\x0'; /*append terminator*/

        return dwDestSize;
    }
    else
        return 0; /*ERROR - null pointer, or size isn't a multiple of 8*/
}

/****************************** Base64 Decoding ******************************/

static const size_t BASE64_INPUT = 4;
static const size_t BASE64_OUTPUT = 3;
static const size_t BASE64_MAX_PADDING = 2;
static const unsigned char BASE64_MAX_VALUE = 63;
static const unsigned char BASE64_TABLE[ 0x80 ] = {
    /*00-07*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*08-0f*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*10-17*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*18-1f*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*20-27*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /*28-2f*/ 0xFF, 0xFF, 0xFF, 0x3e, 0xFF, 0xFF, 0xFF, 0x3f, /*2 = '+' and '/'*/
    /*30-37*/ 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, /*8 = '0'-'7'*/
    /*38-3f*/ 0x3c, 0x3d, 0xFF, 0xFF, 0xFF, 0x40, 0xFF, 0xFF, /*2 = '8'-'9' and '='*/
    /*40-47*/ 0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, /*7 = 'A'-'G'*/
    /*48-4f*/ 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, /*8 = 'H'-'O'*/
    /*50-57*/ 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, /*8 = 'P'-'W'*/
    /*58-5f*/ 0x17, 0x18, 0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /*3 = 'X'-'Z'*/
    /*60-67*/ 0xFF, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, /*7 = 'a'-'g'*/
    /*68-6f*/ 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, /*8 = 'h'-'o'*/
    /*70-77*/ 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, /*8 = 'p'-'w'*/
    /*78-7f*/ 0x31, 0x32, 0x33, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF  /*3 = 'x'-'z'*/
};

int cyoBase64Validate( const char* src, size_t size )
{
    return cyoBaseXXValidate( src, size, BASE64_INPUT, BASE64_MAX_PADDING, BASE64_MAX_VALUE, BASE64_TABLE );
}

size_t cyoBase64DecodeGetLength( size_t size )
{
    return cyoBaseXXDecodeGetLength( size, BASE64_INPUT, BASE64_OUTPUT );
}

size_t cyoBase64Decode( void* dest, const char* src, size_t size )
{
    /*
     * output 3 bytes for every 4 input:
     *
     *               outputs: 1        2        3
     * inputs: 1 = --111111 = 111111--
     *         2 = --22XXXX = ------22 XXXX----
     *         3 = --3333XX =          ----3333 XX------
     *         4 = --444444 =                   --444444
     */

    if (dest && src && (size % BASE64_INPUT == 0))
    {
        unsigned char* pDest = (unsigned char*)dest;
        size_t dwSrcSize = size;
        size_t dwDestSize = 0;
        unsigned char in1, in2, in3, in4;

        while (dwSrcSize >= 1)
        {
            /* 4 inputs */
            in1 = *src++;
            in2 = *src++;
            in3 = *src++;
            in4 = *src++;
            dwSrcSize -= BASE64_INPUT;

            /* Validate ascii */
            if (in1 >= 0x80 || in2 >= 0x80 || in3 >= 0x80 || in4 >= 0x80)
                return 0; /*ERROR - invalid base64 character*/

            /* Convert ascii to base64 */
            in1 = BASE64_TABLE[ in1 ];
            in2 = BASE64_TABLE[ in2 ];
            in3 = BASE64_TABLE[ in3 ];
            in4 = BASE64_TABLE[ in4 ];

            /* Validate base64 */
            if (in1 > BASE64_MAX_VALUE || in2 > BASE64_MAX_VALUE)
                return 0; /*ERROR - invalid base64 character*/
            /*the following can be padding*/
            if (in3 > BASE64_MAX_VALUE + 1 || in4 > BASE64_MAX_VALUE + 1)
                return 0; /*ERROR - invalid base64 character*/

            /* 3 outputs */
            *pDest++ = ((in1 & 0x3f) << 2) | ((in2 & 0x30) >> 4);
            *pDest++ = ((in2 & 0x0f) << 4) | ((in3 & 0x3c) >> 2);
            *pDest++ = ((in3 & 0x03) << 6) | (in4 & 0x3f);
            dwDestSize += BASE64_OUTPUT;

            /* Padding */
            if (in4 == BASE64_MAX_VALUE + 1)
            {
                --dwDestSize;
                if (in3 == BASE64_MAX_VALUE + 1)
                {
                    --dwDestSize;
                }
            }
        }
        *pDest++ = '\x0'; /*append terminator*/

        return dwDestSize;
    }
    else
        return 0; /*ERROR - null pointer, or size isn't a multiple of 4*/
}
