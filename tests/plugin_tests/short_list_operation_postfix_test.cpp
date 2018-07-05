#include <string>
#include <cstdint>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include "database_fixture.hpp"
#include "comment_reward.hpp"
#include "options_fixture.hpp"


using namespace golos::test;


struct short_list_options_postfix {
    typedef std::vector<std::string> opt_type;
    std::string key = "history-whitelist-ops";
    std::string opt =
        "account_create_operation," \
        "delete_comment," \
        "comment";
};


static const int SHORT_LIST_LEN = 3;


BOOST_FIXTURE_TEST_CASE(operation_history_blocks, operation_options_fixture) {
    auto tops = test_options<short_list_options_postfix>();
    init_plugin(tops);

    size_t _chacked_ops_count = 0;
    for (auto it = _founded_ops.begin(); it != _founded_ops.end(); ++it) {
        auto iter = _db_init._added_ops.find(it->first);
        bool is_found = (iter != _db_init._added_ops.end());
        if (is_found) {
            BOOST_TEST_MESSAGE("Found operation \"" + it->second + "\" by \"" + it->first + "\"");
            BOOST_CHECK_EQUAL(iter->second, it->second);
            if (iter->second == it->second) {
                ++_chacked_ops_count;
            }
        }
    }
    BOOST_CHECK_EQUAL(_chacked_ops_count, SHORT_LIST_LEN);
}
