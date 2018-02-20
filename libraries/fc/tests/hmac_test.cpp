#define BOOST_TEST_MODULE HmacTest
#include <boost/test/unit_test.hpp>
#include <fc/array.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/crypto/hmac.hpp>
#include <fc/crypto/sha224.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/sha512.hpp>

// See http://tools.ietf.org/html/rfc4231

static const fc::string TEST1_KEY  = "0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b";
static const fc::string TEST1_DATA = "4869205468657265";
static const fc::string TEST1_224 = "896fb1128abbdf196832107cd49df33f47b4b1169912ba4f53684b22";
static const fc::string TEST1_256 = "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7";
static const fc::string TEST1_512 = "87aa7cdea5ef619d4ff0b4241a1d6cb02379f4e2ce4ec2787ad0b30545e17cde"
                                    "daa833b7d6b8a702038b274eaea3f4e4be9d914eeb61f1702e696c203a126854";

static const fc::string TEST2_KEY  = "4a656665";
static const fc::string TEST2_DATA = "7768617420646f2079612077616e7420666f72206e6f7468696e673f";
static const fc::string TEST2_224 = "a30e01098bc6dbbf45690f3a7e9e6d0f8bbea2a39e6148008fd05e44";
static const fc::string TEST2_256 = "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843";
static const fc::string TEST2_512 = "164b7a7bfcf819e2e395fbe73b56e0a387bd64222e831fd610270cd7ea250554"
                                    "9758bf75c05a994a6d034f65f8f0e6fdcaeab1a34d4a6b4b636e070a38bce737";

static const fc::string TEST3_KEY  = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
static const fc::string TEST3_DATA = "dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"
                                     "dddddddddddddddddddddddddddddddddddd";
static const fc::string TEST3_224 = "7fb3cb3588c6c1f6ffa9694d7d6ad2649365b0c1f65d69d1ec8333ea";
static const fc::string TEST3_256 = "773ea91e36800e46854db8ebd09181a72959098b3ef8c122d9635514ced565fe";
static const fc::string TEST3_512 = "fa73b0089d56a284efb0f0756c890be9b1b5dbdd8ee81a3655f83e33b2279d39"
                                    "bf3e848279a722c806b485a47e67c807b946a337bee8942674278859e13292fb";

static const fc::string TEST4_KEY  = "0102030405060708090a0b0c0d0e0f10111213141516171819";
static const fc::string TEST4_DATA = "cdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcd"
                                     "cdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcd";
static const fc::string TEST4_224 = "6c11506874013cac6a2abc1bb382627cec6a90d86efc012de7afec5a";
static const fc::string TEST4_256 = "82558a389a443c0ea4cc819899f2083a85f0faa3e578f8077a2e3ff46729665b";
static const fc::string TEST4_512 = "b0ba465637458c6990e5a8c5f61d4af7e576d97ff94b872de76f8050361ee3db"
                                    "a91ca5c11aa25eb4d679275cc5788063a5f19741120c4f2de2adebeb10a298dd";

// test 5 skipped - truncated

static const fc::string TEST6_KEY  = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                     "aaaaaa";
static const fc::string TEST6_DATA = "54657374205573696e67204c6172676572205468616e20426c6f636b2d53697a"
                                     "65204b6579202d2048617368204b6579204669727374";
static const fc::string TEST6_224 = "95e9a0db962095adaebe9b2d6f0dbce2d499f112f2d2b7273fa6870e";
static const fc::string TEST6_256 = "60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54";
static const fc::string TEST6_512 = "80b24263c7c1a3ebb71493c1dd7be8b49b46d1f41b4aeec1121b013783f8f352"
                                    "6b56d037e05f2598bd0fd2215d6a1e5295e64f73f63f0aec8b915a985d786598";

static const fc::string TEST7_KEY  = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                     "aaaaaa";
static const fc::string TEST7_DATA = "5468697320697320612074657374207573696e672061206c6172676572207468"
                                     "616e20626c6f636b2d73697a65206b657920616e642061206c61726765722074"
                                     "68616e20626c6f636b2d73697a6520646174612e20546865206b6579206e6565"
                                     "647320746f20626520686173686564206265666f7265206265696e6720757365"
                                     "642062792074686520484d414320616c676f726974686d2e";
static const fc::string TEST7_224 = "3a854166ac5d9f023f54d517d0b39dbd946770db9c2b95c9f6f565d1";
static const fc::string TEST7_256 = "9b09ffa71b942fcb27635fbcd5b0e944bfdc63644f0713938a7f51535c3a35e2";
static const fc::string TEST7_512 = "e37b6a775dc87dbaa4dfa9f96e5e3ffddebd71f8867289865df5a32d20cdc944"
                                    "b6022cac3c4982b10d5eeb55c3e4de15134676fb6de0446065c97440fa8c6a58";

static fc::hmac<fc::sha224> mac_224;
static fc::hmac<fc::sha256> mac_256;
static fc::hmac<fc::sha512> mac_512;

template<int N,int M>
static void run_test( const fc::string& key, const fc::string& data, const fc::string& expect_224,
                      const fc::string& expect_256, const fc::string& expect_512 )
{

    fc::array<char,N> key_arr;
    BOOST_CHECK_EQUAL( fc::from_hex( key, key_arr.begin(), key_arr.size() ), N );
    fc::array<char,M> data_arr;
    BOOST_CHECK_EQUAL( fc::from_hex( data, data_arr.begin(), data_arr.size() ), M );

    BOOST_CHECK_EQUAL( mac_224.digest( key_arr.begin(), N, data_arr.begin(), M ).str(), expect_224 );
    BOOST_CHECK_EQUAL( mac_256.digest( key_arr.begin(), N, data_arr.begin(), M ).str(), expect_256 );
    BOOST_CHECK_EQUAL( mac_512.digest( key_arr.begin(), N, data_arr.begin(), M ).str(), expect_512 );
}

BOOST_AUTO_TEST_CASE(hmac_test_1)
{
    run_test<20,8>( TEST1_KEY, TEST1_DATA, TEST1_224, TEST1_256, TEST1_512 );
}

BOOST_AUTO_TEST_CASE(hmac_test_2)
{
    run_test<4,28>( TEST2_KEY, TEST2_DATA, TEST2_224, TEST2_256, TEST2_512 );
}

BOOST_AUTO_TEST_CASE(hmac_test_3)
{
    run_test<20,50>( TEST3_KEY, TEST3_DATA, TEST3_224, TEST3_256, TEST3_512 );
}

BOOST_AUTO_TEST_CASE(hmac_test_4)
{
    run_test<25,50>( TEST4_KEY, TEST4_DATA, TEST4_224, TEST4_256, TEST4_512 );
}

BOOST_AUTO_TEST_CASE(hmac_test_6)
{
    run_test<131,54>( TEST6_KEY, TEST6_DATA, TEST6_224, TEST6_256, TEST6_512 );
}

BOOST_AUTO_TEST_CASE(hmac_test_7)
{
    run_test<131,152>( TEST7_KEY, TEST7_DATA, TEST7_224, TEST7_256, TEST7_512 );
}
