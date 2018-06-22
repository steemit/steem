/// Runnung example:
///     ./plugin_test --log_level=test_suite --run_test=black_options_postfix

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include "database_fixture.hpp"
#include "comment_reward.hpp"
#include "options_postfix.hpp"


using namespace golos::test;


struct blacklist_key {
    std::string key = "history-blacklist-ops";
};


BOOST_FIXTURE_TEST_CASE(black_options_postfix, options_fixture) {
    init_plugin<test_options_postfix<combine_postfix<blacklist_key>>>();

    size_t _chacked_ops_count = 0;
    for (const auto &co : _db_init._added_ops) {
        auto iter = _finded_ops.find(co.first);
        bool is_not_finded = (iter == _finded_ops.end());
        BOOST_CHECK(is_not_finded);
        if (is_not_finded) {
            ++_chacked_ops_count;
        } else {
            BOOST_TEST_MESSAGE("Operation \"" + co.second + "\" by \"" + co.first + "\" is found");
        }
    }
    BOOST_CHECK_EQUAL(_chacked_ops_count, _db_init._added_ops.size());
}
