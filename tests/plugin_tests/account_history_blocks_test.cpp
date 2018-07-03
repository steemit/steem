#include <string>
#include <cstdint>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include "database_fixture.hpp"
#include "comment_reward.hpp"
#include "options_fixture.hpp"


using namespace golos::test;


static uint32_t HISTORY_BLOCKS = 3;


struct account_options {
    typedef uint32_t opt_type;
    std::string key = "history-blocks";
    std::string opt = std::to_string(HISTORY_BLOCKS);
};


BOOST_FIXTURE_TEST_CASE(account_history_blocks, account_options_fixture) {
    auto tops = test_options<account_options>();
    init_plugin(tops);

    std::set<uint32_t> blocks;
    for (auto a : _founded_accs) {
        for (auto n : a.second) {
            ilog("block:" + std::to_string(a.first) + ", \"" + n + "\"");
            auto iter = _db_init._account_names.find(n);
            bool is_found = (iter != _db_init._account_names.end());
            BOOST_CHECK(is_found);
            if (is_found) {
                blocks.insert(a.first);
            } else {
                BOOST_TEST_MESSAGE("Account [" + std::to_string(a.first) + "]: \"" + n + "\" is not found");
            }
        }
    }
    BOOST_CHECK_EQUAL(blocks.size(), HISTORY_BLOCKS);
}
