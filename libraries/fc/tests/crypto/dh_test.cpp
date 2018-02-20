#include <boost/test/unit_test.hpp>

#include <fc/crypto/dh.hpp>
#include <fc/exception/exception.hpp>

BOOST_AUTO_TEST_SUITE(fc_crypto)

BOOST_AUTO_TEST_CASE(dh_test)
{
    fc::diffie_hellman alice;
    BOOST_CHECK( alice.generate_params( 128, 5 ) );
    BOOST_CHECK( alice.generate_pub_key() );

    fc::diffie_hellman bob;
    bob.p = alice.p;
    BOOST_CHECK( bob.validate() );
    BOOST_CHECK( bob.generate_pub_key() );

    fc::diffie_hellman charlie;
    BOOST_CHECK( !charlie.validate() );
    BOOST_CHECK( !charlie.generate_pub_key() );
    charlie.p = alice.p;
    BOOST_CHECK( charlie.validate() );
    BOOST_CHECK( charlie.generate_pub_key() );

    BOOST_CHECK( alice.compute_shared_key( bob.pub_key ) );
    BOOST_CHECK( bob.compute_shared_key( alice.pub_key ) );
    BOOST_CHECK_EQUAL( alice.shared_key.size(), bob.shared_key.size() );
    BOOST_CHECK( !memcmp( alice.shared_key.data(), bob.shared_key.data(), alice.shared_key.size() ) );
    std::vector<char> alice_bob = alice.shared_key;

    BOOST_CHECK( alice.compute_shared_key( charlie.pub_key ) );
    BOOST_CHECK( charlie.compute_shared_key( alice.pub_key ) );
    BOOST_CHECK_EQUAL( alice.shared_key.size(), charlie.shared_key.size() );
    BOOST_CHECK( !memcmp( alice.shared_key.data(), charlie.shared_key.data(), alice.shared_key.size() ) );
    std::vector<char> alice_charlie = alice.shared_key;

    BOOST_CHECK( charlie.compute_shared_key( bob.pub_key ) );
    BOOST_CHECK( bob.compute_shared_key( charlie.pub_key ) );
    BOOST_CHECK_EQUAL( charlie.shared_key.size(), bob.shared_key.size() );
    BOOST_CHECK( !memcmp( charlie.shared_key.data(), bob.shared_key.data(), bob.shared_key.size() ) );
    std::vector<char> bob_charlie = charlie.shared_key;

    BOOST_CHECK_EQUAL( alice_bob.size(), alice_charlie.size() );
    BOOST_CHECK( memcmp( alice_bob.data(), alice_charlie.data(), alice_bob.size() ) );

    BOOST_CHECK_EQUAL( alice_bob.size(), bob_charlie.size() );
    BOOST_CHECK( memcmp( alice_bob.data(), bob_charlie.data(), alice_bob.size() ) );

    BOOST_CHECK_EQUAL( alice_charlie.size(), bob_charlie.size() );
    BOOST_CHECK( memcmp( alice_charlie.data(), bob_charlie.data(), alice_charlie.size() ) );

    alice.p.clear(); alice.p.push_back(100); alice.p.push_back(2);
    BOOST_CHECK( !alice.validate() );
    alice.p = bob.p;
    alice.g = 9;
    BOOST_CHECK( !alice.validate() );

// It ain't easy...
//    alice.g = 2;
//    BOOST_CHECK( alice.validate() );
//    BOOST_CHECK( alice.generate_pub_key() );
//    BOOST_CHECK( alice.compute_shared_key( bob.pub_key ) );
//    BOOST_CHECK( bob.compute_shared_key( alice.pub_key ) );
//    BOOST_CHECK_EQUAL( alice.shared_key.size(), bob.shared_key.size() );
//    BOOST_CHECK( memcmp( alice.shared_key.data(), bob.shared_key.data(), alice.shared_key.size() ) );
}

BOOST_AUTO_TEST_SUITE_END()
