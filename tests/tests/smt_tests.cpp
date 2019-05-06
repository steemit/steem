#include <fc/macros.hpp>

#if defined IS_TEST_NET && defined STEEM_ENABLE_SMT

FC_TODO(Extend testing scenarios to support multiple NAIs per account)

#include <boost/test/unit_test.hpp>

#include <steem/chain/steem_fwd.hpp>

#include <steem/protocol/exceptions.hpp>
#include <steem/protocol/hardfork.hpp>

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

BOOST_AUTO_TEST_CASE( smt_refund_validate )
{
   try
   {
      ACTORS( (creator) )
      generate_block();
      asset_symbol_type creator_symbol = create_smt("creator", creator_private_key, 0);

      smt_refund_operation op;
      op.executor = "executor";
      op.contributor = "contributor";
      op.contribution_id = 0;
      op.symbol = creator_symbol;
      op.amount = ASSET( "1.000 TESTS" );
      op.validate();

      op.executor = "@@@@@";
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );
      op.executor = "executor";

      op.contributor = "@@@@@";
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );
      op.contributor = "contributor";

      op.symbol = op.amount.symbol;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );
      op.symbol = creator_symbol;

      op.amount = asset( 1, creator_symbol );
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );
      op.amount = ASSET( "1.000 TESTS" );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_refund_authorities )
{
   try
   {
      smt_refund_operation op;
      op.executor = "executor";

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.get_required_posting_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      expected.insert( "executor" );
      op.get_required_active_authorities( auths );
      BOOST_REQUIRE( auths == expected );
   }
   FC_LOG_AND_RETHROW()
}

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

