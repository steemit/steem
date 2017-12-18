#include <fc/macros.hpp>

#if defined IS_TEST_NET && defined STEEM_ENABLE_SMT

FC_TODO(Extend testing scenarios to support multiple NAIs per account)

#include <boost/test/unit_test.hpp>

#include <steem/protocol/exceptions.hpp>
#include <steem/protocol/hardfork.hpp>

#include <steem/chain/database.hpp>
#include <steem/chain/database_exceptions.hpp>
#include <steem/chain/steem_objects.hpp>
#include <steem/chain/smt_objects.hpp>

#include "../db_fixture/database_fixture.hpp"

using namespace steem::chain;
using namespace steem::protocol;
using fc::string;
using boost::container::flat_set;

BOOST_FIXTURE_TEST_SUITE( smt_tests, smt_database_fixture )

BOOST_AUTO_TEST_CASE( asset_symbol_validate )
{
   try
   {
      auto check_validate = [&]( const std::string& name, uint8_t decimal_places )
      {
         asset_symbol_type sym = name_to_asset_symbol( name, decimal_places );
         sym.validate();
      };

      // specific cases in https://github.com/steemit/steem/issues/1738
      check_validate( "0", 0 );
      check_validate( "d2", 1 );
      check_validate( "da1", 1 );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_create_validate )
{
   try
   {
      SMT_SYMBOL( alice, 3 );

      smt_create_operation op;
      op.smt_creation_fee = ASSET( "1.000 TESTS" );
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      op.control_account = "alice";
      op.symbol = alice_symbol;
      op.precision = op.symbol.decimals();
      op.validate();

      op.smt_creation_fee.amount = -op.smt_creation_fee.amount;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      // passes with MAX_SHARE_SUPPLY
      op.smt_creation_fee.amount = STEEM_MAX_SHARE_SUPPLY;
      op.validate();

      // fails with MAX_SHARE_SUPPLY+1
      ++op.smt_creation_fee.amount;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      op.smt_creation_fee = ASSET( "1.000000 VESTS" );
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      op.smt_creation_fee = ASSET( "1.000 TESTS" );
      // Valid, but doesn't match decimals stored in symbol.
      op.precision = 0;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_create_authorities )
{
   try
   {
      SMT_SYMBOL( alice, 3 );

      smt_create_operation op;
      op.control_account = "alice";
      op.symbol = alice_symbol;
      op.smt_creation_fee = ASSET( "1.000 TESTS" );

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.get_required_posting_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      expected.insert( "alice" );
      op.get_required_active_authorities( auths );
      BOOST_REQUIRE( auths == expected );
   }
   FC_LOG_AND_RETHROW()
}

#define OP2TX(OP,TX,KEY) \
TX.operations.push_back( OP ); \
TX.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION ); \
TX.sign( KEY, db->get_chain_id() );

#define PUSH_OP(OP,KEY) \
{ \
   signed_transaction tx; \
   OP2TX(OP,tx,KEY) \
   db->push_transaction( tx, 0 ); \
}

#define PUSH_OP_TWICE(OP,KEY) \
{ \
   signed_transaction tx; \
   OP2TX(OP,tx,KEY) \
   db->push_transaction( tx, 0 ); \
   db->push_transaction( tx, database::skip_transaction_dupe_check ); \
}

#define FAIL_WITH_OP(OP,KEY,EXCEPTION) \
{ \
   signed_transaction tx; \
   OP2TX(OP,tx,KEY) \
   STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), EXCEPTION ); \
}

