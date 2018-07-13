#include <string>
#include <cstdint>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include "database_fixture.hpp"
#include "comment_reward.hpp"
#include "options_fixture.hpp"


using namespace golos::test;


struct directionlist_key {
    std::string key = "history-whitelist-ops";
};


typedef std::vector<std::string> founded_need;


struct any_founded_need {
    founded_need v = {
        "[cyberfounder] golos::protocol::custom_operation",
        "[alice] golos::protocol::account_create_operation",
        "[alice] golos::protocol::transfer_operation",
        "[alice] golos::protocol::transfer_to_vesting_operation",
        "[bob] golos::protocol::account_create_operation",
        "[bob] golos::protocol::comment_operation",
        "[bob] golos::protocol::transfer_operation",
        "[bob] golos::protocol::transfer_to_vesting_operation",
        "[cyberfounder] golos::protocol::account_create_operation",
        "[cyberfounder] golos::protocol::transfer_operation",
        "[cyberfounder] golos::protocol::transfer_to_vesting_operation",
        "[sam] golos::protocol::account_create_operation",
        "[sam] golos::protocol::transfer_operation",
        "[sam] golos::protocol::transfer_to_vesting_operation",
        "[alice] golos::protocol::vote_operation",
        "[bob] golos::protocol::vote_operation",
        "[sam] golos::protocol::vote_operation",
        "[bob] golos::protocol::delete_comment_operation",
        "[cyberfounder] golos::protocol::account_create_operation",
        "[dave] golos::protocol::account_create_operation"
    };
};


struct sender_founded_need {
    founded_need v = {
        "[alice] golos::protocol::transfer_to_vesting_operation",
        "[bob] golos::protocol::comment_operation",
        "[bob] golos::protocol::transfer_to_vesting_operation",
        "[cyberfounder] golos::protocol::account_create_operation",
        "[cyberfounder] golos::protocol::transfer_operation",
        "[cyberfounder] golos::protocol::transfer_to_vesting_operation",
        "[sam] golos::protocol::transfer_to_vesting_operation",
        "[alice] golos::protocol::vote_operation",
        "[sam] golos::protocol::vote_operation",
        "[cyberfounder] golos::protocol::account_create_operation"
    };
};


struct receiver_founded_need {
    founded_need v = {
        "[alice] golos::protocol::account_create_operation",
        "[alice] golos::protocol::transfer_operation",
        "[bob] golos::protocol::account_create_operation",
        "[bob] golos::protocol::transfer_operation",
        "[sam] golos::protocol::account_create_operation",
        "[sam] golos::protocol::transfer_operation",
        "[bob] golos::protocol::vote_operation",
        "[dave] golos::protocol::account_create_operation"
    };
};


BOOST_FIXTURE_TEST_CASE(account_history_direction, account_direction_fixture) {
    init_plugin(test_options<combine_postfix<directionlist_key>>());

    auto check_fn = [](const chacked_accounts_set& founded, const founded_need& need) {
        for (auto n : need) {
            BOOST_TEST_MESSAGE("\"" + n + "\"");
            BOOST_CHECK(founded.find(n) != founded.end());
        }
    };

    BOOST_TEST_MESSAGE("start any direction");
    check_fn(_any_founded_accs, any_founded_need().v);

    BOOST_TEST_MESSAGE("start sender direction");
    check_fn(_sender_founded_accs, sender_founded_need().v);

    BOOST_TEST_MESSAGE("start receiver direction");
    check_fn(_receiver_founded_accs, receiver_founded_need().v);
}
