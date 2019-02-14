#include <boost/test/unit_test.hpp>

#include <steem/protocol/exceptions.hpp>
#include <steem/protocol/hardfork.hpp>
#include <steem/protocol/sps_operations.hpp>

#include <steem/chain/database.hpp>
#include <steem/chain/database_exceptions.hpp>
#include <steem/chain/steem_objects.hpp>

#include <steem/chain/util/reward.hpp>

#include <steem/plugins/rc/rc_objects.hpp>
#include <steem/plugins/rc/resource_count.hpp>

#include <steem/chain/sps_objects.hpp>

#include <fc/macros.hpp>
#include <fc/crypto/digest.hpp>

#include "../db_fixture/database_fixture.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace steem;
using namespace steem::chain;
using namespace steem::protocol;
using fc::string;

BOOST_FIXTURE_TEST_SUITE( proposal_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( proposal_object_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: create_proposal_operation" );

      ACTORS( (alice)(bob) )
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      generate_block();

      auto fee = asset( STEEM_TREASURY_FEE, SBD_SYMBOL );

      FUND( "alice", ASSET( "80.000 TBD" ) );

      auto creator = "alice";
      auto receiver = "bob";

      auto start_date = db->head_block_time() + fc::days( 1 );
      auto end_date = start_date + fc::days( 2 );

      auto daily_pay = asset( 100, SBD_SYMBOL );

      auto subject = "hello";
      auto url = "http:://something.html";

      signed_transaction tx;

      const account_object& before_alice_account = db->get_account( creator );
      const account_object& before_bob_account = db->get_account( receiver );

      auto before_alice_sbd_balance = before_alice_account.sbd_balance;
      auto before_bob_sbd_balance = before_bob_account.sbd_balance;

      create_proposal_operation op;

      op.creator = creator;
      op.receiver = receiver;

      op.start_date = start_date;
      op.end_date = end_date;

      op.daily_pay = daily_pay;

      op.subject = subject;
      op.url = url;

      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );
      tx.operations.clear();
      tx.signatures.clear();

      const account_object& after_alice_account = db->get_account( creator );
      const account_object& after_bob_account = db->get_account( receiver );

      auto after_alice_sbd_balance = after_alice_account.sbd_balance;
      auto after_bob_sbd_balance = after_bob_account.sbd_balance;

      BOOST_REQUIRE( before_alice_sbd_balance == after_alice_sbd_balance + fee );
      BOOST_REQUIRE( before_bob_sbd_balance == after_bob_sbd_balance );

      const auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_creator >();
      auto found = proposal_idx.find( creator );
      BOOST_REQUIRE( found != proposal_idx.end() );

      BOOST_REQUIRE( found->creator == creator );
      BOOST_REQUIRE( found->receiver == receiver );
      BOOST_REQUIRE( found->start_date == start_date );
      BOOST_REQUIRE( found->end_date == end_date );
      BOOST_REQUIRE( found->daily_pay == daily_pay );
      BOOST_REQUIRE( found->subject == subject );
      BOOST_REQUIRE( found->url == url );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( proposal_object_validate )
{
   // try
   // {
   //    BOOST_TEST_MESSAGE( "Testing: account_update_validate" );

   //    ACTORS( (alice) )

   //    account_update_operation op;
   //    op.account = "alice";
   //    op.posting = authority();
   //    op.posting->weight_threshold = 1;
   //    op.posting->add_authorities( "abcdefghijklmnopq", 1 );

   //    try
   //    {
   //       op.validate();

   //       signed_transaction tx;
   //       tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
   //       tx.operations.push_back( op );
   //       sign( tx, alice_private_key );
   //       db->push_transaction( tx, 0 );

   //       BOOST_FAIL( "An exception was not thrown for an invalid account name" );
   //    }
   //    catch( fc::exception& ) {}

   //    validate_database();
   // }
   // FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( proposal_object_authorities )
{
   // try
   // {
   //    BOOST_TEST_MESSAGE( "Testing: account_update_authorities" );

   //    ACTORS( (alice)(bob) )
   //    private_key_type active_key = generate_private_key( "new_key" );

   //    db->modify( db->get< account_authority_object, by_account >( "alice" ), [&]( account_authority_object& a )
   //    {
   //       a.active = authority( 1, active_key.get_public_key(), 1 );
   //    });

   //    account_update_operation op;
   //    op.account = "alice";
   //    op.json_metadata = "{\"success\":true}";

   //    signed_transaction tx;
   //    tx.operations.push_back( op );
   //    tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );

   //    BOOST_TEST_MESSAGE( "  Tests when owner authority is not updated ---" );
   //    BOOST_TEST_MESSAGE( "--- Test failure when no signature" );
   //    STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );

   //    BOOST_TEST_MESSAGE( "--- Test failure when wrong signature" );
   //    sign( tx, bob_private_key );
   //    STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );

   //    BOOST_TEST_MESSAGE( "--- Test failure when containing additional incorrect signature" );
   //    sign( tx, alice_private_key );
   //    STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_irrelevant_sig );

   //    BOOST_TEST_MESSAGE( "--- Test failure when containing duplicate signatures" );
   //    tx.signatures.clear();
   //    sign( tx, active_key );
   //    sign( tx, active_key );
   //    STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_duplicate_sig );

   //    BOOST_TEST_MESSAGE( "--- Test success on active key" );
   //    tx.signatures.clear();
   //    sign( tx, active_key );
   //    db->push_transaction( tx, 0 );

   //    BOOST_TEST_MESSAGE( "--- Test success on owner key alone" );
   //    tx.signatures.clear();
   //    sign( tx, alice_private_key );
   //    db->push_transaction( tx, database::skip_transaction_dupe_check );

   //    BOOST_TEST_MESSAGE( "  Tests when owner authority is updated ---" );
   //    BOOST_TEST_MESSAGE( "--- Test failure when updating the owner authority with an active key" );
   //    tx.signatures.clear();
   //    tx.operations.clear();
   //    op.owner = authority( 1, active_key.get_public_key(), 1 );
   //    tx.operations.push_back( op );
   //    sign( tx, active_key );
   //    STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_owner_auth );

   //    BOOST_TEST_MESSAGE( "--- Test failure when owner key and active key are present" );
   //    sign( tx, alice_private_key );
   //    STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_irrelevant_sig );

   //    BOOST_TEST_MESSAGE( "--- Test failure when incorrect signature" );
   //    tx.signatures.clear();
   //    sign( tx, alice_post_key );
   //    STEEM_REQUIRE_THROW( db->push_transaction( tx, 0), tx_missing_owner_auth );

   //    BOOST_TEST_MESSAGE( "--- Test failure when duplicate owner keys are present" );
   //    tx.signatures.clear();
   //    sign( tx, alice_private_key );
   //    sign( tx, alice_private_key );
   //    STEEM_REQUIRE_THROW( db->push_transaction( tx, 0), tx_duplicate_sig );

   //    BOOST_TEST_MESSAGE( "--- Test success when updating the owner authority with an owner key" );
   //    tx.signatures.clear();
   //    sign( tx, alice_private_key );
   //    db->push_transaction( tx, 0 );

   //    validate_database();
   // }
   // FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
