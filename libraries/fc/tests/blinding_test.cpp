#define BOOST_TEST_MODULE BlindingTest
#include <boost/test/unit_test.hpp>
#include <fc/array.hpp>
#include <fc/crypto/base58.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/crypto/openssl.hpp>
#include <fc/exception/exception.hpp>

// See https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki#Test_Vectors

static fc::string TEST1_SEED = "000102030405060708090a0b0c0d0e0f";
static fc::string TEST1_M_PUB = "xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8";
static fc::string TEST1_M_PRIV = "xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4stbPy6cq3jPPqjiChkVvvNKmPGJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHi";
static fc::string TEST1_M_0H_PUB = "xpub68Gmy5EdvgibQVfPdqkBBCHxA5htiqg55crXYuXoQRKfDBFA1WEjWgP6LHhwBZeNK1VTsfTFUHCdrfp1bgwQ9xv5ski8PX9rL2dZXvgGDnw";
static fc::string TEST1_M_0H_PRIV = "xprv9uHRZZhk6KAJC1avXpDAp4MDc3sQKNxDiPvvkX8Br5ngLNv1TxvUxt4cV1rGL5hj6KCesnDYUhd7oWgT11eZG7XnxHrnYeSvkzY7d2bhkJ7";
static fc::string TEST1_M_0H_1_PUB = "xpub6ASuArnXKPbfEwhqN6e3mwBcDTgzisQN1wXN9BJcM47sSikHjJf3UFHKkNAWbWMiGj7Wf5uMash7SyYq527Hqck2AxYysAA7xmALppuCkwQ";
static fc::string TEST1_M_0H_1_PRIV = "xprv9wTYmMFdV23N2TdNG573QoEsfRrWKQgWeibmLntzniatZvR9BmLnvSxqu53Kw1UmYPxLgboyZQaXwTCg8MSY3H2EU4pWcQDnRnrVA1xe8fs";
static fc::string TEST1_M_0H_1_2H_PUB = "xpub6D4BDPcP2GT577Vvch3R8wDkScZWzQzMMUm3PWbmWvVJrZwQY4VUNgqFJPMM3No2dFDFGTsxxpG5uJh7n7epu4trkrX7x7DogT5Uv6fcLW5";
static fc::string TEST1_M_0H_1_2H_PRIV = "xprv9z4pot5VBttmtdRTWfWQmoH1taj2axGVzFqSb8C9xaxKymcFzXBDptWmT7FwuEzG3ryjH4ktypQSAewRiNMjANTtpgP4mLTj34bhnZX7UiM";
static fc::string TEST1_M_0H_1_2H_2_PUB = "xpub6FHa3pjLCk84BayeJxFW2SP4XRrFd1JYnxeLeU8EqN3vDfZmbqBqaGJAyiLjTAwm6ZLRQUMv1ZACTj37sR62cfN7fe5JnJ7dh8zL4fiyLHV";
static fc::string TEST1_M_0H_1_2H_2_PRIV = "xprvA2JDeKCSNNZky6uBCviVfJSKyQ1mDYahRjijr5idH2WwLsEd4Hsb2Tyh8RfQMuPh7f7RtyzTtdrbdqqsunu5Mm3wDvUAKRHSC34sJ7in334";
static fc::string TEST1_M_0H_1_2H_2_1g_PUB = "xpub6H1LXWLaKsWFhvm6RVpEL9P4KfRZSW7abD2ttkWP3SSQvnyA8FSVqNTEcYFgJS2UaFcxupHiYkro49S8yGasTvXEYBVPamhGW6cFJodrTHy";
static fc::string TEST1_M_0H_1_2H_2_1g_PRIV = "xprvA41z7zogVVwxVSgdKUHDy1SKmdb533PjDz7J6N6mV6uS3ze1ai8FHa8kmHScGpWmj4WggLyQjgPie1rFSruoUihUZREPSL39UNdE3BBDu76";

