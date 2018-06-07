/// Runnung example:
///     ./plugin_test --log_level=test_suite --run_test=options_postfix

#include <memory>

#include <boost/test/unit_test.hpp>

#include <golos/plugins/operation_history/plugin.hpp>



namespace golos { namespace tests {

typedef golos::plugins::operation_history::plugin plugin;


struct option_with_postfix {
    std::string opt = "history-whitelist-ops = " \
        "account_create_operation " \
        "account_update_operation " \
        "comment_operation " \
        "delete_comment_operation";
};


struct option_without_postfix {
    std::string opt = "history-whitelist-ops = " \
        "account_create " \
        "account_update " \
        "comment " \
        "delete_comment";
};


template<class opt_type>
class test_options_postfix : public opt_type {
    std::unique_ptr<plugin> plg;
    
public:
    test_options_postfix() {
        BOOST_TEST_MESSAGE("\"" + opt_type::opt + "\"");
        plg.reset(new plugin);
        plg->initialise();
    }
    
    ~test_options_postfix() {
        plg.reset();
    }
};
}}


typedef golos::tests::option_with_postfix option_with_postfix;
typedef golos::tests::option_without_postfix option_without_postfix;
typedef golos::tests::test_options_postfix<option_with_postfix> test_with_postfix;
typedef golos::tests::test_options_postfix<option_without_postfix> test_without_postfix;


BOOST_AUTO_TEST_SUITE(options_postfix);

BOOST_AUTO_TEST_CASE(options_with_postfix) {
    BOOST_TEST_MESSAGE("options_with_postfix: ");
    test_with_postfix();
}


BOOST_AUTO_TEST_CASE(options_without_postfix) {
    BOOST_TEST_MESSAGE("options_without_postfix: ");
    test_without_postfix();
}

BOOST_AUTO_TEST_SUITE_END()
