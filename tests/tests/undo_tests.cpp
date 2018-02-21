#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <steem/protocol/exceptions.hpp>
#include <steem/protocol/hardfork.hpp>

#include <steem/chain/database.hpp>
#include <steem/chain/database_exceptions.hpp>
#include <steem/chain/steem_objects.hpp>

#include <steem/chain/util/reward.hpp>

#include <steem/plugins/witness/witness_objects.hpp>

#include <fc/macros.hpp>
#include <fc/crypto/digest.hpp>

#include "../db_fixture/database_fixture.hpp"
#include "../undo_data/undo.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace steem;
using namespace steem::chain;
using namespace steem::protocol;
using fc::string;

BOOST_FIXTURE_TEST_SUITE( undo_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( undo_basic )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: undo_basic" );

      account_object_scenario ao( *db );

      ao.add_operations( "cm" );

      BOOST_REQUIRE( ao.run() );

   }
   FC_LOG_AND_RETHROW()
}


BOOST_AUTO_TEST_SUITE_END()
#endif