static fc::string TEST2_SEED = "fffcf9f6f3f0edeae7e4e1dedbd8d5d2cfccc9c6c3c0bdbab7b4b1aeaba8a5a29f9c999693908d8a8784817e7b7875726f6c696663605d5a5754514e4b484542";
static fc::string TEST2_M_PUB = "xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB";
static fc::string TEST2_M_PRIV = "xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U";
static fc::string TEST2_M_0_PUB = "xpub69H7F5d8KSRgmmdJg2KhpAK8SR3DjMwAdkxj3ZuxV27CprR9LgpeyGmXUbC6wb7ERfvrnKZjXoUmmDznezpbZb7ap6r1D3tgFxHmwMkQTPH";
static fc::string TEST2_M_0_PRIV = "xprv9vHkqa6EV4sPZHYqZznhT2NPtPCjKuDKGY38FBWLvgaDx45zo9WQRUT3dKYnjwih2yJD9mkrocEZXo1ex8G81dwSM1fwqWpWkeS3v86pgKt";
static fc::string TEST2_M_0_m1_PUB = "xpub6ASAVgeehLbnwdqV6UKMHVzgqAG8Gr6riv3Fxxpj8ksbH9ebxaEyBLZ85ySDhKiLDBrQSARLq1uNRts8RuJiHjaDMBU4Zn9h8LZNnBC5y4a";
static fc::string TEST2_M_0_m1_PRIV = "xprv9wSp6B7kry3Vj9m1zSnLvN3xH8RdsPP1Mh7fAaR7aRLcQMKTR2vidYEeEg2mUCTAwCd6vnxVrcjfy2kRgVsFawNzmjuHc2YmYRmagcEPdU9";
static fc::string TEST2_M_0_m1_1_PUB = "xpub6DF8uhdarytz3FWdA8TvFSvvAh8dP3283MY7p2V4SeE2wyWmG5mg5EwVvmdMVCQcoNJxGoWaU9DCWh89LojfZ537wTfunKau47EL2dhHKon";
static fc::string TEST2_M_0_m1_1_PRIV = "xprv9zFnWC6h2cLgpmSA46vutJzBcfJ8yaJGg8cX1e5StJh45BBciYTRXSd25UEPVuesF9yog62tGAQtHjXajPPdbRCHuWS6T8XA2ECKADdw4Ef";
static fc::string TEST2_M_0_m1_1_m2_PUB = "xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL";
static fc::string TEST2_M_0_m1_1_m2_PRIV = "xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc";
static fc::string TEST2_M_0_m1_1_m2_2_PUB = "xpub6FnCn6nSzZAw5Tw7cgR9bi15UV96gLZhjDstkXXxvCLsUXBGXPdSnLFbdpq8p9HmGsApME5hQTZ3emM2rnY5agb9rXpVGyy3bdW6EEgAtqt";
static fc::string TEST2_M_0_m1_1_m2_2_PRIV = "xprvA2nrNbFZABcdryreWet9Ea4LvTJcGsqrMzxHx98MMrotbir7yrKCEXw7nadnHM8Dq38EGfSh6dqA9QWTyefMLEcBYJUuekgW4BYPJcr9E7j";

static fc::string BLIND_K_X = "08be5c5076f8cbd1283cf98e74bff873032b9bc79a0769962bf3900f33c2df6e";
static fc::string BLIND_T_X = "80deff382af8a8e4a5f297588e44d5bf858f30a524f74b13efcefeb54f4b3f47";
static fc::string BLINDED_HASH = "7196e80cdafdfdfb7496323ad24bf47dda8447febd7426e444facc04940c7309";
static fc::string BLIND_SIG = "40d6a477d849cc860df8ad159481f2ffc5b4dc3131b86a799d7d10460824dd53";
static fc::string UNBLINDED = "700092a72a05e33509f9b068aa1d7c5336d8b5692b4157da199d7ec1e10fd7c0";

