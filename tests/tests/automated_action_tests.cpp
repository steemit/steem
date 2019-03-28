#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <steem/chain/steem_fwd.hpp>

#include <steem/protocol/exceptions.hpp>

#include <steem/chain/pending_required_action_object.hpp>
#include <steem/chain/pending_optional_action_object.hpp>

#include <steem/plugins/witness/block_producer.hpp>

#include "../db_fixture/database_fixture.hpp"

using namespace steem;
using namespace steem::chain;
using namespace steem::protocol;
using fc::string;

BOOST_FIXTURE_TEST_SUITE( automated_action_tests, clean_database_fixture )


BOOST_AUTO_TEST_CASE( push_pending_required_actions )
{
   BOOST_TEST_MESSAGE( "Testing: push_pending_required_actions" );

   BOOST_TEST_MESSAGE( "--- Failure pushing invalid required action" );
   example_required_action req_action;
   req_action.account = "";
   BOOST_REQUIRE_THROW( req_action.validate(), fc::assert_exception );

   BOOST_REQUIRE_THROW( db->push_required_action( req_action ), fc::assert_exception );

   BOOST_TEST_MESSAGE( "--- Success pushing future action" );
   req_action.account = STEEM_INIT_MINER_NAME;
   req_action.validate();
   db->push_required_action( req_action, db->head_block_time() + STEEM_BLOCK_INTERVAL );
   auto pending_req_action = db->get_index< pending_required_action_index, by_execution >().begin()->action.get< example_required_action >();
   auto pending_execution = db->get_index< pending_required_action_index, by_execution> ().begin()->execution_time;
   BOOST_REQUIRE( pending_req_action == req_action );
   BOOST_REQUIRE( pending_execution == db->head_block_time() + STEEM_BLOCK_INTERVAL );

   BOOST_TEST_MESSAGE( "--- Failure pushing past action" );
   BOOST_REQUIRE_THROW( db->push_required_action( req_action, db->head_block_time() - STEEM_BLOCK_INTERVAL ), fc::assert_exception );

   BOOST_TEST_MESSAGE( "--- Success pushing action now" );
   req_action.account = STEEM_TEMP_ACCOUNT;
   db->push_required_action( req_action );
   pending_req_action = db->get_index< pending_required_action_index, by_execution >().begin()->action.get< example_required_action >();
   pending_execution = db->get_index< pending_required_action_index, by_execution> ().begin()->execution_time;
   BOOST_REQUIRE( pending_req_action == req_action );
   BOOST_REQUIRE( pending_execution == db->head_block_time() );
}


BOOST_AUTO_TEST_CASE( push_pending_optional_actions )
{
   BOOST_TEST_MESSAGE( "Testing: push_pending_optional_actions" );

   BOOST_TEST_MESSAGE( "--- Failure pushing invalid required action" );
   example_optional_action opt_action;
   opt_action.account = "";
   BOOST_REQUIRE_THROW( opt_action.validate(), fc::assert_exception );

   BOOST_REQUIRE_THROW( db->push_optional_action( opt_action ), fc::assert_exception );

   BOOST_TEST_MESSAGE( "--- Success pushing future action" );
   opt_action.account = STEEM_INIT_MINER_NAME;
   opt_action.validate();
   db->push_optional_action( opt_action, db->head_block_time() + STEEM_BLOCK_INTERVAL );
   auto pending_opt_action = db->get_index< pending_optional_action_index, by_execution >().begin()->action.get< example_optional_action >();
   auto pending_execution = db->get_index< pending_optional_action_index, by_execution> ().begin()->execution_time;
   BOOST_REQUIRE( pending_opt_action.account == opt_action.account );
   BOOST_REQUIRE( pending_execution == db->head_block_time() + STEEM_BLOCK_INTERVAL );

   BOOST_TEST_MESSAGE( "--- Failure pushing past action" );
   BOOST_REQUIRE_THROW( db->push_optional_action( opt_action, db->head_block_time() - STEEM_BLOCK_INTERVAL ), fc::assert_exception );

   BOOST_TEST_MESSAGE( "--- Success pushing action now" );
   opt_action.account = STEEM_TEMP_ACCOUNT;
   db->push_optional_action( opt_action );
   pending_opt_action = db->get_index< pending_optional_action_index, by_execution >().begin()->action.get< example_optional_action >();
   pending_execution = db->get_index< pending_optional_action_index, by_execution> ().begin()->execution_time;
   BOOST_REQUIRE( pending_opt_action.account == opt_action.account );
   BOOST_REQUIRE( pending_execution == db->head_block_time() );
}


