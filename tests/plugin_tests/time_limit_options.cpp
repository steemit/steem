/// Runnung example:
///     ./plugin_test --log_level=test_suite --run_test=black_options_postfix

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include "database_fixture.hpp"
#include "comment_reward.hpp"
#include "options_postfix.hpp"


using namespace golos::test;



BOOST_FIXTURE_TEST_CASE(time_limit_options, options_fixture) {
}

// hystory-blocks