BOOST_AUTO_TEST_CASE(test_extended_keys_1)
{
    char seed[16];
    fc::from_hex( TEST1_SEED, seed, sizeof(seed) );
    fc::ecc::extended_private_key master = fc::ecc::extended_private_key::generate_master( seed, sizeof(seed) );
    BOOST_CHECK_EQUAL( master.str(), TEST1_M_PRIV );
    BOOST_CHECK_EQUAL( master.get_extended_public_key().str(), TEST1_M_PUB );

    BOOST_CHECK_EQUAL( fc::ecc::extended_private_key::from_base58(TEST1_M_PRIV).str(), TEST1_M_PRIV );
    BOOST_CHECK_EQUAL( fc::ecc::extended_public_key::from_base58(TEST1_M_PUB).str(), TEST1_M_PUB );
    BOOST_CHECK_EQUAL( fc::ecc::extended_private_key::from_base58(TEST1_M_0H_PRIV).str(), TEST1_M_0H_PRIV );
    BOOST_CHECK_EQUAL( fc::ecc::extended_public_key::from_base58(TEST1_M_0H_PUB).str(), TEST1_M_0H_PUB );

    fc::ecc::extended_private_key m_0 = master.derive_child(0x80000000);
    BOOST_CHECK_EQUAL( m_0.str(), TEST1_M_0H_PRIV );
    BOOST_CHECK_EQUAL( m_0.get_extended_public_key().str(), TEST1_M_0H_PUB );

    fc::ecc::extended_private_key m_0_1 = m_0.derive_child(1);
    BOOST_CHECK_EQUAL( m_0_1.str(), TEST1_M_0H_1_PRIV );
    BOOST_CHECK_EQUAL( m_0_1.get_extended_public_key().str(), TEST1_M_0H_1_PUB );
    BOOST_CHECK_EQUAL( m_0.get_extended_public_key().derive_child(1).str(), TEST1_M_0H_1_PUB );

    fc::ecc::extended_private_key m_0_1_2 = m_0_1.derive_child(0x80000002);
    BOOST_CHECK_EQUAL( m_0_1_2.str(), TEST1_M_0H_1_2H_PRIV );
    BOOST_CHECK_EQUAL( m_0_1_2.get_extended_public_key().str(), TEST1_M_0H_1_2H_PUB );

    fc::ecc::extended_private_key m_0_1_2_2 = m_0_1_2.derive_child(2);
    BOOST_CHECK_EQUAL( m_0_1_2_2.str(), TEST1_M_0H_1_2H_2_PRIV );
    BOOST_CHECK_EQUAL( m_0_1_2_2.get_extended_public_key().str(), TEST1_M_0H_1_2H_2_PUB );
    BOOST_CHECK_EQUAL( m_0_1_2.get_extended_public_key().derive_child(2).str(), TEST1_M_0H_1_2H_2_PUB );

    fc::ecc::extended_private_key m_0_1_2_2_1g = m_0_1_2_2.derive_child(1000000000);
    BOOST_CHECK_EQUAL( m_0_1_2_2_1g.str(), TEST1_M_0H_1_2H_2_1g_PRIV );
    BOOST_CHECK_EQUAL( m_0_1_2_2_1g.get_extended_public_key().str(), TEST1_M_0H_1_2H_2_1g_PUB );
    BOOST_CHECK_EQUAL( m_0_1_2_2.get_extended_public_key().derive_child(1000000000).str(), TEST1_M_0H_1_2H_2_1g_PUB );
}