BOOST_AUTO_TEST_CASE( full_block )
{ try {
   resize_shared_mem( 1024 * 1024 * 32 ); // Due to number of objects in the test, it requires a large file. (32 MB)

   // Verify correct delay semantics when a 25% of the block is full of required actions
   BOOST_TEST_MESSAGE( "Testing full block action delay" );

   generate_block();

   example_required_action req_action;
   example_optional_action opt_action;
   uint32_t num_actions = 0;

   db_plugin->debug_update( [&num_actions, &req_action, &opt_action](database& db)
   {
      uint64_t block_size = 0;
      uint64_t action_partition_size = ( db.get_dynamic_global_properties().maximum_block_size * db.get_dynamic_global_properties().required_actions_partition_percent ) / STEEM_100_PERCENT;

      while( block_size < action_partition_size )
      {
         req_action.account = STEEM_INIT_MINER_NAME + fc::to_string( num_actions );
         db.push_required_action( req_action );
         block_size += fc::raw::pack_size( required_automated_action( req_action ) );
         num_actions++;
      }

      opt_action.account = STEEM_TEMP_ACCOUNT;
      db.push_optional_action( opt_action );
   });

   generate_block();
   signed_block block = *(db->fetch_block_by_number( db->head_block_num() ));

   db->pop_block();

   // In a full block scenario, there would be no optional actions included nor the last required action
   // Clear optional actions and the last required action and resign.
   block.extensions.erase( *block.extensions.end() );
   block.extensions.begin()->get< required_automated_actions >().pop_back();
   block.sign( STEEM_INIT_PRIVATE_KEY );

   db->push_block( block );

   {
      const auto& pending_req_index = db->get_index< pending_required_action_index, by_execution >();
      const auto& pending_opt_index = db->get_index< pending_optional_action_index, by_execution >();

      auto pending_req_action = pending_req_index.begin();
      auto pending_opt_action = pending_opt_index.begin();

      BOOST_REQUIRE( pending_req_action != pending_req_index.end() );
      BOOST_REQUIRE( pending_req_action->action.get< example_required_action >() == req_action );
      BOOST_REQUIRE( pending_req_action->execution_time == db->head_block_time() - STEEM_BLOCK_INTERVAL );
      BOOST_REQUIRE( pending_opt_action != pending_opt_index.end() );
      BOOST_REQUIRE( pending_opt_action->action.get< example_optional_action >().account == opt_action.account );
      BOOST_REQUIRE( pending_opt_action->execution_time == db->head_block_time() - STEEM_BLOCK_INTERVAL );
   }

   BOOST_TEST_MESSAGE( "--- Testing inclusion of delayed action" );
   generate_block();

   {
      const auto& pending_req_index = db->get_index< pending_required_action_index, by_execution >();

      auto pending_req_action = pending_req_index.begin();

      BOOST_REQUIRE( pending_req_action == pending_req_index.end() );

      auto block = db->fetch_block_by_number( db->head_block_num() );
      auto extensions_itr = block->extensions.begin();
      BOOST_REQUIRE( req_action == extensions_itr->get< required_automated_actions >().begin()->get< example_required_action >() );
      ++extensions_itr;
      BOOST_REQUIRE( opt_action.account == extensions_itr->get< optional_automated_actions >().begin()->get< example_optional_action >().account );
   }

} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_CASE( pending_required_execution )
{ try {
   // Check correct inclusion semantics when the next pending action has a later execution time
   BOOST_TEST_MESSAGE( "Testing correct inclusion for scheduling actions in the future." );

   example_required_action req_action;

   db_plugin->debug_update( [&req_action](database& db)
   {
      req_action.account = STEEM_INIT_MINER_NAME;
      db.push_required_action( req_action );

      req_action.account = STEEM_NULL_ACCOUNT;
      db.push_required_action( req_action, db.head_block_time() + (2 * STEEM_BLOCK_INTERVAL ) );
   });

   auto pending_itr = db->get_index< pending_required_action_index, by_execution >().begin();
   BOOST_REQUIRE( !( pending_itr->action.get< example_required_action >() == req_action ) );

   generate_block();

   pending_itr = db->get_index< pending_required_action_index, by_execution >().begin();
   BOOST_REQUIRE( pending_itr->action.get< example_required_action >() == req_action );

} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_CASE( unexpected_required_action )
{ try {
   // Check failure when block includes an unexpected required action
   BOOST_TEST_MESSAGE( "Testing rejection of a block with an expected required action" );

   // This test will log the later error and the next block contains many transactions.
   // Generate one block to make the error smaller.
   generate_block();

   generate_block();
   signed_block block = *(db->fetch_block_by_number( db->head_block_num() ));

   db->pop_block();

   example_required_action req_action;
   req_action.account = STEEM_TEMP_ACCOUNT;
   required_automated_actions req_actions;
   req_actions.push_back( req_action );
   block.extensions.insert( req_actions );
   block.sign( STEEM_INIT_PRIVATE_KEY );

   BOOST_REQUIRE_THROW( db->push_block( block ), fc::assert_exception );

} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_CASE( missing_required_action )
{ try {
   // Check failure when block does not include an expected required action
   BOOST_TEST_MESSAGE( "Testing rejection of a block with a missing required action" );

   // This test will log the later error and the next block contains many transactions.
   // Generate one block to make the error smaller.
   generate_block();

   example_required_action req_action;

   db_plugin->debug_update( [&req_action](database& db)
   {
      req_action.account = STEEM_INIT_MINER_NAME;
      db.push_required_action( req_action );
   });

   generate_block();
   signed_block block = *(db->fetch_block_by_number( db->head_block_num() ));

   db->pop_block();

   block.extensions.clear();
   block.sign( STEEM_INIT_PRIVATE_KEY );

   BOOST_REQUIRE_THROW( db->push_block( block ), fc::assert_exception );

} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_CASE( optional_action_expiration )
{ try {
   BOOST_TEST_MESSAGE( "Testing local expiration of an optional action" );

   for( uint32_t i = 0; i < STEEM_MAX_WITNESSES; i++ )
   {
      generate_block();
   }

   auto next_lib_time = db->fetch_block_by_number( db->get_dynamic_global_properties().last_irreversible_block_num + 1 )->timestamp;

   db_plugin->debug_update( [=]( database& db )
   {
      db.create< pending_optional_action_object >( [&]( pending_optional_action_object& o )
      {
         example_optional_action opt_action;
         opt_action.account = STEEM_NULL_ACCOUNT;
         o.action = opt_action;
         o.execution_time = next_lib_time;
      });
   });

   generate_block();

   signed_block block = *(db->fetch_block_by_number( db->head_block_num() ));

   db->pop_block();

   block.extensions.erase( *block.extensions.rbegin() );
   block.sign( STEEM_INIT_PRIVATE_KEY );

   db->push_block( block );

   const auto& opt_action_idx = db->get_index< pending_optional_action_index, by_execution >();
   auto opt_itr = opt_action_idx.begin();

   BOOST_REQUIRE( opt_itr == opt_action_idx.end() );

} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_CASE( unexpected_optional_action )
{ try {
   // Check success including an optional action not present in pending state
   BOOST_TEST_MESSAGE( "Testing an unexpected valid optional action" );

   generate_block();

   signed_block block = *(db->fetch_block_by_number( db->head_block_num() ));

   db->pop_block();

   example_optional_action opt_action;
   opt_action.account = STEEM_NULL_ACCOUNT;
   optional_automated_actions opt_actions = { opt_action };
   block.extensions.insert( opt_actions );
   block.sign( STEEM_INIT_PRIVATE_KEY );

   db->push_block( block );

} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_CASE( reject_optional_action )
{ try {
   // Check rejection of an invalid optional action
   BOOST_TEST_MESSAGE( "Testing an unexpected invalid optional action" );

   // This test will log the later error and the next block contains many transactions.
   // Generate one block to make the error smaller.
   generate_block();

   generate_block();

   signed_block block = *(db->fetch_block_by_number( db->head_block_num() ));

   db->pop_block();

   example_optional_action opt_action;
   opt_action.account = "_foobar";
   optional_automated_actions opt_actions = { opt_action };
   block.extensions.insert( opt_actions );
   block.sign( STEEM_INIT_PRIVATE_KEY );

   BOOST_REQUIRE_THROW( db->push_block( block ), fc::assert_exception );

} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_SUITE_END()
#endif
