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
         boost::make_tuple( account_name_type( "alice" ), account_name_type( "bob" ), VESTS_SYMBOL ) );

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

   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

#endif
