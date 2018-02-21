/**********************************************************************
 * Copyright (c) 2015 Gregory Maxwell                                 *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _SECP256K1_RANGEPROOF__
#define _SECP256K1_RANGEPROOF__

#include "scalar.h"
#include "group.h"

typedef struct {
    secp256k1_ge_storage_t (*prec)[1005];
} secp256k1_rangeproof_context_t;


static void secp256k1_rangeproof_context_init(secp256k1_rangeproof_context_t* ctx);
static void secp256k1_rangeproof_context_build(secp256k1_rangeproof_context_t* ctx);
static void secp256k1_rangeproof_context_clone(secp256k1_rangeproof_context_t *dst,
                                               const secp256k1_rangeproof_context_t* src);
static void secp256k1_rangeproof_context_clear(secp256k1_rangeproof_context_t* ctx);
static int secp256k1_rangeproof_context_is_built(const secp256k1_rangeproof_context_t* ctx);

static int secp256k1_rangeproof_verify_impl(const secp256k1_ecmult_context_t* ecmult_ctx,
 const secp256k1_ecmult_gen_context_t* ecmult_gen_ctx,
 const secp256k1_ecmult_gen2_context_t* ecmult_gen2_ctx, const secp256k1_rangeproof_context_t* rangeproof_ctx,
 unsigned char *blindout, uint64_t *value_out, unsigned char *message_out, int *outlen, const unsigned char *nonce,
 uint64_t *min_value, uint64_t *max_value, const unsigned char *commit, const unsigned char *proof, int plen);

#endif