BOOST_AUTO_TEST_CASE(test_extended_keys_2)
{
    char seed[64];
    fc::from_hex( TEST2_SEED, seed, sizeof(seed) );
    fc::ecc::extended_private_key master = fc::ecc::extended_private_key::generate_master( seed, sizeof(seed) );
    BOOST_CHECK_EQUAL( master.str(), TEST2_M_PRIV );
    BOOST_CHECK_EQUAL( master.get_extended_public_key().str(), TEST2_M_PUB );

    fc::ecc::extended_private_key m_0 = master.derive_child(0);
    BOOST_CHECK_EQUAL( m_0.str(), TEST2_M_0_PRIV );
    BOOST_CHECK_EQUAL( m_0.get_extended_public_key().str(), TEST2_M_0_PUB );
    BOOST_CHECK_EQUAL( master.get_extended_public_key().derive_child(0).str(), TEST2_M_0_PUB );

    fc::ecc::extended_private_key m_0_m1 = m_0.derive_child(-1);
    BOOST_CHECK_EQUAL( m_0_m1.str(), TEST2_M_0_m1_PRIV );
    BOOST_CHECK_EQUAL( m_0_m1.get_extended_public_key().str(), TEST2_M_0_m1_PUB );

    fc::ecc::extended_private_key m_0_m1_1 = m_0_m1.derive_child(1);
    BOOST_CHECK_EQUAL( m_0_m1_1.str(), TEST2_M_0_m1_1_PRIV );
    BOOST_CHECK_EQUAL( m_0_m1_1.get_extended_public_key().str(), TEST2_M_0_m1_1_PUB );
    BOOST_CHECK_EQUAL( m_0_m1.get_extended_public_key().derive_child(1).str(), TEST2_M_0_m1_1_PUB );

    fc::ecc::extended_private_key m_0_m1_1_m2 = m_0_m1_1.derive_child(-2);
    BOOST_CHECK_EQUAL( m_0_m1_1_m2.str(), TEST2_M_0_m1_1_m2_PRIV );
    BOOST_CHECK_EQUAL( m_0_m1_1_m2.get_extended_public_key().str(), TEST2_M_0_m1_1_m2_PUB );

    fc::ecc::extended_private_key m_0_m1_1_m2_2 = m_0_m1_1_m2.derive_child(2);
    BOOST_CHECK_EQUAL( m_0_m1_1_m2_2.str(), TEST2_M_0_m1_1_m2_2_PRIV );
    BOOST_CHECK_EQUAL( m_0_m1_1_m2_2.get_extended_public_key().str(), TEST2_M_0_m1_1_m2_2_PUB );
    BOOST_CHECK_EQUAL( m_0_m1_1_m2.get_extended_public_key().derive_child(2).str(), TEST2_M_0_m1_1_m2_2_PUB );
}

//static void print(const unsigned char* data, int len) {
//    for (int i = 0; i < len; i++) {
//        printf("%02x", *data++);
//    }
//}

BOOST_AUTO_TEST_CASE(test_blinding_1)
{
    char buffer[7] = "test_";
    fc::ecc::extended_private_key alice = fc::ecc::extended_private_key::generate_master( "master" );
    fc::ecc::extended_private_key bob = fc::ecc::extended_private_key::generate_master( "puppet" );

    for ( int i = 0; i < 30; i++ )
    {
        buffer[5] = '0' + i;
        fc::ecc::extended_public_key bob_pub = bob.get_extended_public_key();
        fc::sha256 hash = fc::sha256::hash( buffer, sizeof(buffer) );
        fc::ecc::public_key t = alice.blind_public_key( bob_pub, i );
        fc::ecc::blinded_hash blinded = alice.blind_hash( hash, i );
        fc::ecc::blind_signature blind_sig = bob.blind_sign( blinded, i );
        try {
            fc::ecc::compact_signature sig = alice.unblind_signature( bob_pub, blind_sig, hash, i );
            fc::ecc::public_key validate( sig, hash );
//            printf("Validated: "); print((unsigned char*) validate.serialize().begin(), 33);
//            printf("\nT: "); print((unsigned char*) t.serialize().begin(), 33); printf("\n");
            BOOST_CHECK( validate.serialize() == t.serialize() );
        } catch (const fc::exception& e) {
            printf( "Test %d: %s\n", i, e.to_string().c_str() );
        }
        alice = alice.derive_child( i );
        bob = bob.derive_child( i | 0x80000000 );
    }
}

