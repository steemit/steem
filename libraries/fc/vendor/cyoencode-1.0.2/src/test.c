/*
 * test.c - part of the CyoEncode library
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
#include "CyoDecode.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define TEST_BASExx(base,str,expected) \
    printf( "TEST_BASE%s('%s')='%s'", #base, str, expected ); \
    required = cyoBase##base##EncodeGetLength( strlen( str )); \
    encoded = (char*)malloc( required ); \
    if (encoded == NULL) { \
        printf( "\n*** ERROR: Unable to allocate buffer for encoding ***\n" ); \
        goto exit; \
    } \
    cyoBase##base##Encode( encoded, str, strlen( str )); \
    if (strcmp( encoded, expected ) != 0) { \
        printf( "\n*** ERROR: Encoding failure ***\n" ); \
        goto exit; \
    } \
    valid = cyoBase##base##Validate( encoded, strlen( encoded )); \
    if (valid < 0) \
    { \
        printf( "\n*** ERROR: Unable to validate encoding (error %d) ***\n", valid ); \
        goto exit; \
    } \
    printf( " [passed]\n" ); \
    free( encoded ); encoded = NULL;

#define TEST_BASE64(str,expected) TEST_BASExx(64,str,expected)
#define TEST_BASE32(str,expected) TEST_BASExx(32,str,expected)
#define TEST_BASE16(str,expected) TEST_BASExx(16,str,expected)

#define CHECK_INVALID_BASExx(base,str,res) \
    printf( "CHECK_INVALID_BASE%s('%s')=%d", #base, str, res ); \
    valid = cyoBase##base##Validate( str, strlen( str )); \
    if (valid == 0) \
    { \
        printf( "\n*** ERROR: This is a valid encoding! ***\n" ); \
        goto exit; \
    } \
    if (valid != res) \
    { \
        printf( "\n*** ERROR: Expected a different return code! (%d) ***\n", valid ); \
        goto exit; \
    } \
    printf( " [passed]\n", #base, str ); \

#define CHECK_INVALID_BASE16(enc,res) CHECK_INVALID_BASExx(16,enc,res)
#define CHECK_INVALID_BASE32(enc,res) CHECK_INVALID_BASExx(32,enc,res)
#define CHECK_INVALID_BASE64(enc,res) CHECK_INVALID_BASExx(64,enc,res)

int main( void )
{
    const char* const original = "A wise man speaks when he has something to say";
    size_t required = 0;
    char* encoded = NULL;
    char* decoded = NULL;
    int valid = 0;
    int retcode = 1;

    printf( "Running CyoEncode tests...\n" );

    /* Encode using Base64 */

    printf( "Original = '%s'\n", original );
    required = cyoBase64EncodeGetLength( strlen( original ));
    encoded = (char*)malloc( required );
    if (encoded == NULL)
    {
        printf( "*** ERROR: Unable to allocate buffer for encoding ***\n" );
        goto exit;
    }
    cyoBase64Encode( encoded, original, strlen( original ));
    printf( "Encoded = '%s'\n", encoded );
    
    /* Validate encoding */
    
    valid = cyoBase64Validate( encoded, strlen( encoded ));
    if (valid < 0)
    {
        printf( "*** ERROR: Encoding failure (error %d) ***\n", valid );
        goto exit;
    }

    /* Decode using Base64 */

    required = cyoBase64DecodeGetLength( strlen( encoded ));
    decoded = (char*)malloc( required );
    if (decoded == NULL)
    {
        printf( "*** ERROR: Unable to allocate buffer for decoding ***\n" );
        goto exit;
    }
    cyoBase64Decode( decoded, encoded, strlen( encoded ));
    printf( "Decoded = '%s'\n", decoded );

    /* Validate */

    if (strcmp( original, decoded ) != 0)
    {
        printf( "*** ERROR: Encoding/decoding failure ***\n" );
        goto exit;
    }

    free( encoded );
    encoded = NULL;
    free( decoded );
    decoded = NULL;

    /* Test vectors from RFC 4648 */

    TEST_BASE16( "", "" );
    TEST_BASE16( "f", "66" );
    TEST_BASE16( "fo", "666F" );
    TEST_BASE16( "foo", "666F6F" );
    TEST_BASE16( "foob", "666F6F62" );
    TEST_BASE16( "fooba", "666F6F6261" );
    TEST_BASE16( "foobar", "666F6F626172" );
    
    TEST_BASE32( "", "" );
    TEST_BASE32( "f", "MY======" );
    TEST_BASE32( "fo", "MZXQ====" );
    TEST_BASE32( "foo", "MZXW6===" );
    TEST_BASE32( "foob", "MZXW6YQ=" );
    TEST_BASE32( "fooba", "MZXW6YTB" );
    TEST_BASE32( "foobar", "MZXW6YTBOI======" );

    TEST_BASE64( "", "" );
    TEST_BASE64( "f", "Zg==" );
    TEST_BASE64( "fo", "Zm8=" );
    TEST_BASE64( "foo", "Zm9v" );
    TEST_BASE64( "foob", "Zm9vYg==" );
    TEST_BASE64( "fooba", "Zm9vYmE=" );
    TEST_BASE64( "foobar", "Zm9vYmFy" );

    /* Other tests */

    CHECK_INVALID_BASE16( "1", -1 );
    CHECK_INVALID_BASE16( "123", -1 );
    CHECK_INVALID_BASE16( "1G", -2 );

    CHECK_INVALID_BASE32( "A", -1 );
    CHECK_INVALID_BASE32( "ABCDEFG", -1 );
    CHECK_INVALID_BASE32( "ABCDEFG1", -2 );
    CHECK_INVALID_BASE32( "A=======", -2 );

    CHECK_INVALID_BASE64( "A", -1 );
    CHECK_INVALID_BASE64( "ABCDE", -1 );
    CHECK_INVALID_BASE64( "A&B=", -2 );
    CHECK_INVALID_BASE64( "A===", -2 );

    printf( "*** All tests passed ***\n" );
    retcode = 0;

exit:
    if (encoded != NULL)
        free( encoded );
    if (decoded != NULL)
        free( decoded );

    return retcode;
}
