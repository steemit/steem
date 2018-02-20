/*
 * CyoDecode.h - part of the CyoEncode library
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

#ifndef __CYODECODE_H
#define __CYODECODE_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Base16 Decoding */
int    cyoBase16Validate( const char* src, size_t size );
size_t cyoBase16DecodeGetLength( size_t size );
size_t cyoBase16Decode( void* dest, const char* src, size_t size );

/* Base32 Decoding */
int    cyoBase32Validate( const char* src, size_t size );
size_t cyoBase32DecodeGetLength( size_t size );
size_t cyoBase32Decode( void* dest, const char* src, size_t size );

/* Base64 Decoding */
int    cyoBase64Validate( const char* src, size_t size );
size_t cyoBase64DecodeGetLength( size_t size );
size_t cyoBase64Decode( void* dest, const char* src, size_t size );

#ifdef __cplusplus
}
#endif

#endif /*__CYODECODE_H*/