BOOST_AUTO_TEST_CASE( comment_votable_assers_validate )
{
   try
   {
      BOOST_TEST_MESSAGE( "Test Comment Votable Assets Validate" );
      ACTORS((alice));

      generate_block();

      std::array<asset_symbol_type, SMT_MAX_VOTABLE_ASSETS + 1> smts;
      /// Create one more than limit to test negative cases
      for(size_t i = 0; i < SMT_MAX_VOTABLE_ASSETS + 1; ++i)
      {
         asset_symbol_type smt = create_smt("alice", alice_private_key, 0);
         smts[i] = std::move(smt);
      }

      {
         comment_options_operation op;

         op.author = "alice";
         op.permlink = "test";

         BOOST_TEST_MESSAGE( "--- Testing valid configuration: no votable_assets" );
         allowed_vote_assets ava;
         op.extensions.insert( ava );
         op.validate();
      }

      {
         comment_options_operation op;

         op.author = "alice";
         op.permlink = "test";

         BOOST_TEST_MESSAGE( "--- Testing valid configuration of votable_assets" );
         allowed_vote_assets ava;
         for(size_t i = 0; i < SMT_MAX_VOTABLE_ASSETS; ++i)
         {
            const auto& smt = smts[i];
            ava.add_votable_asset(smt, share_type(10 + i), (i & 2) != 0);
         }

         op.extensions.insert( ava );
         op.validate();
      }

      {
         comment_options_operation op;

         op.author = "alice";
         op.permlink = "test";

         BOOST_TEST_MESSAGE( "--- Testing invalid configuration of votable_assets - too much assets specified" );
         allowed_vote_assets ava;
         for(size_t i = 0; i < smts.size(); ++i)
         {
            const auto& smt = smts[i];
            ava.add_votable_asset(smt, share_type(20 + i), (i & 2) != 0);
         }

         op.extensions.insert( ava );
         STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      }

      {
         comment_options_operation op;

         op.author = "alice";
         op.permlink = "test";

         BOOST_TEST_MESSAGE( "--- Testing invalid configuration of votable_assets - STEEM added to container" );
         allowed_vote_assets ava;
         const auto& smt = smts.front();
         ava.add_votable_asset(smt, share_type(20), false);
         ava.add_votable_asset(STEEM_SYMBOL, share_type(20), true);
         op.extensions.insert( ava );
         STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      }
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( cap_commit_reveal_validate )
{
   try
   {
      smt_revealed_cap reveal0;
      reveal0.amount = 0;
      reveal0.nonce = 0;
      smt_revealed_cap revealMaxPlus;
      revealMaxPlus.amount = STEEM_MAX_SHARE_SUPPLY + 1;
      revealMaxPlus.nonce = 0;
      smt_revealed_cap reveal1M;
      reveal1M.amount = 1000000;
      reveal1M.nonce = 0;
      smt_revealed_cap reveal1M1234;
      reveal1M1234.amount = 1000000;
      reveal1M1234.nonce = 1234;
      smt_revealed_cap reveal1K1234;
      reveal1K1234.amount = 1000;
      reveal1K1234.nonce = 1234;
      smt_revealed_cap reveal1G1234;
      reveal1G1234.amount = 1000000000;
      reveal1G1234.nonce = 1234;
      smt_revealed_cap reveal1M4321;
      reveal1M1234.amount = 1000000;
      reveal1M1234.nonce = 4321;

      // Public (i.e. non-hidden) cap
      // Test cap value too low
      smt_cap_commitment public_cap0;
      public_cap0.lower_bound = public_cap0.upper_bound = 0;
      public_cap0.hash = fc::sha256::hash( reveal0 );
      STEEM_REQUIRE_THROW( public_cap0.validate(), fc::assert_exception );
      // Test cap value too big
      smt_cap_commitment public_capMaxPlus;
      public_capMaxPlus.lower_bound = public_capMaxPlus.upper_bound = STEEM_MAX_SHARE_SUPPLY + 1;
      public_capMaxPlus.hash = fc::sha256::hash( revealMaxPlus );
      STEEM_REQUIRE_THROW( public_capMaxPlus.validate(), fc::assert_exception );
      // Test valid commitment cap ...
      smt_cap_commitment public_cap1M;
      public_cap1M.lower_bound = public_cap1M.upper_bound = 1000000;
      public_cap1M.hash = fc::sha256::hash( reveal1M );
      public_cap1M.validate();
      // ... matching reveal
      reveal1M.validate( public_cap1M );

      // Secret (hidden) cap
      smt_cap_commitment hidden_cap1M1234;
      hidden_cap1M1234.lower_bound =  500000;
      hidden_cap1M1234.upper_bound = 1500000;
      hidden_cap1M1234.hash = fc::sha256::hash( reveal1M1234 );
      hidden_cap1M1234.validate();
      // Test reveal too low
      STEEM_REQUIRE_THROW( reveal1K1234.validate(hidden_cap1M1234), fc::assert_exception );
      // Test reveal too big
      STEEM_REQUIRE_THROW( reveal1G1234.validate(hidden_cap1M1234), fc::assert_exception );
      // Test wrong nonce
      STEEM_REQUIRE_THROW( reveal1M4321.validate(hidden_cap1M1234), fc::assert_exception );
      // Test valid commitment cap matching reveal
      reveal1M1234.validate( hidden_cap1M1234 );
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
      op.generation_begin_time = STEEM_GENESIS_TIME;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      fc::time_point_sec start_time = fc::variant( "2018-03-07T00:00:00" ).as< fc::time_point_sec >();
      fc::time_point_sec t50 = start_time + fc::seconds( 50 );
      fc::time_point_sec t100 = start_time + fc::seconds( 100 );
      fc::time_point_sec t200 = start_time + fc::seconds( 200 );
      fc::time_point_sec t300 = start_time + fc::seconds( 300 );

      //FC_ASSERT( generation_end_time > generation_begin_time )
      op.generation_begin_time = t100;
      op.generation_end_time = t50;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      //FC_ASSERT( generation_end_time > generation_begin_time )
      op.generation_end_time = t100;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      //FC_ASSERT( announced_launch_time >= generation_end_time )
      op.announced_launch_time = t200;
      op.generation_end_time = t300;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      //FC_ASSERT( launch_expiration_time >= announced_launch_time )
      op.generation_begin_time = t50;
      op.generation_end_time = t100;
      op.announced_launch_time = t300;
      op.launch_expiration_time = t200;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      op.announced_launch_time = t200;
      op.launch_expiration_time = t300;
      smt_capped_generation_policy gp = get_capped_generation_policy
      (
         get_generation_unit( { { "xyz", 1 } }, { { "xyz2", 2 } } )/*pre_soft_cap_unit*/,
         get_generation_unit()/*post_soft_cap_unit*/,
         get_cap_commitment( 1 )/*min_steem_units_commitment*/,
         get_cap_commitment( SMT_MIN_HARD_CAP_STEEM_UNITS + 1 )/*hard_cap_steem_units_commitment*/,
         STEEM_100_PERCENT/*soft_cap_percent*/,
         1/*min_unit_ratio*/,
         2/*max_unit_ratio*/
      );
      op.initial_generation_policy = gp;
      op.validate();

      //FC_ASSERT(decimal_places <= SMT_MAX_DECIMAL_PLACES)
      op.decimal_places = SMT_MAX_DECIMAL_PLACES + 1;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      op.decimal_places = 3;

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
      op.validate();

      //FC_ASSERT( lower_bound > 0 )
      gp.min_steem_units_commitment.lower_bound = 0;
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      //FC_ASSERT( lower_bound >= SMT_MIN_HARD_CAP_STEEM_UNITS )
      gp.min_steem_units_commitment.lower_bound = SMT_MIN_HARD_CAP_STEEM_UNITS - 1;
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      //FC_ASSERT( upper_bound <= STEEM_MAX_SHARE_SUPPLY )
      gp.min_steem_units_commitment.upper_bound = STEEM_MAX_SHARE_SUPPLY + 1;
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      //FC_ASSERT( lower_bound <= upper_bound )
      gp.min_steem_units_commitment.lower_bound = STEEM_MAX_SHARE_SUPPLY - 1;
      gp.min_steem_units_commitment.upper_bound = gp.min_steem_units_commitment.lower_bound - 1;
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      gp.min_steem_units_commitment.lower_bound = 1 * SMT_MIN_HARD_CAP_STEEM_UNITS;
      gp.min_steem_units_commitment.upper_bound = 2 * SMT_MIN_HARD_CAP_STEEM_UNITS;
      gp.hard_cap_steem_units_commitment.lower_bound = 1 * SMT_MIN_HARD_CAP_STEEM_UNITS;
      gp.hard_cap_steem_units_commitment.upper_bound = 2 * SMT_MIN_HARD_CAP_STEEM_UNITS;
      op.initial_generation_policy = gp;
      gp.validate();

      //FC_ASSERT( soft_cap_percent > 0 )
      gp.soft_cap_percent = 0;
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      //FC_ASSERT( soft_cap_percent <= STEEM_100_PERCENT )
      gp.soft_cap_percent = STEEM_100_PERCENT + 1;
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      //FC_ASSERT( soft_cap_percent == STEEM_100_PERCENT && post_soft_cap_unit.steem_unit.size() == 0 )
      gp.soft_cap_percent = STEEM_100_PERCENT;
      gp.post_soft_cap_unit.steem_unit = { { "bob", 2 } };
      gp.post_soft_cap_unit.token_unit = {};
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      //FC_ASSERT( soft_cap_percent == STEEM_100_PERCENT && post_soft_cap_unit.token_unit.size() == 0 )
      gp.soft_cap_percent = STEEM_100_PERCENT;
      gp.post_soft_cap_unit.steem_unit = {};
      gp.post_soft_cap_unit.token_unit = { { "alice", 3 } };
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      //FC_ASSERT( soft_cap_percent != STEEM_100_PERCENT && post_soft_cap_unit.steem_unit.size() > 0 )
      gp.soft_cap_percent = STEEM_100_PERCENT / 2;
      gp.post_soft_cap_unit.steem_unit = {};
      gp.post_soft_cap_unit.token_unit = {};
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      gp.soft_cap_percent = STEEM_100_PERCENT;
      gp.post_soft_cap_unit.steem_unit = {};
      gp.post_soft_cap_unit.token_unit = {};
      op.initial_generation_policy = gp;
      op.validate();

      //FC_ASSERT( min_steem_units_commitment.lower_bound <= hard_cap_steem_units_commitment.lower_bound )
      gp.min_steem_units_commitment.lower_bound = 10 * SMT_MIN_HARD_CAP_STEEM_UNITS;
      gp.min_steem_units_commitment.upper_bound = 20 * SMT_MIN_HARD_CAP_STEEM_UNITS;
      gp.hard_cap_steem_units_commitment.lower_bound = 9 * SMT_MIN_HARD_CAP_STEEM_UNITS;
      gp.hard_cap_steem_units_commitment.upper_bound = 20 * SMT_MIN_HARD_CAP_STEEM_UNITS;
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      //FC_ASSERT( min_steem_units_commitment.upper_bound <= hard_cap_steem_units_commitment.upper_bound )
      gp.hard_cap_steem_units_commitment.lower_bound = 11 * SMT_MIN_HARD_CAP_STEEM_UNITS;
      gp.hard_cap_steem_units_commitment.upper_bound = 19 * SMT_MIN_HARD_CAP_STEEM_UNITS;
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      //FC_ASSERT( hard_cap_steem_units_commitment.lower_bound >= SMT_MIN_SATURATION_STEEM_UNITS * uint64_t( max_unit_ratio ) )
      gp.hard_cap_steem_units_commitment.lower_bound = 11 * SMT_MIN_HARD_CAP_STEEM_UNITS;
      gp.hard_cap_steem_units_commitment.upper_bound = 21 * SMT_MIN_HARD_CAP_STEEM_UNITS;
      gp.max_unit_ratio = ( ( 11 * SMT_MIN_HARD_CAP_STEEM_UNITS ) / SMT_MIN_SATURATION_STEEM_UNITS ) * 2;
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      gp.hard_cap_steem_units_commitment.lower_bound = 11 * SMT_MIN_HARD_CAP_STEEM_UNITS;
      gp.hard_cap_steem_units_commitment.upper_bound = 21 * SMT_MIN_HARD_CAP_STEEM_UNITS;
      gp.max_unit_ratio = 2;
      op.initial_generation_policy = gp;
      op.validate();

      smt_capped_generation_policy gp_valid = gp;

      //FC_ASSERT( min_soft_cap >= SMT_MIN_SOFT_CAP_STEEM_UNITS )
      gp.soft_cap_percent = 1;
      gp.min_steem_units_commitment.lower_bound = 1 * SMT_MIN_HARD_CAP_STEEM_UNITS;
      gp.min_steem_units_commitment.upper_bound = 2 * SMT_MIN_HARD_CAP_STEEM_UNITS;
      gp.hard_cap_steem_units_commitment.lower_bound = 1 * SMT_MIN_HARD_CAP_STEEM_UNITS;
      gp.hard_cap_steem_units_commitment.upper_bound = 2 * SMT_MIN_HARD_CAP_STEEM_UNITS;
      gp.post_soft_cap_unit.steem_unit = { { "bob", 2 } };
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      gp = gp_valid;
      op.initial_generation_policy = gp;
      op.validate();

      uint16_t max_val_16 = std::numeric_limits<uint16_t>::max();
      uint32_t max_val_32 = std::numeric_limits<uint32_t>::max();

      //FC_ASSERT( max_tokens_created <= max_share_supply_u128 )
      gp.soft_cap_percent = STEEM_100_PERCENT - 1;
      gp.min_unit_ratio = max_val_32;
      gp.post_soft_cap_unit.steem_unit = { { "abc", 1 } };
      gp.post_soft_cap_unit.token_unit = { { "abc1", max_val_16 } };
      gp.pre_soft_cap_unit.token_unit = { { "abc2", max_val_16 } };
      gp.min_steem_units_commitment.upper_bound = STEEM_MAX_SHARE_SUPPLY;
      gp.hard_cap_steem_units_commitment.upper_bound = STEEM_MAX_SHARE_SUPPLY;
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      //FC_ASSERT( max_steem_accepted <= max_share_supply_u128 )
      gp.min_unit_ratio = 1;
      gp.post_soft_cap_unit.token_unit = { { "abc1", 1 } };
      gp.pre_soft_cap_unit.token_unit = { { "abc2", 1 } };
      gp.post_soft_cap_unit.steem_unit = { { "abc3", max_val_16 } };
      gp.pre_soft_cap_unit.steem_unit = { { "abc34", max_val_16 } };
      op.initial_generation_policy = gp;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );
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
      ACTORS( (alice)(bob) )

      generate_block();

      FUND( "alice", 10 * 1000 * 1000 );
      FUND( "bob", 10 * 1000 * 1000 );

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );

      smt_setup_operation op;
      op.control_account = "alice";

      smt_capped_generation_policy gp = get_capped_generation_policy
      (
         get_generation_unit( { { "xyz", 1 } }, { { "xyz2", 2 } } )/*pre_soft_cap_unit*/,
         get_generation_unit()/*post_soft_cap_unit*/,
         get_cap_commitment( 1 )/*min_steem_units_commitment*/,
         get_cap_commitment( SMT_MIN_HARD_CAP_STEEM_UNITS + 1 )/*hard_cap_steem_units_commitment*/,
         STEEM_100_PERCENT/*soft_cap_percent*/,
         1/*min_unit_ratio*/,
         2/*max_unit_ratio*/
      );

      fc::time_point_sec start_time        = fc::variant( "2021-01-01T00:00:00" ).as< fc::time_point_sec >();
      fc::time_point_sec start_time_plus_1 = start_time + fc::seconds(1);

      op.initial_generation_policy = gp;
      op.generation_begin_time = start_time;
      op.generation_end_time = op.announced_launch_time = op.launch_expiration_time = start_time_plus_1;

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
      op.decimal_places = 3;
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );
      tx.operations.clear();
      tx.signatures.clear();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_cap_reveal_validate )
{
   try
   {
   smt_cap_reveal_operation op;
   // Set operation's fields to valid values.
   op.control_account = "alice";
   op.symbol = get_new_smt_symbol( 4, db );
   op.validate();
   // Check invalid control account name.
   op.control_account = "@@@@@";
   STEEM_REQUIRE_THROW( op.validate(), fc::exception );
   op.control_account = "alice";
   // Check invalid SMT symbol.
   op.symbol = op.symbol.get_paired_symbol();
   STEEM_REQUIRE_THROW( op.validate(), fc::exception );
   op.symbol = op.symbol.get_paired_symbol();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_cap_reveal_authorities )
{
   try
   {
      smt_cap_reveal_operation op;
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

void setup_smt_and_reveal_caps( const account_name_type& control_account, const fc::ecc::private_key& private_key, const asset_symbol_type& smt_symbol,
                                const share_type& min_cap_val, const share_type& max_cap_val, fc::uint128_t invalid_val,
                                fc::uint128_t nonce, database* db, smt_database_fixture& dbf )
{
   smt_cap_reveal_operation op;
   op.control_account = control_account;
   op.symbol = dbf.get_new_smt_symbol( 4, db );
   // Fail due to SMT not created yet.
   FAIL_WITH_OP( op, private_key, fc::assert_exception );

   op.symbol = smt_symbol;
   // Fail due to SMT setup phase not complete.
   FAIL_WITH_OP( op, private_key, fc::assert_exception );

   // Setup SMT
   {
      smt_setup_operation setup_op;
      setup_op.control_account = control_account;
      setup_op.symbol = op.symbol;
      setup_op.decimal_places = op.symbol.decimals();
      smt_capped_generation_policy gp = dbf.get_capped_generation_policy
      (
         dbf.get_generation_unit( { { "xyz", 1 } }, { { "xyz2", 2 } } )/*pre_soft_cap_unit*/,
         dbf.get_generation_unit()/*post_soft_cap_unit*/,
         dbf.get_cap_commitment( min_cap_val, nonce )/*min_steem_units_commitment*/,
         dbf.get_cap_commitment( max_cap_val, nonce )/*hard_cap_steem_units_commitment*/,
         STEEM_100_PERCENT/*soft_cap_percent*/,
         1/*min_unit_ratio*/,
         2/*max_unit_ratio*/
      );

      fc::time_point_sec start_time        = fc::variant( "2021-01-01T00:00:00" ).as< fc::time_point_sec >();
      fc::time_point_sec start_time_plus_1 = start_time + fc::seconds(1);

      setup_op.initial_generation_policy = gp;
      setup_op.generation_begin_time = start_time;
      setup_op.generation_end_time = setup_op.announced_launch_time = setup_op.launch_expiration_time = start_time_plus_1;
      PUSH_OP( setup_op, private_key );
   }

   const auto& smt_object = db->get< smt_token_object, by_symbol >( smt_symbol );
   FC_ASSERT( smt_object.steem_units_min_cap < 0 && smt_object.steem_units_hard_cap < 0 );
   // Try to reveal correct value with invalid nonce.
   op.cap.nonce = invalid_val;
   op.cap.amount = max_cap_val;
   FAIL_WITH_OP( op, private_key, fc::assert_exception );
   FC_ASSERT( smt_object.steem_units_min_cap < 0 && smt_object.steem_units_hard_cap < 0 );
   // Try to reveal invalid amount with correct nonce. Note that the value will be tested against both cap's commitments.
   op.cap.nonce = nonce;
   op.cap.amount = invalid_val.to_uint64();
   FAIL_WITH_OP( op, private_key, fc::assert_exception );
   FC_ASSERT( smt_object.steem_units_min_cap < 0 && smt_object.steem_units_hard_cap < 0 );
   // Reveal max hard cap.
   op.cap.amount = max_cap_val;
   PUSH_OP( op, private_key );
   FC_ASSERT( smt_object.steem_units_min_cap < 0 && smt_object.steem_units_hard_cap == max_cap_val );
   // Try to reveal max hard cap again.
   FAIL_WITH_OP( op, private_key, fc::assert_exception );
   // Try to reveal invalid amount again. Note that this time it will be tested against min cap only (as max hard cap has been already revealed).
   op.cap.amount = invalid_val.to_uint64();
   FAIL_WITH_OP( op, private_key, fc::assert_exception );
   FC_ASSERT( smt_object.steem_units_min_cap < 0 && smt_object.steem_units_hard_cap == max_cap_val );
   // Reveal min cap.
   op.cap.amount = min_cap_val;
   PUSH_OP( op, private_key );
   FC_ASSERT( smt_object.steem_units_min_cap == min_cap_val && smt_object.steem_units_hard_cap == max_cap_val );
   // Try to reveal min cap again.
   FAIL_WITH_OP( op, private_key, fc::assert_exception );
}

BOOST_AUTO_TEST_CASE( smt_cap_reveal_apply )
{
   try
   {
      ACTORS( (alice)(bob) )

      generate_block();

      auto smts = create_smt_3("alice", alice_private_key);
      // Test non-hidden caps (zero nonce).
      setup_smt_and_reveal_caps("alice", alice_private_key, smts[0], 1, SMT_MIN_HARD_CAP_STEEM_UNITS + 1, 20000, 0, db, *this);
      // Test hidden caps (1234 nonce).
      setup_smt_and_reveal_caps("alice", alice_private_key, smts[1], 10000, SMT_MIN_HARD_CAP_STEEM_UNITS + 1, 20000, 1234, db, *this);
   }
   FC_LOG_AND_RETHROW()
}

/*
 * SMT legacy tests
 *
 * The logic tests in legacy tests *should* be entirely duplicated in smt_operation_tests.cpp
 * We are keeping these tests around to provide an additional layer re-assurance for the time being
 */
FC_TODO( "Remove SMT legacy tests and ensure code coverage is not reduced" );

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