BOOST_AUTO_TEST_CASE(test_blinding_2)
{
    char message[7] = "test_0";
    fc::ecc::extended_private_key alice = fc::ecc::extended_private_key::generate_master( "master" );
    fc::ecc::extended_private_key bob = fc::ecc::extended_private_key::generate_master( "puppet" );
    fc::ecc::extended_public_key bob_pub = bob.get_extended_public_key();
    fc::sha256 hash = fc::sha256::hash( message, sizeof(message) );

    fc::ecc::public_key t = alice.blind_public_key( bob_pub, 0 );
    fc::ecc::public_key_data pub = t.serialize();
    char buffer[32];
    fc::from_hex( BLIND_T_X, buffer, sizeof(buffer) );
    BOOST_CHECK( !memcmp( pub.begin() + 1, buffer, sizeof(buffer) ) );

    fc::ecc::blinded_hash blinded = alice.blind_hash( hash, 0 );
    fc::from_hex( BLINDED_HASH, buffer, sizeof(buffer) );
    BOOST_CHECK( !memcmp( blinded.data(), buffer, sizeof(buffer) ) );

    fc::ecc::blind_signature blind_sig = bob.blind_sign( blinded, 0 );
    fc::from_hex( BLIND_SIG, buffer, sizeof(buffer) );
    BOOST_CHECK( !memcmp( blind_sig.data(), buffer, sizeof(buffer) ) );

    fc::ecc::compact_signature sig = alice.unblind_signature( bob_pub, blind_sig, hash, 0 );
    fc::from_hex( BLIND_K_X, buffer, sizeof(buffer) );
    BOOST_CHECK( !memcmp( sig.begin() + 1, buffer, sizeof(buffer) ) );
    fc::from_hex( UNBLINDED, buffer, sizeof(buffer) );
    BOOST_CHECK( !memcmp( sig.begin() + 33, buffer, sizeof(buffer) ) );
}

static void to_bignum(const char* data32, fc::ssl_bignum& out) {
    unsigned char dummy[33]; dummy[0] = 0;
    memcpy(dummy, data32, 32);
    BN_bin2bn((unsigned char*) data32, 32, out);
}

//static void print(const fc::sha256 hash) {
//    print((unsigned char*) hash.data(), hash.data_size());
//}
//
//static void print(const BIGNUM* bn) {
//    unsigned char buffer[64];
//    int len = BN_num_bytes(bn);
//    if (len > sizeof(buffer)) {
//        printf("BN too long - %d bytes?!", len);
//        return;
//    }
//    BN_bn2bin(bn, buffer);
//    print(buffer, len);
//}
//
//static void print(const fc::ec_group& curve, const fc::ec_point& p, fc::bn_ctx& ctx) {
//    fc::ssl_bignum x;
//    fc::ssl_bignum y;
//    EC_POINT_get_affine_coordinates_GFp(curve, p, x, y, ctx);
//    printf("(");
//    print(x);
//    printf(", ");
//    print(y);
//    printf(")");
//}

namespace fc {
SSL_TYPE(ec_key,       EC_KEY,       EC_KEY_free)
}

