#include <boost/test/unit_test.hpp>

#include <steem/protocol/exceptions.hpp>
#include <steem/protocol/hardfork.hpp>
#include <steem/protocol/sps_operations.hpp>

#include <steem/chain/database.hpp>
#include <steem/chain/database_exceptions.hpp>
#include <steem/chain/steem_objects.hpp>

#include <steem/chain/util/reward.hpp>

#include <steem/plugins/rc/rc_objects.hpp>
#include <steem/plugins/rc/resource_count.hpp>

#include <steem/chain/sps_objects.hpp>

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

BOOST_FIXTURE_TEST_SUITE( proposal_tests, proposal_database_fixture )

BOOST_AUTO_TEST_CASE( proposal_object_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: create_proposal_operation" );

      ACTORS( (alice)(bob) )
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      generate_block();

      auto fee = asset( STEEM_TREASURY_FEE, SBD_SYMBOL );

      auto creator = "alice";
      auto receiver = "bob";

      auto start_date = db->head_block_time() + fc::days( 1 );
      auto end_date = start_date + fc::days( 2 );

      auto daily_pay = asset( 100, SBD_SYMBOL );

      auto subject = "hello";
      auto url = "http:://something.html";

      FUND( creator, ASSET( "80.000 TBD" ) );

      signed_transaction tx;

      const account_object& before_alice_account = db->get_account( creator );
      const account_object& before_bob_account = db->get_account( receiver );

      auto before_alice_sbd_balance = before_alice_account.sbd_balance;
      auto before_bob_sbd_balance = before_bob_account.sbd_balance;

      create_proposal_operation op;

      op.creator = creator;
      op.receiver = receiver;

      op.start_date = start_date;
      op.end_date = end_date;

      op.daily_pay = daily_pay;

      op.subject = subject;
      op.url = url;

      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );
      tx.operations.clear();
      tx.signatures.clear();

      const account_object& after_alice_account = db->get_account( creator );
      const account_object& after_bob_account = db->get_account( receiver );

      auto after_alice_sbd_balance = after_alice_account.sbd_balance;
      auto after_bob_sbd_balance = after_bob_account.sbd_balance;

      BOOST_REQUIRE( before_alice_sbd_balance == after_alice_sbd_balance + fee );
      BOOST_REQUIRE( before_bob_sbd_balance == after_bob_sbd_balance - fee );

      const auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_creator >();
      auto found = proposal_idx.find( creator );
      BOOST_REQUIRE( found != proposal_idx.end() );

      BOOST_REQUIRE( found->creator == creator );
      BOOST_REQUIRE( found->receiver == receiver );
      BOOST_REQUIRE( found->start_date == start_date );
      BOOST_REQUIRE( found->end_date == end_date );
      BOOST_REQUIRE( found->daily_pay == daily_pay );
      BOOST_REQUIRE( found->subject == subject );
      BOOST_REQUIRE( found->url == url );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( proposal_vote_object_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: proposal_vote_object_operation" );

      ACTORS( (alice)(bob)(carol)(dan) )
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      generate_block();

      auto creator = "alice";
      auto receiver = "bob";

      auto start_date = db->head_block_time() + fc::days( 1 );
      auto end_date = start_date + fc::days( 2 );

      auto daily_pay = asset( 100, SBD_SYMBOL );

      FUND( creator, ASSET( "80.000 TBD" ) );

      int64_t id_proposal_00 = create_proposal( creator, receiver, start_date, end_date, daily_pay, alice_private_key );

      signed_transaction tx;
      update_proposal_votes_operation op;
      const auto& proposal_vote_idx = db->get_index< proposal_vote_index >().indices().get< by_voter_proposal >();

      auto voter_01 = "carol";
      auto voter_01_key = carol_private_key;

      {
         BOOST_TEST_MESSAGE( "---Voting for proposal( `id_proposal_00` )---" );
         op.voter = voter_01;
         op.proposal_ids.insert( id_proposal_00 );
         op.approve = true;

         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         sign( tx, voter_01_key );
         db->push_transaction( tx, 0 );
         tx.operations.clear();
         tx.signatures.clear();

         auto found = proposal_vote_idx.find( std::make_tuple( voter_01, id_proposal_00 ) );
         BOOST_REQUIRE( found->voter == voter_01 );
         BOOST_REQUIRE( static_cast< int64_t >( found->proposal_id ) == id_proposal_00 );
      }

      {
         BOOST_TEST_MESSAGE( "---Unvoting proposal( `id_proposal_00` )---" );
         op.voter = voter_01;
         op.proposal_ids.clear();
         op.proposal_ids.insert( id_proposal_00 );
         op.approve = false;

         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         sign( tx, voter_01_key );
         db->push_transaction( tx, 0 );
         tx.operations.clear();
         tx.signatures.clear();

         auto found = proposal_vote_idx.find( std::make_tuple( voter_01, id_proposal_00 ) );
         BOOST_REQUIRE( found == proposal_vote_idx.end() );
      }

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( proposal_vote_object_01_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: proposal_vote_object_operation" );

      ACTORS( (alice)(bob)(carol)(dan) )
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      generate_block();

      auto creator = "alice";
      auto receiver = "bob";

      auto start_date = db->head_block_time() + fc::days( 1 );
      auto end_date = start_date + fc::days( 2 );

      auto daily_pay_00 = asset( 100, SBD_SYMBOL );
      auto daily_pay_01 = asset( 101, SBD_SYMBOL );
      auto daily_pay_02 = asset( 102, SBD_SYMBOL );

      FUND( creator, ASSET( "80.000 TBD" ) );

      int64_t id_proposal_00 = create_proposal( creator, receiver, start_date, end_date, daily_pay_00, alice_private_key );
      int64_t id_proposal_01 = create_proposal( creator, receiver, start_date, end_date, daily_pay_01, alice_private_key );

      signed_transaction tx;
      update_proposal_votes_operation op;
      const auto& proposal_vote_idx = db->get_index< proposal_vote_index >().indices().get< by_voter_proposal >();

      std::string voter_01 = "carol";
      auto voter_01_key = carol_private_key;

      {
         BOOST_TEST_MESSAGE( "---Voting by `voter_01` for proposals( `id_proposal_00`, `id_proposal_01` )---" );
         op.voter = voter_01;
         op.proposal_ids.insert( id_proposal_00 );
         op.proposal_ids.insert( id_proposal_01 );
         op.approve = true;

         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         sign( tx, voter_01_key );
         db->push_transaction( tx, 0 );
         tx.operations.clear();
         tx.signatures.clear();

         int32_t cnt = 0;
         auto found = proposal_vote_idx.find( voter_01 );
         while( found != proposal_vote_idx.end() && found->voter == voter_01 )
         {
            ++cnt;
            ++found;
         }
         BOOST_REQUIRE( cnt == 2 );
      }

      int64_t id_proposal_02 = create_proposal( creator, receiver, start_date, end_date, daily_pay_02, alice_private_key );
      std::string voter_02 = "dan";
      auto voter_02_key = dan_private_key;

      {
         BOOST_TEST_MESSAGE( "---Voting by `voter_02` for proposals( `id_proposal_00`, `id_proposal_01`, `id_proposal_02` )---" );
         op.voter = voter_02;
         op.proposal_ids.clear();
         op.proposal_ids.insert( id_proposal_02 );
         op.proposal_ids.insert( id_proposal_00 );
         op.proposal_ids.insert( id_proposal_01 );
         op.approve = true;

         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         sign( tx, voter_02_key );
         db->push_transaction( tx, 0 );
         tx.operations.clear();
         tx.signatures.clear();

         int32_t cnt = 0;
         auto found = proposal_vote_idx.find( voter_02 );
         while( found != proposal_vote_idx.end() && found->voter == voter_02 )
         {
            ++cnt;
            ++found;
         }
         BOOST_REQUIRE( cnt == 3 );
      }

      {
         BOOST_TEST_MESSAGE( "---Voting by `voter_02` for proposals( `id_proposal_00` )---" );
         op.voter = voter_02;
         op.proposal_ids.clear();
         op.proposal_ids.insert( id_proposal_00 );
         op.approve = true;

         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         sign( tx, voter_02_key );
         db->push_transaction( tx, 0 );
         tx.operations.clear();
         tx.signatures.clear();

         int32_t cnt = 0;
         auto found = proposal_vote_idx.find( voter_02 );
         while( found != proposal_vote_idx.end() && found->voter == voter_02 )
         {
            ++cnt;
            ++found;
         }
        BOOST_REQUIRE( cnt == 3 );
      }

      {
         BOOST_TEST_MESSAGE( "---Unvoting by `voter_01` proposals( `id_proposal_02` )---" );
         op.voter = voter_01;
         op.proposal_ids.clear();
         op.proposal_ids.insert( id_proposal_02 );
         op.approve = false;

         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         sign( tx, voter_01_key );
         db->push_transaction( tx, 0 );
         tx.operations.clear();
         tx.signatures.clear();

         int32_t cnt = 0;
         auto found = proposal_vote_idx.find( voter_01 );
         while( found != proposal_vote_idx.end() && found->voter == voter_01 )
         {
            ++cnt;
            ++found;
         }
         BOOST_REQUIRE( cnt == 2 );
      }

      {
         BOOST_TEST_MESSAGE( "---Unvoting by `voter_01` proposals( `id_proposal_00` )---" );
         op.voter = voter_01;
         op.proposal_ids.clear();
         op.proposal_ids.insert( id_proposal_00 );
         op.approve = false;

         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         sign( tx, voter_01_key );
         db->push_transaction( tx, 0 );
         tx.operations.clear();
         tx.signatures.clear();

         int32_t cnt = 0;
         auto found = proposal_vote_idx.find( voter_01 );
         while( found != proposal_vote_idx.end() && found->voter == voter_01 )
         {
            ++cnt;
            ++found;
         }
         BOOST_REQUIRE( cnt == 1 );
      }

      {
         BOOST_TEST_MESSAGE( "---Unvoting by `voter_02` proposals( `id_proposal_00`, `id_proposal_01`, `id_proposal_02` )---" );
         op.voter = voter_02;
         op.proposal_ids.clear();
         op.proposal_ids.insert( id_proposal_02 );
         op.proposal_ids.insert( id_proposal_01 );
         op.proposal_ids.insert( id_proposal_00 );
         op.approve = false;

         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         sign( tx, voter_02_key );
         db->push_transaction( tx, 0 );
         tx.operations.clear();
         tx.signatures.clear();

         int32_t cnt = 0;
         auto found = proposal_vote_idx.find( voter_02 );
         while( found != proposal_vote_idx.end() && found->voter == voter_02 )
         {
            ++cnt;
            ++found;
         }
         BOOST_REQUIRE( cnt == 0 );
      }

      {
         BOOST_TEST_MESSAGE( "---Unvoting by `voter_01` proposals( `id_proposal_01` )---" );
         op.voter = voter_01;
         op.proposal_ids.clear();
         op.proposal_ids.insert( id_proposal_01 );
         op.approve = false;

         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         sign( tx, voter_01_key );
         db->push_transaction( tx, 0 );
         tx.operations.clear();
         tx.signatures.clear();

         int32_t cnt = 0;
         auto found = proposal_vote_idx.find( voter_01 );
         while( found != proposal_vote_idx.end() && found->voter == voter_01 )
         {
            ++cnt;
            ++found;
         }
         BOOST_REQUIRE( cnt == 0 );
      }

      {
         BOOST_TEST_MESSAGE( "---Voting by `voter_01` for nothing---" );
         op.voter = voter_01;
         op.proposal_ids.clear();
         op.approve = true;

         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         sign( tx, voter_01_key );
         db->push_transaction( tx, 0 );
         tx.operations.clear();
         tx.signatures.clear();

         int32_t cnt = 0;
         auto found = proposal_vote_idx.find( voter_01 );
         while( found != proposal_vote_idx.end() && found->voter == voter_01 )
         {
            ++cnt;
            ++found;
         }
         BOOST_REQUIRE( cnt == 0 );
      }

      {
         BOOST_TEST_MESSAGE( "---Unvoting by `voter_01` nothing---" );
         op.voter = voter_01;
         op.proposal_ids.clear();
         op.approve = false;

         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         sign( tx, voter_01_key );
         db->push_transaction( tx, 0 );
         tx.operations.clear();
         tx.signatures.clear();

         int32_t cnt = 0;
         auto found = proposal_vote_idx.find( voter_01 );
         while( found != proposal_vote_idx.end() && found->voter == voter_01 )
         {
            ++cnt;
            ++found;
         }
         BOOST_REQUIRE( cnt == 0 );
      }

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
