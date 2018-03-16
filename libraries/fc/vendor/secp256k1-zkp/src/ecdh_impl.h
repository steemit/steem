/**********************************************************************
 * Copyright (c) 2015 Pieter Wuille, Andrew Poelstra                  *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _SECP256K1_ECDH_IMPL_
#define _SECP256K1_ECDH_IMPL_

#include "scalar.h"
#include "group.h"
#include "ecdh.h"
#include "ecmult_impl.h"

/** Convert a number to WNAF notation. The number becomes represented by sum(2^{wi} * wnaf[i], i=0..return_val)
 *  with the following guarantees:
 *  - each wnaf[i] an odd integer between -(1 << w) and (1 << w)
 *  - each wnaf[i] is nonzero
 *  - the number of words set is returned; this is always (256 + w - 1) / w
 *
 *  Adapted from `The Width-w NAF Method Provides Small Memory and Fast Elliptic Scalar
 *  Multiplications Secure against Side Channel Attacks`, Okeya and Tagaki. M. Joye (Ed.)
 *  CT-RSA 2003, LNCS 2612, pp. 328-443, 2003. Springer-Verlagy Berlin Heidelberg 2003
 *
 *  Numbers reference steps of `Algorithm SPA-resistant Width-w NAF with Odd Scalar` on pp. 335
 */
static int secp256k1_ecdh_wnaf(int *wnaf, const secp256k1_scalar_t *a, int w) {
    secp256k1_scalar_t s = *a;
    /* Negate to force oddness */
    int global_sign = secp256k1_scalar_wnaf_force_odd(&s);

    int bit = 0;
    /* 1 2 3 */
    int u_last = secp256k1_scalar_shr_int(&s, w);
    int u;
    /* 4 */
    while (bit * w < 256) {
        int sign;
        int even;

        /* 4.1 4.4 */
        u = secp256k1_scalar_shr_int(&s, w);
        /* 4.2 */
        even = ((u & 1) == 0);
        sign = 2 * (u_last > 0) - 1;
        u += sign * even;
        u_last -= sign * even * (1 << w);

        /* 4.3, adapted for global sign change */
        wnaf[bit++] = u_last * global_sign;

        u_last = u;
    }
    wnaf[bit] = u * global_sign;
    return bit;
}


static void secp256k1_ecdh_point_multiply(secp256k1_gej_t *r, const secp256k1_ge_t *a, const secp256k1_scalar_t *scalar) {
    secp256k1_ge_t pre_a[ECMULT_TABLE_SIZE(WINDOW_A)];
    secp256k1_ge_t tmpa;
    secp256k1_fe_t Z;

    int wnaf[256];
    int n_words;

    int i;
    int is_zero = secp256k1_scalar_is_zero(scalar);
    secp256k1_scalar_t sc = *scalar;
    /* the wNAF ladder cannot handle zero, so bump this to one .. we will
     * correct the result after the fact */
    sc.d[0] += is_zero;

    /* build wnaf representation for q. */
    n_words = secp256k1_ecdh_wnaf(wnaf, &sc, WINDOW_A - 1);

    /* Calculate odd multiples of a.
     * All multiples are brought to the same Z 'denominator', which is stored
     * in Z. Due to secp256k1' isomorphism we can do all operations pretending
     * that the Z coordinate was 1, use affine addition formulae, and correct
     * the Z coordinate of the result once at the end.
     */
    secp256k1_gej_set_ge(r, a);
    secp256k1_ecmult_odd_multiples_table_globalz_windowa(pre_a, &Z, r);
    secp256k1_gej_set_infinity(r);

    for (i = n_words; i >= 0; i--) {
        int n;
        int j;
        for (j = 0; j < WINDOW_A - 1; ++j) {
            secp256k1_gej_double_var(r, r, NULL);
        }
        n = wnaf[i];
        VERIFY_CHECK(n != 0);
        ECMULT_TABLE_GET_GE(&tmpa, pre_a, n, WINDOW_A);
        secp256k1_gej_add_ge(r, r, &tmpa);
    }

    if (!r->infinity) {
        secp256k1_fe_mul(&r->z, &r->z, &Z);
    }

    /* correct for zero */
    r->infinity |= is_zero;
}

#endif
