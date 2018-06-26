#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include "database_fixture.hpp"
#include "comment_reward.hpp"
#include "options_fixture.hpp"


using namespace golos::test;

struct time_limit_key {
    std::string key = "history-bloks";
};


BOOST_FIXTURE_TEST_CASE(time_limit_options, options_fixture) {

}
