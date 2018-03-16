/**********************************************************************
 * Copyright (c) 2013-2015 Pieter Wuille, Gregory Maxwell             *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#define SECP256K1_BUILD (1)

#include "include/secp256k1.h"

#include "util.h"
#include "num_impl.h"
#include "field_impl.h"
#include "scalar_impl.h"
#include "group_impl.h"
#include "ecdsa_impl.h"
#include "ecdh_impl.h"
#include "ecmult_impl.h"
#include "ecmult_gen_impl.h"
#include "ecdsa_impl.h"
#include "eckey_impl.h"
#include "hash_impl.h"
#include "borromean_impl.h"
#include "rangeproof_impl.h"

struct secp256k1_context_struct {
    secp256k1_ecmult_context_t ecmult_ctx;
    secp256k1_ecmult_gen_context_t ecmult_gen_ctx;
    secp256k1_ecmult_gen2_context_t ecmult_gen2_ctx;
    secp256k1_rangeproof_context_t rangeproof_ctx;
};

secp256k1_context_t* secp256k1_context_create(int flags) {
    secp256k1_context_t* ret = (secp256k1_context_t*)checked_malloc(sizeof(secp256k1_context_t));

    secp256k1_ecmult_context_init(&ret->ecmult_ctx);
    secp256k1_ecmult_gen_context_init(&ret->ecmult_gen_ctx);
    secp256k1_ecmult_gen2_context_init(&ret->ecmult_gen2_ctx);
    secp256k1_rangeproof_context_init(&ret->rangeproof_ctx);

    if (flags & SECP256K1_CONTEXT_SIGN) {
        secp256k1_ecmult_gen_context_build(&ret->ecmult_gen_ctx);
    }
    if (flags & SECP256K1_CONTEXT_VERIFY) {
        secp256k1_ecmult_context_build(&ret->ecmult_ctx);
    }
    if (flags & SECP256K1_CONTEXT_COMMIT) {
        secp256k1_ecmult_gen2_context_build(&ret->ecmult_gen2_ctx);
    }
    if (flags & SECP256K1_CONTEXT_RANGEPROOF) {
        secp256k1_rangeproof_context_build(&ret->rangeproof_ctx);
    }

    return ret;
}

secp256k1_context_t* secp256k1_context_clone(const secp256k1_context_t* ctx) {
    secp256k1_context_t* ret = (secp256k1_context_t*)checked_malloc(sizeof(secp256k1_context_t));
    secp256k1_ecmult_context_clone(&ret->ecmult_ctx, &ctx->ecmult_ctx);
    secp256k1_ecmult_gen_context_clone(&ret->ecmult_gen_ctx, &ctx->ecmult_gen_ctx);
    secp256k1_ecmult_gen2_context_clone(&ret->ecmult_gen2_ctx, &ctx->ecmult_gen2_ctx);
    secp256k1_rangeproof_context_clone(&ret->rangeproof_ctx, &ctx->rangeproof_ctx);
    return ret;
}

void secp256k1_context_destroy(secp256k1_context_t* ctx) {
    secp256k1_ecmult_context_clear(&ctx->ecmult_ctx);
    secp256k1_ecmult_gen_context_clear(&ctx->ecmult_gen_ctx);
    secp256k1_ecmult_gen2_context_clear(&ctx->ecmult_gen2_ctx);
    secp256k1_rangeproof_context_clear(&ctx->rangeproof_ctx);

    free(ctx);
}

int secp256k1_ecdsa_verify(const secp256k1_context_t* ctx, const unsigned char *msg32, const unsigned char *sig, int siglen, const unsigned char *pubkey, int pubkeylen) {
    secp256k1_ge_t q;
    secp256k1_ecdsa_sig_t s;
    secp256k1_scalar_t m;
    int ret = -3;
    DEBUG_CHECK(ctx != NULL);
    DEBUG_CHECK(secp256k1_ecmult_context_is_built(&ctx->ecmult_ctx));
    DEBUG_CHECK(msg32 != NULL);
    DEBUG_CHECK(sig != NULL);
    DEBUG_CHECK(pubkey != NULL);

    secp256k1_scalar_set_b32(&m, msg32, NULL);

    if (secp256k1_eckey_pubkey_parse(&q, pubkey, pubkeylen)) {
        if (secp256k1_ecdsa_sig_parse(&s, sig, siglen)) {
            if (secp256k1_ecdsa_sig_verify(&ctx->ecmult_ctx, &s, &q, &m)) {
                /* success is 1, all other values are fail */
                ret = 1;
            } else {
                ret = 0;
            }
        } else {
            ret = -2;
        }
    } else {
        ret = -1;
    }

    return ret;
}

