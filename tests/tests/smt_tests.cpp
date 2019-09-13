#include <fc/macros.hpp>

#if defined IS_TEST_NET

FC_TODO(Extend testing scenarios to support multiple NAIs per account)

#include <boost/test/unit_test.hpp>

#include <steem/chain/steem_fwd.hpp>

#include <steem/protocol/exceptions.hpp>
#include <steem/protocol/hardfork.hpp>
#include <steem/protocol/smt_util.hpp>

#include <steem/chain/database.hpp>
#include <steem/chain/database_exceptions.hpp>
#include <steem/chain/steem_objects.hpp>
#include <steem/chain/smt_objects.hpp>

#include <steem/chain/util/smt_token.hpp>

#include "../db_fixture/database_fixture.hpp"

using namespace steem::chain;
using namespace steem::protocol;
using fc::string;
using boost::container::flat_set;
using boost::container::flat_map;

BOOST_FIXTURE_TEST_SUITE( smt_tests, smt_database_fixture )

BOOST_AUTO_TEST_CASE( smt_transfer_validate )
{
   try
   {
      ACTORS( (alice) )

      generate_block();

      asset_symbol_type alice_symbol = create_smt("alice", alice_private_key, 0);

      transfer_operation op;
      op.from = "alice";
      op.to = "bob";
      op.amount = asset(100, alice_symbol);
      op.validate();

      validate_database();
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

      generate_block();

      // Create SMT.
      asset_symbol_type alice_symbol = create_smt("alice", alice_private_key, 0);
      asset_symbol_type bob_symbol = create_smt("bob", bob_private_key, 1);

      // Give some SMT to creators.
      FUND( "alice", asset( 100, alice_symbol ) );
      FUND( "bob", asset( 110, bob_symbol ) );

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

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( asset_symbol_vesting_methods )
{
   try
   {
      BOOST_TEST_MESSAGE( "Test asset_symbol vesting methods" );

      asset_symbol_type Steem = STEEM_SYMBOL;
      FC_ASSERT( Steem.is_vesting() == false );
      FC_ASSERT( Steem.get_paired_symbol() == VESTS_SYMBOL );

      asset_symbol_type Vests = VESTS_SYMBOL;
      FC_ASSERT( Vests.is_vesting() );
      FC_ASSERT( Vests.get_paired_symbol() == STEEM_SYMBOL );

      asset_symbol_type Sbd = SBD_SYMBOL;
      FC_ASSERT( Sbd.is_vesting() == false );
      FC_ASSERT( Sbd.get_paired_symbol() == SBD_SYMBOL );

      ACTORS( (alice) )
      generate_block();
      auto smts = create_smt_3("alice", alice_private_key);
      {
         for( const asset_symbol_type& liquid_smt : smts )
         {
            FC_ASSERT( liquid_smt.is_vesting() == false );
            auto vesting_smt = liquid_smt.get_paired_symbol();
            FC_ASSERT( vesting_smt != liquid_smt );
            FC_ASSERT( vesting_smt.is_vesting() );
            FC_ASSERT( vesting_smt.get_paired_symbol() == liquid_smt );
         }
      }
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( vesting_smt_creation )
{
   try
   {
      BOOST_TEST_MESSAGE( "Test Creation of vesting SMT" );

      ACTORS((alice));
      generate_block();

      asset_symbol_type liquid_symbol = create_smt("alice", alice_private_key, 6);
      // Use liquid symbol/NAI to confirm smt object was created.
      auto liquid_object_by_symbol = util::smt::find_token( *db, liquid_symbol );
      FC_ASSERT( liquid_object_by_symbol != nullptr );

      asset_symbol_type vesting_symbol = liquid_symbol.get_paired_symbol();
      // Use vesting symbol/NAI to confirm smt object was created.
      auto vesting_object_by_symbol = util::smt::find_token( *db, vesting_symbol );
      FC_ASSERT( vesting_object_by_symbol != nullptr );

      // Check that liquid and vesting objects are the same one.
      FC_ASSERT( liquid_object_by_symbol == vesting_object_by_symbol );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_founder_vesting )
{
   using namespace steem::protocol;
   BOOST_TEST_MESSAGE( "Testing: is_founder_vesting and get_unit_target_account" );

   BOOST_TEST_MESSAGE( " -- Valid founder vesting" );
   BOOST_REQUIRE( smt::unit_target::is_founder_vesting( "$alice.vesting" ) );

   BOOST_TEST_MESSAGE( " -- Account name parsing" );
   BOOST_REQUIRE( smt::unit_target::get_unit_target_account( "$alice.vesting" ) == account_name_type( "alice" ) );

   BOOST_TEST_MESSAGE( " -- No possible room for an account name" );
   BOOST_REQUIRE( smt::unit_target::is_founder_vesting( "$.vesting" ) == false );

   BOOST_TEST_MESSAGE( " -- Meant to be founder vesting" );
   BOOST_REQUIRE( smt::unit_target::is_founder_vesting( "$@.vesting" ) );

   BOOST_TEST_MESSAGE( " -- Invalid account name upon retrieval" );
   BOOST_REQUIRE_THROW( smt::unit_target::get_unit_target_account( "$@.vesting" ), fc::assert_exception );

   BOOST_TEST_MESSAGE( " -- SMT special destinations" );
   BOOST_REQUIRE( smt::unit_target::is_founder_vesting( SMT_DESTINATION_FROM_VESTING ) == false );
   BOOST_REQUIRE( smt::unit_target::is_founder_vesting( SMT_DESTINATION_REWARDS ) == false );
   BOOST_REQUIRE( smt::unit_target::is_founder_vesting( SMT_DESTINATION_MARKET_MAKER ) == false );
   BOOST_REQUIRE( smt::unit_target::is_founder_vesting( SMT_DESTINATION_FROM ) == false );
   BOOST_REQUIRE( smt::unit_target::is_founder_vesting( SMT_DESTINATION_FROM_VESTING ) == false );
   BOOST_REQUIRE( smt::unit_target::is_founder_vesting( SMT_DESTINATION_VESTING ) == false );

   BOOST_TEST_MESSAGE( " -- Partial founder vesting special name" );
   BOOST_REQUIRE( smt::unit_target::is_founder_vesting( "$bob" ) == false );
   BOOST_REQUIRE_THROW( smt::unit_target::get_unit_target_account( "$bob" ), fc::assert_exception );

   BOOST_REQUIRE( smt::unit_target::is_founder_vesting( "bob.vesting" ) == false );

   BOOST_TEST_MESSAGE( " -- Valid account name that appears to be founder vesting" );
   BOOST_REQUIRE( smt::unit_target::get_unit_target_account( "bob.vesting" ) == account_name_type( "bob.vesting" ) );
}

BOOST_AUTO_TEST_CASE( tick_pricing_rules_validation )
{
   BOOST_TEST_MESSAGE( "Testing: tick_pricing_rules_validatation" );

   auto symbol = get_new_smt_symbol( 5, db );

   price tick_price;
   tick_price.base  = asset( 56, STEEM_SYMBOL );
   tick_price.quote = asset( 10, symbol );

   BOOST_TEST_MESSAGE( " -- Test success when STEEM is base symbol when paired with a token" );
   validate_tick_pricing( tick_price );

   BOOST_TEST_MESSAGE( " -- Test failure STEEM must be the base symbol when trading tokens" );
   tick_price = ~tick_price;
   BOOST_REQUIRE_THROW( validate_tick_pricing( tick_price ), fc::assert_exception );
   tick_price = ~tick_price;

   BOOST_TEST_MESSAGE( " -- Test failure quote symbol must be a power of 10" );
   tick_price.quote = asset( 11, symbol );
   BOOST_REQUIRE_THROW( validate_tick_pricing( tick_price ), fc::assert_exception );
   tick_price.quote = asset( 10, symbol );

   BOOST_TEST_MESSAGE( " -- Test failure SBD must be the base symbol when trading SBDs" );
   tick_price.quote = asset( 10, SBD_SYMBOL );
   tick_price.base  = asset( 10, STEEM_SYMBOL );
   BOOST_REQUIRE_THROW( validate_tick_pricing( tick_price ), fc::assert_exception );

   BOOST_TEST_MESSAGE( " -- Test success when SBD is base symbol when paired with STEEM" );
   tick_price = ~tick_price;
   tick_price.base.amount = 11;
   validate_tick_pricing( tick_price );
}

BOOST_AUTO_TEST_CASE( tick_pricing_rules )
{
   BOOST_TEST_MESSAGE( "Testing: tick_pricing_rules" );

   ACTORS( (alice)(bob)(charlie)(creator) )
   const auto& token = create_smt( "creator", creator_private_key, 3 );
   fund( "alice", asset( 1000000, STEEM_SYMBOL) );
   fund( "alice", asset( 1000000, SBD_SYMBOL) );
   fund( "alice", asset( 1000000, token) );
   fund( "bob", asset( 1000000, STEEM_SYMBOL) );
   fund( "bob", asset( 1000000, SBD_SYMBOL) );
   fund( "bob", asset( 1000000, token) );
   fund( "charlie", asset( 1000000, STEEM_SYMBOL) );

   witness_create( "charlie", charlie_private_key, "foo.bar", charlie_private_key.get_public_key(), 1000 );
   signed_transaction tx;

   BOOST_TEST_MESSAGE( "--- Test failure publishing price feed when quote is not power of 10" );
   feed_publish_operation fop;
   fop.publisher = "charlie";
   fop.exchange_rate = price( asset( 1000, SBD_SYMBOL ), asset( 1100000, STEEM_SYMBOL ) );
   tx.operations.push_back( fop );
   tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
   sign( tx, charlie_private_key );
   BOOST_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), fc::assert_exception );
   tx.operations.clear();
   tx.signatures.clear();

   BOOST_TEST_MESSAGE( "--- Test failure publishing price feed when SBD is not base" );
   fop.exchange_rate = price( asset( 1000000, STEEM_SYMBOL ), asset( 1000, SBD_SYMBOL ) );
   tx.operations.push_back( fop );
   tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
   sign( tx, charlie_private_key );
   BOOST_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), fc::assert_exception );
   tx.operations.clear();
   tx.signatures.clear();

   BOOST_TEST_MESSAGE( "--- Test success publishing price feed" );
   fop.exchange_rate = ~fop.exchange_rate;
   tx.operations.push_back( fop );
   tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
   sign( tx, charlie_private_key );
   db->push_transaction( tx, database::skip_transaction_dupe_check );
   tx.operations.clear();
   tx.signatures.clear();

   limit_order_create2_operation op;

   BOOST_TEST_MESSAGE( " -- Test success when matching two orders of the same token" );
   op.owner = "alice";
   op.amount_to_sell = asset( 1000, token );
   op.exchange_rate = price( asset( 125000, STEEM_SYMBOL ), asset( 1000, token ) );
   op.expiration = db->head_block_time() + fc::seconds( STEEM_MAX_LIMIT_ORDER_EXPIRATION );
   tx.operations.push_back( op );
   tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
   sign( tx, alice_private_key );
   db->push_transaction( tx, database::skip_transaction_dupe_check );
   tx.operations.clear();
   tx.signatures.clear();

   op.owner = "bob";
   op.amount_to_sell = asset( 125000, STEEM_SYMBOL );
   op.exchange_rate = price( asset( 125000, STEEM_SYMBOL ), asset( 1000, token ) );
   op.expiration = db->head_block_time() + fc::seconds( STEEM_MAX_LIMIT_ORDER_EXPIRATION );
   tx.operations.push_back( op );
   tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
   sign( tx, bob_private_key );
   db->push_transaction( tx, database::skip_transaction_dupe_check );
   tx.operations.clear();
   tx.signatures.clear();

   BOOST_REQUIRE( db->get_balance( "alice", STEEM_SYMBOL ) == asset( 1000000 + 125000, STEEM_SYMBOL ) );
   BOOST_REQUIRE( db->get_balance( "alice", token )        == asset( 1000000 - 1000, token ) );

   BOOST_REQUIRE( db->get_balance( "bob", STEEM_SYMBOL ) == asset( 1000000 - 125000, STEEM_SYMBOL ) );
   BOOST_REQUIRE( db->get_balance( "bob", token )        == asset( 1000000 + 1000, token ) );

   BOOST_TEST_MESSAGE( " -- Test failure when quote is not a power of 10" );
   op.owner = "bob";
   op.amount_to_sell = asset( 125000, STEEM_SYMBOL );
   op.exchange_rate = price( asset( 1000, SBD_SYMBOL ), asset( 1100000, STEEM_SYMBOL ) );
   op.expiration = db->head_block_time() + fc::seconds( STEEM_MAX_LIMIT_ORDER_EXPIRATION );
   tx.operations.push_back( op );
   tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
   sign( tx, bob_private_key );
   BOOST_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), fc::assert_exception );
   tx.operations.clear();
   tx.signatures.clear();

   BOOST_TEST_MESSAGE( " -- Test failure when STEEM is not base" );
   op.owner = "bob";
   op.amount_to_sell = asset( 125000, STEEM_SYMBOL );
   op.exchange_rate = price( asset( 10000, token ), asset( 125000, STEEM_SYMBOL ) );
   op.expiration = db->head_block_time() + fc::seconds( STEEM_MAX_LIMIT_ORDER_EXPIRATION );
   tx.operations.push_back( op );
   tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
   sign( tx, bob_private_key );
   BOOST_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), fc::assert_exception );
   tx.operations.clear();
   tx.signatures.clear();

   BOOST_TEST_MESSAGE( " -- Test failure when SBD is not base" );
   op.owner = "bob";
   op.amount_to_sell = asset( 1000000, STEEM_SYMBOL );
   op.exchange_rate = price( asset( 10000, STEEM_SYMBOL ), asset( 1000000, SBD_SYMBOL ) );
   op.expiration = db->head_block_time() + fc::seconds( STEEM_MAX_LIMIT_ORDER_EXPIRATION );
   tx.operations.push_back( op );
   tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
   sign( tx, bob_private_key );
   BOOST_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), fc::assert_exception );
   tx.operations.clear();
   tx.signatures.clear();

   BOOST_TEST_MESSAGE( " -- Test success when SBD is base" );
   op.owner = "bob";
   op.amount_to_sell = asset( 10000, STEEM_SYMBOL );
   op.exchange_rate = price( asset( 1000000, SBD_SYMBOL ), asset( 10000, STEEM_SYMBOL ) );
   op.expiration = db->head_block_time() + fc::seconds( STEEM_MAX_LIMIT_ORDER_EXPIRATION );
   tx.operations.push_back( op );
   tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
   sign( tx, bob_private_key );
   db->push_transaction( tx, database::skip_transaction_dupe_check );
   tx.operations.clear();
   tx.signatures.clear();

   BOOST_TEST_MESSAGE( " -- Test matching SBD:STEEM" );
   op.owner = "alice";
   op.amount_to_sell = asset( 1000000, SBD_SYMBOL );
   op.exchange_rate = price( asset( 1000000, SBD_SYMBOL ), asset( 10000, STEEM_SYMBOL ) );
   op.expiration = db->head_block_time() + fc::seconds( STEEM_MAX_LIMIT_ORDER_EXPIRATION );
   tx.operations.push_back( op );
   tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
   sign( tx, alice_private_key );
   db->push_transaction( tx, database::skip_transaction_dupe_check );
   tx.operations.clear();
   tx.signatures.clear();

   BOOST_REQUIRE( db->get_balance( "alice", STEEM_SYMBOL ) == asset( 1000000 + 125000 + 10000, STEEM_SYMBOL ) );
   BOOST_REQUIRE( db->get_balance( "alice", SBD_SYMBOL )   == asset( 1000000 - 1000000, SBD_SYMBOL ) );

   BOOST_REQUIRE( db->get_balance( "bob", STEEM_SYMBOL ) == asset( 1000000 - 125000 - 10000, STEEM_SYMBOL ) );
   BOOST_REQUIRE( db->get_balance( "bob", SBD_SYMBOL )   == asset( 1000000 + 1000000, SBD_SYMBOL ) );
}

