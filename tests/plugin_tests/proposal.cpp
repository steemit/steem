#include <boost/test/unit_test.hpp>

#include <steem/chain/account_object.hpp>
#include <steem/protocol/steem_operations.hpp>

#include <steem/plugins/sps/sps_plugin.hpp>

#include "../db_fixture/database_fixture.hpp"

using namespace steem::chain;
using namespace steem::protocol;

BOOST_FIXTURE_TEST_SUITE( rewarding_proposal_tests, proposal_database_fixture_for_plugin )

BOOST_AUTO_TEST_CASE( generating_payments )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: generating payments" );

      plugin_prepare();

      ACTORS( (alice)(bob)(carol) )
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      generate_block();

      //=====================preparing=====================
      auto creator = "alice";
      auto receiver = "bob";

      auto start_date = db->head_block_time();
      auto end_date = start_date + fc::days( 2 );

      auto daily_pay = asset( 48, SBD_SYMBOL );
      auto hourly_pay = asset( daily_pay.amount.value / 24, SBD_SYMBOL );

      FUND( creator, ASSET( "160.000 TESTS" ) );
      FUND( creator, ASSET( "80.000 TBD" ) );
      FUND( STEEM_TREASURY_ACCOUNT, ASSET( "5000.000 TBD" ) );

      auto voter_01 = "carol";
      auto voter_vests_amount_01 = ASSET( "1.000 TESTS" );
      //=====================preparing=====================

      //Needed basic operations
      int64_t id_proposal_00 = create_proposal( creator, receiver, start_date, end_date, daily_pay, alice_private_key );
      generate_blocks( 1 );

      vote_proposal( voter_01, { id_proposal_00 }, true/*approve*/, carol_private_key );
      generate_blocks( 1 );

      transfer_vests( creator, voter_01, voter_vests_amount_01, alice_private_key );
      generate_blocks( 1 );

      //skipping interest generating is necessary
      transfer( creator, receiver, ASSET( "0.001 TBD" ), alice_private_key );
      generate_block( 5 );
      transfer( creator, STEEM_TREASURY_ACCOUNT, ASSET( "0.001 TBD" ), alice_private_key );
      generate_block( 5 );

      const account_object& _creator = db->get_account( creator );
      const account_object& _receiver = db->get_account( receiver );
      const account_object& _voter_01 = db->get_account( voter_01 );
      const account_object& _treasury = db->get_account( STEEM_TREASURY_ACCOUNT );

      {
         BOOST_TEST_MESSAGE( "---Payment---" );

         auto before_creator_sbd_balance = _creator.sbd_balance;
         auto before_receiver_sbd_balance = _receiver.sbd_balance;
         auto before_voter_01_sbd_balance = _voter_01.sbd_balance;
         auto before_treasury_sbd_balance = _treasury.sbd_balance;
      
         auto next_block = get_nr_blocks_until_maintenance_block();
         generate_blocks( next_block - 1 );
         generate_blocks( 1 );

         auto after_creator_sbd_balance = _creator.sbd_balance;
         auto after_receiver_sbd_balance = _receiver.sbd_balance;
         auto after_voter_01_sbd_balance = _voter_01.sbd_balance;
         auto after_treasury_sbd_balance = _treasury.sbd_balance;
   
         BOOST_REQUIRE( before_creator_sbd_balance == after_creator_sbd_balance );
         BOOST_REQUIRE( before_receiver_sbd_balance == after_receiver_sbd_balance - hourly_pay );
         BOOST_REQUIRE( before_voter_01_sbd_balance == after_voter_01_sbd_balance );
         BOOST_REQUIRE( before_treasury_sbd_balance == after_treasury_sbd_balance + hourly_pay );
      }

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( proposals_maintenance )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: removing inactive proposals" );
      //Update see issue #85 -> https://github.com/blocktradesdevs/steem/issues/85
      //Remove proposal will be automatic action - this test shall be temporary disabled.
      return;

      plugin_prepare();

      ACTORS( (alice)(bob) )
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      generate_block();

      //=====================preparing=====================
      auto creator = "alice";
      auto receiver = "bob";

      auto start_time = db->head_block_time();

      auto start_date_00 = start_time + fc::seconds( 30 );
      auto end_date_00 = start_time + fc::minutes( 10 );

      auto start_date_01 = start_time + fc::seconds( 40 );
      auto end_date_01 = start_time + fc::minutes( 30 );

      auto start_date_02 = start_time + fc::seconds( 50 );
      auto end_date_02 = start_time + fc::minutes( 20 );

      auto daily_pay = asset( 100, SBD_SYMBOL );

      FUND( creator, ASSET( "100.000 TBD" ) );
      //=====================preparing=====================

      int64_t id_proposal_00 = create_proposal( creator, receiver, start_date_00, end_date_00, daily_pay, alice_private_key );
      generate_block();

      int64_t id_proposal_01 = create_proposal( creator, receiver, start_date_01, end_date_01, daily_pay, alice_private_key );
      generate_block();

      int64_t id_proposal_02 = create_proposal( creator, receiver, start_date_02, end_date_02, daily_pay, alice_private_key );
      generate_block();

      {
         BOOST_REQUIRE( exist_proposal( id_proposal_00 ) );
         BOOST_REQUIRE( exist_proposal( id_proposal_01 ) );
         BOOST_REQUIRE( exist_proposal( id_proposal_02 ) );

         generate_blocks( start_time + fc::seconds( STEEM_PROPOSAL_MAINTENANCE_CLEANUP ) );
         start_time = db->head_block_time();

         BOOST_REQUIRE( exist_proposal( id_proposal_00 ) );
         BOOST_REQUIRE( exist_proposal( id_proposal_01 ) );
         BOOST_REQUIRE( exist_proposal( id_proposal_02 ) );

         generate_blocks( start_time + fc::minutes( 11 ) );
         BOOST_REQUIRE( !exist_proposal( id_proposal_00 ) );
         BOOST_REQUIRE( exist_proposal( id_proposal_01 ) );
         BOOST_REQUIRE( exist_proposal( id_proposal_02 ) );

         generate_blocks( start_time + fc::minutes( 21 ) );
         BOOST_REQUIRE( !exist_proposal( id_proposal_00 ) );
         BOOST_REQUIRE( exist_proposal( id_proposal_01 ) );
         BOOST_REQUIRE( !exist_proposal( id_proposal_02 ) );

         generate_blocks( start_time + fc::minutes( 31 ) );
         BOOST_REQUIRE( !exist_proposal( id_proposal_00 ) );
         BOOST_REQUIRE( !exist_proposal( id_proposal_01 ) );
         BOOST_REQUIRE( !exist_proposal( id_proposal_02 ) );
      }

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
