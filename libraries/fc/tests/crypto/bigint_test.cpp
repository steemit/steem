#include <boost/test/unit_test.hpp>

#include <fc/variant.hpp>
#include <fc/crypto/bigint.hpp>

BOOST_AUTO_TEST_SUITE(fc_crypto)

BOOST_AUTO_TEST_CASE(bigint_test_1)
{
    int64_t counter = 0, accu = 0, c_sq;
    fc::bigint bi_accu(accu);
    do {
        c_sq = counter * counter;
        fc::bigint add(c_sq);
        bi_accu += add;
        accu += c_sq;
        BOOST_CHECK( fc::bigint(accu) == bi_accu );

        bi_accu = bi_accu + add;
        accu = accu + c_sq;
        BOOST_CHECK_EQUAL( accu, bi_accu.to_int64() );

        bi_accu = fc::bigint( bi_accu.dup() );

        counter++;
    } while (c_sq < 1000000);

    fc::variant test;
    fc::to_variant( bi_accu, test );
    fc::bigint other;
    fc::from_variant( test, other );
    BOOST_CHECK( other == bi_accu );
}

BOOST_AUTO_TEST_CASE(bigint_test_2)
{
    const fc::bigint bi_1(1), bi_3(3), bi_17(17), bi_65537(65537);
    fc::bigint bi_accu(bi_1);
    do {
        std::vector<char> bytes = bi_accu;
        fc::bigint a_1( bytes );
        a_1 = a_1 + bi_1;
        BOOST_CHECK( bi_accu < a_1 );

        bi_accu = a_1 * bi_accu;
        BOOST_CHECK( bi_accu >= a_1 );
    } while ( bi_accu.log2() <= 128 );

    bi_accu = bi_accu;

    BOOST_CHECK( bi_accu && !bi_accu.is_negative() && bi_accu != bi_1 );

    BOOST_CHECK( bi_3.exp( bi_accu.log2() ) > bi_accu );

    fc::bigint big(1);
    big <<= 30; big += bi_17; big <<= 30; big++;
    big <<= 30; big -= bi_17; big >>= 5; big--;
    fc::bigint rest = bi_accu % big;
    BOOST_CHECK( (bi_accu - rest) / big == bi_accu / big );

    fc::bigint big2; big2 = big;
    big2 *= bi_65537.exp(3);
    big2 /= bi_65537.exp(3);
    BOOST_CHECK( big2 == big );
    big--;

    BOOST_CHECK_EQUAL( (std::string) bi_1, "1" );
    BOOST_CHECK_EQUAL( (std::string) bi_3, "3" );
    BOOST_CHECK_EQUAL( (std::string) bi_17, "17" );
    BOOST_CHECK_EQUAL( (std::string) bi_65537, "65537" );
    BOOST_CHECK_EQUAL( (std::string) bi_65537.exp(3), "281487861809153" );
    BOOST_CHECK_EQUAL( (std::string) bi_accu, "12864938683278671740537145998360961546653259485195806" );
    BOOST_CHECK_EQUAL( (std::string) big, "38685626840157682946539517" );
}

BOOST_AUTO_TEST_SUITE_END()

