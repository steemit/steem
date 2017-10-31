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
#include <steem/chain/util/scheduler.hpp>

#include "../db_fixture/database_fixture.hpp"

using namespace steem::chain;
using namespace steem::protocol;
using fc::string;
using boost::container::flat_set;

BOOST_FIXTURE_TEST_SUITE( smt_tests, smt_database_fixture )

BOOST_AUTO_TEST_CASE( smt_create_validate )
{
   try
   {
      SMT_SYMBOL( alice, 3 );

      smt_create_operation op;
      op.control_account = "@@@@@";
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

BOOST_AUTO_TEST_CASE( smt_create_apply )
{
   try
   {
      set_price_feed( price( ASSET( "1.000 TESTS" ), ASSET( "1.000 TBD" ) ) );

      ACTORS( (alice)(bob) )
      SMT_SYMBOL( alice, 3 );

      fund( "alice", 10 * 1000 * 1000 );

      smt_create_operation op;

      op.smt_creation_fee = ASSET( "1000.000 TBD" );
      op.control_account = "alice";
      op.symbol = alice_symbol;
      op.precision = op.symbol.decimals();

      signed_transaction tx;

      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      // throw due to insufficient balance
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      convert( "alice", ASSET( "5000.000 TESTS" ) );
      db->push_transaction( tx, 0 );

      // Check the SMT cannot be created twice
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), fc::exception );

      // TODO:
      // - Check that 1000 TESTS throws
      // - Check that less than 1000 TBD throws
      // - Check that more than 1000 TBD succeeds
      //
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
      op.control_account = "@@@@@";
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
      set_price_feed( price( ASSET( "1.000 TESTS" ), ASSET( "1.000 TBD" ) ) );
      ACTORS( (alice)(bob) )
      SMT_SYMBOL( alice, 3 );

      smt_setup_emissions_operation op;
      op.control_account = "alice";
      fc::time_point now = fc::time_point::now();
      op.schedule_time = now;
      op.emissions_unit.token_unit["bob"] = 10;
      op.lep_abs_amount = op.rep_abs_amount = asset( 1000, alice_symbol );

      signed_transaction tx;

      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );

      // Throw due to non-existing SMT (too early).
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );

      // Create SMT.
      signed_transaction ty;
      op.symbol = create_smt(ty, "alice", alice_private_key, 3);
      FC_ASSERT( op.symbol == alice_symbol, "SMT symbol mismatch ${s1} vs ${s2}",
         ("s1", op.symbol.to_nai())("s2", alice_symbol.to_nai()) );

      // TODO: Replace the code below with account setup operation execution once its implemented.
      const steem::chain::smt_token_object* smt = db->find< steem::chain::smt_token_object, by_symbol >( alice_symbol );
      FC_ASSERT( smt != nullptr, "The SMT has just been created!" );
      FC_ASSERT( smt->phase < steem::chain::smt_token_object::smt_phase::setup_completed, "Who closed setup phase?!" );
      db->modify( *smt, [&]( steem::chain::smt_token_object& token )
      {
         token.phase = steem::chain::smt_token_object::smt_phase::setup_completed;
      });
      signed_transaction tz;
      tz.operations.push_back( op );
      tz.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tz.sign( alice_private_key, db->get_chain_id() );
      // Throw due to closed setup phase (too late).
      STEEM_REQUIRE_THROW( db->push_transaction( tz, database::skip_transaction_dupe_check ), fc::assert_exception );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( set_setup_parameters_apply )
{
   try
   {
      set_price_feed( price( ASSET( "1.000 TESTS" ), ASSET( "1.000 TBD" ) ) );
      ACTORS( (dany)(eddy) )

      fund( "dany", 5000 );
      convert( "dany", ASSET( "5000.000 TESTS" ) );

      signed_transaction tx;
      
      smt_set_setup_parameters_operation op;
      op.control_account = "dany";

      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( dany_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception ); // no SMT

      // create SMT
      signed_transaction ty;
      op.symbol = create_smt(ty, "dany", dany_private_key, 3);

      signed_transaction tz;
      tz.operations.push_back( op );
      tz.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tz.sign( dany_private_key, db->get_chain_id() );

      db->push_transaction( tz, 0 );

      db->push_transaction( tz, database::skip_transaction_dupe_check );

      signed_transaction tx1;

      op.setup_parameters.clear();
      op.setup_parameters.emplace( smt_param_allow_vesting() );
      op.setup_parameters.emplace( smt_param_allow_voting() );
      tx1.operations.push_back( op );
      tx1.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx1.sign( eddy_private_key, db->get_chain_id() );
      
      STEEM_REQUIRE_THROW( db->push_transaction( tx1, 0 ), fc::exception ); // wrong private key
      
      signed_transaction tx2;

      op.setup_parameters.clear();
      op.setup_parameters.emplace( smt_param_allow_vesting({false}) );
      op.setup_parameters.emplace( smt_param_allow_voting({false}) );
      tx2.operations.push_back( op );
      tx2.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx2.sign( dany_private_key, db->get_chain_id() );

      db->push_transaction( tx2, 0 );
      
      signed_transaction tx3;

      op.setup_parameters.clear();
      op.setup_parameters.emplace( smt_param_allow_vesting() );
      tx3.operations.push_back( op );
      tx3.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx3.sign( dany_private_key, db->get_chain_id() );

      db->push_transaction( tx3, 0 );
      
      // TODO:
      // - check applying smt_set_setup_parameters_operation after setup completed
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
      set_price_feed( price( ASSET( "1.000 TESTS" ), ASSET( "1.000 TBD" ) ) );

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

      signed_transaction tx;

      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );

      //First we should create SMT
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      tx.operations.clear();
      tx.signatures.clear();

      //Try to create SMT
      op.symbol = create_smt( tx, "alice", alice_private_key, 3 );
      tx.operations.clear();
      tx.signatures.clear();

      //Make transaction again.
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( scheduler_basic )
{
   try
   {
      set_price_feed( price( ASSET( "1.000 TESTS" ), ASSET( "1.000 TBD" ) ) );

      ACTORS( (aaa)(bbb)(ccc)(ddd)(eee)(fff) )

      fc::time_point_sec t900( 900 );
      fc::time_point_sec t1000( 1000 );
      fc::time_point_sec t1500( 1500 );
      fc::time_point_sec t1800( 1800 );
      fc::time_point_sec t2000( 2000 );
      fc::time_point_sec t2100( 2100 );
      fc::time_point_sec tmilion( 1000000 );                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       

      phase_times times = { phase_time( phase::contribution_end, tmilion ) };

      asset_symbol_type a_symbol = prepare_scheduler_data( "aaa", aaa_private_key, 3, times );
      asset_symbol_type b_symbol = prepare_scheduler_data( "bbb", bbb_private_key, 3, times );
      asset_symbol_type c_symbol = prepare_scheduler_data( "ccc", ccc_private_key, 3, times );
      asset_symbol_type d_symbol = prepare_scheduler_data( "ddd", ddd_private_key, 3, times );
      asset_symbol_type e_symbol = prepare_scheduler_data( "eee", eee_private_key, 3, times );
      asset_symbol_type f_symbol = prepare_scheduler_data( "fff", fff_private_key, 3, times );

//==========================================================
      db->scheduler_add( t1000, util::contribution_begin_scheduler_event( a_symbol ) );
      BOOST_REQUIRE( db->scheduler_size() == 1 );
      BOOST_REQUIRE( db->scheduler_size( t1000 ) == 1 );

      db->scheduler_run( t2000 );
      BOOST_REQUIRE( db->scheduler_size() == 1 );
      BOOST_REQUIRE( db->scheduler_size( tmilion ) == 1 );
      db->scheduler_close();
//==========================================================
      db->scheduler_add( t1000, util::contribution_begin_scheduler_event( a_symbol ) );
      BOOST_REQUIRE( db->scheduler_size() == 1 );
      BOOST_REQUIRE( db->scheduler_size( t1000 ) == 1 );
      BOOST_REQUIRE( db->scheduler_size( t2000 ) == 0 );

      db->scheduler_add( t1000, util::contribution_begin_scheduler_event( b_symbol ) );
      BOOST_REQUIRE( db->scheduler_size() == 2 );
      BOOST_REQUIRE( db->scheduler_size( t1000 ) == 2 );
      BOOST_REQUIRE( db->scheduler_size( t2000 ) == 0 );

      db->scheduler_add( t1500, util::contribution_begin_scheduler_event( c_symbol ) );
      BOOST_REQUIRE( db->scheduler_size() == 3 );
      BOOST_REQUIRE( db->scheduler_size( t1000 ) == 2 );
      BOOST_REQUIRE( db->scheduler_size( t1500 ) == 1 );
      BOOST_REQUIRE( db->scheduler_size( t2000 ) == 0 );

      db->scheduler_run( t900 );
      BOOST_REQUIRE( db->scheduler_size() == 3 );
      BOOST_REQUIRE( db->scheduler_size( t1000 ) == 2 );
      BOOST_REQUIRE( db->scheduler_size( t1500 ) == 1 );

      db->scheduler_run( t1500 );
      BOOST_REQUIRE( db->scheduler_size() == 3 );
      BOOST_REQUIRE( db->scheduler_size( tmilion ) == 3 );
      db->scheduler_close();
//==========================================================
      db->scheduler_add( t1000, util::contribution_begin_scheduler_event( a_symbol ) );
      db->scheduler_add( t2000, util::contribution_begin_scheduler_event( b_symbol ) );
      BOOST_REQUIRE( db->scheduler_size() == 2 );
      BOOST_REQUIRE( db->scheduler_size( t1000 ) == 1 );
      BOOST_REQUIRE( db->scheduler_size( t1500 ) == 0 );
      BOOST_REQUIRE( db->scheduler_size( t2000 ) == 1 );

      db->scheduler_add( t2000, util::contribution_begin_scheduler_event( c_symbol ) );
      BOOST_REQUIRE( db->scheduler_size() == 3 );
      BOOST_REQUIRE( db->scheduler_size( t1000 ) == 1 );
      BOOST_REQUIRE( db->scheduler_size( t2000 ) == 2 );

      db->scheduler_run( t1000 );
      BOOST_REQUIRE( db->scheduler_size() == 3 );
      BOOST_REQUIRE( db->scheduler_size( tmilion ) == 1 );
      BOOST_REQUIRE( db->scheduler_size( t2000 ) == 2 );

      db->scheduler_run( t1500 );
      BOOST_REQUIRE( db->scheduler_size() == 3 );
      BOOST_REQUIRE( db->scheduler_size( tmilion ) == 1 );
      BOOST_REQUIRE( db->scheduler_size( t2000 ) == 2 );

      db->scheduler_run( t2000 );
      BOOST_REQUIRE( db->scheduler_size() == 3 );
      BOOST_REQUIRE( db->scheduler_size( tmilion ) == 3 );
      db->scheduler_close();
//==========================================================
      db->scheduler_add( t1000, util::contribution_begin_scheduler_event( a_symbol ) );
      db->scheduler_add( t1000, util::contribution_begin_scheduler_event( b_symbol ) );
      db->scheduler_add( t1000, util::contribution_begin_scheduler_event( c_symbol ) );
      db->scheduler_add( t2000, util::contribution_begin_scheduler_event( d_symbol ) );
      db->scheduler_add( t2000, util::contribution_begin_scheduler_event( e_symbol ) );
      BOOST_REQUIRE( db->scheduler_size() == 5 );

      db->scheduler_run( t1500 );
      BOOST_REQUIRE( db->scheduler_size() == 5 );
      BOOST_REQUIRE( db->scheduler_size( tmilion ) == 3 );
      BOOST_REQUIRE( db->scheduler_size( t2000 ) == 2 );

      db->scheduler_run( t2000 );
      BOOST_REQUIRE( db->scheduler_size() == 5 );
      BOOST_REQUIRE( db->scheduler_size( tmilion ) == 5 );
      db->scheduler_close();
//==========================================================
      db->scheduler_add( t1000, util::contribution_begin_scheduler_event( a_symbol ) );
      db->scheduler_add( t1500, util::contribution_begin_scheduler_event( b_symbol ) );
      db->scheduler_add( t1500, util::contribution_begin_scheduler_event( c_symbol ) );
      db->scheduler_add( t2000, util::contribution_begin_scheduler_event( d_symbol ) );
      db->scheduler_add( t2000, util::contribution_begin_scheduler_event( e_symbol ) );
      db->scheduler_add( t2000, util::contribution_begin_scheduler_event( f_symbol ) );
      BOOST_REQUIRE( db->scheduler_size() == 6 );
      BOOST_REQUIRE( db->scheduler_size( t1000 ) == 1 );
      BOOST_REQUIRE( db->scheduler_size( t1500 ) == 2 );
      BOOST_REQUIRE( db->scheduler_size( t2000 ) == 3 );

      db->scheduler_run( t1500 );
      BOOST_REQUIRE( db->scheduler_size() == 6 );
      BOOST_REQUIRE( db->scheduler_size( tmilion ) == 3 );
      BOOST_REQUIRE( db->scheduler_size( t2000 ) == 3 );

      db->scheduler_run( t2100 );
      BOOST_REQUIRE( db->scheduler_size() == 6 );
      BOOST_REQUIRE( db->scheduler_size( tmilion ) == 6 );
      db->scheduler_close();
//==========================================================
      db->scheduler_add( t2000, util::contribution_begin_scheduler_event( a_symbol ) );
      db->scheduler_add( t1000, util::contribution_begin_scheduler_event( b_symbol ) );
      db->scheduler_add( t1800, util::contribution_begin_scheduler_event( c_symbol ) );
      db->scheduler_add( t1500, util::contribution_begin_scheduler_event( d_symbol ) );
      BOOST_REQUIRE( db->scheduler_size() == 4 );
      BOOST_REQUIRE( db->scheduler_size( t2000 ) == 1 );
      BOOST_REQUIRE( db->scheduler_size( t1000 ) == 1 );
      BOOST_REQUIRE( db->scheduler_size( t1800 ) == 1 );
      BOOST_REQUIRE( db->scheduler_size( t1500 ) == 1 );

      db->scheduler_run( t2000 );
      BOOST_REQUIRE( db->scheduler_size() == 4 );
      BOOST_REQUIRE( db->scheduler_size( tmilion ) == 4 );
      db->scheduler_close();
//==========================================================
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( scheduler_cycle )
{
   try
   {
      set_price_feed( price( ASSET( "1.000 TESTS" ), ASSET( "1.000 TBD" ) ) );

      ACTORS( (aaa) )

      fc::time_point_sec t09( 9 );
      fc::time_point_sec t10( 10 );
      fc::time_point_sec t15( 15 );
      fc::time_point_sec t20( 20 );
      fc::time_point_sec t30( 30 );
      fc::time_point_sec t40( 40 );
      fc::time_point_sec t41( 41 );

      phase_times times = {
         phase_time( phase::contribution_end, t20 ),
         phase_time( phase::launch, t30 ),
         phase_time( phase::launch_expiration, t40 )
         };

      asset_symbol_type a_symbol = prepare_scheduler_data( "aaa", aaa_private_key, 3, times );

      db->scheduler_add( t10, util::contribution_begin_scheduler_event( a_symbol ) );

      db->scheduler_run( t09 );
      BOOST_REQUIRE( db->scheduler_size() == 1 );
      BOOST_REQUIRE( db->scheduler_size( t09 ) == 0 );
      BOOST_REQUIRE( db->scheduler_size( t10 ) == 1 );

      db->scheduler_run( t10 );
      BOOST_REQUIRE( db->scheduler_size() == 1 );
      BOOST_REQUIRE( db->scheduler_size( t10 ) == 0 );
      BOOST_REQUIRE( db->scheduler_size( t20 ) == 1 );

      db->scheduler_run( t15 );
      BOOST_REQUIRE( db->scheduler_size() == 1 );
      BOOST_REQUIRE( db->scheduler_size( t10 ) == 0 );
      BOOST_REQUIRE( db->scheduler_size( t15 ) == 0 );
      BOOST_REQUIRE( db->scheduler_size( t20 ) == 1 );

      db->scheduler_run( t20 );
      BOOST_REQUIRE( db->scheduler_size() == 1 );
      BOOST_REQUIRE( db->scheduler_size( t10 ) == 0 );
      BOOST_REQUIRE( db->scheduler_size( t20 ) == 0 );
      BOOST_REQUIRE( db->scheduler_size( t30 ) == 1 );

      db->scheduler_run( t30 );
      BOOST_REQUIRE( db->scheduler_size() == 2 );
      BOOST_REQUIRE( db->scheduler_size( t10 ) == 0 );
      BOOST_REQUIRE( db->scheduler_size( t20 ) == 0 );
      BOOST_REQUIRE( db->scheduler_size( t30 ) == 1 );
      BOOST_REQUIRE( db->scheduler_size( t40 ) == 1 );

      db->scheduler_run( t40 );
      BOOST_REQUIRE( db->scheduler_size() == 1 );
      BOOST_REQUIRE( db->scheduler_size( t30 ) == 1 );

      db->scheduler_run( t41 );
      BOOST_REQUIRE( db->scheduler_size() == 0 );
   }
   FC_LOG_AND_RETHROW()
}


BOOST_AUTO_TEST_SUITE_END()
#endif
