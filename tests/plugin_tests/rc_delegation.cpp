#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <steem/chain/account_object.hpp>
#include <steem/chain/comment_object.hpp>
#include <steem/protocol/steem_operations.hpp>

#include <steem/plugins/rc/rc_objects.hpp>
#include <steem/plugins/rc/rc_operations.hpp>
#include <steem/plugins/rc/rc_plugin.hpp>

#include "../db_fixture/database_fixture.hpp"

using namespace steem::chain;
using namespace steem::protocol;
using namespace steem::plugins::rc;

BOOST_FIXTURE_TEST_SUITE( rc_delegation, clean_database_fixture )

BOOST_AUTO_TEST_CASE( rc_delegate_to_pool_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing:  rc_delegate_to_pool_apply" );
      ACTORS( (alice)(bob)(dave) )

      vest( STEEM_INIT_MINER_NAME, "alice", ASSET( "10.000 TESTS" ) );

      int64_t alice_vests = alice.vesting_shares.amount.value;
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

BOOST_AUTO_TEST_SUITE_END()

#endif
