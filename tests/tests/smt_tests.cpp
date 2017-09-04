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

BOOST_AUTO_TEST_SUITE_END()
#endif
