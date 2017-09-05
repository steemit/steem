#ifdef IS_TEST_NET

#include <boost/test/unit_test.hpp>

#include <steemit/protocol/exceptions.hpp>
#include <steemit/protocol/hardfork.hpp>

#include <steemit/chain/database.hpp>
#include <steemit/chain/database_exceptions.hpp>
#include <steemit/chain/steem_objects.hpp>

#include "../db_fixture/database_fixture.hpp"

using namespace steemit::chain;
using namespace steemit::protocol;
using fc::string;
using boost::container::flat_set;

BOOST_FIXTURE_TEST_SUITE( smt_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( elevate_account_validate )
{
   try
   {
      smt_elevate_account_operation op;
      op.account = "@@@@@";
      op.fee = ASSET( "1.000 TESTS" );
      STEEMIT_REQUIRE_THROW( op.validate(), fc::exception );

      op.account = "alice";
      op.validate();

      op.fee.amount = -op.fee.amount;
      STEEMIT_REQUIRE_THROW( op.validate(), fc::exception );

      // passes with MAX_SHARE_SUPPLY
      op.fee.amount = STEEMIT_MAX_SHARE_SUPPLY;
      op.validate();

      // fails with MAX_SHARE_SUPPLY+1
      ++op.fee.amount;
      STEEMIT_REQUIRE_THROW( op.validate(), fc::exception );

      op.fee = ASSET( "1.000 WRONG" );
      STEEMIT_REQUIRE_THROW( op.validate(), fc::exception );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( elevate_account_authorities )
{
   try
   {
      smt_elevate_account_operation op;
      op.account = "alice";
      op.fee = ASSET( "1.000 TESTS" );

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

BOOST_AUTO_TEST_CASE( elevate_account_apply )
{
   try
   {
      set_price_feed( price( ASSET( "1.000 TESTS" ), ASSET( "1.000 TBD" ) ) );

      ACTORS( (alice)(bob) )

      fund( "alice", 10 * 1000 * 1000 );

      smt_elevate_account_operation op;

      op.fee = ASSET( "1000.000 TBD" );
      op.account = "alice";

      signed_transaction tx;

      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEMIT_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      // throw due to insufficient balance
      STEEMIT_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      convert( "alice", ASSET( "5000.000 TESTS" ) );
      db->push_transaction( tx, 0 );

      // Check the account cannot elevate itself twice
      STEEMIT_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), fc::exception );

      // TODO:
      // - Check that 1000 TESTS throws
      // - Check that less than 1000 SBD throws
      // - Check that more than 1000 SBD succeeds
      //
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
#endif
