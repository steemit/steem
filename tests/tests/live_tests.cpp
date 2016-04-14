#include <boost/test/unit_test.hpp>

#include <steemit/chain/database.hpp>
#include <steemit/chain/exceptions.hpp>
#include <steemit/chain/hardfork.hpp>

#include <steemit/chain/steem_objects.hpp>

#include <fc/crypto/digest.hpp>

#include "../common/database_fixture.hpp"

using namespace steemit::chain;
using namespace steemit::chain::test;
/*
BOOST_FIXTURE_TEST_SUITE( live_tests, live_database_fixture )

BOOST_AUTO_TEST_CASE( vests_stock_split )
{
   try
   {
      idump( (db.get_dynamic_global_properties()) );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()*/