/*
 * SMT legacy tests
 *
 * The logic tests in legacy tests *should* be entirely duplicated in smt_operation_tests.cpp
 * We are keeping these tests around to provide an additional layer re-assurance for the time being
 */
FC_TODO( "Remove SMT legacy tests and ensure code coverage is not reduced" );

BOOST_AUTO_TEST_CASE( setup_validate )
{
   try
   {
      smt_setup_operation op;

      ACTORS( (alice) )
      generate_block();
      asset_symbol_type alice_symbol = create_smt("alice", alice_private_key, 4);

      op.control_account = "";
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      //Invalid account
      op.control_account = "&&&&&&";
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      //FC_ASSERT( max_supply > 0 )
      op.control_account = "abcd";
      op.max_supply = -1;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      op.symbol = alice_symbol;

      //FC_ASSERT( max_supply > 0 )
      op.max_supply = 0;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      //FC_ASSERT( max_supply <= STEEM_MAX_SHARE_SUPPLY )
      op.max_supply = STEEM_MAX_SHARE_SUPPLY + 1;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      //FC_ASSERT( generation_begin_time > STEEM_GENESIS_TIME )
      op.max_supply = STEEM_MAX_SHARE_SUPPLY / 1000;
      op.contribution_begin_time = STEEM_GENESIS_TIME;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      fc::time_point_sec start_time = fc::variant( "2018-03-07T00:00:00" ).as< fc::time_point_sec >();
      fc::time_point_sec t50 = start_time + fc::seconds( 50 );
      fc::time_point_sec t100 = start_time + fc::seconds( 100 );
      fc::time_point_sec t200 = start_time + fc::seconds( 200 );
      fc::time_point_sec t300 = start_time + fc::seconds( 300 );

      op.contribution_begin_time = t100;
      op.contribution_end_time = t50;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      op.contribution_begin_time = t100;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      op.launch_time = t200;
      op.contribution_end_time = t300;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      op.contribution_begin_time = t50;
      op.contribution_end_time = t100;
      op.launch_time = t300;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      op.launch_time = t200;
      smt_capped_generation_policy gp = get_capped_generation_policy
      (
         get_generation_unit( { { "xyz", 1 } }, { { "xyz2", 2 } } )/*pre_soft_cap_unit*/,
         get_generation_unit( { { "xyz", 1 } }, { { "xyz2", 2 } } )/*post_soft_cap_unit*/,
         1/*min_unit_ratio*/,
         2/*max_unit_ratio*/
      );
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      units to_many_units;
      for( uint32_t i = 0; i < SMT_MAX_UNIT_ROUTES + 1; ++i )
         to_many_units.emplace( "alice" + std::to_string( i ), 1 );

      //FC_ASSERT( steem_unit.size() <= SMT_MAX_UNIT_ROUTES )
      gp.pre_soft_cap_unit.steem_unit = to_many_units;
      gp.pre_soft_cap_unit.token_unit = { { "bob",3 } };
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      gp.pre_soft_cap_unit.steem_unit = { { "bob2", 33 } };
      gp.pre_soft_cap_unit.token_unit = to_many_units;
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      //Invalid account
      gp.pre_soft_cap_unit.steem_unit = { { "{}{}", 12 } };
      gp.pre_soft_cap_unit.token_unit = { { "xyz", 13 } };
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      gp.pre_soft_cap_unit.steem_unit = { { "xyz2", 14 } };
      gp.pre_soft_cap_unit.token_unit = { { "{}", 15 } };
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      //Invalid account -> valid is '$from'
      gp.pre_soft_cap_unit.steem_unit = { { "$fromx", 1 } };
      gp.pre_soft_cap_unit.token_unit = { { "$from", 2 } };
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      gp.pre_soft_cap_unit.steem_unit = { { "$from", 3 } };
      gp.pre_soft_cap_unit.token_unit = { { "$from_", 4 } };
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      //Invalid account -> valid is '$from.vesting'
      gp.pre_soft_cap_unit.steem_unit = { { "$from.vestingx", 2 } };
      gp.pre_soft_cap_unit.token_unit = { { "$from.vesting", 222 } };
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      gp.pre_soft_cap_unit.steem_unit = { { "$from.vesting", 13 } };
      gp.pre_soft_cap_unit.token_unit = { { "$from.vesting.vesting", 3 } };
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      //FC_ASSERT( steem_unit.value > 0 );
      gp.pre_soft_cap_unit.steem_unit = { { "$from.vesting", 0 } };
      gp.pre_soft_cap_unit.token_unit = { { "$from.vesting", 2 } };
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      gp.pre_soft_cap_unit.steem_unit = { { "$from.vesting", 10 } };
      gp.pre_soft_cap_unit.token_unit = { { "$from.vesting", 0 } };
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      //FC_ASSERT( steem_unit.value > 0 );
      gp.pre_soft_cap_unit.steem_unit = { { "$from", 0 } };
      gp.pre_soft_cap_unit.token_unit = { { "$from", 100 } };
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      gp.pre_soft_cap_unit.steem_unit = { { "$from", 33 } };
      gp.pre_soft_cap_unit.token_unit = { { "$from", 0 } };
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      //FC_ASSERT( steem_unit.value > 0 );
      gp.pre_soft_cap_unit.steem_unit = { { "qprst", 0 } };
      gp.pre_soft_cap_unit.token_unit = { { "qprst", 67 } };
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      gp.pre_soft_cap_unit.steem_unit = { { "my_account2", 55 } };
      gp.pre_soft_cap_unit.token_unit = { { "my_account", 0 } };
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      gp.pre_soft_cap_unit.steem_unit = { { "bob", 2 }, { "$from.vesting", 3 }, { "$from", 4 } };
      gp.pre_soft_cap_unit.token_unit = { { "alice", 5 }, { "$from", 3 } };
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      gp.post_soft_cap_unit.steem_unit = { { "bob", 2 } };
      gp.post_soft_cap_unit.token_unit = {};
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      gp.post_soft_cap_unit.steem_unit = {};
      gp.post_soft_cap_unit.token_unit = { { "alice", 3 } };
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      gp.post_soft_cap_unit.steem_unit = {};
      gp.post_soft_cap_unit.token_unit = {};
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      gp.post_soft_cap_unit.steem_unit = {};
      gp.post_soft_cap_unit.token_unit = {};
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      gp.post_soft_cap_unit.steem_unit = { { "alice", 3 } };
      gp.post_soft_cap_unit.token_unit = { { "alice", 3 } };
      gp.pre_soft_cap_unit.steem_unit = { { "alice", 3 } };
      gp.pre_soft_cap_unit.token_unit = { { "alice", 3 } };
      op.initial_generation_policy = gp;
      op.steem_units_soft_cap = SMT_MIN_SOFT_CAP_STEEM_UNITS;
      op.steem_units_hard_cap = SMT_MIN_HARD_CAP_STEEM_UNITS;
      op.validate();

      gp.max_unit_ratio = ( ( 11 * SMT_MIN_HARD_CAP_STEEM_UNITS ) / SMT_MIN_SATURATION_STEEM_UNITS ) * 2;
      op.initial_generation_policy = gp;
      op.validate();

      gp.max_unit_ratio = 2;
      op.initial_generation_policy = gp;
      op.validate();

      smt_capped_generation_policy gp_valid = gp;

      gp.post_soft_cap_unit.steem_unit = { { "bob", 2 } };
      op.initial_generation_policy = gp;
      op.validate();

      gp = gp_valid;
      op.initial_generation_policy = gp;
      op.validate();

      uint16_t max_val_16 = std::numeric_limits<uint16_t>::max();
      uint32_t max_val_32 = std::numeric_limits<uint32_t>::max();

      gp.min_unit_ratio = max_val_32;
      gp.post_soft_cap_unit.steem_unit = { { "abc", 1 } };
      gp.post_soft_cap_unit.token_unit = { { "abc1", max_val_16 } };
      gp.pre_soft_cap_unit.token_unit = { { "abc2", max_val_16 } };
      op.initial_generation_policy = gp;
      op.validate();

      gp.min_unit_ratio = 1;
      gp.post_soft_cap_unit.token_unit = { { "abc1", 1 } };
      gp.pre_soft_cap_unit.token_unit = { { "abc2", 1 } };
      gp.post_soft_cap_unit.steem_unit = { { "abc3", max_val_16 } };
      gp.pre_soft_cap_unit.steem_unit = { { "abc34", max_val_16 } };
      op.initial_generation_policy = gp;
      op.validate();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( setup_authorities )
{
   try
   {
      smt_setup_operation op;
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

BOOST_AUTO_TEST_CASE( setup_apply )
{
   try
   {
      ACTORS( (alice)(bob)(xyz)(xyz2) )

      generate_block();

      FUND( "alice", 10 * 1000 * 1000 );
      FUND( "bob", 10 * 1000 * 1000 );

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );

      smt_setup_operation op;
      op.control_account = "alice";
      op.steem_units_soft_cap = SMT_MIN_SOFT_CAP_STEEM_UNITS;
      op.steem_units_hard_cap = SMT_MIN_HARD_CAP_STEEM_UNITS;

      smt_capped_generation_policy gp = get_capped_generation_policy
      (
         get_generation_unit( { { "xyz", 1 } }, { { "xyz2", 2 } } )/*pre_soft_cap_unit*/,
         get_generation_unit( { { "xyz", 1 } }, { { "xyz2", 2 } } )/*post_soft_cap_unit*/,
         1/*min_unit_ratio*/,
         2/*max_unit_ratio*/
      );

      fc::time_point_sec start_time        = fc::variant( "2021-01-01T00:00:00" ).as< fc::time_point_sec >();
      fc::time_point_sec start_time_plus_1 = start_time + fc::seconds(1);

      op.initial_generation_policy = gp;
      op.contribution_begin_time = start_time;
      op.contribution_end_time = op.launch_time = start_time_plus_1;

      signed_transaction tx;

      //SMT doesn't exist
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, alice_private_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      tx.operations.clear();
      tx.signatures.clear();

      //Try to elevate account
      asset_symbol_type alice_symbol = create_smt( "alice", alice_private_key, 3 );
      tx.operations.clear();
      tx.signatures.clear();

      //Make transaction again. Everything is correct.
      op.symbol = alice_symbol;
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );
      tx.operations.clear();
      tx.signatures.clear();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_create_apply )
{
   try
   {
      ACTORS( (alice)(bob) )

      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );

      const dynamic_global_property_object& dgpo = db->get_dynamic_global_properties();
      asset required_creation_fee = dgpo.smt_creation_fee;
      unsigned int test_amount = required_creation_fee.amount.value;

      smt_create_operation op;
      op.control_account = "alice";
      op.symbol = get_new_smt_symbol( 3, db );
      op.precision = op.symbol.decimals();

      BOOST_TEST_MESSAGE( " -- SMT create with insufficient SBD balance" );
      // Fund with STEEM, and set fee with SBD.
      FUND( "alice", test_amount );
      // Declare fee in SBD/TBD though alice has none.
      op.smt_creation_fee = asset( test_amount, SBD_SYMBOL );
      // Throw due to insufficient balance of SBD/TBD.
      FAIL_WITH_OP(op, alice_private_key, fc::assert_exception);

      BOOST_TEST_MESSAGE( " -- SMT create with insufficient STEEM balance" );
      // Now fund with SBD, and set fee with STEEM.
      convert( "alice", asset( test_amount, STEEM_SYMBOL ) );
      // Declare fee in STEEM though alice has none.
      op.smt_creation_fee = asset( test_amount, STEEM_SYMBOL );
      // Throw due to insufficient balance of STEEM.
      FAIL_WITH_OP(op, alice_private_key, fc::assert_exception);

      BOOST_TEST_MESSAGE( " -- SMT create with available funds" );
      // Push valid operation.
      op.smt_creation_fee = asset( test_amount, SBD_SYMBOL );
      PUSH_OP( op, alice_private_key );

      BOOST_TEST_MESSAGE( " -- SMT cannot be created twice even with different precision" );
      create_conflicting_smt(op.symbol, "alice", alice_private_key);

      BOOST_TEST_MESSAGE( " -- Another user cannot create an SMT twice even with different precision" );
      // Check that another user/account can't be used to create duplicating SMT even with different precision.
      create_conflicting_smt(op.symbol, "bob", bob_private_key);

      BOOST_TEST_MESSAGE( " -- Check that an SMT cannot be created with decimals greater than STEEM_MAX_DECIMALS" );
      // Check that invalid SMT can't be created
      create_invalid_smt("alice", alice_private_key);

      BOOST_TEST_MESSAGE( " -- Check that an SMT cannot be created with a creation fee lower than required" );
      // Check fee set too low.
      asset fee_too_low = required_creation_fee;
      unsigned int too_low_fee_amount = required_creation_fee.amount.value-1;
      fee_too_low.amount -= 1;

      SMT_SYMBOL( bob, 0, db );
      op.control_account = "bob";
      op.symbol = bob_symbol;
      op.precision = op.symbol.decimals();

      BOOST_TEST_MESSAGE( " -- Check that we cannot create an SMT with an insufficent STEEM creation fee" );
      // Check too low fee in STEEM.
      FUND( "bob", too_low_fee_amount );
      op.smt_creation_fee = asset( too_low_fee_amount, STEEM_SYMBOL );
      FAIL_WITH_OP(op, bob_private_key, fc::assert_exception);

      BOOST_TEST_MESSAGE( " -- Check that we cannot create an SMT with an insufficent SBD creation fee" );
      // Check too low fee in SBD.
      convert( "bob", asset( too_low_fee_amount, STEEM_SYMBOL ) );
      op.smt_creation_fee = asset( too_low_fee_amount, SBD_SYMBOL );
      FAIL_WITH_OP(op, bob_private_key, fc::assert_exception);

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
#endif
