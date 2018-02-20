#include <boost/test/unit_test.hpp>

#include <fc/crypto/hex.hpp>
#include <fc/crypto/base32.hpp>
#include <fc/crypto/base36.hpp>
#include <fc/crypto/base58.hpp>
#include <fc/crypto/base64.hpp>
#include <fc/exception/exception.hpp>

#include <iostream>

static const std::string TEST1("");
static const std::string TEST2("\0\00101", 4);
static const std::string TEST3("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
static const std::string TEST4("\377\376\000\375\001\374", 6);
static const std::string TEST5("\0\0\0", 3);

static void test_16( const std::string& test, const std::string& expected )
{
    std::vector<char> vec( test.begin(), test.end() );
    fc::string enc1 = fc::to_hex( vec );
    fc::string enc2 = fc::to_hex( test.c_str(), test.size() );
    BOOST_CHECK_EQUAL( enc1, enc2 );
    BOOST_CHECK_EQUAL( expected, enc2 );

    char out[32];
    int len = fc::from_hex( enc1, out, 32 );
    BOOST_CHECK_EQUAL( test.size(), len );
    BOOST_CHECK( !memcmp( test.c_str(), out, len ) );
    if (len > 10) {
        BOOST_CHECK( fc::from_hex( enc1, out, 10 ) <= 10 );
    }
}

BOOST_AUTO_TEST_SUITE(fc_crypto)

BOOST_AUTO_TEST_CASE(hex_test)
{
    test_16( TEST1, "" );
    test_16( TEST2, "00013031" );
    test_16( TEST3, "4142434445464748494a4b4c4d4e4f505152535455565758595a" );
    test_16( TEST4, "fffe00fd01fc" );
    test_16( TEST5, "000000" );
}


static void test_32( const std::string& test, const std::string& expected )
{
    std::vector<char> vec( test.begin(), test.end() );
    fc::string enc1 = fc::to_base32( vec );
    fc::string enc2 = fc::to_base32( test.c_str(), test.size() );
    BOOST_CHECK_EQUAL( enc1, enc2 );
    BOOST_CHECK_EQUAL( expected, enc2 );

    std::vector<char> dec = fc::from_base32( enc1 );
    BOOST_CHECK_EQUAL( vec.size(), dec.size() );
    BOOST_CHECK( !memcmp( vec.data(), dec.data(), vec.size() ) );
}

BOOST_AUTO_TEST_CASE(base32_test)
{
    test_32( TEST1, "" );
    test_32( TEST2, "AAATAMI=" );
    test_32( TEST3, "IFBEGRCFIZDUQSKKJNGE2TSPKBIVEU2UKVLFOWCZLI======" );
    test_32( TEST4, "777AB7IB7Q======" );
    test_32( TEST5, "AAAAA===" );
}


static void test_36( const std::string& test, const std::string& expected )
{
    std::vector<char> vec( test.begin(), test.end() );
    fc::string enc1 = fc::to_base36( vec );
    fc::string enc2 = fc::to_base36( test.c_str(), test.size() );
    BOOST_CHECK_EQUAL( enc1, enc2 );
    BOOST_CHECK_EQUAL( expected, enc2 );

    std::vector<char> dec = fc::from_base36( enc1 );
    BOOST_CHECK_EQUAL( vec.size(), dec.size() );
    BOOST_CHECK( !memcmp( vec.data(), dec.data(), vec.size() ) );
}

BOOST_AUTO_TEST_CASE(base36_test)
{
    test_36( TEST1, "" );
    test_36( TEST2, "01o35" );
    test_36( TEST3, "l4ksdleyi5pnl0un5raue268ptj43dwjwmz15ie2" );
    test_36( TEST4, "2rrrvpb7y4" );
    test_36( TEST5, "000" );
}


static void test_58( const std::string& test, const std::string& expected )
{
    std::vector<char> vec( test.begin(), test.end() );
    fc::string enc1 = fc::to_base58( vec );
    fc::string enc2 = fc::to_base58( test.c_str(), test.size() );
    BOOST_CHECK_EQUAL( enc1, enc2 );
    BOOST_CHECK_EQUAL( expected, enc2 );

    std::vector<char> dec = fc::from_base58( enc1 );
    BOOST_CHECK_EQUAL( vec.size(), dec.size() );
    BOOST_CHECK( !memcmp( vec.data(), dec.data(), vec.size() ) );

    char buffer[64];
    size_t len = fc::from_base58( enc1, buffer, 64 );
    BOOST_CHECK( len <= 64 );
    BOOST_CHECK( !memcmp( vec.data(), buffer, len ) );
    if ( len > 10 ) {
        try {
            len = fc::from_base58( enc1, buffer, 10 );
            BOOST_CHECK( len <= 10 );
        } catch ( fc::exception expected ) {}
    }

}

BOOST_AUTO_TEST_CASE(base58_test)
{
    test_58( TEST1, "" );
    test_58( TEST2, "1Q9e" );
    test_58( TEST3, "2zuFXTJSTRK6ESktqhM2QDBkCnH1U46CnxaD" );
    test_58( TEST4, "3CUeREErf" );
    test_58( TEST5, "111" );
}


static void test_64( const std::string& test, const std::string& expected )
{
    fc::string enc1 = fc::base64_encode( test );
    fc::string enc2 = fc::base64_encode( test.c_str(), test.size() );
    BOOST_CHECK_EQUAL( enc1, enc2 );
    BOOST_CHECK_EQUAL( expected, enc2 );

    std::string dec = fc::base64_decode( enc1 );
    BOOST_CHECK_EQUAL( test.size(), dec.size() );
    BOOST_CHECK_EQUAL( test, dec );
}

BOOST_AUTO_TEST_CASE(base64_test)
{
    test_64( TEST1, "" );
    test_64( TEST2, "AAEwMQ==" );
    test_64( TEST3, "QUJDREVGR0hJSktMTU5PUFFSU1RVVldYWVo=" );
    test_64( TEST4, "//4A/QH8" );
    test_64( TEST5, "AAAA" );
}


BOOST_AUTO_TEST_SUITE_END()
