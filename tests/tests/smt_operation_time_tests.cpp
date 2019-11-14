
#if defined IS_TEST_NET

#include <boost/test/unit_test.hpp>

#include <steem/chain/steem_fwd.hpp>

#include <steem/protocol/exceptions.hpp>
#include <steem/protocol/hardfork.hpp>
#include <steem/protocol/smt_util.hpp>

#include <steem/chain/block_summary_object.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/history_object.hpp>
#include <steem/chain/steem_objects.hpp>
#include <steem/chain/smt_objects.hpp>

#include <steem/chain/util/reward.hpp>
#include <steem/plugins/rc/rc_objects.hpp>
#include <steem/plugins/rc/rc_operations.hpp>
#include <steem/plugins/rc/rc_plugin.hpp>
#include <steem/plugins/debug_node/debug_node_plugin.hpp>

#include <fc/crypto/digest.hpp>

#include "../db_fixture/database_fixture.hpp"

#include <cmath>

using namespace steem;
using namespace steem::chain;
using namespace steem::protocol;

BOOST_FIXTURE_TEST_SUITE( smt_operation_time_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( smt_refunds )
{
   try
   {

      BOOST_TEST_MESSAGE( "Testing SMT contribution refunds" );
      ACTORS( (alice)(bob)(sam)(dave) )

      generate_block();

      auto bobs_balance  = asset( 1000000, STEEM_SYMBOL );
      auto sams_balance  = asset( 800000, STEEM_SYMBOL );
      auto daves_balance = asset( 600000, STEEM_SYMBOL );

      FUND( "bob", bobs_balance );
      FUND( "sam", sams_balance );
      FUND( "dave", daves_balance );

      generate_block();

      BOOST_TEST_MESSAGE( " --- SMT creation" );
      auto symbol = create_smt( "alice", alice_private_key, 3 );
      const auto& token = db->get< smt_token_object, by_symbol >( symbol );

      BOOST_TEST_MESSAGE( " --- SMT setup" );
      signed_transaction tx;
      smt_setup_operation setup_op;

      uint64_t contribution_window_blocks = 10;
      setup_op.control_account = "alice";
      setup_op.symbol = symbol;
      setup_op.contribution_begin_time = db->head_block_time() + STEEM_BLOCK_INTERVAL;
      setup_op.contribution_end_time = setup_op.contribution_begin_time + ( STEEM_BLOCK_INTERVAL * contribution_window_blocks );
      setup_op.steem_units_min      = 2400001;
      setup_op.steem_units_soft_cap = 2400001;
      setup_op.steem_units_hard_cap = 4000000;
      setup_op.max_supply = STEEM_MAX_SHARE_SUPPLY;
      setup_op.launch_time = setup_op.contribution_end_time + STEEM_BLOCK_INTERVAL;
      setup_op.initial_generation_policy = get_capped_generation_policy
      (
         get_generation_unit( { { "alice", 1 } }, { { "alice", 2 } } ), /* pre_soft_cap_unit */
         get_generation_unit( { { "alice", 1 } }, { { "alice", 2 } } ), /* post_soft_cap_unit */
         1,                                                             /* min_unit_ratio */
         2                                                              /* max_unit_ratio */
      );

      tx.operations.push_back( setup_op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );
      tx.operations.clear();
      tx.signatures.clear();

      BOOST_REQUIRE( token.phase == smt_phase::setup_completed );

      generate_block();

      BOOST_REQUIRE( token.phase == smt_phase::ico );

      BOOST_TEST_MESSAGE( " --- SMT contributions" );
      uint32_t num_contributions = 0;
      for ( uint64_t i = 0; i < contribution_window_blocks; i++ )
      {
         smt_contribute_operation contrib_op;

         contrib_op.symbol = symbol;
         contrib_op.contribution_id = i;

         contrib_op.contributor = "bob";
         contrib_op.contribution = asset( bobs_balance.amount / contribution_window_blocks, STEEM_SYMBOL );


         tx.operations.push_back( contrib_op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         sign( tx, bob_private_key );
         db->push_transaction( tx, 0 );
         tx.operations.clear();
         tx.signatures.clear();
         num_contributions++;

         contrib_op.contributor = "sam";
         contrib_op.contribution = asset( sams_balance.amount / contribution_window_blocks, STEEM_SYMBOL );

         tx.operations.push_back( contrib_op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         sign( tx, sam_private_key );
         db->push_transaction( tx, 0 );
         tx.operations.clear();
         tx.signatures.clear();
         num_contributions++;

         contrib_op.contributor = "dave";
         contrib_op.contribution = asset( daves_balance.amount / contribution_window_blocks, STEEM_SYMBOL );

         tx.operations.push_back( contrib_op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         sign( tx, dave_private_key );
         db->push_transaction( tx, 0 );
         tx.operations.clear();
         tx.signatures.clear();
         num_contributions++;

         if ( i < contribution_window_blocks - 1 )
            generate_block();

         validate_database();
      }

      BOOST_REQUIRE( token.phase == smt_phase::ico );

      BOOST_TEST_MESSAGE( " --- Checking contributor balances" );

      BOOST_REQUIRE( db->get_balance( "bob", STEEM_SYMBOL ) == asset( 0, STEEM_SYMBOL ) );
      BOOST_REQUIRE( db->get_balance( "sam", STEEM_SYMBOL ) == asset( 0, STEEM_SYMBOL ) );
      BOOST_REQUIRE( db->get_balance( "dave", STEEM_SYMBOL ) == asset( 0, STEEM_SYMBOL ) );

      generate_block();

      BOOST_REQUIRE( token.phase == smt_phase::launch_failed );

      validate_database();

      BOOST_TEST_MESSAGE( " --- Starting the cascading refunds" );

      generate_blocks( num_contributions / 2 );

      validate_database();

      generate_blocks( num_contributions / 2 + 1 );

      BOOST_TEST_MESSAGE( " --- Checking contributor balances" );

      BOOST_REQUIRE( db->get_balance( "bob", STEEM_SYMBOL ) == bobs_balance );
      BOOST_REQUIRE( db->get_balance( "sam", STEEM_SYMBOL ) == sams_balance );
      BOOST_REQUIRE( db->get_balance( "dave", STEEM_SYMBOL ) == daves_balance );

      validate_database();

      auto& ico_idx = db->get_index< smt_ico_index, by_symbol >();
      BOOST_REQUIRE( ico_idx.find( symbol ) == ico_idx.end() );
      auto& contribution_idx = db->get_index< smt_contribution_index, by_symbol_id >();
      BOOST_REQUIRE( contribution_idx.find( boost::make_tuple( symbol, 0 ) ) == contribution_idx.end() );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_ico_payouts )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing SMT ICO payouts" );
      ACTORS( (creator)(alice)(bob)(charlie)(dan)(elaine)(fred)(george)(henry) )

      generate_block();

      auto alices_balance    = asset( 5000000, STEEM_SYMBOL );
      auto bobs_balance      = asset( 25000000, STEEM_SYMBOL );
      auto charlies_balance  = asset( 10000000, STEEM_SYMBOL );
      auto dans_balance      = asset( 25000000, STEEM_SYMBOL );
      auto elaines_balance   = asset( 60000000, STEEM_SYMBOL );
      auto freds_balance     = asset( 0, STEEM_SYMBOL );
      auto georges_balance   = asset( 0, STEEM_SYMBOL );
      auto henrys_balance    = asset( 0, STEEM_SYMBOL );

      std::map< std ::string, std::tuple< share_type, fc::ecc::private_key > > contributor_contributions {
         { "alice",   { alices_balance.amount,   alice_private_key   } },
         { "bob",     { bobs_balance.amount,     bob_private_key     } },
         { "charlie", { charlies_balance.amount, charlie_private_key } },
         { "dan",     { dans_balance.amount,     dan_private_key     } },
         { "elaine",  { elaines_balance.amount,  elaine_private_key  } },
         { "fred",    { freds_balance.amount,    fred_private_key    } },
         { "george",  { georges_balance.amount,  george_private_key  } },
         { "henry",   { henrys_balance.amount,   henry_private_key   } }
      };

      for ( auto& e : contributor_contributions )
      {
         FUND( e.first, asset( std::get< 0 >( e.second ), STEEM_SYMBOL ) );
      }

      generate_block();

      BOOST_TEST_MESSAGE( " --- SMT creation" );
      auto symbol = create_smt( "creator", creator_private_key, 3 );
      const auto& token = db->get< smt_token_object, by_symbol >( symbol );

      BOOST_TEST_MESSAGE( " --- SMT setup" );
      signed_transaction tx;
      smt_setup_operation setup_op;

      uint64_t contribution_window_blocks = 5;
      setup_op.control_account = "creator";
      setup_op.symbol = symbol;
      setup_op.contribution_begin_time = db->head_block_time() + STEEM_BLOCK_INTERVAL;
      setup_op.contribution_end_time = setup_op.contribution_begin_time + ( STEEM_BLOCK_INTERVAL * contribution_window_blocks );
      setup_op.steem_units_min      = 0;
      setup_op.steem_units_soft_cap = 100000000;
      setup_op.steem_units_hard_cap = 150000000;
      setup_op.max_supply = STEEM_MAX_SHARE_SUPPLY;
      setup_op.launch_time = setup_op.contribution_end_time + STEEM_BLOCK_INTERVAL;
      setup_op.initial_generation_policy = get_capped_generation_policy
      (
         get_generation_unit(
         {
            { "fred", 3 },
            { "george", 2 }
         },
         {
            { SMT_DESTINATION_FROM, 7 },
            { "george", 1 },
            { "henry", 2 }
         } ), /* pre_soft_cap_unit */
         get_generation_unit(
         {
            { "fred", 3 },
            { "george", 2 }
         },
         {
            { SMT_DESTINATION_FROM, 7 },
            { "george", 1 },
            { "henry", 2 }
         } ), /* post_soft_cap_unit */
         50,                                                            /* min_unit_ratio */
         100                                                            /* max_unit_ratio */
      );

      tx.operations.push_back( setup_op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, creator_private_key );
      db->push_transaction( tx, 0 );
      tx.operations.clear();
      tx.signatures.clear();

      BOOST_REQUIRE( token.phase == smt_phase::setup_completed );

      generate_block();

      BOOST_REQUIRE( token.phase == smt_phase::ico );

      BOOST_TEST_MESSAGE( " --- SMT contributions" );

      uint32_t num_contributions = 0;
      for ( auto& e : contributor_contributions )
      {
         if ( std::get< 0 >( e.second ) == 0 )
            continue;

         smt_contribute_operation contrib_op;

         contrib_op.symbol = symbol;
         contrib_op.contribution_id = 0;
         contrib_op.contributor = e.first;
         contrib_op.contribution = asset( std::get< 0 >( e.second ), STEEM_SYMBOL );

         tx.operations.push_back( contrib_op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         sign( tx, std::get< 1 >( e.second ) );
         db->push_transaction( tx, 0 );
         tx.operations.clear();
         tx.signatures.clear();

         generate_block();
         num_contributions++;
      }

      validate_database();

      generate_block();

      BOOST_REQUIRE( token.phase == smt_phase::launch_success );

      BOOST_TEST_MESSAGE( " --- Starting the cascading payouts" );

      generate_blocks( num_contributions / 2 );

      validate_database();

      generate_blocks( num_contributions / 2 + 1 );

      BOOST_TEST_MESSAGE( " --- Checking contributor balances" );

      BOOST_REQUIRE( db->get_balance( "alice", STEEM_SYMBOL ).amount == 0 );
      BOOST_REQUIRE( db->get_balance( "bob", STEEM_SYMBOL ).amount == 0 );
      BOOST_REQUIRE( db->get_balance( "charlie", STEEM_SYMBOL ).amount == 0 );
      BOOST_REQUIRE( db->get_balance( "dan", STEEM_SYMBOL ).amount == 0 );
      BOOST_REQUIRE( db->get_balance( "elaine", STEEM_SYMBOL ).amount == 0 );
      BOOST_REQUIRE( db->get_balance( "fred", STEEM_SYMBOL ).amount == 75000000 );
      BOOST_REQUIRE( db->get_balance( "george", STEEM_SYMBOL ).amount == 50000000 );
      BOOST_REQUIRE( db->get_balance( "henry", STEEM_SYMBOL ).amount == 0 );

      BOOST_REQUIRE( db->get_balance( "alice", symbol ).amount == 420000000 );
      BOOST_REQUIRE( db->get_balance( "bob", symbol ).amount == 2100000000 );
      BOOST_REQUIRE( db->get_balance( "charlie", symbol ).amount == 840000000 );
      BOOST_REQUIRE( db->get_balance( "dan", symbol ).amount == 2100000000 );
      BOOST_REQUIRE( db->get_balance( "elaine", symbol ).amount == 5040000000 );
      BOOST_REQUIRE( db->get_balance( "fred", symbol ).amount == 0 );
      BOOST_REQUIRE( db->get_balance( "george", symbol ).amount == 1500000000 );
      BOOST_REQUIRE( db->get_balance( "henry", symbol ).amount == 3000000000 );

      validate_database();

      auto& ico_idx = db->get_index< smt_ico_index, by_symbol >();
      BOOST_REQUIRE( ico_idx.find( symbol ) == ico_idx.end() );
      auto& contribution_idx = db->get_index< smt_contribution_index, by_symbol_id >();
      BOOST_REQUIRE( contribution_idx.find( boost::make_tuple( symbol, 0 ) ) == contribution_idx.end() );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_ico_payouts_special_destinations )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing SMT ICO payouts special destinations" );
      ACTORS( (creator)(alice)(bob)(charlie)(dan)(elaine)(fred)(george)(henry) )

      generate_block();

      auto alices_balance    = asset( 5000000, STEEM_SYMBOL );
      auto bobs_balance      = asset( 25000000, STEEM_SYMBOL );
      auto charlies_balance  = asset( 10000000, STEEM_SYMBOL );
      auto dans_balance      = asset( 25000000, STEEM_SYMBOL );
      auto elaines_balance   = asset( 60000000, STEEM_SYMBOL );
      auto freds_balance     = asset( 0, STEEM_SYMBOL );
      auto georges_balance   = asset( 0, STEEM_SYMBOL );
      auto henrys_balance    = asset( 0, STEEM_SYMBOL );

      std::map< std ::string, std::tuple< share_type, fc::ecc::private_key > > contributor_contributions {
         { "alice",   { alices_balance.amount,   alice_private_key   } },
         { "bob",     { bobs_balance.amount,     bob_private_key     } },
         { "charlie", { charlies_balance.amount, charlie_private_key } },
         { "dan",     { dans_balance.amount,     dan_private_key     } },
         { "elaine",  { elaines_balance.amount,  elaine_private_key  } },
         { "fred",    { freds_balance.amount,    fred_private_key    } },
         { "george",  { georges_balance.amount,  george_private_key  } },
         { "henry",   { henrys_balance.amount,   henry_private_key   } }
      };

      for ( auto& e : contributor_contributions )
      {
         FUND( e.first, asset( std::get< 0 >( e.second ), STEEM_SYMBOL ) );
      }

      generate_block();

      BOOST_TEST_MESSAGE( " --- SMT creation" );
      auto symbol = create_smt( "creator", creator_private_key, 3 );
      const auto& token = db->get< smt_token_object, by_symbol >( symbol );

      BOOST_TEST_MESSAGE( " --- SMT setup" );
      signed_transaction tx;
      smt_setup_operation setup_op;

      uint64_t contribution_window_blocks = 5;
      setup_op.control_account = "creator";
      setup_op.symbol = symbol;
      setup_op.contribution_begin_time = db->head_block_time() + STEEM_BLOCK_INTERVAL;
      setup_op.contribution_end_time = setup_op.contribution_begin_time + ( STEEM_BLOCK_INTERVAL * contribution_window_blocks );
      setup_op.steem_units_min      = 0;
      setup_op.steem_units_soft_cap = 100000000;
      setup_op.steem_units_hard_cap = 150000000;
      setup_op.max_supply = STEEM_MAX_SHARE_SUPPLY;
      setup_op.launch_time = setup_op.contribution_end_time + STEEM_BLOCK_INTERVAL;
      setup_op.initial_generation_policy = get_capped_generation_policy
      (
         get_generation_unit(
         {
            { SMT_DESTINATION_MARKET_MAKER, 3 },
            { "george", 2 }
         },
         {
            { SMT_DESTINATION_FROM, 7 },
            { SMT_DESTINATION_MARKET_MAKER, 1 },
            { SMT_DESTINATION_REWARDS, 2 }
         } ), /* pre_soft_cap_unit */
         get_generation_unit(
         {
            { SMT_DESTINATION_MARKET_MAKER, 3 },
            { "$!george.vesting", 2 }
         },
         {
            { SMT_DESTINATION_FROM, 5 },
            { SMT_DESTINATION_MARKET_MAKER, 1 },
            { SMT_DESTINATION_REWARDS, 2 },
            { "$!george.vesting", 2 }
         } ), /* post_soft_cap_unit */
         50,                                                            /* min_unit_ratio */
         100                                                            /* max_unit_ratio */
      );

      tx.operations.push_back( setup_op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, creator_private_key );
      db->push_transaction( tx, 0 );
      tx.operations.clear();
      tx.signatures.clear();

      BOOST_REQUIRE( token.phase == smt_phase::setup_completed );

      generate_block();

      BOOST_REQUIRE( token.phase == smt_phase::ico );

      BOOST_TEST_MESSAGE( " --- SMT contributions" );

      uint32_t num_contributions = 0;
      for ( auto& e : contributor_contributions )
      {
         if ( std::get< 0 >( e.second ) == 0 )
            continue;

         smt_contribute_operation contrib_op;

         contrib_op.symbol = symbol;
         contrib_op.contribution_id = 0;
         contrib_op.contributor = e.first;
         contrib_op.contribution = asset( std::get< 0 >( e.second ), STEEM_SYMBOL );

         tx.operations.push_back( contrib_op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         sign( tx, std::get< 1 >( e.second ) );
         db->push_transaction( tx, 0 );
         tx.operations.clear();
         tx.signatures.clear();

         generate_block();
         num_contributions++;
      }

      validate_database();

      generate_block();

      BOOST_REQUIRE( token.phase == smt_phase::launch_success );

      BOOST_TEST_MESSAGE( " --- Starting the cascading payouts" );

      generate_blocks( num_contributions / 2 );

      validate_database();

      generate_blocks( num_contributions / 2 + 1 );

      BOOST_TEST_MESSAGE( " --- Checking contributor balances" );

      BOOST_REQUIRE( db->get_balance( "alice", STEEM_SYMBOL ).amount == 0 );
      BOOST_REQUIRE( db->get_balance( "bob", STEEM_SYMBOL ).amount == 0 );
      BOOST_REQUIRE( db->get_balance( "charlie", STEEM_SYMBOL ).amount == 0 );
      BOOST_REQUIRE( db->get_balance( "dan", STEEM_SYMBOL ).amount == 0 );
      BOOST_REQUIRE( db->get_balance( "elaine", STEEM_SYMBOL ).amount == 0 );
      BOOST_REQUIRE( db->get_balance( "fred", STEEM_SYMBOL ).amount == 0 );
      BOOST_REQUIRE( db->get_balance( "george", STEEM_SYMBOL ).amount == 40000000 );
      BOOST_REQUIRE( db->get_balance( "henry", STEEM_SYMBOL ).amount == 0 );

      BOOST_REQUIRE( db->get_account( "george" ).vesting_shares.amount == 5076086140430482 );

      BOOST_REQUIRE( db->get_balance( "alice", symbol ).amount == 420000000 );
      BOOST_REQUIRE( db->get_balance( "bob", symbol ).amount == 2100000000 );
      BOOST_REQUIRE( db->get_balance( "charlie", symbol ).amount == 840000000 );
      BOOST_REQUIRE( db->get_balance( "dan", symbol ).amount == 2100000000 );
      BOOST_REQUIRE( db->get_balance( "elaine", symbol ).amount == 4440000000 );
      BOOST_REQUIRE( db->get_balance( "fred", symbol ).amount == 0 );
      BOOST_REQUIRE( db->get_balance( "george", symbol ).amount == 0 );
      BOOST_REQUIRE( db->get_balance( "henry", symbol ).amount == 0 );

      BOOST_REQUIRE( db->get_balance( "george", symbol.get_paired_symbol() ).amount == 600000000000000 );

      BOOST_TEST_MESSAGE( " --- Checking market maker and rewards fund balances" );

      BOOST_REQUIRE( token.market_maker.steem_balance == asset( 75000000, STEEM_SYMBOL ) );
      BOOST_REQUIRE( token.market_maker.token_balance == asset( 1500000000, symbol ) );
      BOOST_REQUIRE( token.reward_balance == asset( 3000000000, symbol ) );

      validate_database();

      auto& ico_idx = db->get_index< smt_ico_index, by_symbol >();
      BOOST_REQUIRE( ico_idx.find( symbol ) == ico_idx.end() );
      auto& contribution_idx = db->get_index< smt_contribution_index, by_symbol_id >();
      BOOST_REQUIRE( contribution_idx.find( boost::make_tuple( symbol, 0 ) ) == contribution_idx.end() );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_vesting_withdrawals )
{
   BOOST_TEST_MESSAGE( "Testing: SMT vesting withdrawals" );

   ACTORS( (alice)(creator) )
   generate_block();

   auto symbol = create_smt( "creator", creator_private_key, 3 );
   fund( "alice", asset( 100000, symbol ) );
   vest( "alice", asset( 100000, symbol ) );

   const auto& token = db->get< smt_token_object, by_symbol >( symbol );

   auto key = boost::make_tuple( "alice", symbol );
   const auto* balance_obj = db->find< account_regular_balance_object, by_name_liquid_symbol >( key );
   BOOST_REQUIRE( balance_obj != nullptr );

   BOOST_TEST_MESSAGE( " -- Setting up withdrawal" );

   signed_transaction tx;
   withdraw_vesting_operation op;
   op.account = "alice";
   op.vesting_shares = asset( balance_obj->vesting_shares.amount / 2, symbol.get_paired_symbol() );
   tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
   tx.operations.push_back( op );
   sign( tx, alice_private_key );
   db->push_transaction( tx, 0 );

   auto next_withdrawal = db->head_block_time() + SMT_VESTING_WITHDRAW_INTERVAL_SECONDS;
   asset vesting_shares = balance_obj->vesting_shares;
   asset original_vesting = vesting_shares;
   asset withdraw_rate = balance_obj->vesting_withdraw_rate;

   BOOST_TEST_MESSAGE( " -- Generating block up to first withdrawal" );
   generate_blocks( next_withdrawal - ( STEEM_BLOCK_INTERVAL / 2 ), true);

   BOOST_REQUIRE( balance_obj->vesting_shares.amount.value == vesting_shares.amount.value );

   BOOST_TEST_MESSAGE( " -- Generating block to cause withdrawal" );
   generate_block();

   auto fill_op = get_last_operations( 1 )[ 0 ].get< fill_vesting_withdraw_operation >();

   BOOST_REQUIRE( balance_obj->vesting_shares.amount.value == ( vesting_shares - withdraw_rate ).amount.value );
   BOOST_REQUIRE( ( withdraw_rate * token.get_vesting_share_price() ).amount.value - balance_obj->liquid.amount.value <= 1 ); // Check a range due to differences in the share price
   BOOST_REQUIRE( fill_op.from_account == "alice" );
   BOOST_REQUIRE( fill_op.to_account == "alice" );
   BOOST_REQUIRE( fill_op.withdrawn.amount.value == withdraw_rate.amount.value );
   BOOST_REQUIRE( std::abs( ( fill_op.deposited - fill_op.withdrawn * token.get_vesting_share_price() ).amount.value ) <= 1 );
   validate_database();

   BOOST_TEST_MESSAGE( " -- Generating the rest of the blocks in the withdrawal" );

   vesting_shares = balance_obj->vesting_shares;
   auto balance = balance_obj->liquid;
   auto old_next_vesting = balance_obj->next_vesting_withdrawal;

   for( int i = 1; i < STEEM_VESTING_WITHDRAW_INTERVALS - 1; i++ )
   {
      generate_blocks( db->head_block_time() + SMT_VESTING_WITHDRAW_INTERVAL_SECONDS );

      fill_op = get_last_operations( 2 )[ 1 ].get< fill_vesting_withdraw_operation >();

      BOOST_REQUIRE( balance_obj->vesting_shares.amount.value == ( vesting_shares - withdraw_rate ).amount.value );
      BOOST_REQUIRE( balance.amount.value + ( withdraw_rate * token.get_vesting_share_price() ).amount.value - balance_obj->liquid.amount.value <= 1 );
      BOOST_REQUIRE( fill_op.from_account == "alice" );
      BOOST_REQUIRE( fill_op.to_account == "alice" );
      BOOST_REQUIRE( fill_op.withdrawn.amount.value == withdraw_rate.amount.value );
      BOOST_REQUIRE( std::abs( ( fill_op.deposited - fill_op.withdrawn * token.get_vesting_share_price() ).amount.value ) <= 1 );

      if ( i == STEEM_VESTING_WITHDRAW_INTERVALS - 1 )
         BOOST_REQUIRE( balance_obj->next_vesting_withdrawal == fc::time_point_sec::maximum() );
      else
         BOOST_REQUIRE( balance_obj->next_vesting_withdrawal.sec_since_epoch() == ( old_next_vesting + SMT_VESTING_WITHDRAW_INTERVAL_SECONDS ).sec_since_epoch() );

      validate_database();

      vesting_shares = balance_obj->vesting_shares;
      balance = balance_obj->liquid;
      old_next_vesting = balance_obj->next_vesting_withdrawal;
   }

   BOOST_TEST_MESSAGE( " -- Generating one more block to finish vesting withdrawal" );
   generate_blocks( db->head_block_time() + SMT_VESTING_WITHDRAW_INTERVAL_SECONDS, true );

   BOOST_REQUIRE( balance_obj->next_vesting_withdrawal.sec_since_epoch() == fc::time_point_sec::maximum().sec_since_epoch() );
   BOOST_REQUIRE( balance_obj->vesting_shares.amount.value == ( original_vesting - op.vesting_shares ).amount.value );

   validate_database();
}

BOOST_AUTO_TEST_CASE( recent_claims_decay )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: recent_rshares_2decay" );
      ACTORS( (alice)(bob)(charlie) )
      generate_block();

      auto alice_symbol = create_smt( "alice", alice_private_key, 3 );
      auto bob_symbol = create_smt( "bob", bob_private_key, 3 );
      auto charlie_symbol = create_smt( "charlie", charlie_private_key, 3 );

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      generate_block();

      uint64_t alice_recent_claims = 1000000ull;
      uint64_t bob_recent_claims = 1000000ull;
      uint64_t charlie_recent_claims = 1000000ull;
      time_point_sec last_claims_update = db->head_block_time();

      db_plugin->debug_update( [=]( database& db )
      {
         auto alice_vests = db.create_vesting( db.get_account( "alice" ), asset( 100000, alice_symbol ), false );
         auto bob_vests = db.create_vesting( db.get_account( "bob" ), asset( 100000, bob_symbol ), false );
         auto now = db.head_block_time();

         db.modify( db.get< smt_token_object, by_symbol >( alice_symbol ), [=]( smt_token_object& smt )
         {
            smt.phase = smt_phase::launch_success;
            smt.current_supply = 100000;
            smt.total_vesting_shares = alice_vests.amount;
            smt.total_vesting_fund_smt = 100000;
            smt.votes_per_regeneration_period = 50;
            smt.vote_regeneration_period_seconds = 5*24*60*60;
            smt.recent_claims = uint128_t( 0, alice_recent_claims );
            smt.last_reward_update = now;
         });

         db.modify( db.get< smt_token_object, by_symbol >( bob_symbol ), [=]( smt_token_object& smt )
         {
            smt.phase = smt_phase::launch_success;
            smt.current_supply = 100000;
            smt.total_vesting_shares = bob_vests.amount;
            smt.total_vesting_fund_smt = 100000;
            smt.votes_per_regeneration_period = 50;
            smt.vote_regeneration_period_seconds = 5*24*60*60;
            smt.recent_claims = uint128_t( 0, bob_recent_claims );
            smt.last_reward_update = now;
            smt.author_reward_curve = curve_id::linear;
         });

         db.modify( db.get< smt_token_object, by_symbol >( charlie_symbol ), [=]( smt_token_object& smt )
         {
            smt.phase = smt_phase::launch_success;
            smt.current_supply = 0;
            smt.total_vesting_shares = 0;
            smt.total_vesting_fund_smt = 0;
            smt.votes_per_regeneration_period = 50;
            smt.vote_regeneration_period_seconds = 5*24*60*60;
            smt.recent_claims = uint128_t( 0, charlie_recent_claims );
            smt.last_reward_update = now;
         });
      });
      generate_block();

      comment_operation comment;
      vote2_operation vote;
      signed_transaction tx;

      comment.author = "alice";
      comment.permlink = "test";
      comment.parent_permlink = "test";
      comment.title = "foo";
      comment.body = "bar";
      allowed_vote_assets ava;
      ava.votable_assets[ alice_symbol ] = votable_asset_options();
      ava.votable_assets[ bob_symbol ] = votable_asset_options();
      comment_options_operation comment_opts;
      comment_opts.author = "alice";
      comment_opts.permlink = "test";
      comment_opts.extensions.insert( ava );
      vote.voter = "alice";
      vote.author = "alice";
      vote.permlink = "test";
      vote.rshares[ STEEM_SYMBOL ] = 10000 + STEEM_VOTE_DUST_THRESHOLD;
      vote.rshares[ alice_symbol ] = -5000 - STEEM_VOTE_DUST_THRESHOLD;
      tx.operations.push_back( comment );
      tx.operations.push_back( comment_opts );
      tx.operations.push_back( vote );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );

      auto alice_vshares = util::evaluate_reward_curve( db->get_comment( "alice", string( "test" ) ).net_rshares.value,
         db->get< reward_fund_object, by_name >( STEEM_POST_REWARD_FUND_NAME ).author_reward_curve,
         db->get< reward_fund_object, by_name >( STEEM_POST_REWARD_FUND_NAME ).content_constant );

      generate_blocks( 5 );

      comment.author = "bob";
      ava.votable_assets.clear();
      ava.votable_assets[ bob_symbol ] = votable_asset_options();
      comment_opts.author = "bob";
      comment_opts.extensions.clear();
      comment_opts.extensions.insert( ava );
      vote.voter = "bob";
      vote.author = "bob";
      vote.rshares.clear();
      vote.rshares[ STEEM_SYMBOL ] = 10000 + STEEM_VOTE_DUST_THRESHOLD;
      vote.rshares[ bob_symbol ] = 5000 + STEEM_VOTE_DUST_THRESHOLD;
      tx.clear();
      tx.operations.push_back( comment );
      tx.operations.push_back( comment_opts );
      tx.operations.push_back( vote );
      sign( tx, bob_private_key );
      db->push_transaction( tx, 0 );

      generate_blocks( db->get_comment( "alice", string( "test" ) ).cashout_time );

      {
         const auto& post_rf = db->get< reward_fund_object, by_name >( STEEM_POST_REWARD_FUND_NAME );
         BOOST_REQUIRE( post_rf.recent_claims == alice_vshares );

         fc::microseconds decay_time = STEEM_RECENT_RSHARES_DECAY_TIME_HF19;
         const auto& alice_rf = db->get< smt_token_object, by_symbol >( alice_symbol );
         alice_recent_claims -= ( ( db->head_block_time() - last_claims_update ).to_seconds() * alice_recent_claims ) / decay_time.to_seconds();

         BOOST_REQUIRE( alice_rf.recent_claims.to_uint64() == alice_recent_claims );
         BOOST_REQUIRE( alice_rf.last_reward_update == db->head_block_time() );

         const auto& bob_rf = db->get< smt_token_object, by_symbol >( bob_symbol );
         BOOST_REQUIRE( bob_rf.recent_claims.to_uint64() == bob_recent_claims );
         BOOST_REQUIRE( bob_rf.last_reward_update == last_claims_update );

         const auto& charlie_rf = db->get< smt_token_object, by_symbol >( charlie_symbol );
         BOOST_REQUIRE( charlie_rf.recent_claims.to_uint64() == charlie_recent_claims );
         BOOST_REQUIRE( charlie_rf.last_reward_update == last_claims_update );

         validate_database();
      }

      auto bob_cashout_time = db->get_comment( "bob", string( "test" ) ).cashout_time;
      auto bob_vshares = util::evaluate_reward_curve( db->get_comment( "bob", string( "test" ) ).net_rshares.value,
         db->get< reward_fund_object, by_name >( STEEM_POST_REWARD_FUND_NAME ).author_reward_curve,
         db->get< reward_fund_object, by_name >( STEEM_POST_REWARD_FUND_NAME ).content_constant );

      generate_block();

      while( db->head_block_time() < bob_cashout_time )
      {
         alice_vshares -= ( alice_vshares * STEEM_BLOCK_INTERVAL ) / STEEM_RECENT_RSHARES_DECAY_TIME_HF19.to_seconds();
         const auto& post_rf = db->get< reward_fund_object, by_name >( STEEM_POST_REWARD_FUND_NAME );

         BOOST_REQUIRE( post_rf.recent_claims == alice_vshares );

         generate_block();
      }

      {
         alice_vshares -= ( alice_vshares * STEEM_BLOCK_INTERVAL ) / STEEM_RECENT_RSHARES_DECAY_TIME_HF19.to_seconds();
         const auto& post_rf = db->get< reward_fund_object, by_name >( STEEM_POST_REWARD_FUND_NAME );

         BOOST_REQUIRE( post_rf.recent_claims == alice_vshares + bob_vshares );

         const auto& alice_rf = db->get< smt_token_object, by_symbol >( alice_symbol );
         BOOST_REQUIRE( alice_rf.recent_claims.to_uint64() == alice_recent_claims );

         const auto& bob_rf = db->get< smt_token_object, by_symbol >( bob_symbol );
         fc::microseconds decay_time = STEEM_RECENT_RSHARES_DECAY_TIME_HF19;
         bob_recent_claims -= ( ( db->head_block_time() - last_claims_update ).to_seconds() * bob_recent_claims ) / decay_time.to_seconds();
         bob_recent_claims += util::evaluate_reward_curve(
            vote.rshares[ bob_symbol ] - STEEM_VOTE_DUST_THRESHOLD,
            bob_rf.author_reward_curve,
            bob_rf.content_constant ).to_uint64();

         BOOST_REQUIRE( bob_rf.recent_claims.to_uint64() == bob_recent_claims );
         BOOST_REQUIRE( bob_rf.last_reward_update == db->head_block_time() );

         const auto& charlie_rf = db->get< smt_token_object, by_symbol >( charlie_symbol );
         BOOST_REQUIRE( charlie_rf.recent_claims.to_uint64() == charlie_recent_claims );
         BOOST_REQUIRE( charlie_rf.last_reward_update == last_claims_update );

         validate_database();
      }
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_rewards )
{
   try
   {
      ACTORS( (alice)(bob)(charlie)(dave) )
      fund( "alice", 100000 );
      vest( "alice", 100000 );
      fund( "bob", 100000 );
      vest( "bob", 100000 );
      fund( "charlie", 100000 );
      vest( "charlie", 100000 );
      fund( "dave", 100000 );
      vest( "dave", 100000 );

      auto alice_symbol = create_smt( "alice", alice_private_key, 3 );

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      generate_block();

      uint64_t recent_claims = 10000000000ull;

      db_plugin->debug_update( [=]( database& db )
      {
         auto alice_smt_vests = db.create_vesting( db.get_account( "alice" ), asset( 1000000, alice_symbol ), false );
         alice_smt_vests     += db.create_vesting( db.get_account( "bob" ), asset( 1000000, alice_symbol ), false );
         alice_smt_vests     += db.create_vesting( db.get_account( "charlie" ), asset( 1000000, alice_symbol ), false );
         alice_smt_vests     += db.create_vesting( db.get_account( "dave" ), asset( 1000000, alice_symbol ), false );
         auto now = db.head_block_time();

         db.modify( db.get< smt_token_object, by_symbol >( alice_symbol ), [&]( smt_token_object& smt )
         {
            smt.phase = smt_phase::launch_success;
            smt.current_supply = 4000000;
            smt.total_vesting_shares = alice_smt_vests.amount;
            smt.total_vesting_fund_smt = 4000000;
            smt.votes_per_regeneration_period = 50;
            smt.vote_regeneration_period_seconds = 5*24*60*60;
            smt.recent_claims = uint128_t( 0, recent_claims );
            smt.author_reward_curve = curve_id::convergent_linear;
            smt.curation_reward_curve = convergent_square_root;
            smt.content_constant = STEEM_CONTENT_CONSTANT_HF21;
            smt.percent_curation_rewards = 50 * STEEM_1_PERCENT;
            smt.last_reward_update = now;
         });

         db.modify( db.get( reward_fund_id_type() ), [&]( reward_fund_object& rfo )
         {
            rfo.recent_claims = uint128_t( 0, recent_claims );
            rfo.last_update = now;
         });
      });
      generate_block();

      comment_operation comment;
      comment.author = "alice";
      comment.permlink = "test";
      comment.parent_permlink = "test";
      comment.title = "foo";
      comment.body = "bar";
      allowed_vote_assets ava;
      ava.votable_assets[ alice_symbol ] = votable_asset_options();
      comment_options_operation comment_opts;
      comment_opts.author = "alice";
      comment_opts.permlink = "test";
      comment_opts.percent_steem_dollars = 0;
      comment_opts.extensions.insert( ava );
      vote2_operation vote;
      vote.voter = "alice";
      vote.author = comment.author;
      vote.permlink = comment.permlink;
      vote.rshares[ STEEM_SYMBOL ] = 100ull * STEEM_VOTE_DUST_THRESHOLD;
      vote.rshares[ alice_symbol ] = vote.rshares[ STEEM_SYMBOL ];

      signed_transaction tx;
      tx.operations.push_back( comment );
      tx.operations.push_back( comment_opts );
      tx.operations.push_back( vote );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );

      comment.author = "bob";
      comment_opts.author = "bob";
      vote.voter = "bob";
      vote.author = comment.author;
      tx.clear();
      tx.operations.push_back( comment );
      tx.operations.push_back( comment_opts );
      tx.operations.push_back( vote );
      sign( tx, bob_private_key );
      db->push_transaction( tx, 0 );

      generate_blocks( db->head_block_time() + ( STEEM_REVERSE_AUCTION_WINDOW_SECONDS_HF21 / 2 ), true );

      tx.clear();
      vote.voter = "bob";
      vote.rshares[ STEEM_SYMBOL ] = 50ull * STEEM_VOTE_DUST_THRESHOLD;
      vote.rshares[ alice_symbol ] = vote.rshares[ STEEM_SYMBOL ];
      tx.operations.push_back( vote );
      sign( tx, bob_private_key );
      db->push_transaction( tx, 0 );

      generate_blocks( db->head_block_time() + STEEM_REVERSE_AUCTION_WINDOW_SECONDS_HF21, true );

      tx.clear();
      vote.voter = "charlie";
      vote.rshares[ STEEM_SYMBOL ] = 50ull * STEEM_VOTE_DUST_THRESHOLD;
      vote.rshares[ alice_symbol ] = vote.rshares[ STEEM_SYMBOL ];
      tx.operations.push_back( vote );
      sign( tx, charlie_private_key );
      db->push_transaction( tx, 0 );

      generate_blocks( db->get_comment( comment.author, comment.permlink ).cashout_time - ( STEEM_UPVOTE_LOCKOUT_SECONDS / 2 ), true );

      tx.clear();
      vote.voter = "dave";
      vote.author = "alice";
      vote.rshares[ STEEM_SYMBOL ] = 100ull * STEEM_VOTE_DUST_THRESHOLD;
      vote.rshares[ alice_symbol ] = vote.rshares[ STEEM_SYMBOL ];
      tx.operations.push_back( vote );
      sign( tx, dave_private_key );
      db->push_transaction( tx, 0 );

      generate_blocks( db->get_comment( comment.author, comment.permlink ).cashout_time - STEEM_BLOCK_INTERVAL, true );

      share_type reward_fund = 10000000;

      db_plugin->debug_update( [=]( database& db )
      {
         auto now = db.head_block_time();

         db.modify( db.get< smt_token_object, by_symbol >( alice_symbol ), [=]( smt_token_object& smt )
         {
            smt.recent_claims = uint128_t( 0, recent_claims );
            smt.reward_balance = asset( reward_fund, alice_symbol );
            smt.current_supply += reward_fund;
            smt.last_reward_update = now;
         });

         share_type reward_delta = 0;

         db.modify( db.get( reward_fund_id_type() ), [&]( reward_fund_object& rfo )
         {
            reward_delta = reward_fund - rfo.reward_balance.amount - 60; //60 adjusts for inflation
            rfo.reward_balance += asset( reward_delta, STEEM_SYMBOL );
            rfo.recent_claims = uint128_t( 0, recent_claims );
            rfo.last_update = now;
         });

         db.modify( db.get_dynamic_global_properties(), [&]( dynamic_global_property_object& gpo )
         {
            gpo.current_supply += asset( reward_delta, STEEM_SYMBOL );
         });
      });

      generate_block();

      const auto& rf = db->get( reward_fund_id_type() );
      const auto& alice_smt = db->get< smt_token_object, by_symbol >( alice_symbol );

      BOOST_REQUIRE( rf.recent_claims == alice_smt.recent_claims );
      BOOST_REQUIRE( rf.reward_balance.amount == alice_smt.reward_balance.amount );

      const auto& alice_smt_balance = db->get< account_rewards_balance_object, by_name_liquid_symbol >( boost::make_tuple( "alice", alice_symbol ) );
      const auto& alice_reward_balance = db->get_account( "alice" );
      BOOST_REQUIRE( alice_reward_balance.reward_vesting_steem.amount == alice_smt_balance.pending_vesting_value.amount );

      const auto& bob_smt_balance = db->get< account_rewards_balance_object, by_name_liquid_symbol >( boost::make_tuple( "bob", alice_symbol ) );
      const auto& bob_reward_balance = db->get_account( "bob" );
      BOOST_REQUIRE( bob_reward_balance.reward_vesting_steem.amount == bob_smt_balance.pending_vesting_value.amount );

      const auto& charlie_smt_balance = db->get< account_rewards_balance_object, by_name_liquid_symbol >( boost::make_tuple( "charlie", alice_symbol ) );
      const auto& charlie_reward_balance = db->get_account( "charlie" );
      BOOST_REQUIRE( charlie_reward_balance.reward_vesting_steem.amount == charlie_smt_balance.pending_vesting_value.amount );

      const auto& dave_smt_balance = db->get< account_rewards_balance_object, by_name_liquid_symbol >( boost::make_tuple( "dave", alice_symbol ) );
      const auto& dave_reward_balance = db->get_account( "dave" );
      BOOST_REQUIRE( dave_reward_balance.reward_vesting_steem.amount == dave_smt_balance.pending_vesting_value.amount );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_token_emissions )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing SMT token emissions" );
      ACTORS( (creator)(alice)(bob)(charlie)(dan)(elaine)(fred)(george)(henry) )

      vest( STEEM_INIT_MINER_NAME, "creator", ASSET( "100000.000 TESTS" ) );

      generate_block();

      auto alices_balance    = asset( 5000000, STEEM_SYMBOL );
      auto bobs_balance      = asset( 25000000, STEEM_SYMBOL );
      auto charlies_balance  = asset( 10000000, STEEM_SYMBOL );
      auto dans_balance      = asset( 25000000, STEEM_SYMBOL );
      auto elaines_balance   = asset( 60000000, STEEM_SYMBOL );
      auto freds_balance     = asset( 0, STEEM_SYMBOL );
      auto georges_balance   = asset( 0, STEEM_SYMBOL );
      auto henrys_balance    = asset( 0, STEEM_SYMBOL );

      std::map< std ::string, std::tuple< share_type, fc::ecc::private_key > > contributor_contributions {
         { "alice",   { alices_balance.amount,   alice_private_key   } },
         { "bob",     { bobs_balance.amount,     bob_private_key     } },
         { "charlie", { charlies_balance.amount, charlie_private_key } },
         { "dan",     { dans_balance.amount,     dan_private_key     } },
         { "elaine",  { elaines_balance.amount,  elaine_private_key  } },
         { "fred",    { freds_balance.amount,    fred_private_key    } },
         { "george",  { georges_balance.amount,  george_private_key  } },
         { "henry",   { henrys_balance.amount,   henry_private_key   } }
      };

      for ( auto& e : contributor_contributions )
      {
         FUND( e.first, asset( std::get< 0 >( e.second ), STEEM_SYMBOL ) );
      }

      generate_block();

      BOOST_TEST_MESSAGE( " --- SMT creation" );
      auto symbol = create_smt( "creator", creator_private_key, 3 );
      const auto& token = db->get< smt_token_object, by_symbol >( symbol );


      signed_transaction tx;
      BOOST_TEST_MESSAGE( " --- SMT setup emissions" );
      smt_setup_emissions_operation emissions_op;
      emissions_op.control_account = "creator";
      emissions_op.emissions_unit.token_unit[ SMT_DESTINATION_REWARDS ]      = 2;
      emissions_op.emissions_unit.token_unit[ SMT_DESTINATION_MARKET_MAKER ] = 2;
      emissions_op.emissions_unit.token_unit[ SMT_DESTINATION_VESTING ]      = 2;
      emissions_op.emissions_unit.token_unit[ "george" ]                     = 1;
      emissions_op.interval_seconds = SMT_EMISSION_MIN_INTERVAL_SECONDS;
      emissions_op.interval_count   = 24;
      emissions_op.symbol = symbol;
      emissions_op.schedule_time  = db->head_block_time() + ( STEEM_BLOCK_INTERVAL * 10 );
      emissions_op.lep_time       = emissions_op.schedule_time + ( STEEM_BLOCK_INTERVAL * STEEM_BLOCKS_PER_DAY * 2 );
      emissions_op.rep_time       = emissions_op.lep_time + ( STEEM_BLOCK_INTERVAL * STEEM_BLOCKS_PER_DAY * 2 );
      emissions_op.lep_abs_amount = 20000000;
      emissions_op.rep_abs_amount = 40000000;
      emissions_op.lep_rel_amount_numerator = 1;
      emissions_op.rep_rel_amount_numerator = 2;
      emissions_op.rel_amount_denom_bits    = 7;
      emissions_op.floor_emissions = false;

      tx.operations.push_back( emissions_op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, creator_private_key );
      db->push_transaction( tx, 0 );
      tx.operations.clear();
      tx.signatures.clear();

      smt_setup_emissions_operation emissions_op2;
      emissions_op2.control_account = "creator";
      emissions_op2.emissions_unit.token_unit[ SMT_DESTINATION_REWARDS ]      = 1;
      emissions_op2.emissions_unit.token_unit[ SMT_DESTINATION_MARKET_MAKER ] = 1;
      emissions_op2.emissions_unit.token_unit[ SMT_DESTINATION_VESTING ]      = 1;
      emissions_op2.emissions_unit.token_unit[ "george" ]                     = 1;
      emissions_op2.interval_seconds = SMT_EMISSION_MIN_INTERVAL_SECONDS;
      emissions_op2.interval_count   = 24;
      emissions_op2.symbol = symbol;
      emissions_op2.schedule_time  = emissions_op.schedule_time + ( emissions_op.interval_seconds * emissions_op.interval_count ) + SMT_EMISSION_MIN_INTERVAL_SECONDS;
      emissions_op2.lep_time       = emissions_op2.schedule_time + ( STEEM_BLOCK_INTERVAL * STEEM_BLOCKS_PER_DAY * 2 );
      emissions_op2.rep_time       = emissions_op2.lep_time + ( STEEM_BLOCK_INTERVAL * STEEM_BLOCKS_PER_DAY * 2 );
      emissions_op2.lep_abs_amount = 50000000;
      emissions_op2.rep_abs_amount = 100000000;
      emissions_op2.lep_rel_amount_numerator = 1;
      emissions_op2.rep_rel_amount_numerator = 2;
      emissions_op2.rel_amount_denom_bits    = 10;
      emissions_op2.floor_emissions = false;

      tx.operations.push_back( emissions_op2 );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, creator_private_key );
      db->push_transaction( tx, 0 );
      tx.operations.clear();
      tx.signatures.clear();

      smt_setup_emissions_operation emissions_op3;
      emissions_op3.control_account = "creator";
      emissions_op3.emissions_unit.token_unit[ SMT_DESTINATION_REWARDS ]      = 1;
      emissions_op3.emissions_unit.token_unit[ SMT_DESTINATION_MARKET_MAKER ] = 1;
      emissions_op3.emissions_unit.token_unit[ SMT_DESTINATION_VESTING ]      = 1;
      emissions_op3.emissions_unit.token_unit[ "george" ]                     = 1;
      emissions_op3.interval_seconds = SMT_EMISSION_MIN_INTERVAL_SECONDS;
      emissions_op3.interval_count   = SMT_EMIT_INDEFINITELY;
      emissions_op3.symbol = symbol;
      emissions_op3.schedule_time  = emissions_op2.schedule_time + ( emissions_op2.interval_seconds * emissions_op2.interval_count ) + SMT_EMISSION_MIN_INTERVAL_SECONDS;
      emissions_op3.lep_time       = emissions_op3.schedule_time;
      emissions_op3.rep_time       = emissions_op3.schedule_time;
      emissions_op3.lep_abs_amount = 100000000;
      emissions_op3.rep_abs_amount = 100000000;
      emissions_op3.lep_rel_amount_numerator = 0;
      emissions_op3.rep_rel_amount_numerator = 0;
      emissions_op3.rel_amount_denom_bits    = 0;
      emissions_op3.floor_emissions = false;

      tx.operations.push_back( emissions_op3 );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, creator_private_key );
      db->push_transaction( tx, 0 );
      tx.operations.clear();
      tx.signatures.clear();

      BOOST_TEST_MESSAGE( " --- SMT setup" );
      smt_setup_operation setup_op;

      uint64_t contribution_window_blocks = 5;
      setup_op.control_account = "creator";
      setup_op.symbol = symbol;
      setup_op.contribution_begin_time = db->head_block_time() + STEEM_BLOCK_INTERVAL;
      setup_op.contribution_end_time = setup_op.contribution_begin_time + ( STEEM_BLOCK_INTERVAL * contribution_window_blocks );
      setup_op.steem_units_min      = 0;
      setup_op.steem_units_soft_cap = 100000000;
      setup_op.steem_units_hard_cap = 150000000;
      setup_op.max_supply = 22400000000;
      setup_op.launch_time = setup_op.contribution_end_time + STEEM_BLOCK_INTERVAL;
      setup_op.initial_generation_policy = get_capped_generation_policy
      (
         get_generation_unit(
         {
            { "fred", 3 },
            { "george", 2 }
         },
         {
            { SMT_DESTINATION_FROM, 7 },
            { "george", 1 },
            { "henry", 2 }
         } ), /* pre_soft_cap_unit */
         get_generation_unit(
         {
            { "fred", 3 },
            { "george", 2 }
         },
         {
            { SMT_DESTINATION_FROM, 7 },
            { "george", 1 },
            { "henry", 2 }
         } ), /* post_soft_cap_unit */
         50,                                                            /* min_unit_ratio */
         100                                                            /* max_unit_ratio */
      );

      tx.operations.push_back( setup_op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, creator_private_key );
      db->push_transaction( tx, 0 );
      tx.operations.clear();
      tx.signatures.clear();

      BOOST_REQUIRE( token.phase == smt_phase::setup_completed );

      generate_block();

      BOOST_REQUIRE( token.phase == smt_phase::ico );

      BOOST_TEST_MESSAGE( " --- SMT contributions" );

      uint32_t num_contributions = 0;
      for ( auto& e : contributor_contributions )
      {
         if ( std::get< 0 >( e.second ) == 0 )
            continue;

         smt_contribute_operation contrib_op;

         contrib_op.symbol = symbol;
         contrib_op.contribution_id = 0;
         contrib_op.contributor = e.first;
         contrib_op.contribution = asset( std::get< 0 >( e.second ), STEEM_SYMBOL );

         tx.operations.push_back( contrib_op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         sign( tx, std::get< 1 >( e.second ) );
         db->push_transaction( tx, 0 );
         tx.operations.clear();
         tx.signatures.clear();

         generate_block();
         num_contributions++;
      }

      validate_database();

      generate_block();

      BOOST_REQUIRE( token.phase == smt_phase::launch_success );

      steem::plugins::rc::rc_plugin_skip_flags rc_skip;
      rc_skip.skip_reject_not_enough_rc = 0;
      rc_skip.skip_deduct_rc = 0;
      rc_skip.skip_negative_rc_balance = 0;
      rc_skip.skip_reject_unknown_delta_vests = 0;
      appbase::app().get_plugin< steem::plugins::rc::rc_plugin >().set_rc_plugin_skip_flags( rc_skip );

      steem::plugins::rc::delegate_to_pool_operation del_op;
      custom_json_operation custom_op;

      del_op.from_account = "creator";
      del_op.to_pool = symbol.to_nai_string();
      del_op.amount = asset( db->get_account( "creator" ).vesting_shares.amount / 2, VESTS_SYMBOL );
      custom_op.json = fc::json::to_string( steem::plugins::rc::rc_plugin_operation( del_op ) );
      custom_op.id = STEEM_RC_PLUGIN_NAME;
      custom_op.required_auths.insert( "creator" );

      tx.operations.push_back( custom_op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, creator_private_key );
      db->push_transaction( tx, 0 );
      tx.operations.clear();
      tx.signatures.clear();

      BOOST_TEST_MESSAGE( " --- SMT token emissions" );

      share_type supply = token.current_supply;
      share_type rewards = token.reward_balance.amount;
      share_type market_maker = token.market_maker.token_balance.amount;
      share_type vesting = token.total_vesting_fund_smt;
      share_type george_share = db->get_balance( db->get_account( "george" ), symbol ).amount;

      auto emission_time = emissions_op.schedule_time;
      generate_blocks( emission_time );
      generate_block();

      auto approximately_equal = []( share_type a, share_type b, uint32_t epsilon = 10 ) { return std::abs( a.value - b.value ) < epsilon; };

      for ( uint32_t i = 0; i <= emissions_op.interval_count; i++ )
      {
         validate_database();

         uint32_t rel_amount_numerator;
         share_type abs_amount;
         if ( emission_time <= emissions_op.lep_time )
         {
            abs_amount = emissions_op.lep_abs_amount;
            rel_amount_numerator = emissions_op.lep_rel_amount_numerator;
         }
         else if ( emission_time >= emissions_op.rep_time )
         {
            abs_amount = emissions_op.rep_abs_amount;
            rel_amount_numerator = emissions_op.rep_rel_amount_numerator;
         }
         else
         {
            fc::uint128 lep_abs_val{ emissions_op.lep_abs_amount.value },
                        rep_abs_val{ emissions_op.rep_abs_amount.value },
                        lep_rel_num{ emissions_op.lep_rel_amount_numerator    },
                        rep_rel_num{ emissions_op.rep_rel_amount_numerator    };

            uint32_t lep_dist    = emission_time.sec_since_epoch() - emissions_op.lep_time.sec_since_epoch();
            uint32_t rep_dist    = emissions_op.rep_time.sec_since_epoch() - emission_time.sec_since_epoch();
            uint32_t total_dist  = emissions_op.rep_time.sec_since_epoch() - emissions_op.lep_time.sec_since_epoch();
            abs_amount           = ( ( lep_abs_val * lep_dist + rep_abs_val * rep_dist ) / total_dist ).to_int64();
            rel_amount_numerator = ( ( lep_rel_num * lep_dist + rep_rel_num * rep_dist ) / total_dist ).to_uint64();
         }

         share_type rel_amount = ( fc::uint128( supply.value ) * rel_amount_numerator >> emissions_op.rel_amount_denom_bits ).to_int64();

         share_type new_token_supply;
         if ( emissions_op.floor_emissions )
            new_token_supply = std::min( abs_amount, rel_amount );
         else
            new_token_supply = std::max( abs_amount, rel_amount );

         share_type new_rewards = new_token_supply * emissions_op.emissions_unit.token_unit[ SMT_DESTINATION_REWARDS ] / emissions_op.emissions_unit.token_unit_sum();
         share_type new_market_maker = new_token_supply * emissions_op.emissions_unit.token_unit[ SMT_DESTINATION_MARKET_MAKER ] / emissions_op.emissions_unit.token_unit_sum();
         share_type new_vesting = new_token_supply * emissions_op.emissions_unit.token_unit[ SMT_DESTINATION_VESTING ] / emissions_op.emissions_unit.token_unit_sum();
         share_type new_george = new_token_supply * emissions_op.emissions_unit.token_unit[ "george" ] / emissions_op.emissions_unit.token_unit_sum();

         BOOST_REQUIRE( approximately_equal( new_rewards + new_market_maker + new_vesting + new_george, new_token_supply ) );

         supply += new_token_supply;
         BOOST_REQUIRE( approximately_equal( token.current_supply, supply ) );

         market_maker += new_market_maker;
         BOOST_REQUIRE( approximately_equal( token.market_maker.token_balance.amount, market_maker ) );

         vesting += new_vesting;
         BOOST_REQUIRE( approximately_equal( token.total_vesting_fund_smt, vesting ) );

         rewards += new_rewards;
         BOOST_REQUIRE( approximately_equal( token.reward_balance.amount, rewards ) );

         george_share += new_george;
         BOOST_REQUIRE( approximately_equal( db->get_balance( db->get_account( "george" ), symbol ).amount, george_share ) );

         // Prevent any sort of drift
         supply = token.current_supply;
         market_maker = token.market_maker.token_balance.amount;
         vesting = token.total_vesting_fund_smt;
         rewards = token.reward_balance.amount;
         george_share = db->get_balance( db->get_account( "george" ), symbol ).amount;

         emission_time += SMT_EMISSION_MIN_INTERVAL_SECONDS;
         generate_blocks( emission_time );
         generate_block();
      }

      for ( uint32_t i = 0; i <= emissions_op2.interval_count; i++ )
      {
         validate_database();

         uint32_t rel_amount_numerator;
         share_type abs_amount;
         if ( emission_time <= emissions_op2.lep_time )
         {
            abs_amount = emissions_op2.lep_abs_amount;
            rel_amount_numerator = emissions_op2.lep_rel_amount_numerator;
         }
         else if ( emission_time >= emissions_op2.rep_time )
         {
            abs_amount = emissions_op2.rep_abs_amount;
            rel_amount_numerator = emissions_op2.rep_rel_amount_numerator;
         }
         else
         {
            fc::uint128 lep_abs_val{ emissions_op2.lep_abs_amount.value },
                        rep_abs_val{ emissions_op2.rep_abs_amount.value },
                        lep_rel_num{ emissions_op2.lep_rel_amount_numerator    },
                        rep_rel_num{ emissions_op2.rep_rel_amount_numerator    };

            uint32_t lep_dist    = emission_time.sec_since_epoch() - emissions_op2.lep_time.sec_since_epoch();
            uint32_t rep_dist    = emissions_op2.rep_time.sec_since_epoch() - emission_time.sec_since_epoch();
            uint32_t total_dist  = emissions_op2.rep_time.sec_since_epoch() - emissions_op2.lep_time.sec_since_epoch();
            abs_amount           = ( ( lep_abs_val * lep_dist + rep_abs_val * rep_dist ) / total_dist ).to_int64();
            rel_amount_numerator = ( ( lep_rel_num * lep_dist + rep_rel_num * rep_dist ) / total_dist ).to_uint64();
         }

         share_type rel_amount = ( fc::uint128( supply.value ) * rel_amount_numerator >> emissions_op2.rel_amount_denom_bits ).to_int64();

         share_type new_token_supply;
         if ( emissions_op2.floor_emissions )
            new_token_supply = std::min( abs_amount, rel_amount );
         else
            new_token_supply = std::max( abs_amount, rel_amount );

         share_type new_rewards = new_token_supply * emissions_op2.emissions_unit.token_unit[ SMT_DESTINATION_REWARDS ] / emissions_op2.emissions_unit.token_unit_sum();
         share_type new_market_maker = new_token_supply * emissions_op2.emissions_unit.token_unit[ SMT_DESTINATION_MARKET_MAKER ] / emissions_op2.emissions_unit.token_unit_sum();
         share_type new_vesting = new_token_supply * emissions_op2.emissions_unit.token_unit[ SMT_DESTINATION_VESTING ] / emissions_op2.emissions_unit.token_unit_sum();
         share_type new_george = new_token_supply * emissions_op2.emissions_unit.token_unit[ "george" ] / emissions_op2.emissions_unit.token_unit_sum();

         BOOST_REQUIRE( approximately_equal( new_rewards + new_market_maker + new_vesting + new_george, new_token_supply ) );

         supply += new_token_supply;
         BOOST_REQUIRE( approximately_equal( token.current_supply, supply ) );

         market_maker += new_market_maker;
         BOOST_REQUIRE( approximately_equal( token.market_maker.token_balance.amount, market_maker ) );

         vesting += new_vesting;
         BOOST_REQUIRE( approximately_equal( token.total_vesting_fund_smt, vesting ) );

         rewards += new_rewards;
         BOOST_REQUIRE( approximately_equal( token.reward_balance.amount, rewards ) );

         george_share += new_george;
         BOOST_REQUIRE( approximately_equal( db->get_balance( db->get_account( "george" ), symbol ).amount, george_share ) );

         // Prevent any sort of drift
         supply = token.current_supply;
         market_maker = token.market_maker.token_balance.amount;
         vesting = token.total_vesting_fund_smt;
         rewards = token.reward_balance.amount;
         george_share = db->get_balance( db->get_account( "george" ), symbol ).amount;

         emission_time += SMT_EMISSION_MIN_INTERVAL_SECONDS;
         generate_blocks( emission_time );
         generate_block();
      }

      BOOST_TEST_MESSAGE( " --- SMT token emissions catch-up logic" );

      share_type new_token_supply = emissions_op3.lep_abs_amount;
      share_type new_rewards = new_token_supply * emissions_op3.emissions_unit.token_unit[ SMT_DESTINATION_REWARDS ] / emissions_op3.emissions_unit.token_unit_sum();
      share_type new_market_maker = new_token_supply * emissions_op3.emissions_unit.token_unit[ SMT_DESTINATION_MARKET_MAKER ] / emissions_op3.emissions_unit.token_unit_sum();
      share_type new_vesting = new_token_supply * emissions_op3.emissions_unit.token_unit[ SMT_DESTINATION_VESTING ] / emissions_op3.emissions_unit.token_unit_sum();
      share_type new_george = new_token_supply * emissions_op3.emissions_unit.token_unit[ "george" ] / emissions_op3.emissions_unit.token_unit_sum();

      BOOST_REQUIRE( approximately_equal( new_rewards + new_market_maker + new_vesting + new_george, new_token_supply ) );

      supply += new_token_supply;
      BOOST_REQUIRE( approximately_equal( token.current_supply, supply ) );

      market_maker += new_market_maker;
      BOOST_REQUIRE( approximately_equal( token.market_maker.token_balance.amount, market_maker ) );

      vesting += new_vesting;
      BOOST_REQUIRE( approximately_equal( token.total_vesting_fund_smt, vesting ) );

      rewards += new_rewards;
      BOOST_REQUIRE( approximately_equal( token.reward_balance.amount, rewards ) );

      george_share += new_george;
      BOOST_REQUIRE( approximately_equal( db->get_balance( db->get_account( "george" ), symbol ).amount, george_share ) );

      // Prevent any sort of drift
      supply = token.current_supply;
      market_maker = token.market_maker.token_balance.amount;
      vesting = token.total_vesting_fund_smt;
      rewards = token.reward_balance.amount;
      george_share = db->get_balance( db->get_account( "george" ), symbol ).amount;

      emission_time += SMT_EMISSION_MIN_INTERVAL_SECONDS * 2;
      generate_blocks( emission_time );
      generate_blocks( 3 );

      new_token_supply = ( emissions_op3.lep_abs_amount * 2 );

      new_rewards = new_token_supply * emissions_op3.emissions_unit.token_unit[ SMT_DESTINATION_REWARDS ] / emissions_op3.emissions_unit.token_unit_sum();
      new_market_maker = new_token_supply * emissions_op3.emissions_unit.token_unit[ SMT_DESTINATION_MARKET_MAKER ] / emissions_op3.emissions_unit.token_unit_sum();
      new_vesting = new_token_supply * emissions_op3.emissions_unit.token_unit[ SMT_DESTINATION_VESTING ] / emissions_op3.emissions_unit.token_unit_sum();
      new_george = new_token_supply * emissions_op3.emissions_unit.token_unit[ "george" ] / emissions_op3.emissions_unit.token_unit_sum();

      BOOST_REQUIRE( approximately_equal( new_rewards + new_market_maker + new_vesting + new_george, new_token_supply ) );

      supply += new_token_supply;
      BOOST_REQUIRE( approximately_equal( token.current_supply, supply ) );

      market_maker += new_market_maker;
      BOOST_REQUIRE( approximately_equal( token.market_maker.token_balance.amount, market_maker ) );

      vesting += new_vesting;
      BOOST_REQUIRE( approximately_equal( token.total_vesting_fund_smt, vesting ) );

      rewards += new_rewards;
      BOOST_REQUIRE( approximately_equal( token.reward_balance.amount, rewards ) );

      george_share += new_george;
      BOOST_REQUIRE( approximately_equal( db->get_balance( db->get_account( "george" ), symbol ).amount, george_share ) );

      // Prevent any sort of drift
      supply = token.current_supply;
      market_maker = token.market_maker.token_balance.amount;
      vesting = token.total_vesting_fund_smt;
      rewards = token.reward_balance.amount;
      george_share = db->get_balance( db->get_account( "george" ), symbol ).amount;

      emission_time += SMT_EMISSION_MIN_INTERVAL_SECONDS * 6;
      generate_blocks( emission_time );
      generate_blocks( 11 );

      new_token_supply = emissions_op3.lep_abs_amount * 6;

      new_rewards = new_token_supply * emissions_op3.emissions_unit.token_unit[ SMT_DESTINATION_REWARDS ] / emissions_op3.emissions_unit.token_unit_sum();
      new_market_maker = new_token_supply * emissions_op3.emissions_unit.token_unit[ SMT_DESTINATION_MARKET_MAKER ] / emissions_op3.emissions_unit.token_unit_sum();
      new_vesting = new_token_supply * emissions_op3.emissions_unit.token_unit[ SMT_DESTINATION_VESTING ] / emissions_op3.emissions_unit.token_unit_sum();
      new_george = new_token_supply * emissions_op3.emissions_unit.token_unit[ "george" ] / emissions_op3.emissions_unit.token_unit_sum();

      BOOST_REQUIRE( approximately_equal( new_rewards + new_market_maker + new_vesting + new_george, new_token_supply ) );

      supply += new_token_supply;
      BOOST_REQUIRE( approximately_equal( token.current_supply, supply ) );

      market_maker += new_market_maker;
      BOOST_REQUIRE( approximately_equal( token.market_maker.token_balance.amount, market_maker ) );

      vesting += new_vesting;
      BOOST_REQUIRE( approximately_equal( token.total_vesting_fund_smt, vesting ) );

      rewards += new_rewards;
      BOOST_REQUIRE( approximately_equal( token.reward_balance.amount, rewards ) );

      george_share += new_george;
      BOOST_REQUIRE( approximately_equal( db->get_balance( db->get_account( "george" ), symbol ).amount, george_share ) );

      BOOST_REQUIRE( token.current_supply == 22307937127 );

      emission_time += SMT_EMISSION_MIN_INTERVAL_SECONDS * 6;
      generate_blocks( emission_time );
      generate_blocks( 11 );

      BOOST_TEST_MESSAGE( " --- SMT token emissions do not emit passed max supply" );

      BOOST_REQUIRE( approximately_equal( token.current_supply, supply ) );

      BOOST_REQUIRE( approximately_equal( token.market_maker.token_balance.amount, market_maker ) );

      BOOST_REQUIRE( approximately_equal( token.total_vesting_fund_smt, vesting ) );

      BOOST_REQUIRE( approximately_equal( token.reward_balance.amount, rewards ) );

      BOOST_REQUIRE( approximately_equal( db->get_balance( db->get_account( "george" ), symbol ).amount, george_share ) );

      BOOST_REQUIRE( token.current_supply == 22307937127 );
      BOOST_REQUIRE( token.max_supply >= token.current_supply );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
#endif
