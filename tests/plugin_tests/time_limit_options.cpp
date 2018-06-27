#include <string>
#include <cstdint>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include "database_fixture.hpp"
#include "comment_reward.hpp"
#include "options_fixture.hpp"


using namespace golos::test;


struct whitelist_key {
    std::string key = "history-whitelist-ops";
};


static uint32_t HISTORY_BLOCKS = 2;


struct time_limit_key {
    typedef uint32_t opt_type;
    std::string key = "history-blocks";
    std::string opt = std::to_string(HISTORY_BLOCKS);
};


BOOST_FIXTURE_TEST_CASE(time_limit_options, options_fixture) {
    auto tops = test_options<combine_postfix<whitelist_key>>();
    tops.fill_options<time_limit_key>();
    init_plugin(tops);

    size_t _chacked_ops_count = 0;
    for (auto it = _finded_ops.begin(); it not_eq _finded_ops.end(); ++it) {
        auto iter = _db_init._added_ops.find(it->first);
        bool is_finded = (iter not_eq _db_init._added_ops.end());
        BOOST_CHECK(is_finded);
        if (is_finded) {
            BOOST_CHECK_EQUAL(iter->second, it->second);
            if (iter->second == it->second) {
                ++_chacked_ops_count;
            }
        } else {
            BOOST_TEST_MESSAGE("Operation \"" + it->second + "\" by \"" + it->first + "\" is not found");
        }
    }
    BOOST_CHECK_EQUAL(_chacked_ops_count, HISTORY_BLOCKS);

}