BOOST_AUTO_TEST_CASE( smt_create_apply )
{
   try
   {
      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );

      ACTORS( (alice)(bob) )
      SMT_SYMBOL( alice, 3 );

      const dynamic_global_property_object& dgpo = db->get_dynamic_global_properties();
      asset required_creation_fee = dgpo.smt_creation_fee;
      FC_ASSERT( required_creation_fee.amount > 0, "Expected positive smt_creation_fee." );
      unsigned int test_amount = required_creation_fee.amount.value;

      smt_create_operation op;
      op.control_account = "alice";
      op.symbol = alice_symbol;
      op.precision = op.symbol.decimals();

      // Fund with STEEM, and set fee with SBD.
      fund( "alice", test_amount );
      // Declare fee in SBD/TBD though alice has none.
      op.smt_creation_fee = asset( test_amount, SBD_SYMBOL );
      // Throw due to insufficient balance of SBD/TBD.
      FAIL_WITH_OP(op, alice_private_key, fc::assert_exception);

      // Now fund with SBD, and set fee with STEEM.
      convert( "alice", asset( test_amount, STEEM_SYMBOL ) );
      // Declare fee in STEEM though alice has none.
      op.smt_creation_fee = asset( test_amount, STEEM_SYMBOL );
      // Throw due to insufficient balance of STEEM.
      FAIL_WITH_OP(op, alice_private_key, fc::assert_exception);

      // Push valid operation.
      op.smt_creation_fee = asset( test_amount, SBD_SYMBOL );
      PUSH_OP( op, alice_private_key );

      // Check the SMT cannot be created twice even with different precision.
      create_conflicting_smt(op.symbol, "alice", alice_private_key);

      // Check that another user/account can't be used to create duplicating SMT even with different precision.
      create_conflicting_smt(op.symbol, "bob", bob_private_key);

      // Check that invalid SMT can't be created
      create_invalid_smt("alice", alice_private_key);

      // Check fee set too low.
      asset fee_too_low = required_creation_fee;
      unsigned int too_low_fee_amount = required_creation_fee.amount.value-1;
      fee_too_low.amount -= 1;

      SMT_SYMBOL( bob, 0 );
      op.control_account = "bob";
      op.symbol = bob_symbol;
      op.precision = op.symbol.decimals();

      // Check too low fee in STEEM.
      fund( "bob", too_low_fee_amount );
      op.smt_creation_fee = asset( too_low_fee_amount, STEEM_SYMBOL );
      FAIL_WITH_OP(op, bob_private_key, fc::assert_exception);

      // Check too low fee in SBD.
      convert( "bob", asset( too_low_fee_amount, STEEM_SYMBOL ) );
      op.smt_creation_fee = asset( too_low_fee_amount, SBD_SYMBOL );
      FAIL_WITH_OP(op, bob_private_key, fc::assert_exception);
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( setup_emissions_validate )
{
   try
   {
      SMT_SYMBOL( alice, 3 );

      uint64_t h0 = fc::sha256::hash( "alice" )._hash[0];
      uint32_t h0lo = uint32_t( h0 & 0x7FFFFFF );
      uint32_t an = h0lo % (SMT_MAX_NAI+1);

      FC_UNUSED(an);

      ilog( "alice_symbol: ${s}", ("s", alice_symbol) );

      smt_setup_emissions_operation op;
      // Invalid account name.
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      op.control_account = "alice";
      // schedule_time <= STEEM_GENESIS_TIME;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      fc::time_point now = fc::time_point::now();
      op.schedule_time = now;
      // Empty emissions_unit.token_unit
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      op.emissions_unit.token_unit["alice"] = 10;
      // Both absolute amount fields are zero.
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      op.lep_abs_amount = ASSET( "0.000 TESTS" );
      // Amount symbol does NOT match control account name.
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      op.lep_abs_amount = asset( 0, alice_symbol );
      // Mismatch of absolute amount symbols.
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      op.rep_abs_amount = asset( -1, alice_symbol );
      // Negative absolute amount.
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      op.rep_abs_amount = asset( 0, alice_symbol );
      // Both amounts are equal zero.
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      op.rep_abs_amount = asset( 1000, alice_symbol );
      op.validate();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( set_setup_parameters_validate )
{
   try
   {
      smt_set_setup_parameters_operation op;
      op.control_account = "####";
      STEEM_REQUIRE_THROW( op.validate(), fc::exception ); // invalid account name

      op.control_account = "dany";
      op.validate();

      op.setup_parameters.emplace(smt_param_allow_vesting());
      op.setup_parameters.emplace(smt_param_allow_voting());
      op.validate();

      op.setup_parameters.emplace(smt_param_allow_vesting({false}));
      op.setup_parameters.emplace(smt_param_allow_voting({false}));
      op.validate();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( setup_emissions_authorities )
{
   try
   {
      SMT_SYMBOL( alice, 3 );

      smt_setup_emissions_operation op;
      op.control_account = "alice";
      fc::time_point now = fc::time_point::now();
      op.schedule_time = now;
      op.emissions_unit.token_unit["alice"] = 10;
      op.lep_abs_amount = op.rep_abs_amount = asset(1000, alice_symbol);

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.get_required_posting_authorities( auths );
      BOOST_REQUIRE( auths == expected );
      expected.insert( "alice" );
      op.get_required_active_authorities( auths );
      BOOST_REQUIRE( auths == expected );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( set_setup_parameters_authorities )
{
   try
   {
      smt_set_setup_parameters_operation op;
      op.control_account = "dany";

      flat_set<account_name_type> auths;
      flat_set<account_name_type> expected;

      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.get_required_posting_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      expected.insert( "dany" );
      op.get_required_active_authorities( auths );
      BOOST_REQUIRE( auths == expected );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( setup_emissions_apply )
{
   try
   {
      ACTORS( (alice)(bob) )

      smt_setup_emissions_operation fail_op;
      fail_op.control_account = "alice";
      fc::time_point now = fc::time_point::now();
      fail_op.schedule_time = now;
      fail_op.emissions_unit.token_unit["bob"] = 10;

      // Do invalid attempt at SMT creation.
      create_invalid_smt("alice", alice_private_key);      

      // Fail due to non-existing SMT (too early).
      FAIL_WITH_OP(fail_op, alice_private_key, fc::assert_exception)

      // Create SMT(s) and continue.
      create_smt_3("alice", alice_private_key, [&fail_op, this, alice_private_key]
                                               (const asset_symbol_type& smt1, const asset_symbol_type& smt2, const asset_symbol_type& smt3) {
         // Do successful op with one smt.
         smt_setup_emissions_operation valid_op = fail_op;
         valid_op.symbol = smt1;
         valid_op.lep_abs_amount = valid_op.rep_abs_amount = asset( 1000, valid_op.symbol );
         PUSH_OP(valid_op,alice_private_key)

         // Fail with another smt.
         fail_op.symbol = smt2;
         fail_op.lep_abs_amount = fail_op.rep_abs_amount = asset( 1000, fail_op.symbol );
         // TODO: Replace the code below with account setup operation execution once its implemented.
         const steem::chain::smt_token_object* smt = db->find< steem::chain::smt_token_object, by_symbol >( fail_op.symbol );
         FC_ASSERT( smt != nullptr, "The SMT has just been created!" );
         FC_ASSERT( smt->phase < steem::chain::smt_token_object::smt_phase::setup_completed, "Who closed setup phase?!" );
         db->modify( *smt, [&]( steem::chain::smt_token_object& token )
         {
            token.phase = steem::chain::smt_token_object::smt_phase::setup_completed;
         });
         // Fail due to closed setup phase (too late).
         FAIL_WITH_OP(fail_op, alice_private_key, fc::assert_exception)
      });
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( set_setup_parameters_apply )
{
   try
   {
      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      ACTORS( (dany)(eddy) )

      fund( "dany", 5000000 );
      convert( "dany", ASSET( "5000.000 TESTS" ) );

      smt_set_setup_parameters_operation fail_op;
      fail_op.control_account = "dany";

      // Do invalid attempt at SMT creation.
      create_invalid_smt("dany", dany_private_key);
      
      // Fail due to non-existing SMT (too early).
      FAIL_WITH_OP(fail_op, dany_private_key, fc::assert_exception)
      
      // Create SMT(s) and continue.
      create_smt_3("dany", dany_private_key, [&fail_op, this, dany_private_key, eddy_private_key]
                                             (const asset_symbol_type& smt1, const asset_symbol_type& smt2, const asset_symbol_type& smt3) {
         // "Reset" parameters to default value.
         smt_set_setup_parameters_operation valid_op = fail_op;
         valid_op.symbol = smt1;
         PUSH_OP_TWICE(valid_op, dany_private_key);

         // Fail with wrong key.
         fail_op.symbol = smt2;
         fail_op.setup_parameters.clear();
         fail_op.setup_parameters.emplace( smt_param_allow_vesting() );
         fail_op.setup_parameters.emplace( smt_param_allow_voting() );
         FAIL_WITH_OP(fail_op, eddy_private_key, fc::exception);

         // Set both explicitly to false.
         valid_op.setup_parameters.clear();
         valid_op.setup_parameters.emplace( smt_param_allow_vesting({false}) );
         valid_op.setup_parameters.emplace( smt_param_allow_voting({false}) );
         PUSH_OP(valid_op, dany_private_key);

         // Set one to true and another one to false.
         valid_op.setup_parameters.clear();
         valid_op.setup_parameters.emplace( smt_param_allow_vesting() );
         PUSH_OP(valid_op, dany_private_key);

         // TODO:
         // - check applying smt_set_setup_parameters_operation after setup completed
         });
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( runtime_parameters_windows_validate )
{
   try
   {
      BOOST_REQUIRE( SMT_VESTING_WITHDRAW_INTERVAL_SECONDS > SMT_UPVOTE_LOCKOUT );

      smt_set_runtime_parameters_operation op;

      op.control_account = "{}{}{}{}";
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      op.control_account = "alice";
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      smt_param_windows_v1 windows;
      windows.reverse_auction_window_seconds = 2;
      windows.cashout_window_seconds = windows.reverse_auction_window_seconds;
      op.runtime_parameters.insert( windows );
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      op.runtime_parameters.clear();
      windows.reverse_auction_window_seconds = 2;
      windows.cashout_window_seconds = windows.reverse_auction_window_seconds - 1;
      op.runtime_parameters.insert( windows );
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      op.runtime_parameters.clear();
      windows.reverse_auction_window_seconds = SMT_VESTING_WITHDRAW_INTERVAL_SECONDS;
      windows.cashout_window_seconds = windows.reverse_auction_window_seconds + 1;
      op.runtime_parameters.insert( windows );
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      op.runtime_parameters.clear();
      windows.reverse_auction_window_seconds = 1;
      windows.cashout_window_seconds = SMT_VESTING_WITHDRAW_INTERVAL_SECONDS;
      op.runtime_parameters.insert( windows );
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      op.runtime_parameters.clear();
      windows.reverse_auction_window_seconds = 0;
      windows.cashout_window_seconds = SMT_UPVOTE_LOCKOUT;
      op.runtime_parameters.insert( windows );
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      op.runtime_parameters.clear();
      windows.reverse_auction_window_seconds = 0;
      windows.cashout_window_seconds = windows.reverse_auction_window_seconds + SMT_UPVOTE_LOCKOUT + 1;
      op.runtime_parameters.insert( windows );
      op.validate();

      op.runtime_parameters.clear();
      windows.reverse_auction_window_seconds = 0;
      windows.cashout_window_seconds = SMT_VESTING_WITHDRAW_INTERVAL_SECONDS - 1;
      op.runtime_parameters.insert( windows );
      op.validate();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( runtime_parameters_regeneration_period_validate )
{
   try
   {
      smt_set_runtime_parameters_operation op;
      op.control_account = "alice";

      smt_param_vote_regeneration_period_seconds_v1 regeneration;

      regeneration.vote_regeneration_period_seconds = SMT_VESTING_WITHDRAW_INTERVAL_SECONDS;
      op.runtime_parameters.insert( regeneration );
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      op.runtime_parameters.clear();
      regeneration.vote_regeneration_period_seconds = 0;
      op.runtime_parameters.insert( regeneration );
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      op.runtime_parameters.clear();
      regeneration.vote_regeneration_period_seconds = SMT_VESTING_WITHDRAW_INTERVAL_SECONDS - 1;
      op.runtime_parameters.insert( regeneration );
      op.validate();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( runtime_parameters_authorities )
{
   try
   {
      smt_set_runtime_parameters_operation op;
      op.control_account = "alice";

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.get_required_posting_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      expected.insert( "alice" );
      op.get_required_active_authorities( auths );
      BOOST_REQUIRE( auths == expected );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( runtime_parameters_apply )
{
   try
   {
      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );

      ACTORS( (alice) )

      smt_set_runtime_parameters_operation op;

      op.control_account = "alice";

      smt_param_windows_v1 windows;
      windows.reverse_auction_window_seconds = 0;
      windows.cashout_window_seconds = windows.reverse_auction_window_seconds + SMT_UPVOTE_LOCKOUT + 1;

      smt_param_vote_regeneration_period_seconds_v1 regeneration;
      regeneration.vote_regeneration_period_seconds = SMT_VESTING_WITHDRAW_INTERVAL_SECONDS / 2;
      regeneration.votes_per_regeneration_period = 1;

      smt_param_rewards_v1 rewards;
      rewards.content_constant = 1;
      rewards.percent_curation_rewards = 2;
      rewards.percent_content_rewards = 3;

      op.runtime_parameters.insert( windows );
      op.runtime_parameters.insert( regeneration );
      op.runtime_parameters.insert( rewards );

      //First we should create SMT
      FAIL_WITH_OP(op, alice_private_key, fc::assert_exception)

      // Create SMT(s) and continue.
      create_smt_3("alice", alice_private_key, [&op, this, alice_private_key]
                                               (const asset_symbol_type& smt1, const asset_symbol_type& smt2, const asset_symbol_type& smt3) {
         //Make transaction again.
         op.symbol = smt3;
         PUSH_OP(op, alice_private_key);
      });
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_transfer_validate )
{
   try
   {
      ACTORS( (alice) )
      signed_transaction tx;
      asset_symbol_type alice_symbol = create_smt(tx, "alice", alice_private_key, 0);

      transfer_operation op;
      op.from = "alice";
      op.to = "bob";
      op.amount = asset(100, alice_symbol);
      op.validate();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_transfer_apply )
{
   // This simple test touches SMT account balance objects, related functions (get/adjust)
   // and transfer operation that builds on them.
   try
   {
      ACTORS( (alice)(bob) )

      // Create SMT.
      signed_transaction tx, ty;
      asset_symbol_type alice_symbol = create_smt(tx, "alice", alice_private_key, 0);
      asset_symbol_type bob_symbol = create_smt(ty, "bob", bob_private_key, 1);

      // Give some SMT to creators.
      const account_object& alice_account = db->get_account("alice");
      db->adjust_balance(alice_account, asset(100, alice_symbol));
      const account_object& bob_account = db->get_account("bob");
      db->adjust_balance(bob_account, asset(110, bob_symbol));

      // Check pre-tranfer amounts.
      FC_ASSERT( db->get_balance( "alice", alice_symbol ).amount == 100, "SMT balance adjusting error" );
      FC_ASSERT( db->get_balance( "alice", bob_symbol ).amount == 0, "SMT balance adjusting error" );
      FC_ASSERT( db->get_balance( "bob", alice_symbol ).amount == 0, "SMT balance adjusting error" );
      FC_ASSERT( db->get_balance( "bob", bob_symbol ).amount == 110, "SMT balance adjusting error" );

      // Transfer SMT.
      transfer( "alice", "bob", asset(20, alice_symbol) );
      transfer( "bob", "alice", asset(50, bob_symbol) );

      // Check transfer outcome.
      FC_ASSERT( db->get_balance( "alice", alice_symbol ).amount == 80, "SMT transfer error" );
      FC_ASSERT( db->get_balance( "alice", bob_symbol ).amount == 50, "SMT transfer error" );
      FC_ASSERT( db->get_balance( "bob", alice_symbol ).amount == 20, "SMT transfer error" );
      FC_ASSERT( db->get_balance( "bob", bob_symbol ).amount == 60, "SMT transfer error" );
   }
   FC_LOG_AND_RETHROW()   
}

BOOST_AUTO_TEST_SUITE_END()
#endif
