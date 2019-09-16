
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
            { "$george.vesting", 2 }
         },
         {
            { SMT_DESTINATION_FROM, 5 },
            { SMT_DESTINATION_MARKET_MAKER, 1 },
            { SMT_DESTINATION_REWARDS, 2 },
            { "$george.vesting", 2 }
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
      BOOST_REQUIRE( token.rewards_fund == asset( 3000000000, symbol ) );

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

BOOST_AUTO_TEST_SUITE_END()
#endif
