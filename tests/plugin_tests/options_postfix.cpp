/// Runnung example:
///     ./plugin_test --log_level=test_suite --run_test=options_postfix

#include <memory>
#include <vector>
#include <sstream>
#include <fstream>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

//#include <appbase/plugin.hpp>
#include <golos/plugins/operation_history/plugin.hpp>
#include "database_fixture.hpp"


namespace golos { namespace test {
namespace bpo = boost::program_options;
namespace bfs = boost::filesystem;

typedef golos::chain::database_fixture database_fixture;
typedef golos::plugins::operation_history::plugin gpoh_plugin;
typedef golos::plugins::operation_history::applied_operation applied_operation; 
typedef golos::plugins::chain::plugin chain_plugin;
typedef golos::plugins::json_rpc::plugin json_rpc_plugin;


struct key_option_whitelist {
    std::string key = "history-whitelist-ops";
};


struct key_option_blacklist {
    std::string key = "history-blacklist-ops";
};


struct option_with_postfix {
    std::string opt =
        "account_create_operation," \
        "account_update_operation," \
        "comment_operation," \
        "delete_comment_operation";
};


struct option_without_postfix {
    std::string opt =
        "account_create," \
        "account_update," \
        "comment," \
        "delete_comment";
};


template<class key_opt_type, class opt_type>
struct test_options_postfix 
    : public key_opt_type 
    , public opt_type 
{
    bpo::options_description _cfg_opts;
    bpo::options_description _cli_opts;
    bpo::variables_map _vm_opts;

    void print_applied_opts(const applied_operation &opts) {
        std::stringstream ss;
        ss << opts.trx_id << ", "; /// golos::protocol::transaction_id_type
        ss << opts.block << ", ";
        ss << opts.trx_in_block << ", ";
        ss << opts.op_in_trx << ", ";
        ss << opts.virtual_op << ", ";
        ss << opts.timestamp.to_iso_string() << ", "; /// fc::time_point_sec
        //ss << opts.op << "\n"; /// golos::protocol::operation
    }

    void print_options(const std::vector<bpo::option> &args) {
        for (auto arg : args) {
            for (auto val : arg.value) {
                std::string msg("op > {" + arg.string_key + " : " + val + "}");
                BOOST_TEST_MESSAGE(msg);
            }
        }
    }

    void print_vmap(const bpo::variables_map &vm) {
        for (const auto& it : vm) {
            std::stringstream ss;
            ss << it.first.c_str() << " : ";
            auto& value = it.second.value();
            if (auto v = boost::any_cast<uint32_t>(&value)) {
                ss << *v;
            } else if (auto v = boost::any_cast<std::string>(&value)) {
                ss << *v;
            } else {
                ss << "TYPE";
            }
            BOOST_TEST_MESSAGE("vm > {" + ss.str() + "}");
        }
    }
    
    void fill_default_options() {
        bpo::options_description cfg_opts("Application Config Options");
        cfg_opts.add_options()
            ("plugin", bpo::value<std::vector<std::string>>()->composing(), 
                "Plugin(s) to enable, may be specified multiple times")
            ;
        _cfg_opts.add(cfg_opts);
        bpo::options_description cli_opts("Application Command Line Options");
        cli_opts.add_options()
            ("help,h", "Print this help message and exit.")
            ("version,v", "Print version information.")
            ("data-dir,d", bpo::value<bfs::path>()->default_value("./"), 
                "Directory containing configuration file config.ini")
            ("config,c", bpo::value<bfs::path>()->default_value("config.ini"), 
                "Configuration file name relative to data-dir")
            ;
        _cli_opts.add(cli_opts);
    }
    
    void fill_testing_options() {
        bpo::options_description all_opts;
        all_opts
            .add(_cli_opts)
            .add(_cfg_opts)
            ;
        const char *argv = "plugin";
        auto parsed_cmd_line = bpo::parse_command_line(1, &argv, all_opts);
        print_options(parsed_cmd_line.options);
        bpo::store(parsed_cmd_line, _vm_opts); 
        print_vmap(_vm_opts);
        
        std::stringstream ss_opts;
        ss_opts << "history-whitelist-ops = " << opt_type::opt << "\n";
        ss_opts << "history-blacklist-ops = " << opt_type::opt << "\n";
        std::istringstream iss_opts(ss_opts.str());
        _cfg_opts.add_options()
            (key_opt_type::key.c_str(), bpo::value<std::vector<std::string>>())
            ;
        auto parsed_cfg = bpo::parse_config_file<char>(iss_opts, _cfg_opts, true);
        print_options(parsed_cfg.options);
        bpo::store(parsed_cfg, _vm_opts);
        print_vmap(_vm_opts);
    }
    

    test_options_postfix() {
        fill_default_options();
        fill_testing_options();
    }
    
    ~test_options_postfix() {
    }
};
}}

using namespace golos::test;

typedef option_with_postfix opt_with_postfix;
typedef option_without_postfix opt_without_postfix;
typedef key_option_whitelist key_opt_whitelist;
typedef key_option_blacklist key_opt_blacklist;
typedef test_options_postfix<key_opt_whitelist, opt_with_postfix> test_white_with_postfix;
typedef test_options_postfix<key_opt_whitelist, opt_without_postfix> test_white_without_postfix;
typedef test_options_postfix<key_opt_blacklist, opt_with_postfix> test_black_with_postfix;
typedef test_options_postfix<key_opt_blacklist, opt_without_postfix> test_black_without_postfix;


BOOST_FIXTURE_TEST_SUITE(options_postfix, database_fixture);

    BOOST_AUTO_TEST_CASE(options_postfix_case) try {
        BOOST_TEST_MESSAGE("\n@ white options with postfix: ");
        int argc = boost::unit_test::framework::master_test_suite().argc;
        char **argv = boost::unit_test::framework::master_test_suite().argv;
        for (int i = 1; i < argc; i++) {
            const std::string arg = argv[i];
            if (arg == "--record-assert-trip") {
                fc::enable_record_assert_trip = true;
            }
            if (arg == "--show-test-names") {
                std::cout << "running test "
                          << boost::unit_test::framework::current_test_case().p_name
                          << std::endl;
            }
        }
        gpoh_plugin *plg = &appbase::app().register_plugin<gpoh_plugin>();
        BOOST_TEST_REQUIRE(plg);
        appbase::app().initialize<gpoh_plugin>(argc, argv);

        test_white_with_postfix wwo;
        plg->plugin_initialize(wwo._vm_opts);
        
        test_white_without_postfix woo;
        plg->plugin_initialize(woo._vm_opts);

        test_black_with_postfix bwo;
        plg->plugin_initialize(bwo._vm_opts);
        
        test_black_without_postfix boo;
        plg->plugin_initialize(boo._vm_opts);
    } 
    FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_SUITE_END()
