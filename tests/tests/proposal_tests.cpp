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

#include <steem/chain/sps_objects.hpp>
#include <steem/chain/sps_objects/sps_processor.hpp>

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

template< typename PROPOSAL_IDX >
int64_t calc_proposals( const PROPOSAL_IDX& proposal_idx, const std::vector< int64_t >& proposals_id )
{
   auto cnt = 0;
   for( auto id : proposals_id )
      cnt += proposal_idx.find( id ) != proposal_idx.end() ? 1 : 0;
   return cnt;
}

template< typename PROPOSAL_VOTE_IDX >
int64_t calc_proposal_votes( const PROPOSAL_VOTE_IDX& proposal_vote_idx, uint64_t proposal_id )
{
   auto cnt = 0;
   auto found = proposal_vote_idx.find( proposal_id );
   while( found != proposal_vote_idx.end() && static_cast< size_t >( found->proposal_id ) == proposal_id )
   {
      ++cnt;
      ++found;
   }
   return cnt;
}

template< typename PROPOSAL_VOTE_IDX >
int64_t calc_votes( const PROPOSAL_VOTE_IDX& proposal_vote_idx, const std::vector< int64_t >& proposals_id )
{
   auto cnt = 0;
   for( auto id : proposals_id )
      cnt += calc_proposal_votes( proposal_vote_idx, id );
   return cnt;
}

BOOST_FIXTURE_TEST_SUITE( proposal_tests, sps_proposal_database_fixture )

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

      auto start_date = db->head_block_time();
      auto end_date = start_date + fc::days( 2 );

      auto daily_pay = ASSET( "48.000 TBD" );
      auto hourly_pay = ASSET( "1.996 TBD" );// hourly_pay != ASSET( "2.000 TBD" ) because lack of rounding

      FUND( creator, ASSET( "160.000 TESTS" ) );
      FUND( creator, ASSET( "80.000 TBD" ) );
      FUND( STEEM_TREASURY_ACCOUNT, ASSET( "5000.000 TBD" ) );

      auto voter_01 = "carol";
      //=====================preparing=====================

      //Needed basic operations
      int64_t id_proposal_00 = create_proposal( creator, receiver, start_date, end_date, daily_pay, alice_private_key );
      generate_blocks( 1 );

      vote_proposal( voter_01, { id_proposal_00 }, true/*approve*/, carol_private_key );
      generate_blocks( 1 );

      vest(STEEM_INIT_MINER_NAME, voter_01, ASSET( "1.000 TESTS" ));
      generate_blocks( 1 );

      //skipping interest generating is necessary
      transfer( STEEM_INIT_MINER_NAME, receiver, ASSET( "0.001 TBD" ));
      generate_block( 5 );
      transfer( STEEM_INIT_MINER_NAME, STEEM_TREASURY_ACCOUNT, ASSET( "0.001 TBD" ) );
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