BOOST_AUTO_TEST_CASE(openssl_blinding)
{
    // Needed this "test" for producing data for debugging my libsecp256k1 implementation

    char buffer[7] = "test_0";
    fc::ecc::extended_private_key alice = fc::ecc::extended_private_key::generate_master( "master" );
    fc::ecc::extended_private_key bob = fc::ecc::extended_private_key::generate_master( "puppet" );
    fc::ec_group curve(EC_GROUP_new_by_curve_name(NID_secp256k1));
    fc::bn_ctx ctx(BN_CTX_new());
    fc::ssl_bignum n;
    EC_GROUP_get_order(curve, n, ctx);
    fc::ssl_bignum n_half;
    BN_rshift1(n_half, n);
    fc::ssl_bignum zero; BN_zero(zero);

    fc::sha256 hash_ = fc::sha256::hash( buffer, sizeof(buffer) );
    fc::ssl_bignum hash; to_bignum(hash_.data(), hash);
    fc::ssl_bignum a; to_bignum(alice.derive_hardened_child(0).get_secret().data(), a);
    fc::ssl_bignum b; to_bignum(alice.derive_hardened_child(1).get_secret().data(), b);
    fc::ssl_bignum c; to_bignum(alice.derive_hardened_child(2).get_secret().data(), c);
    fc::ssl_bignum d; to_bignum(alice.derive_hardened_child(3).get_secret().data(), d);

    fc::ec_point P(EC_POINT_new(curve));
    fc::ecc::public_key_data Pd = bob.get_extended_public_key().derive_child(0).serialize();
    fc::ssl_bignum Px; to_bignum(Pd.begin() + 1, Px);
    EC_POINT_set_compressed_coordinates_GFp(curve, P, Px, (*Pd.begin()) & 1, ctx);

    fc::ec_point Q(EC_POINT_new(curve));
    fc::ecc::public_key_data Qd = bob.get_extended_public_key().derive_child(1).serialize();
    fc::ssl_bignum Qx; to_bignum(Qd.begin() + 1, Qx);
    EC_POINT_set_compressed_coordinates_GFp(curve, Q, Qx, (*Qd.begin()) & 1, ctx);

    // Alice computes K = (c·a)^-1·P and public key T = (a·Kx)^-1·(b·G + Q + d·c^-1·P).
    fc::ec_point K(EC_POINT_new(curve));
    fc::ssl_bignum tmp;
    BN_mod_mul(tmp, a, c, n, ctx);
    BN_mod_inverse(tmp, tmp, n, ctx);
    EC_POINT_mul(curve, K, zero, P, tmp, ctx);

    fc::ec_point T(EC_POINT_new(curve));
    BN_mod_inverse(tmp, c, n, ctx);
    BN_mod_mul(tmp, d, tmp, n, ctx);
    EC_POINT_mul(curve, T, b, P, tmp, ctx);
    EC_POINT_add(curve, T, T, Q, ctx);
    fc::ssl_bignum Kx;
    fc::ssl_bignum Ky;
    EC_POINT_get_affine_coordinates_GFp(curve, K, Kx, Ky, ctx);
    BN_mod_mul(tmp, a, Kx, n, ctx);
    BN_mod_inverse(tmp, tmp, n, ctx);
    EC_POINT_mul(curve, T, zero, T, tmp, ctx);

    fc::ssl_bignum blinded;
    BN_mod_mul(blinded, a, hash, n, ctx);
    BN_mod_add(blinded, blinded, b, n, ctx);

    fc::ssl_bignum p; to_bignum(bob.derive_normal_child(0).get_secret().data(), p);
    fc::ssl_bignum q; to_bignum(bob.derive_normal_child(1).get_secret().data(), q);
    BN_mod_inverse(p, p, n, ctx);
    BN_mod_mul(q, q, p, n, ctx);
    fc::ssl_bignum blind_sig;
    BN_mod_mul(blind_sig, p, blinded, n, ctx);
    BN_mod_add(blind_sig, blind_sig, q, n, ctx);

    fc::ecdsa_sig sig(ECDSA_SIG_new());
    BN_copy(sig->r, Kx);
    BN_mod_mul(sig->s, c, blind_sig, n, ctx);
    BN_mod_add(sig->s, sig->s, d, n, ctx);

    if (BN_cmp(sig->s, n_half) > 0) {
        BN_sub(sig->s, n, sig->s);
    }

    fc::ec_key verify(EC_KEY_new());
    EC_KEY_set_public_key(verify, T);
    BOOST_CHECK( ECDSA_do_verify( (unsigned char*) hash_.data(), hash_.data_size(), sig, verify ) );
//        printf("a: "); print(a);
//        printf("\nb: "); print(b);
//        printf("\nc: "); print(c);
//        printf("\nd: "); print(d);
//        printf("\nP: "); print(curve, P, ctx);
//        printf("\nQ: "); print(curve, Q, ctx);
//        printf("\nK: "); print(curve, K, ctx);
//        printf("\nT: "); print(curve, T, ctx);
//        printf("\np: "); print(p);
//        printf("\nq: "); print(q);
//        printf("\nhash: "); print(hash_);
//        printf("\nblinded: "); print(blinded);
//        printf("\nblind_sig: "); print(blind_sig);
//        printf("\nunblinded: "); print(sig->s);
//        printf("\n");
}
