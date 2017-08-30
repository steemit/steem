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

BOOST_FIXTURE_TEST_SUITE( smt_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( elevate_account )
{
   try
   {
      set_price_feed( price( ASSET( "1.000 TESTS" ), ASSET( "1.000 TBD" ) ) );

      ACTORS( (alice) )

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

      // TODO:
      // - Check that 1000 TESTS throws
      // - Check that less than 1000 SBD throws
      // - Check that more than 1000 SBD succeeds
      // - Check that account cannot elevate itself twice
      //
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
#endif
