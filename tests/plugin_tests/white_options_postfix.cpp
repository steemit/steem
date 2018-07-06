#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include "database_fixture.hpp"
#include "comment_reward.hpp"
#include "options_fixture.hpp"


using namespace golos::test;


struct whitelist_key {
    std::string key = "history-whitelist-ops";
};


BOOST_FIXTURE_TEST_CASE(white_options_postfix, operation_options_fixture) {
    init_plugin(test_options<combine_postfix<whitelist_key>>());

    size_t _chacked_ops_count = 0;
    for (const auto &co : _db_init._added_ops) {
        auto iter = _founded_ops.find(co.first);
        bool is_found = (iter != _founded_ops.end());
        BOOST_CHECK(is_found);
        if (is_found) {
            BOOST_CHECK_EQUAL(iter->second, co.second);
            if (iter->second == co.second) {
                ++_chacked_ops_count;
            }
        } else {
            BOOST_TEST_MESSAGE("Operation \"" + co.second + "\" by \"" + co.first + "\" is not found");
        }
    }
    BOOST_CHECK_EQUAL(_chacked_ops_count, _db_init._added_ops.size());
}
