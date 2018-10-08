#if defined IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <steem/plugins/transaction_status/transaction_status_plugin.hpp>

#include "../db_fixture/database_fixture.hpp"

using namespace steem::chain;
using namespace steem::protocol;

BOOST_FIXTURE_TEST_SUITE( transaction_status, database_fixture )

BOOST_AUTO_TEST_CASE( transaction_status_test )
{
   using namespace steem::plugins::transaction_status;

   try
   {
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
#endif

