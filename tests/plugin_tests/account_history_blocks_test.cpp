#include <string>
#include <cstdint>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include "database_fixture.hpp"
#include "comment_reward.hpp"
#include "options_fixture.hpp"


using namespace golos::test;


static uint32_t HISTORY_BLOCKS = 2;


struct account_options {
    typedef uint32_t opt_type;
    std::string key = "history-blocks";
    std::string opt = std::to_string(HISTORY_BLOCKS);

};


BOOST_FIXTURE_TEST_CASE(account_history_blocks, account_options_fixture) {
    auto tops = test_options<account_options>();
    init_plugin(tops);
}
