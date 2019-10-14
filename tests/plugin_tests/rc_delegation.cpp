#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <steem/chain/account_object.hpp>
#include <steem/chain/comment_object.hpp>
#include <steem/chain/database_exceptions.hpp>
#include <steem/chain/smt_objects.hpp>
#include <steem/protocol/steem_operations.hpp>

#include <steem/plugins/rc/rc_objects.hpp>
#include <steem/plugins/rc/rc_operations.hpp>
#include <steem/plugins/rc/rc_plugin.hpp>

#include "../db_fixture/database_fixture.hpp"

using namespace steem::chain;
using namespace steem::protocol;
using namespace steem::plugins::rc;

BOOST_FIXTURE_TEST_SUITE( rc_delegation, clean_database_fixture )

BOOST_AUTO_TEST_CASE( rc_delegate_to_pool_validate )
{
   try{
      SMT_SYMBOL( alice, 3, db );

      delegate_to_pool_operation op;
      op.from_account = "alice";
      op.to_pool = "bob";
      op.amount = asset( 1, VESTS_SYMBOL );
      op.validate();

      op.amount.symbol = STEEM_SYMBOL;
      BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );

      op.amount.symbol = SBD_SYMBOL;
      BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );

      op.amount.symbol = alice_symbol;
      BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );

      op.amount.symbol = alice_symbol.get_paired_symbol();
      BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );

      op.amount.symbol = VESTS_SYMBOL;
      op.to_pool = "bob-";
      BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );

      op.to_pool = "@@";
      BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );

      op.to_pool = "@@000000000";
      BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );

      op.to_pool = alice_symbol.to_nai_string();
      op.validate();

      op.to_pool = alice_symbol.get_paired_symbol().to_nai_string();
      BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );

      op.to_pool = "bob";
      op.from_account = alice_symbol.to_nai_string();
      BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( rc_delegate_to_pool_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing:  rc_delegate_to_pool_apply" );
      ACTORS( (alice)(bob)(dave) )

      vest( STEEM_INIT_MINER_NAME, "alice", ASSET( "10.000 TESTS" ) );
      vest( STEEM_INIT_MINER_NAME, "bob", ASSET( "10.000 TESTS" ) );

      int64_t alice_vests = alice.vesting_shares.amount.value;
      int64_t bob_vests = bob.vesting_shares.amount.value;
      signed_transaction tx;

      delegate_to_pool_operation op;
      op.from_account = "alice";
      op.to_pool = "bob";
      op.amount = asset( alice_vests + 1, VESTS_SYMBOL );    // Delegating more VESTS than I have should fail

      custom_json_operation custom_op;
      custom_op.required_auths.insert( "alice" );
      custom_op.id = STEEM_RC_PLUGIN_NAME;
      custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );

      tx.operations.push_back( custom_op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, alice_private_key );
      tx.validate();
      BOOST_CHECK_THROW( db->push_transaction( tx, 0 ), fc::exception );

      int64_t vests_to_bob = alice_vests / 3;
      int64_t vests_to_dave = alice_vests / 2;
      tx.operations.clear();
      tx.signatures.clear();
      op.amount = asset( vests_to_bob, VESTS_SYMBOL );
      custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );

      tx.operations.push_back( custom_op );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );

      // Make sure object is created
      const rc_indel_edge_object* edge = db->find< rc_indel_edge_object, by_edge >(
         boost::make_tuple( account_name_type( "alice" ), VESTS_SYMBOL, account_name_type( "bob" ) ) );

      BOOST_CHECK( edge );
      generate_block();

      // Delegate the remainder of my VESTS to Dave
      // First attempt to delegate too much, to test that we properly subtracted the first delegation
      tx.operations.clear();
      tx.signatures.clear();
      op.to_pool = "dave";
      op.amount = asset( alice_vests - vests_to_bob + 1, VESTS_SYMBOL );
      custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );

      tx.operations.push_back( custom_op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, alice_private_key );
      BOOST_CHECK_THROW( db->push_transaction( tx, 0 ), fc::exception );

      tx.operations.clear();
      tx.signatures.clear();
      op.amount = asset( vests_to_dave, VESTS_SYMBOL );
      custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );

      tx.operations.push_back( custom_op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );

      delegate_vesting_shares_operation del_vests;
      del_vests.delegator = "alice";
      del_vests.delegatee = "bob";
      del_vests.vesting_shares = asset( alice_vests - vests_to_dave + 1, VESTS_SYMBOL );
      tx.clear();
      tx.operations.push_back( del_vests );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );

      edge = db->find< rc_indel_edge_object, by_edge >( boost::make_tuple( account_name_type( "alice" ), VESTS_SYMBOL, account_name_type( "bob" ) ) );
      BOOST_REQUIRE( edge == nullptr );

      edge = db->find< rc_indel_edge_object, by_edge >( boost::make_tuple( account_name_type( "alice" ), VESTS_SYMBOL, account_name_type( "dave" ) ) );
      BOOST_REQUIRE( edge != nullptr );
      BOOST_REQUIRE( edge->amount.amount.value == vests_to_dave - 1 );

      generate_block();
      auto alice_symbol = create_smt( "alice", alice_private_key, 3 );
      SMT_SYMBOL( bob, 3, db );
      generate_block();

      BOOST_TEST_MESSAGE( "--- Test failure delegating to non-existent SMT" );

      op.from_account = "bob";
      op.to_pool = bob_symbol.to_nai_string();
      op.amount = asset( bob_vests / 10, VESTS_SYMBOL );
      custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
      custom_op.required_auths.clear();
      custom_op.required_auths.insert( "bob" );
      tx.clear();
      tx.operations.push_back( custom_op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, bob_private_key );
      BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- Test failure delegating to SMT in setup phase" );

      op.to_pool = alice_symbol.to_nai_string();
      custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
      tx.clear();
      tx.operations.push_back( custom_op );
      sign( tx, bob_private_key );
      BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- Test failure delegating to SMT in setup_completed phase" );

      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get< smt_token_object, by_symbol >( alice_symbol ), [&]( smt_token_object& smt )
         {
            smt.phase = smt_phase::setup_completed;
         });
      });

      generate_block();
      BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- Test failure delegating to SMT in ico phase" );

      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get< smt_token_object, by_symbol >( alice_symbol ), [&]( smt_token_object& smt )
         {
            smt.phase = smt_phase::ico;
         });
      });

      generate_block();
      BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- Test failure delegating to SMT in launch_failed phase" );

      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get< smt_token_object, by_symbol >( alice_symbol ), [&]( smt_token_object& smt )
         {
            smt.phase = smt_phase::launch_failed;
         });
      });

      generate_block();
      BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- Test success delegating to SMT in ico_completed phase" );

      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get< smt_token_object, by_symbol >( alice_symbol ), [&]( smt_token_object& smt )
         {
            smt.phase = smt_phase::ico_completed;
         });
      });

      generate_block();
      db->push_transaction( tx, 0 );

      edge = db->find< rc_indel_edge_object, by_edge >( boost::make_tuple( account_name_type( "bob" ), VESTS_SYMBOL, account_name_type( alice_symbol.to_nai_string() ) ) );
      BOOST_REQUIRE( edge != nullptr );
      BOOST_REQUIRE( edge->amount.amount.value == op.amount.amount.value );


      BOOST_TEST_MESSAGE( "--- Test success delegating to SMT in setup_success phase" );

      op.amount = asset( bob_vests / 5, VESTS_SYMBOL );
      custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
      tx.clear();
      tx.operations.push_back( custom_op );
      sign( tx, bob_private_key );
      db->push_transaction( tx, 0 );

      edge = db->find< rc_indel_edge_object, by_edge >( boost::make_tuple( account_name_type( "bob" ), VESTS_SYMBOL, account_name_type( alice_symbol.to_nai_string() ) ) );
      BOOST_REQUIRE( edge != nullptr );
      BOOST_REQUIRE( edge->amount.amount.value == op.amount.amount.value );

   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( rc_set_slot_delegator )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: rc_set_slot_delegator" );
      ACTORS( (alice)(bob)(charlie) )

      generate_block();

      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get_witness_schedule_object(), [&]( witness_schedule_object& wso )
         {
            wso.median_props.account_creation_fee = ASSET( "0.100 TESTS" );
         });
      });

      fund( "alice", ASSET( "100.000 TESTS" ) );

      auto dave_private_key = generate_private_key( "dave" );
      account_create_operation acc_create;
      acc_create.fee = ASSET( "0.100 TESTS" );
      acc_create.creator = "alice";
      acc_create.new_account_name = "dave";
      acc_create.owner = authority( 1, dave_private_key.get_public_key(), 1 );
      acc_create.active = authority( 1, dave_private_key.get_public_key(), 1 );
      acc_create.posting = authority( 1, dave_private_key.get_public_key(), 1 );
      acc_create.memo_key = dave_private_key.get_public_key();
      acc_create.json_metadata = "";

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( acc_create );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );

      change_recovery_account_operation change_rec;
      change_rec.account_to_recover = "dave";
      change_rec.new_recovery_account = "bob";
      tx.clear();
      tx.operations.push_back( change_rec );
      sign( tx, dave_private_key );
      db->push_transaction( tx, 0 );

      generate_blocks( db->get_index< change_recovery_account_request_index, by_effective_date >().begin()->effective_on );

      custom_json_operation custom_op;
      custom_op.required_auths.insert( "alice" );
      custom_op.id = STEEM_RC_PLUGIN_NAME;

      set_slot_delegator_operation op;
      op.from_pool = STEEM_TEMP_ACCOUNT;
      op.to_account = "dave";
      op.to_slot = -1;
      op.signer = "alice";
      BOOST_CHECK_THROW( op.validate(), fc::assert_exception );

      op.to_slot = STEEM_RC_MAX_SLOTS;
      BOOST_CHECK_THROW( op.validate(), fc::assert_exception );

      op.to_slot = 1;
      tx.clear();
      custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
      tx.operations.push_back( custom_op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, alice_private_key );
      BOOST_CHECK_THROW( db->push_transaction( tx, 0 ), fc::exception );

      op.to_slot = 2;
      tx.clear();
      custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
      tx.operations.push_back( custom_op );
      sign( tx, alice_private_key );
      BOOST_CHECK_THROW( db->push_transaction( tx, 0 ), fc::exception );

      op.to_slot = 0;
      tx.clear();
      custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
      tx.operations.push_back( custom_op );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );

      account_name_type slot_delegator = db->get< rc_account_object, by_name >( "dave" ).indel_slots[ op.to_slot ];
      idump( (slot_delegator) );
      BOOST_REQUIRE( slot_delegator == op.from_pool );


      generate_block();
      custom_op.required_auths.clear();
      custom_op.required_auths.insert( "bob" );
      op.signer = "bob";
      op.to_slot = 0;
      tx.clear();
      custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
      tx.operations.push_back( custom_op );
      sign( tx, bob_private_key );
      BOOST_CHECK_THROW( db->push_transaction( tx, 0 ), fc::exception );

      op.to_slot = 2;
      tx.clear();
      custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
      tx.operations.push_back( custom_op );
      sign( tx, bob_private_key );
      BOOST_CHECK_THROW( db->push_transaction( tx, 0 ), fc::exception );

      op.to_slot = 1;
      tx.clear();
      custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
      tx.operations.push_back( custom_op );
      sign( tx, bob_private_key );
      db->push_transaction( tx, 0 );

      slot_delegator = db->get< rc_account_object, by_name >( "dave" ).indel_slots[ op.to_slot ];
      BOOST_REQUIRE( slot_delegator == op.from_pool );


      generate_block();
      custom_op.required_auths.clear();
      custom_op.required_auths.insert( "dave" );
      op.signer = "dave";
      op.to_slot = 0;
      tx.clear();
      custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
      tx.operations.push_back( custom_op );
      sign( tx, dave_private_key );
      BOOST_CHECK_THROW( db->push_transaction( tx, 0 ), fc::exception );

      op.to_slot = 1;
      tx.clear();
      custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
      tx.operations.push_back( custom_op );
      sign( tx, dave_private_key );
      BOOST_CHECK_THROW( db->push_transaction( tx, 0 ), fc::exception );

      op.to_slot = 2;
      tx.clear();
      custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
      tx.operations.push_back( custom_op );
      sign( tx, dave_private_key );
      db->push_transaction( tx, 0 );

      slot_delegator = db->get< rc_account_object, by_name >( "dave" ).indel_slots[ op.to_slot ];
      BOOST_REQUIRE( slot_delegator == op.from_pool );

      generate_block();
      tx.clear();
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
      tx.operations.push_back( custom_op );
      sign( tx, dave_private_key );
      BOOST_CHECK_THROW( db->push_transaction( tx, 0 ), fc::exception );


      generate_block();
      custom_op.required_auths.clear();
      custom_op.required_auths.insert( "charlie" );
      op.signer = "charlie";
      op.to_slot = 0;
      tx.clear();
      custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
      tx.operations.push_back( custom_op );
      sign( tx, charlie_private_key );
      BOOST_CHECK_THROW( db->push_transaction( tx, 0 ), fc::exception );

      op.to_slot = 1;
      tx.clear();
      custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
      tx.operations.push_back( custom_op );
      sign( tx, charlie_private_key );
      BOOST_CHECK_THROW( db->push_transaction( tx, 0 ), fc::exception );

      op.to_slot = 2;
      tx.clear();
      custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
      tx.operations.push_back( custom_op );
      sign( tx, charlie_private_key );
      BOOST_CHECK_THROW( db->push_transaction( tx, 0 ), fc::exception );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( rc_delegate_drc_from_pool )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: rc_set_slot_delegator" );
      ACTORS( (alice)(bob) )
      generate_block();

      delegate_drc_from_pool_operation op;
      op.from_pool = "bob";
      op.to_account = "alice";
      op.to_slot = 2;
      op.asset_symbol = VESTS_SYMBOL;
      op.drc_max_mana = 100;

      custom_json_operation custom_op;
      custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
      custom_op.required_auths.insert( "alice" );
      custom_op.id = STEEM_RC_PLUGIN_NAME;
      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( custom_op );
      sign( tx, bob_private_key );
      BOOST_CHECK_THROW( db->push_transaction( tx, 0 ), fc::exception );

      set_slot_delegator_operation set_slot;
      set_slot.from_pool = "bob";
      set_slot.to_account = "alice";
      set_slot.to_slot = 2;
      set_slot.signer = "alice";
      custom_op.required_auths.clear();
      custom_op.required_auths.insert( "alice" );
      custom_op.json = fc::json::to_string( rc_plugin_operation( set_slot ) );
      tx.clear();
      tx.operations.push_back( custom_op );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );

      custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
      custom_op.required_auths.clear();
      custom_op.required_auths.insert( "bob" );
      tx.clear();
      tx.operations.push_back( custom_op );
      sign( tx, bob_private_key );
      db->push_transaction( tx, 0 );

      {
         const auto& edge = db->get< rc_outdel_drc_edge_object, by_edge >( boost::make_tuple( op.from_pool, op.to_account, op.asset_symbol ) );
         BOOST_REQUIRE( edge.from_pool == op.from_pool );
         BOOST_REQUIRE( edge.to_account == op.to_account );
         BOOST_REQUIRE( edge.asset_symbol == op.asset_symbol );
         BOOST_REQUIRE( edge.drc_manabar.current_mana == op.drc_max_mana );
         BOOST_REQUIRE( edge.drc_manabar.last_update_time == db->head_block_time().sec_since_epoch() );
         BOOST_REQUIRE( edge.drc_max_mana == op.drc_max_mana );
      }

      generate_block();

      op.drc_max_mana = 50;
      custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
      tx.clear();
      tx.operations.push_back( custom_op );
      sign( tx, bob_private_key );
      db->push_transaction( tx, 0 );

      {
         const auto& edge = db->get< rc_outdel_drc_edge_object, by_edge >( boost::make_tuple( op.from_pool, op.to_account, op.asset_symbol ) );
         BOOST_REQUIRE( edge.from_pool == op.from_pool );
         BOOST_REQUIRE( edge.to_account == op.to_account );
         BOOST_REQUIRE( edge.asset_symbol == op.asset_symbol );
         BOOST_REQUIRE( edge.drc_manabar.current_mana == op.drc_max_mana );
         BOOST_REQUIRE( edge.drc_manabar.last_update_time == db->head_block_time().sec_since_epoch() );
         BOOST_REQUIRE( edge.drc_max_mana == op.drc_max_mana );

         const auto* pool_ptr = db->find< rc_delegation_pool_object, by_account_symbol >( boost::make_tuple( op.from_pool, op.asset_symbol ) );
         BOOST_REQUIRE( pool_ptr != nullptr );
      }

      generate_block();
      op.drc_max_mana = 0;
      custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
      tx.clear();
      tx.operations.push_back( custom_op );
      sign( tx, bob_private_key );
      db->push_transaction( tx, 0 );

      const auto* edge_ptr = db->find< rc_outdel_drc_edge_object, by_edge >( boost::make_tuple( op.from_pool, op.to_account, op.asset_symbol ) );
      BOOST_REQUIRE( edge_ptr == nullptr );
      const auto* pool_ptr = db->find< rc_delegation_pool_object, by_account_symbol >( boost::make_tuple( op.from_pool, op.asset_symbol ) );
      BOOST_REQUIRE( pool_ptr == nullptr );

      op.to_slot = 1;
      custom_op.json = fc::json::to_string( rc_plugin_operation( op ) );
      tx.clear();
      tx.operations.push_back( custom_op );
      sign( tx, bob_private_key );
      BOOST_CHECK_THROW( db->push_transaction( tx, 0 ), fc::exception );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( rc_drc_pool_consumption )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: rc_drc_pool_consumption" );

      using steem::plugins::rc::detail::use_account_rcs;

      /*
      The first test is an edge test
      poola has 100 rc, delegating 50 rc to alice
      poolb has 25 rc, delegating 50 rc to alice
      alice has 50 rc.

      This test will consume 100 rc, which will cause the following changes:

      poola has 50 rc, with 0 remaining to alice
      poolb has 0 rc, with 25 remaining to alice
      alice has 25 rc

      The second test is the general test
      poolc has 200 rc, with 100 rc to bob
      bob has 100 rc

      Bob will consume 50 rc, which will cause the following changes:
      poolc has 150 rc, with 50 remaining to bob
      bob has 100 rc

      The third test will have bob consume 200 rc, which should fail
      */

      ACTORS( (alice)(bob)(poola)(poolb)(poolc) )
      generate_block();

      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get_account( "alice" ), [&]( account_object& a )
         {
            a.vesting_shares = asset( 50, VESTS_SYMBOL );
         });

         db.modify( db.get< rc_account_object, by_name >( "alice" ), [&]( rc_account_object& rca )
         {
            rca.max_rc_creation_adjustment.amount.value = 0;
            rca.rc_manabar.current_mana = 50;
            rca.rc_manabar.last_update_time = db.head_block_time().sec_since_epoch();
            rca.indel_slots[0] = "poola";
            rca.indel_slots[1] = "poolb";
         });

         db.create< rc_delegation_pool_object >( [&]( rc_delegation_pool_object& rc_pool )
         {
            rc_pool.account = "poola";
            rc_pool.asset_symbol = VESTS_SYMBOL;
            rc_pool.rc_pool_manabar.current_mana = 100;
            rc_pool.rc_pool_manabar.last_update_time = db.head_block_time().sec_since_epoch();
            rc_pool.max_rc = 100;
         });

         db.create< rc_outdel_drc_edge_object >( [&]( rc_outdel_drc_edge_object& drc_edge )
         {
            drc_edge.from_pool = "poola";
            drc_edge.to_account = "alice";
            drc_edge.asset_symbol = VESTS_SYMBOL;
            drc_edge.drc_manabar.current_mana = 50;
            drc_edge.drc_manabar.last_update_time = db.head_block_time().sec_since_epoch();
            drc_edge.drc_max_mana = 50;
         });

         db.create< rc_delegation_pool_object >( [&]( rc_delegation_pool_object& rc_pool )
         {
            rc_pool.account = "poolb";
            rc_pool.asset_symbol = VESTS_SYMBOL;
            rc_pool.rc_pool_manabar.current_mana = 25;
            rc_pool.rc_pool_manabar.last_update_time = db.head_block_time().sec_since_epoch();
            rc_pool.max_rc = 25;
         });

         db.create< rc_outdel_drc_edge_object >( [&]( rc_outdel_drc_edge_object& drc_edge )
         {
            drc_edge.from_pool = "poolb";
            drc_edge.to_account = "alice";
            drc_edge.asset_symbol = VESTS_SYMBOL;
            drc_edge.drc_manabar.current_mana = 50;
            drc_edge.drc_manabar.last_update_time = db.head_block_time().sec_since_epoch();
            drc_edge.drc_max_mana = 50;
         });

         db.modify( db.get_account("bob"), [&]( account_object& a )
         {
            a.vesting_shares = asset( 100, VESTS_SYMBOL );
         });

         db.modify( db.get< rc_account_object, by_name >( "bob" ), [&]( rc_account_object& rca )
         {
            rca.max_rc_creation_adjustment.amount.value = 0;
            rca.rc_manabar.current_mana = 100;
            rca.rc_manabar.last_update_time = db.head_block_time().sec_since_epoch();
            rca.indel_slots[0] = "poolc";
         });

         db.create< rc_delegation_pool_object >( [&]( rc_delegation_pool_object& rc_pool )
         {
            rc_pool.account = "poolc";
            rc_pool.asset_symbol = VESTS_SYMBOL;
            rc_pool.rc_pool_manabar.current_mana = 200;
            rc_pool.rc_pool_manabar.last_update_time = db.head_block_time().sec_since_epoch();
            rc_pool.max_rc = 200;
         });

         db.create< rc_outdel_drc_edge_object >( [&]( rc_outdel_drc_edge_object& drc_edge )
         {
            drc_edge.from_pool = "poolc";
            drc_edge.to_account = "bob";
            drc_edge.asset_symbol = VESTS_SYMBOL;
            drc_edge.drc_manabar.current_mana = 100;
            drc_edge.drc_manabar.last_update_time = db.head_block_time().sec_since_epoch();
            drc_edge.drc_max_mana = 100;
         });
      });

      const auto& alice_rc = db->get< rc_account_object, by_name >( "alice" );
      const auto& poola_pool = db->get< rc_delegation_pool_object, by_account_symbol >( boost::make_tuple( "poola", VESTS_SYMBOL ) );
      const auto& poola_drc_alice = db->get< rc_outdel_drc_edge_object, by_edge >( boost::make_tuple( "poola", "alice", VESTS_SYMBOL ) );
      const auto& poolb_pool = db->get< rc_delegation_pool_object, by_account_symbol >( boost::make_tuple( "poolb", VESTS_SYMBOL ) );
      const auto& poolb_drc_alice = db->get< rc_outdel_drc_edge_object, by_edge >( boost::make_tuple( "poolb", "alice", VESTS_SYMBOL ) );
      const auto& bob_rc = db->get< rc_account_object, by_name >( "bob" );
      const auto& poolc_pool = db->get< rc_delegation_pool_object, by_account_symbol >( boost::make_tuple( "poolc", VESTS_SYMBOL ) );
      const auto& poolc_drc_bob = db->get< rc_outdel_drc_edge_object, by_edge >( boost::make_tuple( "poolc", "bob", VESTS_SYMBOL ) );

      const auto& gpo = db->get_dynamic_global_properties();
      rc_plugin_skip_flags skip {0, 0, 0, 0};
      set< account_name_type > whitelist;

      use_account_rcs( *db, gpo, "alice", 100, skip, whitelist );
      BOOST_REQUIRE( poola_pool.rc_pool_manabar.current_mana == 50 );
      BOOST_REQUIRE( poola_drc_alice.drc_manabar.current_mana == 0 );
      BOOST_REQUIRE( poolb_pool.rc_pool_manabar.current_mana == 0 );
      BOOST_REQUIRE( poolb_drc_alice.drc_manabar.current_mana == 25 );
      BOOST_REQUIRE( alice_rc.rc_manabar.current_mana == 25 );

      use_account_rcs( *db, gpo, "bob", 50, skip, whitelist );
      BOOST_REQUIRE( poolc_pool.rc_pool_manabar.current_mana == 150 );
      BOOST_REQUIRE( poolc_drc_bob.drc_manabar.current_mana == 50 );
      BOOST_REQUIRE( bob_rc.rc_manabar.current_mana == 100 );

      db->set_producing( true );
      BOOST_CHECK_THROW( use_account_rcs( *db, gpo, "bob", 200, skip, whitelist ), steem::chain::plugin_exception );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

#endif
