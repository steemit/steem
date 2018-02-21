/*
 * CyoEncode.h - part of the CyoEncode library
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

#ifndef __CYOENCODE_H
#define __CYOENCODE_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Base16 Encoding */
size_t cyoBase16EncodeGetLength( size_t size );
size_t cyoBase16Encode( char* dest, const void* src, size_t size );

/* Base32 Encoding */
size_t cyoBase32EncodeGetLength( size_t size );
size_t cyoBase32Encode( char* dest, const void* src, size_t size );

/* Base64 Encoding */
size_t cyoBase64EncodeGetLength( size_t size );
size_t cyoBase64Encode( char* dest, const void* src, size_t size );

#ifdef __cplusplus
}
#endif

#endif /*__CYOENCODE_H*/

