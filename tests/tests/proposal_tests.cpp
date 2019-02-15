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

template< typename SIGN >
int64_t create_proposal(   std::string creator, std::string receiver,
                           time_point_sec start_date, time_point_sec end_date,
                           asset daily_pay, database* db, SIGN sign )
{
   signed_transaction tx;

   create_proposal_operation op;

   op.creator = creator;
   op.receiver = receiver;

   op.start_date = start_date;
   op.end_date = end_date;

   op.daily_pay = daily_pay;

   static uint32_t cnt = 0;
   op.subject = std::to_string( cnt );
   op.url = "http://" + std::to_string( cnt );

   tx.operations.push_back( op );
   tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
   sign( tx );
   db->push_transaction( tx, 0 );

   const auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_creator >();
   auto found = proposal_idx.find( creator );
   BOOST_REQUIRE( found != proposal_idx.end() );

   ++cnt;

   //It is necessary to find newly created object
   while( found != proposal_idx.end() && found->creator == creator )
   {
      uint32_t val = stoi( found->subject.c_str() );

      if( val == cnt - 1 )
         return found->id;

      ++found;
   }

   return -1;
}

template< typename SIGN >
void vote_proposal( std::string voter, const std::vector< int64_t >& id_proposals, bool approve, database* db, SIGN sign )
{
   update_proposal_votes_operation op;

   op.voter = voter;
   op.proposal_ids = id_proposals;
   op.approve = approve;

   signed_transaction tx;
   tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
   tx.operations.push_back( op );
   sign( tx );
   db->push_transaction( tx, 0 );
}

template< typename SIGN >
void transfer_vests( std::string from, std::string to, asset amount, database* db, SIGN sign )
{
   transfer_to_vesting_operation op;
   op.from = from;
   op.to = to;
   op.amount = amount;

   signed_transaction tx;
   tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
   tx.operations.push_back( op );
   sign( tx );
   db->push_transaction( tx, 0 );
}

BOOST_FIXTURE_TEST_SUITE( proposal_tests, clean_database_fixture )

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

      auto alice_sign = [&]( signed_transaction& tx ){ sign( tx, alice_private_key ); };

      int64_t id_proposal_00 = create_proposal( creator, receiver, start_date, end_date, daily_pay, db, alice_sign );

      signed_transaction tx;
      update_proposal_votes_operation op;
      const auto& proposal_vote_idx = db->get_index< proposal_vote_index >().indices().get< by_voter_proposal >();

      auto voter_01 = "carol";
      auto voter_01_key = carol_private_key;

      {
         BOOST_TEST_MESSAGE( "---Voting for proposal( `id_proposal_00` )---" );
         op.voter = voter_01;
         op.proposal_ids.push_back( id_proposal_00 );
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
         op.proposal_ids.push_back( id_proposal_00 );
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

      auto alice_sign = [&]( signed_transaction& tx ){ sign( tx, alice_private_key ); };

      int64_t id_proposal_00 = create_proposal( creator, receiver, start_date, end_date, daily_pay_00, db, alice_sign );
      int64_t id_proposal_01 = create_proposal( creator, receiver, start_date, end_date, daily_pay_01, db, alice_sign );

      signed_transaction tx;
      update_proposal_votes_operation op;
      const auto& proposal_vote_idx = db->get_index< proposal_vote_index >().indices().get< by_voter_proposal >();

      std::string voter_01 = "carol";
      auto voter_01_key = carol_private_key;

      {
         BOOST_TEST_MESSAGE( "---Voting by `voter_01` for proposals( `id_proposal_00`, `id_proposal_01` )---" );
         op.voter = voter_01;
         op.proposal_ids.push_back( id_proposal_00 );
         op.proposal_ids.push_back( id_proposal_01 );
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

      int64_t id_proposal_02 = create_proposal( creator, receiver, start_date, end_date, daily_pay_02, db, alice_sign );
      std::string voter_02 = "dan";
      auto voter_02_key = dan_private_key;

      {
         BOOST_TEST_MESSAGE( "---Voting by `voter_02` for proposals( `id_proposal_00`, `id_proposal_01`, `id_proposal_02` )---" );
         op.voter = voter_02;
         op.proposal_ids.clear();
         op.proposal_ids.push_back( id_proposal_02 );
         op.proposal_ids.push_back( id_proposal_00 );
         op.proposal_ids.push_back( id_proposal_01 );
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
         op.proposal_ids.push_back( id_proposal_00 );
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
         op.proposal_ids.push_back( id_proposal_02 );
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
         op.proposal_ids.push_back( id_proposal_00 );
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
         op.proposal_ids.push_back( id_proposal_02 );
         op.proposal_ids.push_back( id_proposal_01 );
         op.proposal_ids.push_back( id_proposal_00 );
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
         op.proposal_ids.push_back( id_proposal_01 );
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

BOOST_AUTO_TEST_CASE( generating_payments )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: generating payments" );

      ACTORS( (alice)(bob)(carol) )
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      generate_block();

      //=====================preparing=====================
      auto creator = "alice";
      auto receiver = "bob";

      auto start_date = db->head_block_time() + fc::days( 1 );
      auto end_date = start_date + fc::days( 2 );

      auto daily_pay = asset( 48, SBD_SYMBOL );
      auto hourly_pay = asset( daily_pay.amount.value / 24, SBD_SYMBOL );

      FUND( creator, ASSET( "160.000 TESTS" ) );
      FUND( creator, ASSET( "80.000 TBD" ) );

      auto voter_01 = "carol";
      auto voter_vests_amount_01 = ASSET( "1.000 TESTS" );

      auto creator_sign = [&]( signed_transaction& tx ){ sign( tx, alice_private_key ); };
      auto voter_01_sign = [&]( signed_transaction& tx ){ sign( tx, carol_private_key ); };
      //=====================preparing=====================

      //Needed basic operations
      int64_t id_proposal_00 = create_proposal( creator, receiver, start_date, end_date, daily_pay, db, creator_sign );
      vote_proposal( voter_01, { id_proposal_00 }, true/*approve*/, db, voter_01_sign );
      transfer_vests( creator, voter_01, voter_vests_amount_01, db, creator_sign );

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
      
         generate_blocks( db->head_block_time() + fc::seconds( STEEM_PROPOSAL_MAINTENANCE_PERIOD - 6 ) );
         generate_blocks( db->head_block_time() + fc::seconds( 6 ), false );

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

BOOST_AUTO_TEST_SUITE_END()
