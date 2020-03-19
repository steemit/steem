#include <boost/test/unit_test.hpp>

#include <steem/chain/steem_fwd.hpp>

#include <steem/protocol/exceptions.hpp>
#include <steem/protocol/hardfork.hpp>
#include <steem/protocol/sps_operations.hpp>

#include <steem/chain/database.hpp>
#include <steem/chain/database_exceptions.hpp>
#include <steem/chain/steem_objects.hpp>

#include <steem/chain/util/reward.hpp>

#include <steem/plugins/rc/rc_objects.hpp>
#include <steem/plugins/rc/resource_count.hpp>

#include <fc/macros.hpp>
#include <fc/crypto/digest.hpp>

#include "../db_fixture/database_fixture.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace steem;
using namespace steem::chain;
using namespace steem::protocol;
using fc::string;

BOOST_FIXTURE_TEST_SUITE( hf23_tests, hf23_database_fixture )

BOOST_AUTO_TEST_CASE( basic_test_06 )
{
   try
   {
      BOOST_TEST_MESSAGE( "VEST delegations - object of type `vesting_delegation_expiration_object` is generated" );

      ACTORS( (alice)(bob) )
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      generate_block();

      auto _10 = ASSET( "10.000 TESTS" );
      auto _20 = ASSET( "20.000 TESTS" );
      auto _1v = ASSET( "1.000000 VESTS" );
      auto _2v = ASSET( "2.000000 VESTS" );
      auto _3v = ASSET( "3.000000 VESTS" );
      auto _1000 = ASSET( "1000.000 TESTS" );

      FUND( "alice", _1000 );
      FUND( "bob", _1000 );

      {
         vest( "alice", "alice", _10, alice_private_key );
         vest( "bob", "bob", _20, bob_private_key );
      }
      {
         delegate_vest( "alice", "bob", _3v, alice_private_key );
         generate_block();

         const auto& idx = db->get_index< vesting_delegation_expiration_index, by_account_expiration >();
         BOOST_REQUIRE( idx.lower_bound( "alice" ) == idx.end() );
         BOOST_REQUIRE( idx.lower_bound( "bob" ) == idx.end() );
      }
      {
         delegate_vest( "alice", "bob", _3v, alice_private_key );

         const auto& idx = db->get_index< vesting_delegation_expiration_index, by_account_expiration >();
         BOOST_REQUIRE( idx.lower_bound( "alice" ) == idx.end() );
         BOOST_REQUIRE( idx.lower_bound( "bob" ) == idx.end() );
      }
      {
         delegate_vest( "alice", "bob", _2v, alice_private_key );

         const auto& idx = db->get_index< vesting_delegation_expiration_index, by_account_expiration >();
         BOOST_REQUIRE( idx.lower_bound( "alice" ) != idx.end() );
         BOOST_REQUIRE( idx.lower_bound( "bob" ) == idx.end() );
      }
      {
         delegate_vest( "alice", "bob", _1v, alice_private_key );

         const auto& idx = db->get_index< vesting_delegation_expiration_index, by_account_expiration >();
         BOOST_REQUIRE( idx.lower_bound( "alice" ) != idx.end() );
         BOOST_REQUIRE( idx.lower_bound( "bob" ) == idx.end() );
      }
      {
         db->clear_account( db->get_account( "bob" ) );

         const auto& idx = db->get_index< vesting_delegation_expiration_index, by_account_expiration >();
         BOOST_REQUIRE( idx.lower_bound( "alice" ) != idx.end() );
         BOOST_REQUIRE( idx.lower_bound( "bob" ) == idx.end() );
      }
      {
         db->clear_account( db->get_account( "alice" ) );

         const auto& idx = db->get_index< vesting_delegation_expiration_index, by_account_expiration >();
         BOOST_REQUIRE( idx.lower_bound( "alice" ) == idx.end() );
         BOOST_REQUIRE( idx.lower_bound( "bob" ) == idx.end() );
      }

      database_fixture::validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( basic_test_05 )
{
   try
   {
      BOOST_TEST_MESSAGE( "VEST delegations - more complex scenarios" );

      ACTORS( (alice)(bob)(carol) )
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      generate_block();

      auto _10 = ASSET( "10.000 TESTS" );
      auto _20 = ASSET( "20.000 TESTS" );
      auto _1v = ASSET( "1.000000 VESTS" );
      auto _2v = ASSET( "2.000000 VESTS" );
      auto _3v = ASSET( "3.000000 VESTS" );
      auto _1000 = ASSET( "1000.000 TESTS" );

      FUND( "alice", _1000 );
      FUND( "bob", _1000 );
      FUND( "carol", _1000 );

      {
         vest( "alice", "alice", _10, alice_private_key );
         vest( "bob", "bob", _20, bob_private_key );
         vest( "carol", "carol", _10, carol_private_key );
      }
      {
         delegate_vest( "alice", "bob", _1v, alice_private_key );
         delegate_vest( "alice", "bob", _2v, alice_private_key );

         BOOST_REQUIRE( db->get_account( "alice" ).delegated_vesting_shares.amount.value == _2v.amount.value );

         BOOST_REQUIRE( db->get_account( "bob" ).received_vesting_shares.amount.value == _2v.amount.value );
      }
      {
         delegate_vest( "alice", "carol", _1v, alice_private_key );

         BOOST_REQUIRE( db->get_account( "alice" ).delegated_vesting_shares.amount.value == _3v.amount.value );

         BOOST_REQUIRE( db->get_account( "bob" ).received_vesting_shares.amount.value == _2v.amount.value );
         BOOST_REQUIRE( db->get_account( "carol" ).received_vesting_shares.amount.value == _1v.amount.value );
      }
      {
         delegate_vest( "carol", "bob", _1v, carol_private_key );

         BOOST_REQUIRE( db->get_account( "alice" ).delegated_vesting_shares.amount.value == _3v.amount.value );
         BOOST_REQUIRE( db->get_account( "carol" ).delegated_vesting_shares.amount.value == _1v.amount.value );

         BOOST_REQUIRE( db->get_account( "bob" ).received_vesting_shares.amount.value == _3v.amount.value );
         BOOST_REQUIRE( db->get_account( "carol" ).received_vesting_shares.amount.value == _1v.amount.value );
      }
      {
         db->clear_account( db->get_account( "carol" ) );

         BOOST_REQUIRE( db->get_account( "alice" ).delegated_vesting_shares.amount.value == _3v.amount.value );
         BOOST_REQUIRE( db->get_account( "carol" ).delegated_vesting_shares.amount.value == 0l );

         BOOST_REQUIRE( db->get_account( "bob" ).received_vesting_shares.amount.value == _2v.amount.value );
         BOOST_REQUIRE( db->get_account( "carol" ).received_vesting_shares.amount.value == _1v.amount.value );
      }
      {
         db->clear_account( db->get_account( "alice" ) );

         BOOST_REQUIRE( db->get_account( "alice" ).delegated_vesting_shares.amount.value == 0l );
         BOOST_REQUIRE( db->get_account( "bob" ).delegated_vesting_shares.amount.value == 0l );
         BOOST_REQUIRE( db->get_account( "carol" ).delegated_vesting_shares.amount.value == 0l );

         BOOST_REQUIRE( db->get_account( "alice" ).received_vesting_shares.amount.value == 0l );
         BOOST_REQUIRE( db->get_account( "bob" ).received_vesting_shares.amount.value == 0l );
         BOOST_REQUIRE( db->get_account( "carol" ).received_vesting_shares.amount.value == 0l );
      }

      database_fixture::validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( basic_test_04 )
{
   try
   {
      BOOST_TEST_MESSAGE( "VEST delegations" );

      ACTORS( (alice)(bob)(carol) )
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      generate_block();

      auto _10 = ASSET( "10.000 TESTS" );
      auto _20 = ASSET( "20.000 TESTS" );
      auto _1v = ASSET( "1.000000 VESTS" );
      auto _2v = ASSET( "2.000000 VESTS" );
      auto _1000 = ASSET( "1000.000 TESTS" );

      FUND( "alice", _1000 );
      FUND( "bob", _1000 );
      FUND( "carol", _1000 );

      {
         vest( "alice", "alice", _10, alice_private_key );
         vest( "alice", "alice", _20, alice_private_key );

         delegate_vest( "alice", "bob", _1v, alice_private_key );
         BOOST_REQUIRE( db->get_account( "alice" ).delegated_vesting_shares.amount.value != 0l );
         BOOST_REQUIRE( db->get_account( "bob" ).delegated_vesting_shares.amount.value == 0l );
         BOOST_REQUIRE( db->get_account( "carol" ).delegated_vesting_shares.amount.value == 0l );

         BOOST_REQUIRE( db->get_account( "alice" ).received_vesting_shares.amount.value == 0l );
         BOOST_REQUIRE( db->get_account( "bob" ).received_vesting_shares.amount.value != 0l );
         BOOST_REQUIRE( db->get_account( "carol" ).received_vesting_shares.amount.value == 0l );

         auto previous = db->get_account( "alice" ).delegated_vesting_shares.amount.value;

         delegate_vest( "alice", "carol", _2v, alice_private_key );

         auto next = db->get_account( "alice" ).delegated_vesting_shares.amount.value;
         BOOST_REQUIRE( next > previous );

         BOOST_REQUIRE( db->get_account( "alice" ).delegated_vesting_shares.amount.value != 0l );
         BOOST_REQUIRE( db->get_account( "bob" ).delegated_vesting_shares.amount.value == 0l );
         BOOST_REQUIRE( db->get_account( "carol" ).delegated_vesting_shares.amount.value == 0l );

         BOOST_REQUIRE( db->get_account( "alice" ).received_vesting_shares.amount.value == 0l );
         BOOST_REQUIRE( db->get_account( "bob" ).received_vesting_shares.amount.value != 0l );
         BOOST_REQUIRE( db->get_account( "carol" ).received_vesting_shares.amount.value != 0l );
      }
      {
         db->clear_account( db->get_account( "alice" ) );
         BOOST_REQUIRE( db->get_account( "alice" ).delegated_vesting_shares.amount.value == 0l );
         BOOST_REQUIRE( db->get_account( "bob" ).delegated_vesting_shares.amount.value == 0l );
         BOOST_REQUIRE( db->get_account( "carol" ).delegated_vesting_shares.amount.value == 0l );

         BOOST_REQUIRE( db->get_account( "alice" ).received_vesting_shares.amount.value == 0l );
         BOOST_REQUIRE( db->get_account( "bob" ).received_vesting_shares.amount.value == 0l );
         BOOST_REQUIRE( db->get_account( "carol" ).received_vesting_shares.amount.value == 0l );

         db->clear_account( db->get_account( "bob" ) );
         BOOST_REQUIRE( db->get_account( "alice" ).delegated_vesting_shares.amount.value == 0l );
         BOOST_REQUIRE( db->get_account( "bob" ).delegated_vesting_shares.amount.value == 0l );
         BOOST_REQUIRE( db->get_account( "carol" ).delegated_vesting_shares.amount.value == 0l );

         BOOST_REQUIRE( db->get_account( "alice" ).received_vesting_shares.amount.value == 0l );
         BOOST_REQUIRE( db->get_account( "bob" ).received_vesting_shares.amount.value == 0l );
         BOOST_REQUIRE( db->get_account( "carol" ).received_vesting_shares.amount.value == 0l );
      }

      database_fixture::validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( basic_test_03 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Vesting every account to anothers accounts" );

      ACTORS( (alice)(bob)(carol) )
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      generate_block();

      auto _1 = ASSET( "1.000 TESTS" );
      auto _2 = ASSET( "2.000 TESTS" );
      auto _3 = ASSET( "3.000 TESTS" );
      auto _1000 = ASSET( "1000.000 TESTS" );

      FUND( "alice", _1000 );
      FUND( "bob", _1000 );
      FUND( "carol", _1000 );

      BOOST_REQUIRE( db->get_account( "alice" ).vesting_shares == db->get_account( "bob" ).vesting_shares );
      BOOST_REQUIRE( db->get_account( "bob" ).vesting_shares == db->get_account( "carol" ).vesting_shares );

      {
         vest( "alice", "bob", _1, alice_private_key );
         vest( "bob", "carol", _2, bob_private_key );
         vest( "carol", "alice", _3, carol_private_key );
         BOOST_REQUIRE_GT( db->get_account( "alice" ).vesting_shares.amount.value, db->get_account( "carol" ).vesting_shares.amount.value );
         BOOST_REQUIRE_GT( db->get_account( "carol" ).vesting_shares.amount.value, db->get_account( "bob" ).vesting_shares.amount.value );
      }
      {
         auto vest_bob = db->get_account( "bob" ).vesting_shares.amount.value;
         auto vest_carol = db->get_account( "carol" ).vesting_shares.amount.value;

         db->clear_account( db->get_account( "alice" ) );
         BOOST_REQUIRE( db->get_account( "alice" ).vesting_shares.amount.value == 0l );
         BOOST_REQUIRE( db->get_account( "bob" ).vesting_shares.amount.value == vest_bob );
         BOOST_REQUIRE( db->get_account( "carol" ).vesting_shares.amount.value == vest_carol );

         db->clear_account( db->get_account( "bob" ) );
         BOOST_REQUIRE( db->get_account( "alice" ).vesting_shares.amount.value == 0l );
         BOOST_REQUIRE( db->get_account( "bob" ).vesting_shares.amount.value == 0l );
         BOOST_REQUIRE( db->get_account( "carol" ).vesting_shares.amount.value == vest_carol );

         db->clear_account( db->get_account( "carol" ) );
         BOOST_REQUIRE( db->get_account( "alice" ).vesting_shares.amount.value == 0l );
         BOOST_REQUIRE( db->get_account( "bob" ).vesting_shares.amount.value == 0l );
         BOOST_REQUIRE( db->get_account( "carol" ).vesting_shares.amount.value == 0l );
      }

      database_fixture::validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( basic_test_02 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Vesting" );

      ACTORS( (alice)(bob) )
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      generate_block();

      auto _1 = ASSET( "1.000 TESTS" );
      auto _1000 = ASSET( "1000.000 TESTS" );

      FUND( "alice", _1000 );
      FUND( "bob", _1000 );

      BOOST_REQUIRE( db->get_account( "alice" ).vesting_shares == db->get_account( "bob" ).vesting_shares );

      {
         vest( "alice", "alice", _1, alice_private_key );
         BOOST_REQUIRE_GT( db->get_account( "alice" ).vesting_shares.amount.value, db->get_account( "bob" ).vesting_shares.amount.value );
      }
      {
         auto vest_bob = db->get_account( "bob" ).vesting_shares.amount.value;

         db->clear_account( db->get_account( "alice" ) );
         BOOST_REQUIRE( db->get_account( "alice" ).vesting_shares.amount.value == 0l );
         BOOST_REQUIRE( db->get_account( "bob" ).vesting_shares.amount.value == vest_bob );

         db->clear_account( db->get_account( "bob" ) );
         BOOST_REQUIRE( db->get_account( "alice" ).vesting_shares.amount.value == 0l );
         BOOST_REQUIRE( db->get_account( "bob" ).vesting_shares.amount.value == 0l );
      }

      database_fixture::validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( basic_test_01 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Calling clear_account" );

      ACTORS( (alice) )
      generate_block();

      auto& _alice = db->get_account( "alice" );
      db->clear_account( _alice );

      /*
         Original `clean_database_fixture::validate_database` checks `rc_plugin` as well.
         Is it needed?
      */
      database_fixture::validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