BOOST_AUTO_TEST_CASE( generating_payments_01 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: generating payments" );

      ACTORS( (tester001)(tester002)(tester003)(tester004)(tester005) )
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      generate_block();

      //=====================preparing=====================
      const auto nr_proposals = 5;
      std::vector< int64_t > proposals_id;
      flat_map< std::string, asset > before_tbds;

      struct initial_data
      {
         std::string account;
         fc::ecc::private_key key;
      };

      std::vector< initial_data > inits = {
                                       {"tester001", tester001_private_key },
                                       {"tester002", tester002_private_key },
                                       {"tester003", tester003_private_key },
                                       {"tester004", tester004_private_key },
                                       {"tester005", tester005_private_key },
                                       };

      for( auto item : inits )
      {
         FUND( item.account, ASSET( "400.000 TESTS" ) );
         FUND( item.account, ASSET( "400.000 TBD" ) );
         vest(STEEM_INIT_MINER_NAME, item.account, ASSET( "300.000 TESTS" ));
      }

      auto start_date = db->head_block_time();
      const auto end_time_shift = fc::hours( 5 );
      auto end_date = start_date + end_time_shift;

      auto daily_pay = ASSET( "24.000 TBD" );
      auto paid = ASSET( "4.990 TBD" );// paid != ASSET( "5.000 TBD" ) because lack of rounding

      FUND( STEEM_TREASURY_ACCOUNT, ASSET( "5000000.000 TBD" ) );
      //=====================preparing=====================
      for( int32_t i = 0; i < nr_proposals; ++i )
      {
         auto item = inits[ i % inits.size() ];
         proposals_id.push_back( create_proposal( item.account, item.account, start_date, end_date, daily_pay, item.key ) );
         generate_block();
      }

      for( int32_t i = 0; i < nr_proposals; ++i )
      {
         auto item = inits[ i % inits.size() ];
         vote_proposal( item.account, proposals_id, true/*approve*/, item.key );
         generate_block();
      }

      for( auto item : inits )
      {
         const account_object& account = db->get_account( item.account );
         before_tbds[ item.account ] = account.sbd_balance;
      }

      generate_blocks( start_date + end_time_shift + fc::seconds( 10 ), false );

      for( auto item : inits )
      {
         const account_object& account = db->get_account( item.account );
         auto after_tbd = account.sbd_balance;
         auto before_tbd = before_tbds[ item.account ];
         BOOST_REQUIRE( before_tbd == after_tbd - paid );
      }

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( generating_payments_02 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: generating payments, but proposal are removed" );

      ACTORS(
               (a00)(a01)(a02)(a03)(a04)(a05)(a06)(a07)(a08)(a09)
               (a10)(a11)(a12)(a13)(a14)(a15)(a16)(a17)(a18)(a19)
               (a20)(a21)(a22)(a23)(a24)(a25)(a26)(a27)(a28)(a29)
               (a30)(a31)(a32)(a33)(a34)(a35)(a36)(a37)(a38)(a39)
               (a40)(a41)(a42)(a43)(a44)(a45)(a46)(a47)(a48)(a49)
            )
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      generate_block();

      //=====================preparing=====================
      flat_map< std::string, asset > before_tbds;

      struct initial_data
      {
         std::string account;
         fc::ecc::private_key key;
      };

      std::vector< initial_data > inits = {
         {"a00", a00_private_key }, {"a01", a01_private_key }, {"a02", a02_private_key }, {"a03", a03_private_key }, {"a04", a04_private_key },
         {"a05", a05_private_key }, {"a06", a06_private_key }, {"a07", a07_private_key }, {"a08", a08_private_key }, {"a09", a09_private_key },
         {"a10", a10_private_key }, {"a11", a11_private_key }, {"a12", a12_private_key }, {"a13", a13_private_key }, {"a14", a14_private_key },
         {"a15", a15_private_key }, {"a16", a16_private_key }, {"a17", a17_private_key }, {"a18", a18_private_key }, {"a19", a19_private_key },
         {"a20", a20_private_key }, {"a21", a21_private_key }, {"a22", a22_private_key }, {"a23", a23_private_key }, {"a24", a24_private_key },
         {"a25", a25_private_key }, {"a26", a26_private_key }, {"a27", a27_private_key }, {"a28", a28_private_key }, {"a29", a29_private_key },
         {"a30", a30_private_key }, {"a31", a31_private_key }, {"a32", a32_private_key }, {"a33", a33_private_key }, {"a34", a34_private_key },
         {"a35", a35_private_key }, {"a36", a36_private_key }, {"a37", a37_private_key }, {"a38", a38_private_key }, {"a39", a39_private_key },
         {"a40", a40_private_key }, {"a41", a41_private_key }, {"a42", a42_private_key }, {"a43", a43_private_key }, {"a44", a44_private_key },
         {"a45", a45_private_key }, {"a46", a46_private_key }, {"a47", a47_private_key }, {"a48", a48_private_key }, {"a49", a49_private_key }
      };

      for( auto item : inits )
      {
         FUND( item.account, ASSET( "400.000 TESTS" ) );
         FUND( item.account, ASSET( "400.000 TBD" ) );
         vest(STEEM_INIT_MINER_NAME, item.account, ASSET( "300.000 TESTS" ));
      }

      const auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_proposal_id >();
      const auto& proposal_vote_idx = db->get_index< proposal_vote_index >().indices().get< by_proposal_voter >();

      auto start_date = db->head_block_time() + fc::hours( 2 );
      auto end_date = start_date + fc::days( 15 );

      const auto block_interval = fc::seconds( STEEM_BLOCK_INTERVAL );

      FUND( STEEM_TREASURY_ACCOUNT, ASSET( "5000000.000 TBD" ) );
      //=====================preparing=====================
      auto item_creator = inits[ 0 ];
      create_proposal( item_creator.account, item_creator.account, start_date, end_date, ASSET( "24.000 TBD" ), item_creator.key );
      generate_block();

      for( auto item : inits )
      {
         vote_proposal( item.account, {0}, true/*approve*/, item.key);

         generate_block();

         const account_object& account = db->get_account( item.account );
         before_tbds[ item.account ] = account.sbd_balance;
      }

      generate_blocks( start_date, false );
      generate_blocks( db->get_dynamic_global_properties().next_maintenance_time - block_interval, false );

      {
         remove_proposal( item_creator.account, {0}, item_creator.key );
         auto found_proposals = calc_proposals( proposal_idx, {0} );
         auto found_votes = calc_proposal_votes( proposal_vote_idx, 0 );
         BOOST_REQUIRE( found_proposals == 1 );
         BOOST_REQUIRE( found_votes == 30 );
      }

      {
         generate_block();
         auto found_proposals = calc_proposals( proposal_idx, {0} );
         auto found_votes = calc_proposal_votes( proposal_vote_idx, 0 );
         BOOST_REQUIRE( found_proposals == 1 );
         BOOST_REQUIRE( found_votes == 10 );
      }

      for( auto item : inits )
      {
         const account_object& account = db->get_account( item.account );
         auto after_tbd = account.sbd_balance;
         auto before_tbd = before_tbds[ item.account ];
         BOOST_REQUIRE( before_tbd == after_tbd );
      }

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( generating_payments_03 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: generating payments" );

      std::string tester00_account = "tester00";
      std::string tester01_account = "tester01";
      std::string tester02_account = "tester02";

      ACTORS( (tester00)(tester01)(tester02) )
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      generate_block();

      //=====================preparing=====================
      std::vector< int64_t > proposals_id;
      flat_map< std::string, asset > before_tbds;

      flat_map< std::string, fc::ecc::private_key > inits;
      inits[ "tester00" ] = tester00_private_key;
      inits[ "tester01" ] = tester01_private_key;
      inits[ "tester02" ] = tester02_private_key;

      for( auto item : inits )
      {
         if( item.first == tester02_account )
         {
            FUND( item.first, ASSET( "41.000 TESTS" ) );
            FUND( item.first, ASSET( "41.000 TBD" ) );
            vest(STEEM_INIT_MINER_NAME, item.first, ASSET( "31.000 TESTS" ));
         }
         else
         {
            FUND( item.first, ASSET( "40.000 TESTS" ) );
            FUND( item.first, ASSET( "40.000 TBD" ) );
            vest(STEEM_INIT_MINER_NAME, item.first, ASSET( "30.000 TESTS" ));
         }
      }

      auto start_date = db->head_block_time();
      uint16_t interval = 0;
      std::vector< fc::microseconds > end_time_shift = { fc::hours( 1 ), fc::hours( 2 ), fc::hours( 3 ), fc::hours( 4 ) };
      auto end_date = start_date + end_time_shift[ 3 ];

      auto huge_daily_pay = ASSET( "50000001.000 TBD" );
      auto daily_pay = ASSET( "24.000 TBD" );

      FUND( STEEM_TREASURY_ACCOUNT, ASSET( "5000000.000 TBD" ) );
      //=====================preparing=====================
      uint16_t i = 0;
      for( auto item : inits )
      {
         auto _pay = ( item.first == tester02_account ) ? huge_daily_pay : daily_pay;
         proposals_id.push_back( create_proposal( item.first, item.first, start_date, end_date, _pay, item.second ) );
         generate_block();

         if( item.first == tester02_account )
            continue;

         vote_proposal( item.first, {i++}, true/*approve*/, item.second );
         generate_block();
      }

      for( auto item : inits )
      {
         const account_object& account = db->get_account( item.first );
         before_tbds[ item.first ] = account.sbd_balance;
      }

      auto payment_checker = [&]( const std::vector< asset >& payouts )
      {
         uint16_t i = 0;
         for( const auto& item : inits )
         {
            const account_object& account = db->get_account( item.first );
            auto after_tbd = account.sbd_balance;
            auto before_tbd = before_tbds[ item.first ];
            BOOST_REQUIRE( before_tbd == after_tbd - payouts[i++] );
         }
      };

      /*
         Initial conditions.
            `tester00` has own proposal id = 0 : voted for it
            `tester01` has own proposal id = 1 : voted for it
            `tester02` has own proposal id = 2 : lack of votes
      */

      generate_blocks( start_date + end_time_shift[ interval++ ] + fc::seconds( 10 ), false );
      /*
         `tester00` - got payout
         `tester01` - got payout
         `tester02` - no payout, because of lack of votes
      */
      payment_checker( { ASSET( "0.998 TBD" ), ASSET( "0.998 TBD" ), ASSET( "0.000 TBD" ) } );
      //ideally: ASSET( "1.000 TBD" ), ASSET( "1.000 TBD" ), ASSET( "0.000 TBD" ) but there is lack of rounding

      {
         BOOST_TEST_MESSAGE( "Setting proxy. The account `tester01` don't want to vote. Every decision is made by account `tester00`" );
         account_witness_proxy_operation op;
         op.account = tester01_account;
         op.proxy = tester00_account;

         signed_transaction tx;
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         tx.operations.push_back( op );
         sign( tx, inits[ tester01_account ] );
         db->push_transaction( tx, 0 );
      }

      generate_blocks( start_date + end_time_shift[ interval++ ] + fc::seconds( 10 ), false );
      /*
         `tester00` - got payout
         `tester01` - no payout, because this account set proxy
         `tester02` - no payout, because of lack of votes
      */
      payment_checker( { ASSET( "1.996 TBD" ), ASSET( "0.998 TBD" ), ASSET( "0.000 TBD" ) } );
      //ideally: ASSET( "2.000 TBD" ), ASSET( "1.000 TBD" ), ASSET( "0.000 TBD" ) but there is lack of rounding

      vote_proposal( tester02_account, {2}, true/*approve*/, inits[ tester02_account ] );
      generate_block();

      generate_blocks( start_date + end_time_shift[ interval++ ] + fc::seconds( 10 ), false );
      /*
         `tester00` - got payout, `tester02` has less votes than `tester00`
         `tester01` - no payout, because this account set proxy
         `tester02` - got payout, because voted for his proposal
      */
      payment_checker( { ASSET( "2.994 TBD" ), ASSET( "0.998 TBD" ), ASSET( "2079.015 TBD" ) } );
      //ideally: ASSET( "3.000 TBD" ), ASSET( "1.000 TBD" ), ASSET( "2082.346 TBD" ) but there is lack of rounding

      {
         BOOST_TEST_MESSAGE( "Proxy doesn't exist. Now proposal with id = 3 has the most votes. This proposal grabs all payouts." );
         account_witness_proxy_operation op;
         op.account = tester01_account;
         op.proxy = "";

         signed_transaction tx;
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         tx.operations.push_back( op );
         sign( tx, inits[ tester01_account ] );
         db->push_transaction( tx, 0 );
      }

      generate_blocks( start_date + end_time_shift[ interval++ ] + fc::seconds( 10 ), false );
      /*
         `tester00` - no payout, because is not enough money. `tester01` removed proxy and the most votes has `tester02`. Whole payout goes to `tester02`
         `tester01` - no payout, because is not enough money
         `tester02` - got payout, because voted for his proposal
      */
      payment_checker( { ASSET( "2.994 TBD" ), ASSET( "0.998 TBD" ), ASSET( "4158.162 TBD" ) } );
      //ideally: ASSET( "3.000 TBD" ), ASSET( "1.000 TBD" ), ASSET( "4164.824 TBD" ) but there is lack of rounding

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( proposals_maintenance)
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: removing inactive proposals" );

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
      auto permlink = "somethingpermlink";

      post_comment(creator, permlink, "title", "body", "test", alice_private_key);

      FUND( creator, ASSET( "80.000 TBD" ) );

      signed_transaction tx;

      const account_object& before_treasury_account = db->get_account(STEEM_TREASURY_ACCOUNT);
      const account_object& before_alice_account = db->get_account( creator );
      const account_object& before_bob_account = db->get_account( receiver );

      auto before_alice_sbd_balance = before_alice_account.sbd_balance;
      auto before_bob_sbd_balance = before_bob_account.sbd_balance;
      auto before_treasury_balance = before_treasury_account.sbd_balance;

      create_proposal_operation op;

      op.creator = creator;
      op.receiver = receiver;

      op.start_date = start_date;
      op.end_date = end_date;

      op.daily_pay = daily_pay;

      op.subject = subject;
      op.permlink = permlink;

      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );
      tx.operations.clear();
      tx.signatures.clear();

      const auto& after_treasury_account = db->get_account(STEEM_TREASURY_ACCOUNT);
      const account_object& after_alice_account = db->get_account( creator );
      const account_object& after_bob_account = db->get_account( receiver );

      auto after_alice_sbd_balance = after_alice_account.sbd_balance;
      auto after_bob_sbd_balance = after_bob_account.sbd_balance;
      auto after_treasury_balance = after_treasury_account.sbd_balance;

      BOOST_REQUIRE( before_alice_sbd_balance == after_alice_sbd_balance + fee );
      BOOST_REQUIRE( before_bob_sbd_balance == after_bob_sbd_balance );
      /// Fee shall be paid to treasury account.
      BOOST_REQUIRE(before_treasury_balance == after_treasury_balance - fee);

      const auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_creator >();
      auto found = proposal_idx.find( creator );
      BOOST_REQUIRE( found != proposal_idx.end() );

      BOOST_REQUIRE( found->creator == creator );
      BOOST_REQUIRE( found->receiver == receiver );
      BOOST_REQUIRE( found->start_date == start_date );
      BOOST_REQUIRE( found->end_date == end_date );
      BOOST_REQUIRE( found->daily_pay == daily_pay );
      BOOST_REQUIRE( found->subject == subject );
      BOOST_REQUIRE( found->permlink == permlink );

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

         auto found = proposal_vote_idx.find( boost::make_tuple( voter_01, id_proposal_00 ) );
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

         auto found = proposal_vote_idx.find( boost::make_tuple( voter_01, id_proposal_00 ) );
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
         STEEM_REQUIRE_THROW(db->push_transaction( tx, 0 ), fc::exception);
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
         STEEM_REQUIRE_THROW(db->push_transaction( tx, 0 ), fc::exception);
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


struct create_proposal_data {
      std::string creator    ;
      std::string receiver   ;
      fc::time_point_sec start_date ;
      fc::time_point_sec end_date   ;
      steem::protocol::asset daily_pay ;
      std::string subject ;   
      std::string url     ;   

      create_proposal_data(fc::time_point_sec _start) {
         creator    = "alice";
         receiver   = "bob";
         start_date = _start     + fc::days( 1 );
         end_date   = start_date + fc::days( 2 );
         daily_pay  = asset( 100, SBD_SYMBOL );
         subject    = "hello";
         url        = "http:://something.html";
      }
};

BOOST_AUTO_TEST_CASE( create_proposal_000 )
{
   try {
      BOOST_TEST_MESSAGE( "Testing: create proposal: opration arguments validation - all args are ok" );
      ACTORS( (alice)(bob) )
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      generate_block();

      auto creator    = "alice";
      auto receiver   = "bob";
      auto start_date = db->head_block_time() + fc::days( 1 );
      auto end_date   = start_date + fc::days( 2 );
      auto daily_pay  = asset( 100, SBD_SYMBOL );

      FUND( creator, ASSET( "80.000 TBD" ) );
      {
         int64_t proposal = create_proposal( creator, receiver, start_date, end_date, daily_pay, alice_private_key );
         BOOST_REQUIRE( proposal >= 0 );
      }
      validate_database();
   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( create_proposal_001 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: create proposal: opration arguments validation - invalid creator" );
      {
         create_proposal_data cpd(db->head_block_time());
         ACTORS( (alice)(bob) )
         generate_block();
         FUND( cpd.creator, ASSET( "80.000 TBD" ) );
         STEEM_REQUIRE_THROW( create_proposal( "", cpd.receiver, cpd.start_date, cpd.end_date, cpd.daily_pay, alice_private_key ), fc::exception);
         
      }
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( create_proposal_002 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: create proposal: opration arguments validation - invalid receiver" );
      create_proposal_data cpd(db->head_block_time());
      ACTORS( (alice)(bob) )
      generate_block();
      FUND( cpd.creator, ASSET( "80.000 TBD" ) );
      STEEM_REQUIRE_THROW(create_proposal( cpd.creator, "", cpd.start_date, cpd.end_date, cpd.daily_pay, alice_private_key ), fc::exception);
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( create_proposal_003 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: create proposal: opration arguments validation - invalid start date" );
      create_proposal_data cpd(db->head_block_time());
      ACTORS( (alice)(bob) )
      generate_block();
      FUND( cpd.creator, ASSET( "80.000 TBD" ) );
      cpd.start_date = cpd.end_date + fc::days(2);
      STEEM_REQUIRE_THROW(create_proposal( cpd.creator, cpd.receiver, cpd.start_date, cpd.end_date, cpd.daily_pay, alice_private_key ), fc::exception);
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( create_proposal_004 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: create proposal: opration arguments validation - invalid end date" );
      create_proposal_data cpd(db->head_block_time());
      ACTORS( (alice)(bob) )
      generate_block();
      FUND( cpd.creator, ASSET( "80.000 TBD" ) );
      cpd.end_date = cpd.start_date - fc::days(2);
      STEEM_REQUIRE_THROW(create_proposal( cpd.creator, cpd.receiver, cpd.start_date, cpd.end_date, cpd.daily_pay, alice_private_key ), fc::exception);
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( create_proposal_005 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: create proposal: opration arguments validation - invalid subject(empty)" );
      ACTORS( (alice)(bob) )
      generate_block();
      create_proposal_operation cpo;
      cpo.creator    = "alice";
      cpo.receiver   = "bob";
      cpo.start_date = db->head_block_time() + fc::days( 1 );
      cpo.end_date   = cpo.start_date + fc::days( 2 );
      cpo.daily_pay  = asset( 100, SBD_SYMBOL );
      cpo.subject    = "";
      cpo.permlink        = "http:://something.html";
      FUND( cpo.creator, ASSET( "80.000 TBD" ) );
      generate_block();
      signed_transaction tx;
      tx.operations.push_back( cpo );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, alice_private_key );
      STEEM_REQUIRE_THROW(db->push_transaction( tx, 0 ), fc::exception);
      tx.operations.clear();
      tx.signatures.clear();
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( create_proposal_006 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: create proposal: opration arguments validation - invalid subject(too long)" );
      ACTORS( (alice)(bob) )
      generate_block();
      create_proposal_operation cpo;
      cpo.creator    = "alice";
      cpo.receiver   = "bob";
      cpo.start_date = db->head_block_time() + fc::days( 1 );
      cpo.end_date   = cpo.start_date + fc::days( 2 );
      cpo.daily_pay  = asset( 100, SBD_SYMBOL );
      cpo.subject    = "very very very very very very long long long long long long subject subject subject subject subject subject";
      cpo.permlink        = "http:://something.html";
      FUND( cpo.creator, ASSET( "80.000 TBD" ) );
      generate_block();
      signed_transaction tx;
      tx.operations.push_back( cpo );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, alice_private_key );
      STEEM_REQUIRE_THROW(db->push_transaction( tx, 0 ), fc::exception);
      tx.operations.clear();
      tx.signatures.clear();
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( create_proposal_007 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: create proposal: authorization test" );
      ACTORS( (alice)(bob) )
      generate_block();
      create_proposal_operation cpo;
      cpo.creator    = "alice";
      cpo.receiver   = "bob";
      cpo.start_date = db->head_block_time() + fc::days( 1 );
      cpo.end_date   = cpo.start_date + fc::days( 2 );
      cpo.daily_pay  = asset( 100, SBD_SYMBOL );
      cpo.subject    = "subject";
      cpo.permlink        = "http:://something.html";

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      cpo.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      cpo.get_required_posting_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      expected.insert( "alice" );
      cpo.get_required_active_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( create_proposal_008 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: create proposal: opration arguments validation - invalid daily payement (negative value)" );
      create_proposal_data cpd(db->head_block_time());
      ACTORS( (alice)(bob) )
      generate_block();
      FUND( cpd.creator, ASSET( "80.000 TBD" ) );
      generate_block();
      generate_block();
      cpd.end_date = cpd.start_date + fc::days(20);
      cpd.daily_pay = asset( -10, SBD_SYMBOL );
      STEEM_REQUIRE_THROW(create_proposal( cpd.creator, cpd.receiver, cpd.start_date, cpd.end_date, cpd.daily_pay, alice_private_key ), fc::exception);
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( update_proposal_votes_000 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: update proposal votes: opration arguments validation - all ok (approve true)" );
      create_proposal_data cpd(db->head_block_time());
      ACTORS( (alice)(bob)(carol) )
      generate_block();
      FUND( cpd.creator, ASSET( "80.000 TBD" ) );
      generate_block();

      int64_t proposal_1 = create_proposal( cpd.creator, cpd.receiver, cpd.start_date, cpd.end_date, cpd.daily_pay, alice_private_key );
      BOOST_REQUIRE(proposal_1 >= 0);
      std::vector< int64_t > proposals = {proposal_1};
      vote_proposal("carol", proposals, true, carol_private_key);
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( update_proposal_votes_001 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: update proposal votes: opration arguments validation - all ok (approve false)" );
      create_proposal_data cpd(db->head_block_time());
      ACTORS( (alice)(bob)(carol) )
      generate_block();
      FUND( cpd.creator, ASSET( "80.000 TBD" ) );
      generate_block();

      int64_t proposal_1 = create_proposal( cpd.creator, cpd.receiver, cpd.start_date, cpd.end_date, cpd.daily_pay, alice_private_key );
      BOOST_REQUIRE(proposal_1 >= 0);
      std::vector< int64_t > proposals = {proposal_1};
      vote_proposal("carol", proposals, false, carol_private_key);
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( update_proposal_votes_002 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: update proposal votes: opration arguments validation - all ok (empty array)" );
      create_proposal_data cpd(db->head_block_time());
      ACTORS( (alice)(bob)(carol) )
      generate_block();
      FUND( cpd.creator, ASSET( "80.000 TBD" ) );
      generate_block();

      int64_t proposal_1 = create_proposal( cpd.creator, cpd.receiver, cpd.start_date, cpd.end_date, cpd.daily_pay, alice_private_key );
      BOOST_REQUIRE(proposal_1 >= 0);
      std::vector< int64_t > proposals;
      STEEM_REQUIRE_THROW( vote_proposal("carol", proposals, true, carol_private_key), fc::exception);
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( update_proposal_votes_003 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: update proposal votes: opration arguments validation - all ok (array with negative digits)" );
      create_proposal_data cpd(db->head_block_time());
      ACTORS( (alice)(bob)(carol) )
      generate_block();
      FUND( cpd.creator, ASSET( "80.000 TBD" ) );
      generate_block();

      std::vector< int64_t > proposals = {-1, -2, -3, -4, -5};
      vote_proposal("carol", proposals, true, carol_private_key);
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( update_proposal_votes_004 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: update proposal votes: opration arguments validation - invalid voter" );
      create_proposal_data cpd(db->head_block_time());
      ACTORS( (alice)(bob)(carol) )
      generate_block();
      FUND( cpd.creator, ASSET( "80.000 TBD" ) );
      generate_block();

      int64_t proposal_1 = create_proposal( cpd.creator, cpd.receiver, cpd.start_date, cpd.end_date, cpd.daily_pay, alice_private_key );
      BOOST_REQUIRE(proposal_1 >= 0);
      std::vector< int64_t > proposals = {proposal_1};
      STEEM_REQUIRE_THROW(vote_proposal("urp", proposals, false, carol_private_key), fc::exception);
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( update_proposal_votes_005 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: update proposal votes: opration arguments validation - invalid id array (array with greater number of digits than allowed)" );
      create_proposal_data cpd(db->head_block_time());
      ACTORS( (alice)(bob)(carol) )
      generate_block();
      FUND( cpd.creator, ASSET( "80.000 TBD" ) );
      generate_block();

      std::vector< int64_t > proposals;
      for(int i = 0; i <= STEEM_PROPOSAL_MAX_IDS_NUMBER; i++) {
         proposals.push_back(i);
      }
      STEEM_REQUIRE_THROW(vote_proposal("carol", proposals, true, carol_private_key), fc::exception);
      
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( update_proposal_votes_006 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: update proposal votes: authorization test" );
      ACTORS( (alice)(bob) )
      generate_block();
      update_proposal_votes_operation upv;
      upv.voter = "alice";
      upv.proposal_ids = {0};
      upv.approve = true;

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      upv.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      upv.get_required_posting_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      expected.insert( "alice" );
      upv.get_required_active_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( remove_proposal_000 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: remove proposal: basic verification operation - proposal removal (only one)." );
      create_proposal_data cpd(db->head_block_time());
      ACTORS( (alice)(bob) )
      generate_block();
      FUND( cpd.creator, ASSET( "80.000 TBD" ) );
      generate_block();

      int64_t proposal_1 = create_proposal( cpd.creator, cpd.receiver, cpd.start_date, cpd.end_date, cpd.daily_pay, alice_private_key );
      BOOST_REQUIRE(proposal_1 >= 0);

      auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_creator >();
      auto found = proposal_idx.find( cpd.creator );
      BOOST_REQUIRE( found != proposal_idx.end() );
      BOOST_REQUIRE( proposal_idx.size() == 1 );

      flat_set<int64_t> proposals = { proposal_1 };
      remove_proposal(cpd.creator, proposals, alice_private_key);

      found = proposal_idx.find( cpd.creator );
      BOOST_REQUIRE( found == proposal_idx.end() );
      BOOST_REQUIRE( proposal_idx.empty() );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( remove_proposal_001 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: remove proposal: basic verification operation - proposal removal (one from many)." );
      create_proposal_data cpd(db->head_block_time());
      ACTORS( (alice)(bob) )
      generate_block();
      FUND( cpd.creator, ASSET( "80.000 TBD" ) );
      generate_block();


      int64_t proposal_1 = create_proposal( cpd.creator, cpd.receiver, cpd.start_date, cpd.end_date, cpd.daily_pay, alice_private_key );
      int64_t proposal_2 = create_proposal( cpd.creator, cpd.receiver, cpd.start_date, cpd.end_date, cpd.daily_pay, alice_private_key );
      int64_t proposal_3 = create_proposal( cpd.creator, cpd.receiver, cpd.start_date, cpd.end_date, cpd.daily_pay, alice_private_key );
      BOOST_REQUIRE(proposal_1 >= 0);
      BOOST_REQUIRE(proposal_2 >= 0);
      BOOST_REQUIRE(proposal_3 >= 0);

      auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_creator >();
      auto found = proposal_idx.find( cpd.creator );
      BOOST_REQUIRE( found != proposal_idx.end() );
      BOOST_REQUIRE( proposal_idx.size() == 3 );

      flat_set<int64_t> proposals = { proposal_1 };
      remove_proposal(cpd.creator, proposals, alice_private_key);

      found = proposal_idx.find( cpd.creator );
      BOOST_REQUIRE( found != proposal_idx.end() );   //two left
      BOOST_REQUIRE( proposal_idx.size() == 2 );

      proposals.clear();
      proposals.insert(proposal_2);
      remove_proposal(cpd.creator, proposals, alice_private_key);

      found = proposal_idx.find( cpd.creator );
      BOOST_REQUIRE( found != proposal_idx.end() );   //one left
      BOOST_REQUIRE( proposal_idx.size() == 1 );

      proposals.clear();
      proposals.insert(proposal_3);
      remove_proposal(cpd.creator, proposals, alice_private_key);

      found = proposal_idx.find( cpd.creator );
      BOOST_REQUIRE( found == proposal_idx.end() );   //none
      BOOST_REQUIRE( proposal_idx.empty() );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( remove_proposal_002 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: remove proposal: basic verification operation - proposal removal (n from many in two steps)." );
      create_proposal_data cpd(db->head_block_time());
      ACTORS( (alice)(bob) )
      generate_block();
      FUND( cpd.creator, ASSET( "80.000 TBD" ) );
      generate_block();

      int64_t proposal = -1;
      std::vector<int64_t> proposals;

      for(int i = 0; i < 6; i++) {
         proposal = create_proposal( cpd.creator, cpd.receiver, cpd.start_date, cpd.end_date, cpd.daily_pay, alice_private_key );
         BOOST_REQUIRE(proposal >= 0);
         proposals.push_back(proposal);
      }
      BOOST_REQUIRE(proposals.size() == 6);

      auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_creator >();
      auto found = proposal_idx.find( cpd.creator );
      BOOST_REQUIRE( found != proposal_idx.end() );
      BOOST_REQUIRE(proposal_idx.size() == 6);

      flat_set<int64_t> proposals_to_erase = {proposals[0], proposals[1], proposals[2], proposals[3]};
      remove_proposal(cpd.creator, proposals_to_erase, alice_private_key);

      found = proposal_idx.find( cpd.creator );
      BOOST_REQUIRE( found != proposal_idx.end() );
      BOOST_REQUIRE( proposal_idx.size() == 2 );

      proposals_to_erase.clear();
      proposals_to_erase.insert(proposals[4]);
      proposals_to_erase.insert(proposals[5]);

      remove_proposal(cpd.creator, proposals_to_erase, alice_private_key);
      found = proposal_idx.find( cpd.creator );
      BOOST_REQUIRE( found == proposal_idx.end() );
      BOOST_REQUIRE( proposal_idx.empty() );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( remove_proposal_003 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: remove proposal: basic verification operation - proper proposal deletion check (one at time)." );
      create_proposal_data cpd(db->head_block_time());
      ACTORS( (alice)(bob) )
      generate_block();
      FUND( cpd.creator, ASSET( "80.000 TBD" ) );
      generate_block();

      int64_t proposal = -1;
      std::vector<int64_t> proposals;

      for(int i = 0; i < 2; i++) {
         proposal = create_proposal( cpd.creator, cpd.receiver, cpd.start_date, cpd.end_date, cpd.daily_pay, alice_private_key );
         BOOST_REQUIRE(proposal >= 0);
         proposals.push_back(proposal);
      }
      BOOST_REQUIRE(proposals.size() == 2);

      auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_creator >();
      auto found = proposal_idx.find( cpd.creator );
      BOOST_REQUIRE( found != proposal_idx.end() );
      BOOST_REQUIRE(proposal_idx.size() == 2);

      flat_set<int64_t> proposals_to_erase = {proposals[0]};
      remove_proposal(cpd.creator, proposals_to_erase, alice_private_key);

      found = proposal_idx.find( cpd.creator );
      BOOST_REQUIRE( found != proposal_idx.end() );
      BOOST_REQUIRE( int64_t(found->proposal_id)  == proposals[1]);
      BOOST_REQUIRE( proposal_idx.size() == 1 );

      proposals_to_erase.clear();
      proposals_to_erase.insert(proposals[1]);

      remove_proposal(cpd.creator, proposals_to_erase, alice_private_key);
      found = proposal_idx.find( cpd.creator );
      BOOST_REQUIRE( found == proposal_idx.end() );
      BOOST_REQUIRE( proposal_idx.empty() );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( remove_proposal_004 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: remove proposal: basic verification operation - proper proposal deletion check (two at one time)." );
      create_proposal_data cpd(db->head_block_time());
      ACTORS( (alice)(bob) )
      generate_block();
      FUND( cpd.creator, ASSET( "80.000 TBD" ) );
      generate_block();

      int64_t proposal = -1;
      std::vector<int64_t> proposals;

      for(int i = 0; i < 6; i++) {
         proposal = create_proposal( cpd.creator, cpd.receiver, cpd.start_date, cpd.end_date, cpd.daily_pay, alice_private_key );
         BOOST_REQUIRE(proposal >= 0);
         proposals.push_back(proposal);
      }
      BOOST_REQUIRE(proposals.size() == 6);

      auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_creator >();
      auto found = proposal_idx.find( cpd.creator );
      BOOST_REQUIRE( found != proposal_idx.end() );
      BOOST_REQUIRE(proposal_idx.size() == 6);

      flat_set<int64_t> proposals_to_erase = {proposals[0], proposals[5]};
      remove_proposal(cpd.creator, proposals_to_erase, alice_private_key);

      found = proposal_idx.find( cpd.creator );
      BOOST_REQUIRE( found != proposal_idx.end() );
      for(auto& it : proposal_idx) {
         BOOST_REQUIRE( static_cast< int64_t >(it.proposal_id) != proposals[0] );
         BOOST_REQUIRE( static_cast< int64_t >(it.proposal_id) != proposals[5] );
      }
      BOOST_REQUIRE( proposal_idx.size() == 4 );

      proposals_to_erase.clear();
      proposals_to_erase.insert(proposals[1]);
      proposals_to_erase.insert(proposals[4]);

      remove_proposal(cpd.creator, proposals_to_erase, alice_private_key);
      found = proposal_idx.find( cpd.creator );
      for(auto& it : proposal_idx) {
         BOOST_REQUIRE( static_cast< int64_t >(it.proposal_id) != proposals[0] );
         BOOST_REQUIRE( static_cast< int64_t >(it.proposal_id) != proposals[1] );
         BOOST_REQUIRE( static_cast< int64_t >(it.proposal_id) != proposals[4] );
         BOOST_REQUIRE( static_cast< int64_t >(it.proposal_id) != proposals[5] );
      }
      BOOST_REQUIRE( found != proposal_idx.end() );
      BOOST_REQUIRE( proposal_idx.size() == 2 );

      proposals_to_erase.clear();
      proposals_to_erase.insert(proposals[2]);
      proposals_to_erase.insert(proposals[3]);
      remove_proposal(cpd.creator, proposals_to_erase, alice_private_key);
      found = proposal_idx.find( cpd.creator );
      BOOST_REQUIRE( found == proposal_idx.end() );
      BOOST_REQUIRE( proposal_idx.empty() );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( remove_proposal_005 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: remove proposal: basic verification operation - proposal with votes removal (only one)." );
      create_proposal_data cpd(db->head_block_time());
      ACTORS( (alice)(bob) )
      generate_block();
      FUND( cpd.creator, ASSET( "80.000 TBD" ) );
      generate_block();

      int64_t proposal_1 = create_proposal( cpd.creator, cpd.receiver, cpd.start_date, cpd.end_date, cpd.daily_pay, alice_private_key );
      BOOST_REQUIRE(proposal_1 >= 0);

      auto& proposal_idx      = db->get_index< proposal_index >().indices().get< by_creator >();
      auto found       = proposal_idx.find( cpd.creator );
      
      BOOST_REQUIRE( found != proposal_idx.end() );
      BOOST_REQUIRE( proposal_idx.size() == 1 );

      std::vector<int64_t> vote_proposals = {proposal_1};

      vote_proposal( "bob", vote_proposals, true, bob_private_key );
      BOOST_REQUIRE( find_vote_for_proposal("bob", proposal_1) );

      flat_set<int64_t> proposals = { proposal_1 };
      remove_proposal(cpd.creator, proposals, alice_private_key);

      found = proposal_idx.find( cpd.creator );
      BOOST_REQUIRE( found == proposal_idx.end() );
      BOOST_REQUIRE( proposal_idx.empty() );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( remove_proposal_006 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: remove proposal: basic verification operation - remove proposal with votes and one voteless at same time." );
      create_proposal_data cpd(db->head_block_time());
      ACTORS( (alice)(bob) )
      generate_block();
      FUND( cpd.creator, ASSET( "80.000 TBD" ) );
      generate_block();

      int64_t proposal_1 = create_proposal( cpd.creator, cpd.receiver, cpd.start_date, cpd.end_date, cpd.daily_pay, alice_private_key );
      int64_t proposal_2 = create_proposal( cpd.creator, cpd.receiver, cpd.start_date, cpd.end_date, cpd.daily_pay, alice_private_key );
      BOOST_REQUIRE(proposal_1 >= 0);
      BOOST_REQUIRE(proposal_2 >= 0);

      auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_creator >();
      auto found = proposal_idx.find( cpd.creator );
      BOOST_REQUIRE( found != proposal_idx.end() );
      BOOST_REQUIRE( proposal_idx.size() == 2 );

      std::vector<int64_t> vote_proposals = {proposal_1};

      vote_proposal( "bob",   vote_proposals, true, bob_private_key );
      BOOST_REQUIRE( find_vote_for_proposal("bob", proposal_1) );

      flat_set<int64_t> proposals = { proposal_1, proposal_2 };
      remove_proposal(cpd.creator, proposals, alice_private_key);

      found = proposal_idx.find( cpd.creator );
      BOOST_REQUIRE( found == proposal_idx.end() );
      BOOST_REQUIRE( proposal_idx.empty() );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( remove_proposal_007 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: remove proposal: basic verification operation - remove proposals with votes at same time." );
      create_proposal_data cpd(db->head_block_time());
      ACTORS( (alice)(bob)(carol) )
      generate_block();
      FUND( cpd.creator, ASSET( "80.000 TBD" ) );
      generate_block();

      int64_t proposal_1 = create_proposal( cpd.creator, cpd.receiver, cpd.start_date, cpd.end_date, cpd.daily_pay, alice_private_key );
      int64_t proposal_2 = create_proposal( cpd.creator, cpd.receiver, cpd.start_date, cpd.end_date, cpd.daily_pay, alice_private_key );
      BOOST_REQUIRE(proposal_1 >= 0);
      BOOST_REQUIRE(proposal_2 >= 0);

      auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_creator >();
      auto found = proposal_idx.find( cpd.creator );
      BOOST_REQUIRE( found != proposal_idx.end() );
      BOOST_REQUIRE( proposal_idx.size() == 2 );

      std::vector<int64_t> vote_proposals = {proposal_1};
      vote_proposal( "bob",   vote_proposals, true, bob_private_key );
      BOOST_REQUIRE( find_vote_for_proposal("bob", proposal_1) );
      vote_proposals.clear();
      vote_proposals.push_back(proposal_2);
      vote_proposal( "carol", vote_proposals, true, carol_private_key );
      BOOST_REQUIRE( find_vote_for_proposal("carol", proposal_2) );

      flat_set<int64_t> proposals = { proposal_1, proposal_2 };
      remove_proposal(cpd.creator, proposals, alice_private_key);

      found = proposal_idx.find( cpd.creator );
      BOOST_REQUIRE( found == proposal_idx.end() );
      BOOST_REQUIRE( proposal_idx.empty() );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( remove_proposal_008 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: remove proposal: opration arguments validation - all ok" );
      create_proposal_data cpd(db->head_block_time());
      ACTORS( (alice)(bob) )
      generate_block();
      FUND( cpd.creator, ASSET( "80.000 TBD" ) );
      generate_block();
      flat_set<int64_t> proposals = { 0 };
      remove_proposal(cpd.creator, proposals, alice_private_key); 
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( remove_proposal_009 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: remove proposal: opration arguments validation - invalid deleter" );
      create_proposal_data cpd(db->head_block_time());
      ACTORS( (alice)(bob) )
      generate_block();
      FUND( cpd.creator, ASSET( "80.000 TBD" ) );
      generate_block();
      int64_t proposal_1 = create_proposal( cpd.creator, cpd.receiver, cpd.start_date, cpd.end_date, cpd.daily_pay, alice_private_key );
      flat_set<int64_t> proposals = { proposal_1 };
      STEEM_REQUIRE_THROW(remove_proposal(cpd.receiver, proposals, bob_private_key), fc::exception); 
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( remove_proposal_010 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: remove proposal: opration arguments validation - invalid array(empty array)" );
      create_proposal_data cpd(db->head_block_time());
      ACTORS( (alice)(bob) )
      generate_block();
      FUND( cpd.creator, ASSET( "80.000 TBD" ) );
      generate_block();
      flat_set<int64_t> proposals;
      STEEM_REQUIRE_THROW(remove_proposal(cpd.creator, proposals, bob_private_key), fc::exception); 
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( remove_proposal_011 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: remove proposal: opration arguments validation - invalid array(array with greater number of digits than allowed)" );
      create_proposal_data cpd(db->head_block_time());
      ACTORS( (alice)(bob) )
      generate_block();
      FUND( cpd.creator, ASSET( "80.000 TBD" ) );
      generate_block();
      flat_set<int64_t> proposals;
      for(int i = 0; i <= STEEM_PROPOSAL_MAX_IDS_NUMBER; i++) {
         proposals.insert(create_proposal( cpd.creator, cpd.receiver, cpd.start_date, cpd.end_date, cpd.daily_pay, alice_private_key ));
      }
      STEEM_REQUIRE_THROW(remove_proposal(cpd.creator, proposals, bob_private_key), fc::exception); 
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( remove_proposal_012 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: remove proposal: authorization test" );
      ACTORS( (alice)(bob) )
      generate_block();
      remove_proposal_operation rpo;
      rpo.proposal_owner = "alice";
      rpo.proposal_ids = {1,2,3};

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      rpo.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      rpo.get_required_posting_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      expected.insert( "alice" );
      rpo.get_required_active_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( list_proposal_000 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: list proposals: arguments validation check - all ok" );
      plugin_prepare();

      std::vector<std::string> order_by        {"creator", "start_date", "end_date", "total_votes"};
      for(auto by : order_by) {
         auto order_by        = steem::plugins::sps::to_order_by(by);
         if( by == "creator") {
            BOOST_REQUIRE( order_by == steem::plugins::sps::order_by_type::by_creator );
         } else if ( by == "start_date") {
            BOOST_REQUIRE( order_by == steem::plugins::sps::order_by_type::by_start_date );
         } else if ( by == "end_date") {
            BOOST_REQUIRE( order_by == steem::plugins::sps::order_by_type::by_end_date );
         } else if ( by == "total_votes"){
            BOOST_REQUIRE( order_by == steem::plugins::sps::order_by_type::by_total_votes );
         } else {
            BOOST_REQUIRE( order_by == steem::plugins::sps::order_by_type::by_creator );
         }
      }

      std::vector<std::string> order_direction {"asc", "desc"};
      for(auto direct : order_direction) {
         auto order_direction = steem::plugins::sps::to_order_direction(direct);
         if( direct == "asc") {
            BOOST_REQUIRE( order_direction == steem::plugins::sps::order_direction_type::direction_ascending );
         } else if ( direct == "desc") {
            BOOST_REQUIRE( order_direction == steem::plugins::sps::order_direction_type::direction_descending );
         } else {
            BOOST_REQUIRE( order_direction == steem::plugins::sps::order_direction_type::direction_ascending );
         }
      }

      std::vector<std::string> active          {"active", "inactive", "all"};
      for(auto act : active){
         auto status          = steem::plugins::sps::to_proposal_status(act);
         if( act == "active") {
            BOOST_REQUIRE( status == steem::plugins::sps::proposal_status::active );
         } else if ( act == "inactive") {
            BOOST_REQUIRE( status == steem::plugins::sps::proposal_status::inactive );
         } else if ( act == "all") {
            BOOST_REQUIRE( status == steem::plugins::sps::proposal_status::all );
         } else {
            BOOST_REQUIRE( status == steem::plugins::sps::proposal_status::all );
         }
      }
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( list_proposal_001 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: list proposals: call result check - all ok" );
      plugin_prepare();
      create_proposal_data cpd(db->head_block_time());
      ACTORS( (alice)(bob)(carol) )
      generate_block();
      FUND( cpd.creator, ASSET( "80.000 TBD" ) );
      generate_block();

      auto checker = [this, cpd](bool _empty){
         std::vector<std::string> active          {"active", "inactive", "all"};
         std::vector<std::string> order_by        {"creator", "start_date", "end_date", "total_votes"};
         std::vector<std::string> order_direction {"asc", "desc"};
         fc::variant start = 0;
         for(auto by : order_by) {
            if (by == "creator"){
               start = "";
            } else if (by == "start_date" || by == "end_date") {
               start = "2016-03-01T00:00:00";
            } else {
               start = 0;
            }
            for(auto direct : order_direction) {
               for(auto act : active) {
                  auto resp = list_proposals(start, by, direct,1, act, "");
                  if(_empty) {
                     BOOST_REQUIRE(resp.empty());
                  } else {
                     BOOST_REQUIRE(!resp.empty());
                  }
               }
            }
         }
      };

      checker(true);
      int64_t proposal_1 = create_proposal( cpd.creator, cpd.receiver, cpd.start_date, cpd.end_date, cpd.daily_pay, alice_private_key );
      BOOST_REQUIRE(proposal_1 >= 0);
      auto resp = list_proposals(cpd.creator, "creator", "asc", 10, "all", "");
      BOOST_REQUIRE(!resp.empty());
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( list_voter_proposals_000 )
{
   try
   {
      plugin_prepare();

      std::vector<std::string> order_by        {"creator", "start_date", "end_date", "total_votes"};
      for(auto by : order_by) {
         auto order_by        = steem::plugins::sps::to_order_by(by);
         if( by == "creator") {
            BOOST_REQUIRE( order_by == steem::plugins::sps::order_by_type::by_creator );
         } else if ( by == "start_date") {
            BOOST_REQUIRE( order_by == steem::plugins::sps::order_by_type::by_start_date );
         } else if ( by == "end_date") {
            BOOST_REQUIRE( order_by == steem::plugins::sps::order_by_type::by_end_date );
         } else if ( by == "total_votes"){
            BOOST_REQUIRE( order_by == steem::plugins::sps::order_by_type::by_total_votes );
         } else {
            BOOST_REQUIRE( order_by == steem::plugins::sps::order_by_type::by_creator );
         }
      }

      std::vector<std::string> order_direction {"asc", "desc"};
      for(auto direct : order_direction) {
         auto order_direction = steem::plugins::sps::to_order_direction(direct);
         if( direct == "asc") {
            BOOST_REQUIRE( order_direction == steem::plugins::sps::order_direction_type::direction_ascending );
         } else if ( direct == "desc") {
            BOOST_REQUIRE( order_direction == steem::plugins::sps::order_direction_type::direction_descending );
         } else {
            BOOST_REQUIRE( order_direction == steem::plugins::sps::order_direction_type::direction_ascending );
         }
      }

      std::vector<std::string> active          {"active", "inactive", "all"};
      for(auto act : active){
         auto status          = steem::plugins::sps::to_proposal_status(act);
         if( act == "active") {
            BOOST_REQUIRE( status == steem::plugins::sps::proposal_status::active );
         } else if ( act == "inactive") {
            BOOST_REQUIRE( status == steem::plugins::sps::proposal_status::inactive );
         } else if ( act == "all") {
            BOOST_REQUIRE( status == steem::plugins::sps::proposal_status::all );
         } else {
            BOOST_REQUIRE( status == steem::plugins::sps::proposal_status::all );
         }
      }
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( list_voter_proposals_001 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: list voter proposals: call result check - all ok" );
      plugin_prepare();
      create_proposal_data cpd(db->head_block_time());
      ACTORS( (alice)(bob)(carol) )
      generate_block();
      FUND( cpd.creator, ASSET( "80.000 TBD" ) );
      generate_block();

      auto checker = [this, cpd](bool _empty){
         std::vector<std::string> active          {"active", "inactive", "all"};
         std::vector<std::string> order_by        {"creator", "start_date", "end_date", "total_votes"};
         std::vector<std::string> order_direction {"asc", "desc"};
         for(auto by : order_by) {
            for(auto direct : order_direction) {
               for(auto act : active) {
                  auto resp = list_voter_proposals(cpd.creator, by, direct,10, act, "");
                  if(_empty) {
                     BOOST_REQUIRE(resp.empty());
                  } else {
                     BOOST_REQUIRE(!resp.empty());
                  }
               }
            }
         }
      };

      checker(true);
      int64_t proposal_1 = create_proposal( cpd.creator, cpd.receiver, cpd.start_date, cpd.end_date, cpd.daily_pay, alice_private_key );
      checker(true);
      BOOST_REQUIRE(proposal_1 >= 0);
      std::vector< int64_t > proposals = {proposal_1};
      vote_proposal(cpd.creator, proposals, true, alice_private_key);
      auto resp = list_voter_proposals(cpd.creator, "creator", "asc", 10, "all", "");
      BOOST_REQUIRE(!resp.empty());
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( find_proposals_000 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: find proposals: call result check - all ok" );
      plugin_prepare();
      create_proposal_data cpd(db->head_block_time());
      ACTORS( (alice)(bob)(carol) )
      generate_block();
      FUND( cpd.creator, ASSET( "80.000 TBD" ) );
      generate_block();

      flat_set<uint64_t> prop_before = {0};
      auto resp = find_proposals(prop_before);
      BOOST_REQUIRE(resp.empty());

      uint64_t proposal_1 = create_proposal( cpd.creator, cpd.receiver, cpd.start_date, cpd.end_date, cpd.daily_pay, alice_private_key );
      BOOST_REQUIRE(proposal_1 >= 0);

      flat_set<uint64_t> prop_after = {proposal_1};
      resp = find_proposals(prop_after);
      BOOST_REQUIRE(!resp.empty());

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( proposals_maintenance_01 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: removing of old proposals using threshold" );

      ACTORS( (a00)(a01)(a02)(a03)(a04) )
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      generate_block();

      //=====================preparing=====================
      auto receiver = "steem.dao";

      auto start_time = db->head_block_time();

      const auto start_time_shift = fc::hours( 11 );
      const auto end_time_shift = fc::hours( 10 );
      const auto block_interval = fc::seconds( STEEM_BLOCK_INTERVAL );

      auto start_date_00 = start_time + start_time_shift;
      auto end_date_00 = start_date_00 + end_time_shift;

      auto daily_pay = asset( 100, SBD_SYMBOL );

      const auto nr_proposals = 200;
      std::vector< int64_t > proposals_id;

      struct initial_data
      {
         std::string account;
         fc::ecc::private_key key;
      };

      std::vector< initial_data > inits = {
                                       {"a00", a00_private_key },
                                       {"a01", a01_private_key },
                                       {"a02", a02_private_key },
                                       {"a03", a03_private_key },
                                       {"a04", a04_private_key },
                                       };

      for( auto item : inits )
         FUND( item.account, ASSET( "10000.000 TBD" ) );
      //=====================preparing=====================

      for( int32_t i = 0; i < nr_proposals; ++i )
      {
         auto item = inits[ i % inits.size() ];
         proposals_id.push_back( create_proposal( item.account, receiver, start_date_00, end_date_00, daily_pay, item.key ) );
         generate_block();
      }

      const auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_proposal_id >();

      auto current_active_proposals = nr_proposals;
      BOOST_REQUIRE( calc_proposals( proposal_idx, proposals_id ) == current_active_proposals );

      generate_blocks( start_time + fc::seconds( STEEM_PROPOSAL_MAINTENANCE_CLEANUP ) );
      start_time = db->head_block_time();

      generate_blocks( start_time + ( start_time_shift + end_time_shift - block_interval ) );

      auto threshold = db->get_sps_remove_threshold();
      auto nr_stages = current_active_proposals / threshold;

      for( auto i = 0; i < nr_stages; ++i )
      {
         generate_block();

         current_active_proposals -= threshold;
         auto found = calc_proposals( proposal_idx, proposals_id );

         BOOST_REQUIRE( current_active_proposals == found );
      }

      BOOST_REQUIRE( current_active_proposals == 0 );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( proposals_maintenance_02 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: removing of old proposals + votes using threshold" );

      ACTORS( (a00)(a01)(a02)(a03)(a04) )
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      generate_block();

      //=====================preparing=====================
      auto receiver = "steem.dao";

      auto start_time = db->head_block_time();

      const auto start_time_shift = fc::hours( 11 );
      const auto end_time_shift = fc::hours( 10 );
      const auto block_interval = fc::seconds( STEEM_BLOCK_INTERVAL );

      auto start_date_00 = start_time + start_time_shift;
      auto end_date_00 = start_date_00 + end_time_shift;

      auto daily_pay = asset( 100, SBD_SYMBOL );

      const auto nr_proposals = 10;
      std::vector< int64_t > proposals_id;

      struct initial_data
      {
         std::string account;
         fc::ecc::private_key key;
      };

      std::vector< initial_data > inits = {
                                       {"a00", a00_private_key },
                                       {"a01", a01_private_key },
                                       {"a02", a02_private_key },
                                       {"a03", a03_private_key },
                                       {"a04", a04_private_key },
                                       };

      for( auto item : inits )
         FUND( item.account, ASSET( "10000.000 TBD" ) );
      //=====================preparing=====================

      for( auto i = 0; i < nr_proposals; ++i )
      {
         auto item = inits[ i % inits.size() ];
         proposals_id.push_back( create_proposal( item.account, receiver, start_date_00, end_date_00, daily_pay, item.key ) );
         generate_block();
      }

      auto itr_begin_2 = proposals_id.begin() + STEEM_PROPOSAL_MAX_IDS_NUMBER;
      std::vector< int64_t > v1( proposals_id.begin(), itr_begin_2 );
      std::vector< int64_t > v2( itr_begin_2, proposals_id.end() );

      for( auto item : inits )
      {
         vote_proposal( item.account, v1, true/*approve*/, item.key);
         generate_blocks( 1 );
         vote_proposal( item.account, v2, true/*approve*/, item.key);
         generate_blocks( 1 );
      }

      const auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_proposal_id >();
      const auto& proposal_vote_idx = db->get_index< proposal_vote_index >().indices().get< by_proposal_voter >();

      auto current_active_proposals = nr_proposals;
      BOOST_REQUIRE( calc_proposals( proposal_idx, proposals_id ) == current_active_proposals );

      auto current_active_votes = current_active_proposals * static_cast< int16_t > ( inits.size() );
      BOOST_REQUIRE( calc_votes( proposal_vote_idx, proposals_id ) == current_active_votes );

      generate_blocks( start_time + fc::seconds( STEEM_PROPOSAL_MAINTENANCE_CLEANUP ) );
      start_time = db->head_block_time();

      generate_blocks( start_time + ( start_time_shift + end_time_shift - block_interval ) );

      auto threshold = db->get_sps_remove_threshold();
      auto current_active_anything = current_active_proposals + current_active_votes;
      auto nr_stages = current_active_anything / threshold;

      for( auto i = 0; i < nr_stages; ++i )
      {
         generate_block();

         current_active_anything -= threshold;
         auto found_proposals = calc_proposals( proposal_idx, proposals_id );
         auto found_votes = calc_votes( proposal_vote_idx, proposals_id );

         BOOST_REQUIRE( current_active_anything == found_proposals + found_votes );
      }

      BOOST_REQUIRE( current_active_anything == 0 );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( proposals_removing_with_threshold )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: removing of old proposals + votes using threshold" );

      ACTORS( (a00)(a01)(a02)(a03)(a04) )
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      generate_block();

      //=====================preparing=====================
      auto receiver = "steem.dao";

      auto start_time = db->head_block_time();

      const auto start_time_shift = fc::days( 100 );
      const auto end_time_shift = fc::days( 101 );

      auto start_date_00 = start_time + start_time_shift;
      auto end_date_00 = start_date_00 + end_time_shift;

      auto daily_pay = asset( 100, SBD_SYMBOL );

      const auto nr_proposals = 5;
      std::vector< int64_t > proposals_id;

      struct initial_data
      {
         std::string account;
         fc::ecc::private_key key;
      };

      std::vector< initial_data > inits = {
                                       {"a00", a00_private_key },
                                       {"a01", a01_private_key },
                                       {"a02", a02_private_key },
                                       {"a03", a03_private_key },
                                       {"a04", a04_private_key },
                                       };

      for( auto item : inits )
         FUND( item.account, ASSET( "10000.000 TBD" ) );
      //=====================preparing=====================

      auto item_creator = inits[ 0 ];
      for( auto i = 0; i < nr_proposals; ++i )
      {
         proposals_id.push_back( create_proposal( item_creator.account, receiver, start_date_00, end_date_00, daily_pay, item_creator.key ) );
         generate_block();
      }

      for( auto item : inits )
      {
         vote_proposal( item.account, proposals_id, true/*approve*/, item.key);
         generate_blocks( 1 );
      }

      const auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_proposal_id >();
      const auto& proposal_vote_idx = db->get_index< proposal_vote_index >().indices().get< by_proposal_voter >();

      auto current_active_proposals = nr_proposals;
      BOOST_REQUIRE( calc_proposals( proposal_idx, proposals_id ) == current_active_proposals );

      auto current_active_votes = current_active_proposals * static_cast< int16_t > ( inits.size() );
      BOOST_REQUIRE( calc_votes( proposal_vote_idx, proposals_id ) == current_active_votes );

      auto threshold = db->get_sps_remove_threshold();
      BOOST_REQUIRE( threshold == 20 );

      flat_set<long int> _proposals_id( proposals_id.begin(), proposals_id.end() );

      {
         remove_proposal( item_creator.account,  _proposals_id, item_creator.key );
         auto found_proposals = calc_proposals( proposal_idx, proposals_id );
         auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
         BOOST_REQUIRE( found_proposals == 2 );
         BOOST_REQUIRE( found_votes == 8 );
         generate_blocks( 1 );
      }

      {
         remove_proposal( item_creator.account,  _proposals_id, item_creator.key );
         auto found_proposals = calc_proposals( proposal_idx, proposals_id );
         auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
         BOOST_REQUIRE( found_proposals == 0 );
         BOOST_REQUIRE( found_votes == 0 );
         generate_blocks( 1 );
      }

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( proposals_removing_with_threshold_01 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: removing of old proposals/votes using threshold " );

      ACTORS( (a00)(a01)(a02)(a03)(a04)(a05) )
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      generate_block();

      //=====================preparing=====================
      auto receiver = "steem.dao";

      auto start_time = db->head_block_time();

      const auto start_time_shift = fc::days( 100 );
      const auto end_time_shift = fc::days( 101 );

      auto start_date_00 = start_time + start_time_shift;
      auto end_date_00 = start_date_00 + end_time_shift;

      auto daily_pay = asset( 100, SBD_SYMBOL );

      const auto nr_proposals = 10;
      std::vector< int64_t > proposals_id;

      struct initial_data
      {
         std::string account;
         fc::ecc::private_key key;
      };

      std::vector< initial_data > inits = {
                                       {"a00", a00_private_key },
                                       {"a01", a01_private_key },
                                       {"a02", a02_private_key },
                                       {"a03", a03_private_key },
                                       {"a04", a04_private_key },
                                       {"a05", a05_private_key },
                                       };

      for( auto item : inits )
         FUND( item.account, ASSET( "10000.000 TBD" ) );
      //=====================preparing=====================

      auto item_creator = inits[ 0 ];
      for( auto i = 0; i < nr_proposals; ++i )
      {
         proposals_id.push_back( create_proposal( item_creator.account, receiver, start_date_00, end_date_00, daily_pay, item_creator.key ) );
         generate_block();
      }

      auto itr_begin_2 = proposals_id.begin() + STEEM_PROPOSAL_MAX_IDS_NUMBER;
      std::vector< int64_t > v1( proposals_id.begin(), itr_begin_2 );
      std::vector< int64_t > v2( itr_begin_2, proposals_id.end() );

      for( auto item : inits )
      {
         vote_proposal( item.account, v1, true/*approve*/, item.key);
         generate_block();
         vote_proposal( item.account, v2, true/*approve*/, item.key);
         generate_block();
      }

      const auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_proposal_id >();
      const auto& proposal_vote_idx = db->get_index< proposal_vote_index >().indices().get< by_proposal_voter >();

      auto current_active_proposals = nr_proposals;
      BOOST_REQUIRE( calc_proposals( proposal_idx, proposals_id ) == current_active_proposals );

      auto current_active_votes = current_active_proposals * static_cast< int16_t > ( inits.size() );
      BOOST_REQUIRE( calc_votes( proposal_vote_idx, proposals_id ) == current_active_votes );

      auto threshold = db->get_sps_remove_threshold();
      BOOST_REQUIRE( threshold == 20 );

      /*
         nr_proposals = 10
         nr_votes_per_proposal = 6

         Info:
            Pn - number of proposal
            vn - number of vote
            xx - elements removed in given block
            oo - elements removed earlier
            rr - elements with status `removed`

         Below in matrix is situation before removal any element.

         P0 P1 P2 P3 P4 P5 P6 P7 P8 P9

         v0 v0 v0 v0 v0 v0 v0 v0 v0 v0
         v1 v1 v1 v1 v1 v1 v1 v1 v1 v1
         v2 v2 v2 v2 v2 v2 v2 v2 v2 v2
         v3 v3 v3 v3 v3 v3 v3 v3 v3 v3
         v4 v4 v4 v4 v4 v4 v4 v4 v4 v4
         v5 v5 v5 v5 v5 v5 v5 v5 v5 v5

         Total number of elements to be removed:
         Total = nr_proposals * nr_votes_per_proposal + nr_proposals = 10 * 6 + 10 = 70
      */

      /*
         P0 P1 P2 xx P4 P5 P6 P7 P8 P9

         v0 v0 v0 xx v0 v0 v0 v0 v0 v0
         v1 v1 v1 xx v1 v1 v1 v1 v1 v1
         v2 v2 v2 xx v2 v2 v2 v2 v2 v2
         v3 v3 v3 xx v3 v3 v3 v3 v3 v3
         v4 v4 v4 xx v4 v4 v4 v4 v4 v4
         v5 v5 v5 xx v5 v5 v5 v5 v5 v5
      */
      {
         remove_proposal( item_creator.account, {3}, item_creator.key );
         generate_block();
         auto found_proposals = calc_proposals( proposal_idx, proposals_id );
         auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
         BOOST_REQUIRE( found_proposals == 9 );
         BOOST_REQUIRE( found_votes == 54 );

         int32_t cnt = 0;
         for( auto id : proposals_id )
            cnt += ( id != 3 ) ? calc_proposal_votes( proposal_vote_idx, id ) : 0;
         BOOST_REQUIRE( cnt == found_votes );
      }

      /*
         P0 xx xx oo P4 P5 P6 rr P8 P9

         v0 xx xx oo v0 v0 v0 xx v0 v0
         v1 xx xx oo v1 v1 v1 xx v1 v1
         v2 xx xx oo v2 v2 v2 xx v2 v2
         v3 xx xx oo v3 v3 v3 xx v3 v3
         v4 xx xx oo v4 v4 v4 xx v4 v4
         v5 xx xx oo v5 v5 v5 xx v5 v5
      */
      {
         remove_proposal( item_creator.account, {1,2,7}, item_creator.key );
         auto found_proposals = calc_proposals( proposal_idx, proposals_id );
         auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
         BOOST_REQUIRE( exist_proposal( 7 ) && find_proposal( 7 )->removed );
         BOOST_REQUIRE( found_proposals == 7 );
         BOOST_REQUIRE( found_votes == 36 );
      }


      /*
         P0 oo oo oo P4 P5 P6 xx P8 P9

         v0 oo oo oo v0 v0 v0 oo v0 v0
         v1 oo oo oo v1 v1 v1 oo v1 v1
         v2 oo oo oo v2 v2 v2 oo v2 v2
         v3 oo oo oo v3 v3 v3 oo v3 v3
         v4 oo oo oo v4 v4 v4 oo v4 v4
         v5 oo oo oo v5 v5 v5 oo v5 v5
      */
      {
         //Only the proposal P7 is removed.
         generate_block();

         auto found_proposals = calc_proposals( proposal_idx, proposals_id );
         auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
         BOOST_REQUIRE( !exist_proposal( 7 ) );
         BOOST_REQUIRE( found_proposals == 6 );
         BOOST_REQUIRE( found_votes == 36 );
      }

      /*
         P0 oo oo oo P4 xx xx oo rr rr

         v0 oo oo oo v0 xx xx oo xx rr
         v1 oo oo oo v1 xx xx oo xx rr
         v2 oo oo oo v2 xx xx oo xx rr
         v3 oo oo oo v3 xx xx oo xx rr
         v4 oo oo oo v4 xx xx oo xx rr
         v5 oo oo oo v5 xx xx oo xx rr
      */
      {
         remove_proposal( item_creator.account, {5,6,7/*doesn't exist*/,8,9}, item_creator.key );
         auto found_proposals = calc_proposals( proposal_idx, proposals_id );
         auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
         BOOST_REQUIRE( exist_proposal( 8 ) && find_proposal( 8 )->removed );
         BOOST_REQUIRE( found_proposals == 4 );
         BOOST_REQUIRE( found_votes == 18 );

         int32_t cnt = 0;
         for( auto id : proposals_id )
            cnt += ( id == 0 || id == 4 || id == 8 || id == 9 ) ? calc_proposal_votes( proposal_vote_idx, id ) : 0;
         BOOST_REQUIRE( cnt == found_votes );
      }

      /*
         xx oo oo oo xx oo oo oo xx rr

         xx oo oo oo xx oo oo oo oo rr
         xx oo oo oo xx oo oo oo oo xx
         xx oo oo oo xx oo oo oo oo xx
         xx oo oo oo xx oo oo oo oo xx
         xx oo oo oo xx oo oo oo oo xx
         xx oo oo oo xx oo oo oo oo xx
      */
      {
         remove_proposal( item_creator.account, {0,4,8,9}, item_creator.key );
         auto found_proposals = calc_proposals( proposal_idx, proposals_id );
         auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
         BOOST_REQUIRE( exist_proposal( 9 ) && find_proposal( 9 )->removed );
         BOOST_REQUIRE( found_proposals == 1 );
         BOOST_REQUIRE( found_votes == 1 );
      }

      /*
         oo oo oo oo oo oo oo oo oo xx

         oo oo oo oo oo oo oo oo oo xx
         oo oo oo oo oo oo oo oo oo oo
         oo oo oo oo oo oo oo oo oo oo
         oo oo oo oo oo oo oo oo oo oo
         oo oo oo oo oo oo oo oo oo oo
         oo oo oo oo oo oo oo oo oo oo
      */
      {
         //Only the proposal P9 is removed.
         generate_block();

         auto found_proposals = calc_proposals( proposal_idx, proposals_id );
         auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
         BOOST_REQUIRE( found_proposals == 0 );
         BOOST_REQUIRE( found_votes == 0 );
      }

   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( proposals_removing_with_threshold_02 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: removing of old proposals/votes using threshold " );

      ACTORS(
               (a00)(a01)(a02)(a03)(a04)(a05)(a06)(a07)(a08)(a09)
               (a10)(a11)(a12)(a13)(a14)(a15)(a16)(a17)(a18)(a19)
               (a20)(a21)(a22)(a23)(a24)(a25)(a26)(a27)(a28)(a29)
               (a30)(a31)(a32)(a33)(a34)(a35)(a36)(a37)(a38)(a39)
               (a40)(a41)(a42)(a43)(a44)(a45)(a46)(a47)(a48)(a49)
            )
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      generate_block();

      //=====================preparing=====================
      auto receiver = "steem.dao";

      auto start_time = db->head_block_time();

      const auto start_time_shift = fc::days( 100 );
      const auto end_time_shift = fc::days( 101 );

      auto start_date_00 = start_time + start_time_shift;
      auto end_date_00 = start_date_00 + end_time_shift;

      auto daily_pay = asset( 100, SBD_SYMBOL );

      const auto nr_proposals = 5;
      std::vector< int64_t > proposals_id;

      struct initial_data
      {
         std::string account;
         fc::ecc::private_key key;
      };

      std::vector< initial_data > inits = {
         {"a00", a00_private_key }, {"a01", a01_private_key }, {"a02", a02_private_key }, {"a03", a03_private_key }, {"a04", a04_private_key },
         {"a05", a05_private_key }, {"a06", a06_private_key }, {"a07", a07_private_key }, {"a08", a08_private_key }, {"a09", a09_private_key },
         {"a10", a10_private_key }, {"a11", a11_private_key }, {"a12", a12_private_key }, {"a13", a13_private_key }, {"a14", a14_private_key },
         {"a15", a15_private_key }, {"a16", a16_private_key }, {"a17", a17_private_key }, {"a18", a18_private_key }, {"a19", a19_private_key },
         {"a20", a20_private_key }, {"a21", a21_private_key }, {"a22", a22_private_key }, {"a23", a23_private_key }, {"a24", a24_private_key },
         {"a25", a25_private_key }, {"a26", a26_private_key }, {"a27", a27_private_key }, {"a28", a28_private_key }, {"a29", a29_private_key },
         {"a30", a30_private_key }, {"a31", a31_private_key }, {"a32", a32_private_key }, {"a33", a33_private_key }, {"a34", a34_private_key },
         {"a35", a35_private_key }, {"a36", a36_private_key }, {"a37", a37_private_key }, {"a38", a38_private_key }, {"a39", a39_private_key },
         {"a40", a40_private_key }, {"a41", a41_private_key }, {"a42", a42_private_key }, {"a43", a43_private_key }, {"a44", a44_private_key },
         {"a45", a45_private_key }, {"a46", a46_private_key }, {"a47", a47_private_key }, {"a48", a48_private_key }, {"a49", a49_private_key }
      };

      for( auto item : inits )
         FUND( item.account, ASSET( "10000.000 TBD" ) );
      //=====================preparing=====================

      auto item_creator = inits[ 0 ];
      for( auto i = 0; i < nr_proposals; ++i )
      {
         proposals_id.push_back( create_proposal( item_creator.account, receiver, start_date_00, end_date_00, daily_pay, item_creator.key ) );
         generate_block();
      }

      for( auto item : inits )
      {
         vote_proposal( item.account, proposals_id, true/*approve*/, item.key);
         generate_block();
      }

      const auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_proposal_id >();
      const auto& proposal_vote_idx = db->get_index< proposal_vote_index >().indices().get< by_proposal_voter >();

      auto current_active_proposals = nr_proposals;
      BOOST_REQUIRE( calc_proposals( proposal_idx, proposals_id ) == current_active_proposals );

      auto current_active_votes = current_active_proposals * static_cast< int16_t > ( inits.size() );
      BOOST_REQUIRE( calc_votes( proposal_vote_idx, proposals_id ) == current_active_votes );

      auto threshold = db->get_sps_remove_threshold();
      BOOST_REQUIRE( threshold == 20 );

      /*
         nr_proposals = 5
         nr_votes_per_proposal = 50

         Info:
            Pn - number of proposal
            vn - number of vote
            xx - elements removed in given block
            oo - elements removed earlier
            rr - elements with status `removed`

         Below in matrix is situation before removal any element.

         P0    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
         P1    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
         P2    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
         P3    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
         P4    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49

         Total number of elements to be removed:
         Total = nr_proposals * nr_votes_per_proposal + nr_proposals = 5 * 50 + 5 = 255
      */

      /*
         rr    xx xx xx xx xx xx xx xx xx xx rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr
         P1    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
         P2    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
         rr    xx xx xx xx xx xx xx xx xx xx rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr
         P4    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
      */
      {
         remove_proposal( item_creator.account, {0,3}, item_creator.key );
         auto found_proposals = calc_proposals( proposal_idx, proposals_id );
         auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
         BOOST_REQUIRE( exist_proposal( 0 ) && find_proposal( 0 )->removed );
         BOOST_REQUIRE( exist_proposal( 3 ) && find_proposal( 3 )->removed );
         BOOST_REQUIRE( found_proposals == 5 );
         BOOST_REQUIRE( found_votes == 230 );
      }

      /*
         rr    xx xx xx xx xx xx xx xx xx xx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr
         P1    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
         P2    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
         rr    xx xx xx xx xx xx xx xx xx xx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr
         P4    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
      */
      {
         generate_block();

         auto found_proposals = calc_proposals( proposal_idx, proposals_id );
         auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
         BOOST_REQUIRE( exist_proposal( 0 ) && find_proposal( 0 )->removed );
         BOOST_REQUIRE( exist_proposal( 3 ) && find_proposal( 3 )->removed );
         BOOST_REQUIRE( found_proposals == 5 );
         BOOST_REQUIRE( found_votes == 210 );
      }

      /*
         xx    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx
         P1    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
         P2    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
         xx    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx
         P4    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
      */
      {
         generate_block();
         generate_block();
         generate_block();
         generate_block();

         auto found_proposals = calc_proposals( proposal_idx, proposals_id );
         auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
         BOOST_REQUIRE( !exist_proposal( 0 ) );
         BOOST_REQUIRE( !exist_proposal( 3 ) );
         BOOST_REQUIRE( found_proposals == 3 );
         BOOST_REQUIRE( found_votes == 150 );
      }

      /*
         nothing changed - the same state as in previous block

         oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
         P1    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
         P2    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
         oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
         P4    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
      */
      {
         generate_block();

         auto found_proposals = calc_proposals( proposal_idx, proposals_id );
         auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
         BOOST_REQUIRE( !exist_proposal( 0 ) );
         BOOST_REQUIRE( !exist_proposal( 3 ) );
         BOOST_REQUIRE( found_proposals == 3 );
         BOOST_REQUIRE( found_votes == 150 );
      }

      /*
         nothing changed - the same state as in previous block
      */
      {
         generate_block();

         auto found_proposals = calc_proposals( proposal_idx, proposals_id );
         auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
         BOOST_REQUIRE( !exist_proposal( 0 ) );
         BOOST_REQUIRE( !exist_proposal( 3 ) );
         BOOST_REQUIRE( found_proposals == 3 );
         BOOST_REQUIRE( found_votes == 150 );
      }

      /*
         oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
         rr    xx xx xx xx xx xx xx xx xx xx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr
         P2    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
         oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
         P4    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
      */
      {
         remove_proposal( item_creator.account, {1}, item_creator.key );
         auto found_proposals = calc_proposals( proposal_idx, proposals_id );
         auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
         BOOST_REQUIRE( exist_proposal( 1 ) && find_proposal( 1 )->removed );
         BOOST_REQUIRE( found_proposals == 3 );
         BOOST_REQUIRE( found_votes == 130 );
      }

      /*
         oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
         rr    xx xx xx xx xx xx xx xx xx xx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr
         rr    xx xx xx xx xx xx xx xx xx xx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr rrr
         oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
         P4    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
      */
      {
         remove_proposal( item_creator.account, {2}, item_creator.key );
         auto found_proposals = calc_proposals( proposal_idx, proposals_id );
         auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
         BOOST_REQUIRE( exist_proposal( 1 ) && find_proposal( 1 )->removed );
         BOOST_REQUIRE( exist_proposal( 2 ) && find_proposal( 2 )->removed );
         BOOST_REQUIRE( found_proposals == 3 );
         BOOST_REQUIRE( found_votes == 110 );
      }

      /*
         oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
         oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
         rr    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo rrr
         oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
         P4    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
      */
      {
         generate_block();
         generate_block();
         generate_block();

         auto found_proposals = calc_proposals( proposal_idx, proposals_id );
         auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
         BOOST_REQUIRE( !exist_proposal( 1 ) );
         BOOST_REQUIRE( exist_proposal( 2 ) && find_proposal( 2 )->removed );
         BOOST_REQUIRE( found_proposals == 2 );
         BOOST_REQUIRE( found_votes == 51 );
      }

      /*
         oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
         oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
         xx    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo xxx
         oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
         P4    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
      */
      {
         generate_block();

         auto found_proposals = calc_proposals( proposal_idx, proposals_id );
         auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
         BOOST_REQUIRE( !exist_proposal( 1 ) );
         BOOST_REQUIRE( !exist_proposal( 2 ) );
         BOOST_REQUIRE( found_proposals == 1 );
         BOOST_REQUIRE( found_votes == 50 );
      }

      /*
         oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
         oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
         xx    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo xxx
         oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
         P4    xx xx xx xx xx xx xx xx xx xx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
      */
      {
         remove_proposal( item_creator.account, {4}, item_creator.key );
         auto found_proposals = calc_proposals( proposal_idx, proposals_id );
         auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
         BOOST_REQUIRE( exist_proposal( 4 ) && find_proposal( 4 )->removed );
         BOOST_REQUIRE( found_proposals == 1 );
         BOOST_REQUIRE( found_votes == 30 );

         for( auto item : inits )
            vote_proposal( item.account, {4}, true/*approve*/, item.key);

         found_votes = calc_votes( proposal_vote_idx, proposals_id );
         BOOST_REQUIRE( found_votes == 30 );
      }

      /*
         oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
         oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
         rr    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo rrr
         oo    oo oo oo oo oo oo oo oo oo oo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo ooo
         P4    v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31 v32 v33 v34 v35 v36 v37 v38 v39 v40 v41 v42 v43 v44 v45 v46 v47 v48 v49
      */
      {
         generate_block();
         generate_block();

         auto found_proposals = calc_proposals( proposal_idx, proposals_id );
         auto found_votes = calc_votes( proposal_vote_idx, proposals_id );
         BOOST_REQUIRE( !exist_proposal( 4 ) );
         BOOST_REQUIRE( found_proposals == 0 );
         BOOST_REQUIRE( found_votes == 0 );

         for( auto item : inits )
            vote_proposal( item.account, {4}, true/*approve*/, item.key);

         found_votes = calc_votes( proposal_vote_idx, proposals_id );
         BOOST_REQUIRE( found_votes == 0 );
      }
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_FIXTURE_TEST_SUITE( proposal_tests_performance, sps_proposal_database_fixture_performance )

int32_t get_time( database& db, const std::string& name )
{
   int32_t benchmark_time = -1;

   db.get_benchmark_dumper().dump();

   fc::path benchmark_file( "advanced_benchmark.json" );

   if( !fc::exists( benchmark_file ) )
      return benchmark_time;

   /*
   {
   "total_time": 1421,
   "items": [{
         "op_name": "sps_processor",
         "time": 80
      },...
   */
   auto file = fc::json::from_file( benchmark_file ).as< fc::variant >();
   if( file.is_object() )
   {
      auto vo = file.get_object();
      if( vo.contains( "items" ) )
      {
         auto items = vo[ "items" ];
         if( items.is_array() )
         {
            auto v = items.as< std::vector< fc::variant > >();
            for( auto& item : v )
            {
               if( !item.is_object() )
                  continue;

               auto vo_item = item.get_object();
               if( vo_item.contains( "op_name" ) && vo_item[ "op_name" ].is_string() )
               {
                  std::string str = vo_item[ "op_name" ].as_string();
                  if( str == name )
                  {
                     if( vo_item.contains( "time" ) && vo_item[ "time" ].is_uint64() )
                     {
                        benchmark_time = vo_item[ "time" ].as_int64();
                        break;
                     }
                  }
               }
            }
         }
      }
   }
   return benchmark_time;
}

struct initial_data
{
   std::string account;

   fc::ecc::private_key key;

   initial_data( database_fixture* db, const std::string& _account ): account( _account )
   {
      key = db->generate_private_key( account );

      db->account_create( account, key.get_public_key(), db->generate_private_key( account + "_post" ).get_public_key() );
   }
};

std::vector< initial_data > generate_accounts( database_fixture* db, int32_t number_accounts )
{
   const std::string basic_name = "tester";

   std::vector< initial_data > res;

   for( int32_t i = 0; i< number_accounts; ++i  )
   {
      std::string name = basic_name + std::to_string( i );
      res.push_back( initial_data( db, name ) );

      if( ( i + 1 ) % 100 == 0 )
         db->generate_block();

      if( ( i + 1 ) % 1000 == 0 )
         ilog( "Created: ${accs} accounts",( "accs", i+1 ) );
   }

   db->validate_database();
   db->generate_block();

   return res;
}

BOOST_AUTO_TEST_CASE( proposals_removing_with_threshold_03 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: removing of all proposals/votes in one block using threshold = -1" );

      std::vector< initial_data > inits = generate_accounts( this, 200 );

      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      generate_block();

      //=====================preparing=====================
      auto receiver = "steem.dao";

      auto start_time = db->head_block_time();

      const auto start_time_shift = fc::hours( 20 );
      const auto end_time_shift = fc::hours( 6 );
      const auto block_interval = fc::seconds( STEEM_BLOCK_INTERVAL );

      auto start_date_00 = start_time + start_time_shift;
      auto end_date_00 = start_date_00 + end_time_shift;

      auto daily_pay = asset( 100, SBD_SYMBOL );

      const auto nr_proposals = 200;
      std::vector< int64_t > proposals_id;

      struct initial_data
      {
         std::string account;
         fc::ecc::private_key key;
      };

      for( auto item : inits )
         FUND( item.account, ASSET( "10000.000 TBD" ) );
      //=====================preparing=====================

      for( auto i = 0; i < nr_proposals; ++i )
      {
         auto item = inits[ i ];
         proposals_id.push_back( create_proposal( item.account, receiver, start_date_00, end_date_00, daily_pay, item.key ) );
         if( ( i + 1 ) % 10 == 0 )
            generate_block();
      }
      generate_block();

      std::vector< int64_t > ids;
      uint32_t i = 0;
      for( auto id : proposals_id )
      {
         ids.push_back( id );
         if( ids.size() == STEEM_PROPOSAL_MAX_IDS_NUMBER )
         {
            for( auto item : inits )
            {
               ++i;
               vote_proposal( item.account, ids, true/*approve*/, item.key );
               if( ( i + 1 ) % 10 == 0 )
                  generate_block();
            }
            ids.clear();
         }
      }
      generate_block();

      const auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_proposal_id >();
      const auto& proposal_vote_idx = db->get_index< proposal_vote_index >().indices().get< by_proposal_voter >();

      auto current_active_proposals = nr_proposals;
      BOOST_REQUIRE( calc_proposals( proposal_idx, proposals_id ) == current_active_proposals );

      auto current_active_votes = current_active_proposals * static_cast< int16_t > ( inits.size() );
      BOOST_REQUIRE( calc_votes( proposal_vote_idx, proposals_id ) == current_active_votes );

      auto threshold = db->get_sps_remove_threshold();
      BOOST_REQUIRE( threshold == -1 );

      generate_blocks( start_time + fc::seconds( STEEM_PROPOSAL_MAINTENANCE_CLEANUP ) );
      start_time = db->head_block_time();

      generate_blocks( start_time + ( start_time_shift + end_time_shift - block_interval ) );

      generate_blocks( 1 );

      BOOST_REQUIRE( calc_proposals( proposal_idx, proposals_id ) == 0 );
      BOOST_REQUIRE( calc_votes( proposal_vote_idx, proposals_id ) == 0 );

      int32_t benchmark_time = get_time( *db, sps_processor::get_removing_name() );
      BOOST_REQUIRE( benchmark_time == -1 || benchmark_time < 100 );

      /*
         Local test: 4 cores/16 MB RAM
         nr objects to be removed: `40200`
         time of removing: 80 ms
         speed of removal: ~502/ms
      */
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( generating_payments )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: generating payments for a lot of accounts" );

      std::vector< initial_data > inits = generate_accounts( this, 30000 );

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      generate_block();

      //=====================preparing=====================
      const auto nr_proposals = 5;
      std::vector< int64_t > proposals_id;
      flat_map< std::string, asset > before_tbds;

      auto call = [&]( uint32_t& i, uint32_t max, const std::string& info )
      {
         ++i;
         if( i % max == 0 )
            generate_block();

         if( i % 1000 == 0 )
            ilog( info.c_str(),( "x", i ) );
      };

      uint32_t i = 0;
      for( auto item : inits )
      {
         if( i < 5 )
            FUND( item.account, ASSET( "11.000 TBD" ) );
         vest(STEEM_INIT_MINER_NAME, item.account, ASSET( "30.000 TESTS" ));

         call( i, 50, "${x} accounts got VESTS" );
      }

      auto start_time = db->head_block_time();

      const auto start_time_shift = fc::hours( 10 );
      const auto end_time_shift = fc::hours( 1 );
      const auto block_interval = fc::seconds( STEEM_BLOCK_INTERVAL );

      auto start_date = start_time + start_time_shift;
      auto end_date = start_date + end_time_shift;

      auto daily_pay = ASSET( "24.000 TBD" );
      auto paid = ASSET( "1.000 TBD" );//because only 1 hour

      FUND( STEEM_TREASURY_ACCOUNT, ASSET( "5000000.000 TBD" ) );
      //=====================preparing=====================
      for( int32_t i = 0; i < nr_proposals; ++i )
      {
         auto item = inits[ i % inits.size() ];
         proposals_id.push_back( create_proposal( item.account, item.account, start_date, end_date, daily_pay, item.key ) );
         generate_block();
      }

      i = 0;
      for( auto item : inits )
      {
         vote_proposal( item.account, proposals_id, true/*approve*/, item.key );

         call( i, 100, "${x} accounts voted" );
      }

      for( int32_t i = 0; i < nr_proposals; ++i )
      {
         auto item = inits[ i % inits.size() ];
         const account_object& account = db->get_account( item.account );
         before_tbds[ item.account ] = account.sbd_balance;
      }

      generate_blocks( start_time + ( start_time_shift - block_interval ) );
      start_time = db->head_block_time();
      generate_blocks( start_time + end_time_shift + block_interval );

      for( int32_t i = 0; i < nr_proposals; ++i )
      {
         auto item = inits[ i % inits.size() ];
         const account_object& account = db->get_account( item.account );

         auto after_tbd = account.sbd_balance;
         auto before_tbd = before_tbds[ item.account ];
         BOOST_REQUIRE( before_tbd == after_tbd - paid );
      }

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()