static int nonce_function_rfc6979(unsigned char *nonce32, const unsigned char *msg32, const unsigned char *key32, unsigned int counter, const void *data) {
   secp256k1_rfc6979_hmac_sha256_t rng;
   unsigned int i;
   secp256k1_rfc6979_hmac_sha256_initialize(&rng, key32, 32, msg32, 32, (const unsigned char*)data, data != NULL ? 32 : 0);
   for (i = 0; i <= counter; i++) {
       secp256k1_rfc6979_hmac_sha256_generate(&rng, nonce32, 32);
   }
   secp256k1_rfc6979_hmac_sha256_finalize(&rng);
   return 1;
}

const secp256k1_nonce_function_t secp256k1_nonce_function_rfc6979 = nonce_function_rfc6979;
const secp256k1_nonce_function_t secp256k1_nonce_function_default = nonce_function_rfc6979;

int secp256k1_ecdsa_sign(const secp256k1_context_t* ctx, const unsigned char *msg32, unsigned char *signature, int *signaturelen, const unsigned char *seckey, secp256k1_nonce_function_t noncefp, const void* noncedata) {
    secp256k1_ecdsa_sig_t sig;
    secp256k1_scalar_t sec, non, msg;
    int ret = 0;
    int overflow = 0;
    unsigned int count = 0;
    DEBUG_CHECK(ctx != NULL);
    DEBUG_CHECK(secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    DEBUG_CHECK(msg32 != NULL);
    DEBUG_CHECK(signature != NULL);
    DEBUG_CHECK(signaturelen != NULL);
    DEBUG_CHECK(seckey != NULL);
    if (noncefp == NULL) {
        noncefp = secp256k1_nonce_function_default;
    }

    secp256k1_scalar_set_b32(&sec, seckey, &overflow);
    /* Fail if the secret key is invalid. */
    if (!overflow && !secp256k1_scalar_is_zero(&sec)) {
        secp256k1_scalar_set_b32(&msg, msg32, NULL);
        while (1) {
            unsigned char nonce32[32];
            ret = noncefp(nonce32, msg32, seckey, count, noncedata);
            if (!ret) {
                break;
            }
            secp256k1_scalar_set_b32(&non, nonce32, &overflow);
            memset(nonce32, 0, 32);
            if (!secp256k1_scalar_is_zero(&non) && !overflow) {
                if (secp256k1_ecdsa_sig_sign(&ctx->ecmult_gen_ctx, &sig, &sec, &msg, &non, NULL)) {
                    break;
                }
            }
            count++;
        }
        if (ret) {
            ret = secp256k1_ecdsa_sig_serialize(signature, signaturelen, &sig);
        }
        secp256k1_scalar_clear(&msg);
        secp256k1_scalar_clear(&non);
        secp256k1_scalar_clear(&sec);
    }
    if (!ret) {
        *signaturelen = 0;
    }
    return ret;
}

int secp256k1_ecdsa_sign_compact(const secp256k1_context_t* ctx, const unsigned char *msg32, unsigned char *sig64, const unsigned char *seckey, secp256k1_nonce_function_t noncefp, const void* noncedata, int *recid) {
    secp256k1_ecdsa_sig_t sig;
    secp256k1_scalar_t sec, non, msg;
    int ret = 0;
    int overflow = 0;
    unsigned int count = 0;
    DEBUG_CHECK(ctx != NULL);
    DEBUG_CHECK(secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    DEBUG_CHECK(msg32 != NULL);
    DEBUG_CHECK(sig64 != NULL);
    DEBUG_CHECK(seckey != NULL);
    if (noncefp == NULL) {
        noncefp = secp256k1_nonce_function_default;
    }

    secp256k1_scalar_set_b32(&sec, seckey, &overflow);
    /* Fail if the secret key is invalid. */
    if (!overflow && !secp256k1_scalar_is_zero(&sec)) {
        secp256k1_scalar_set_b32(&msg, msg32, NULL);
        while (1) {
            unsigned char nonce32[32];
            ret = noncefp(nonce32, msg32, seckey, count, noncedata);
            if (!ret) {
                break;
            }
            secp256k1_scalar_set_b32(&non, nonce32, &overflow);
            memset(nonce32, 0, 32);
            if (!secp256k1_scalar_is_zero(&non) && !overflow) {
                if (secp256k1_ecdsa_sig_sign(&ctx->ecmult_gen_ctx, &sig, &sec, &msg, &non, recid)) {
                    break;
                }
            }
            count++;
        }
        if (ret) {
            secp256k1_scalar_get_b32(sig64, &sig.r);
            secp256k1_scalar_get_b32(sig64 + 32, &sig.s);
        }
        secp256k1_scalar_clear(&msg);
        secp256k1_scalar_clear(&non);
        secp256k1_scalar_clear(&sec);
    }
    if (!ret) {
        memset(sig64, 0, 64);
    }
    return ret;
}

int secp256k1_ecdsa_recover_compact(const secp256k1_context_t* ctx, const unsigned char *msg32, const unsigned char *sig64, unsigned char *pubkey, int *pubkeylen, int compressed, int recid) {
    secp256k1_ge_t q;
    secp256k1_ecdsa_sig_t sig;
    secp256k1_scalar_t m;
    int ret = 0;
    int overflow = 0;
    DEBUG_CHECK(ctx != NULL);
    DEBUG_CHECK(secp256k1_ecmult_context_is_built(&ctx->ecmult_ctx));
    DEBUG_CHECK(msg32 != NULL);
    DEBUG_CHECK(sig64 != NULL);
    DEBUG_CHECK(pubkey != NULL);
    DEBUG_CHECK(pubkeylen != NULL);
    DEBUG_CHECK(recid >= 0 && recid <= 3);

    secp256k1_scalar_set_b32(&sig.r, sig64, &overflow);
    if (!overflow) {
        secp256k1_scalar_set_b32(&sig.s, sig64 + 32, &overflow);
        if (!overflow) {
            secp256k1_scalar_set_b32(&m, msg32, NULL);

            if (secp256k1_ecdsa_sig_recover(&ctx->ecmult_ctx, &sig, &q, &m, recid)) {
                ret = secp256k1_eckey_pubkey_serialize(&q, pubkey, pubkeylen, compressed);
            }
        }
    }
    return ret;
}

int secp256k1_point_multiply(unsigned char *point, int *pointlen, const unsigned char *scalar) {
    int ret = 0;
    int overflow = 0;
    secp256k1_gej_t res;
    secp256k1_ge_t pt;
    secp256k1_scalar_t s;
    DEBUG_CHECK(point != NULL);
    DEBUG_CHECK(pointlen != NULL);
    DEBUG_CHECK(scalar != NULL);

    if (secp256k1_eckey_pubkey_parse(&pt, point, *pointlen)) {
        secp256k1_scalar_set_b32(&s, scalar, &overflow);
        if (overflow) {
            ret = -2;
        } else {
            secp256k1_ecdh_point_multiply(&res, &pt, &s);
            secp256k1_ge_set_gej(&pt, &res);
            ret = secp256k1_eckey_pubkey_serialize(&pt, point, pointlen, *pointlen <= 33);
        }
    } else {
        ret = -3;
    }
    secp256k1_scalar_clear(&s);
    return ret;
}

int secp256k1_ec_seckey_verify(const secp256k1_context_t* ctx, const unsigned char *seckey) {
    secp256k1_scalar_t sec;
    int ret;
    int overflow;
    DEBUG_CHECK(ctx != NULL);
    DEBUG_CHECK(seckey != NULL);
    (void)ctx;

    secp256k1_scalar_set_b32(&sec, seckey, &overflow);
    ret = !secp256k1_scalar_is_zero(&sec) && !overflow;
    secp256k1_scalar_clear(&sec);
    return ret;
}

int secp256k1_ec_pubkey_verify(const secp256k1_context_t* ctx, const unsigned char *pubkey, int pubkeylen) {
    secp256k1_ge_t q;
    DEBUG_CHECK(ctx != NULL);
    DEBUG_CHECK(pubkey != NULL);
    (void)ctx;

    return secp256k1_eckey_pubkey_parse(&q, pubkey, pubkeylen);
}

int secp256k1_ec_pubkey_create(const secp256k1_context_t* ctx, unsigned char *pubkey, int *pubkeylen, const unsigned char *seckey, int compressed) {
    secp256k1_gej_t pj;
    secp256k1_ge_t p;
    secp256k1_scalar_t sec;
    int overflow;
    int ret = 0;
    DEBUG_CHECK(ctx != NULL);
    DEBUG_CHECK(secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    DEBUG_CHECK(pubkey != NULL);
    DEBUG_CHECK(pubkeylen != NULL);
    DEBUG_CHECK(seckey != NULL);

    secp256k1_scalar_set_b32(&sec, seckey, &overflow);
    if (!overflow) {
        secp256k1_ecmult_gen(&ctx->ecmult_gen_ctx, &pj, &sec);
        secp256k1_scalar_clear(&sec);
        secp256k1_ge_set_gej(&p, &pj);
        ret = secp256k1_eckey_pubkey_serialize(&p, pubkey, pubkeylen, compressed);
    }
    if (!ret) {
        *pubkeylen = 0;
    }
    return ret;
}

int secp256k1_ec_pubkey_decompress(const secp256k1_context_t* ctx, unsigned char *pubkey, int *pubkeylen) {
    secp256k1_ge_t p;
    int ret = 0;
    DEBUG_CHECK(pubkey != NULL);
    DEBUG_CHECK(pubkeylen != NULL);
    (void)ctx;

    if (secp256k1_eckey_pubkey_parse(&p, pubkey, *pubkeylen)) {
        ret = secp256k1_eckey_pubkey_serialize(&p, pubkey, pubkeylen, 0);
    }
    return ret;
}

int secp256k1_ec_privkey_tweak_add(const secp256k1_context_t* ctx, unsigned char *seckey, const unsigned char *tweak) {
    secp256k1_scalar_t term;
    secp256k1_scalar_t sec;
    int ret = 0;
    int overflow = 0;
    DEBUG_CHECK(ctx != NULL);
    DEBUG_CHECK(seckey != NULL);
    DEBUG_CHECK(tweak != NULL);
    (void)ctx;

    secp256k1_scalar_set_b32(&term, tweak, &overflow);
    secp256k1_scalar_set_b32(&sec, seckey, NULL);

    ret = secp256k1_eckey_privkey_tweak_add(&sec, &term) && !overflow;
    if (ret) {
        secp256k1_scalar_get_b32(seckey, &sec);
    }

    secp256k1_scalar_clear(&sec);
    secp256k1_scalar_clear(&term);
    return ret;
}

int secp256k1_ec_pubkey_tweak_add(const secp256k1_context_t* ctx, unsigned char *pubkey, int pubkeylen, const unsigned char *tweak) {
    secp256k1_ge_t p;
    secp256k1_scalar_t term;
    int ret = 0;
    int overflow = 0;
    DEBUG_CHECK(ctx != NULL);
    DEBUG_CHECK(secp256k1_ecmult_context_is_built(&ctx->ecmult_ctx));
    DEBUG_CHECK(pubkey != NULL);
    DEBUG_CHECK(tweak != NULL);

    secp256k1_scalar_set_b32(&term, tweak, &overflow);
    if (!overflow) {
        ret = secp256k1_eckey_pubkey_parse(&p, pubkey, pubkeylen);
        if (ret) {
            ret = secp256k1_eckey_pubkey_tweak_add(&ctx->ecmult_ctx, &p, &term);
        }
        if (ret) {
            int oldlen = pubkeylen;
            ret = secp256k1_eckey_pubkey_serialize(&p, pubkey, &pubkeylen, oldlen <= 33);
            VERIFY_CHECK(pubkeylen == oldlen);
        }
    }

    return ret;
}

int secp256k1_ec_privkey_tweak_mul(const secp256k1_context_t* ctx, unsigned char *seckey, const unsigned char *tweak) {
    secp256k1_scalar_t factor;
    secp256k1_scalar_t sec;
    int ret = 0;
    int overflow = 0;
    DEBUG_CHECK(ctx != NULL);
    DEBUG_CHECK(seckey != NULL);
    DEBUG_CHECK(tweak != NULL);
    (void)ctx;

    secp256k1_scalar_set_b32(&factor, tweak, &overflow);
    secp256k1_scalar_set_b32(&sec, seckey, NULL);
    ret = secp256k1_eckey_privkey_tweak_mul(&sec, &factor) && !overflow;
    if (ret) {
        secp256k1_scalar_get_b32(seckey, &sec);
    }

    secp256k1_scalar_clear(&sec);
    secp256k1_scalar_clear(&factor);
    return ret;
}

int secp256k1_ec_pubkey_tweak_mul(const secp256k1_context_t* ctx, unsigned char *pubkey, int pubkeylen, const unsigned char *tweak) {
    secp256k1_ge_t p;
    secp256k1_scalar_t factor;
    int ret = 0;
    int overflow = 0;
    DEBUG_CHECK(ctx != NULL);
    DEBUG_CHECK(secp256k1_ecmult_context_is_built(&ctx->ecmult_ctx));
    DEBUG_CHECK(pubkey != NULL);
    DEBUG_CHECK(tweak != NULL);

    secp256k1_scalar_set_b32(&factor, tweak, &overflow);
    if (!overflow) {
        ret = secp256k1_eckey_pubkey_parse(&p, pubkey, pubkeylen);
        if (ret) {
            ret = secp256k1_eckey_pubkey_tweak_mul(&ctx->ecmult_ctx, &p, &factor);
        }
        if (ret) {
            int oldlen = pubkeylen;
            ret = secp256k1_eckey_pubkey_serialize(&p, pubkey, &pubkeylen, oldlen <= 33);
            VERIFY_CHECK(pubkeylen == oldlen);
        }
    }

    return ret;
}

int secp256k1_ec_privkey_export(const secp256k1_context_t* ctx, const unsigned char *seckey, unsigned char *privkey, int *privkeylen, int compressed) {
    secp256k1_scalar_t key;
    int ret = 0;
    DEBUG_CHECK(seckey != NULL);
    DEBUG_CHECK(privkey != NULL);
    DEBUG_CHECK(privkeylen != NULL);
    DEBUG_CHECK(ctx != NULL);
    DEBUG_CHECK(secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));

    secp256k1_scalar_set_b32(&key, seckey, NULL);
    ret = secp256k1_eckey_privkey_serialize(&ctx->ecmult_gen_ctx, privkey, privkeylen, &key, compressed);
    secp256k1_scalar_clear(&key);
    return ret;
}

int secp256k1_ec_privkey_import(const secp256k1_context_t* ctx, unsigned char *seckey, const unsigned char *privkey, int privkeylen) {
    secp256k1_scalar_t key;
    int ret = 0;
    DEBUG_CHECK(seckey != NULL);
    DEBUG_CHECK(privkey != NULL);
    (void)ctx;

    ret = secp256k1_eckey_privkey_parse(&key, privkey, privkeylen);
    if (ret) {
        secp256k1_scalar_get_b32(seckey, &key);
    }
    secp256k1_scalar_clear(&key);
    return ret;
}

int secp256k1_context_randomize(secp256k1_context_t* ctx, const unsigned char *seed32) {
    DEBUG_CHECK(ctx != NULL);
    DEBUG_CHECK(secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    secp256k1_ecmult_gen_blind(&ctx->ecmult_gen_ctx, seed32);
    return 1;
}

/* Generates a pedersen commitment: *commit = blind * G + value * G2. The commitment is 33 bytes, the blinding factor is 32 bytes.*/
int secp256k1_pedersen_commit(const secp256k1_context_t* ctx, unsigned char *commit, unsigned char *blind, uint64_t value) {
    secp256k1_gej_t rj;
    secp256k1_ge_t r;
    secp256k1_scalar_t sec;
    int sz;
    int overflow;
    int ret = 0;
    DEBUG_CHECK(ctx != NULL);
    DEBUG_CHECK(secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    DEBUG_CHECK(secp256k1_ecmult_gen2_context_is_built(&ctx->ecmult_gen2_ctx));
    DEBUG_CHECK(commit != NULL);
    DEBUG_CHECK(blind != NULL);
    secp256k1_scalar_set_b32(&sec, blind, &overflow);
    if (!overflow) {
        secp256k1_ecmult_gen_gen2(&ctx->ecmult_gen_ctx, &ctx->ecmult_gen2_ctx, &rj, &sec, value);
        if (!secp256k1_gej_is_infinity(&rj)) {
            secp256k1_ge_set_gej(&r, &rj);
            sz = 33;
            ret = secp256k1_eckey_pubkey_serialize(&r, commit, &sz, 1);
        }
        secp256k1_gej_clear(&rj);
        secp256k1_ge_clear(&r);
    }
    secp256k1_scalar_clear(&sec);
    return ret;
}

/** Takes a list of n pointers to 32 byte blinding values, the first negs of which are treated with positive sign and the rest
 *  negative, then calculates an additional blinding value that adds to zero.
 */
int secp256k1_pedersen_blind_sum(const secp256k1_context_t* ctx, unsigned char *blind_out, const unsigned char * const *blinds, int n, int npositive) {
    secp256k1_scalar_t acc;
    secp256k1_scalar_t x;
    int i;
    int overflow;
    DEBUG_CHECK(ctx != NULL);
    DEBUG_CHECK(blind_out != NULL);
    DEBUG_CHECK(blinds != NULL);
    secp256k1_scalar_set_int(&acc, 0);
    for (i = 0; i < n; i++) {
        secp256k1_scalar_set_b32(&x, blinds[i], &overflow);
        if (overflow) {
            return 0;
        }
        if (i >= npositive) {
            secp256k1_scalar_negate(&x, &x);
        }
        secp256k1_scalar_add(&acc, &acc, &x);
    }
    secp256k1_scalar_get_b32(blind_out, &acc);
    secp256k1_scalar_clear(&acc);
    secp256k1_scalar_clear(&x);
    return 1;
}
/* Takes two list of 33-byte commitments and sums the first set and subtracts the second and verifies that they sum to excess. */
int secp256k1_pedersen_verify_tally(const secp256k1_context_t* ctx, const unsigned char * const *commits, int pcnt,
 const unsigned char * const *ncommits, int ncnt, int64_t excess) {
    secp256k1_gej_t accj;
    secp256k1_ge_t add;
    int i;
    DEBUG_CHECK(ctx != NULL);
    DEBUG_CHECK(!pcnt || (commits != NULL));
    DEBUG_CHECK(!ncnt || (ncommits != NULL));
    DEBUG_CHECK(secp256k1_ecmult_gen2_context_is_built(&ctx->ecmult_gen2_ctx));
    secp256k1_gej_set_infinity(&accj);
    if (excess) {
        uint64_t ex;
        int neg;
        /* Take the absolute value, and negate the result if the input was negative. */
        neg = secp256k1_sign_and_abs64(&ex, excess);
        secp256k1_ecmult_gen2_small(&ctx->ecmult_gen2_ctx, &accj, ex);
        if (neg) {
            secp256k1_gej_neg(&accj, &accj);
        }
    }
    for (i = 0; i < ncnt; i++) {
        if (!secp256k1_eckey_pubkey_parse(&add, ncommits[i], 33)) {
            return 0;
        }
        secp256k1_gej_add_ge_var(&accj, &accj, &add, NULL);
    }
    secp256k1_gej_neg(&accj, &accj);
    for (i = 0; i < pcnt; i++) {
        if (!secp256k1_eckey_pubkey_parse(&add, commits[i], 33)) {
            return 0;
        }
        secp256k1_gej_add_ge_var(&accj, &accj, &add, NULL);
    }
    return secp256k1_gej_is_infinity(&accj);
}

int secp256k1_rangeproof_info(const secp256k1_context_t* ctx, int *exp, int *mantissa,
 uint64_t *min_value, uint64_t *max_value, const unsigned char *proof, int plen) {
    int offset;
    uint64_t scale;
    DEBUG_CHECK(exp != NULL);
    DEBUG_CHECK(mantissa != NULL);
    DEBUG_CHECK(min_value != NULL);
    DEBUG_CHECK(max_value != NULL);
    offset = 0;
    scale = 1;
    (void)ctx;
    return secp256k1_rangeproof_getheader_impl(&offset, exp, mantissa, &scale, min_value, max_value, proof, plen);
}

int secp256k1_rangeproof_rewind(const secp256k1_context_t* ctx,
 unsigned char *blind_out, uint64_t *value_out, unsigned char *message_out, int *outlen, const unsigned char *nonce,
 uint64_t *min_value, uint64_t *max_value,
 const unsigned char *commit, const unsigned char *proof, int plen) {
    DEBUG_CHECK(ctx != NULL);
    DEBUG_CHECK(commit != NULL);
    DEBUG_CHECK(proof != NULL);
    DEBUG_CHECK(min_value != NULL);
    DEBUG_CHECK(max_value != NULL);
    DEBUG_CHECK(secp256k1_ecmult_context_is_built(&ctx->ecmult_ctx));
    DEBUG_CHECK(secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    DEBUG_CHECK(secp256k1_ecmult_gen2_context_is_built(&ctx->ecmult_gen2_ctx));
    DEBUG_CHECK(secp256k1_rangeproof_context_is_built(&ctx->rangeproof_ctx));
    return secp256k1_rangeproof_verify_impl(&ctx->ecmult_ctx, &ctx->ecmult_gen_ctx, &ctx->ecmult_gen2_ctx, &ctx->rangeproof_ctx,
     blind_out, value_out, message_out, outlen, nonce, min_value, max_value, commit, proof, plen);
}

int secp256k1_rangeproof_verify(const secp256k1_context_t* ctx, uint64_t *min_value, uint64_t *max_value,
 const unsigned char *commit, const unsigned char *proof, int plen) {
    DEBUG_CHECK(ctx != NULL);
    DEBUG_CHECK(commit != NULL);
    DEBUG_CHECK(proof != NULL);
    DEBUG_CHECK(min_value != NULL);
    DEBUG_CHECK(max_value != NULL);
    DEBUG_CHECK(secp256k1_ecmult_context_is_built(&ctx->ecmult_ctx));
    DEBUG_CHECK(secp256k1_ecmult_gen2_context_is_built(&ctx->ecmult_gen2_ctx));
    DEBUG_CHECK(secp256k1_rangeproof_context_is_built(&ctx->rangeproof_ctx));
    return secp256k1_rangeproof_verify_impl(&ctx->ecmult_ctx, NULL, &ctx->ecmult_gen2_ctx, &ctx->rangeproof_ctx,
     NULL, NULL, NULL, NULL, NULL, min_value, max_value, commit, proof, plen);
}

int secp256k1_rangeproof_sign(const secp256k1_context_t* ctx, unsigned char *proof, int *plen, uint64_t min_value,
 const unsigned char *commit, const unsigned char *blind, const unsigned char *nonce, int exp, int min_bits, uint64_t value){
    DEBUG_CHECK(ctx != NULL);
    DEBUG_CHECK(proof != NULL);
    DEBUG_CHECK(plen != NULL);
    DEBUG_CHECK(commit != NULL);
    DEBUG_CHECK(blind != NULL);
    DEBUG_CHECK(nonce != NULL);
    DEBUG_CHECK(secp256k1_ecmult_context_is_built(&ctx->ecmult_ctx));
    DEBUG_CHECK(secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    DEBUG_CHECK(secp256k1_ecmult_gen2_context_is_built(&ctx->ecmult_gen2_ctx));
    DEBUG_CHECK(secp256k1_rangeproof_context_is_built(&ctx->rangeproof_ctx));
    return secp256k1_rangeproof_sign_impl(&ctx->ecmult_ctx, &ctx->ecmult_gen_ctx, &ctx->ecmult_gen2_ctx, &ctx->rangeproof_ctx,
     proof, plen, min_value, commit, blind, nonce, exp, min_bits, value);
}
