/// Runnung example:
///     ./plugin_test --log_level=test_suite --run_test=options_postfix

#include <memory>
#include <vector>
#include <sstream>
#include <fstream>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include <golos/plugins/operation_history/plugin.hpp>
#include "database_fixture.hpp"


namespace golos { namespace test {
namespace bpo = boost::program_options;
namespace bfs = boost::filesystem;

typedef golos::plugins::operation_history::plugin gpoh_plugin;
typedef golos::plugins::operation_history::applied_operation applied_operation; 
typedef golos::chain::database_fixture database_fixture;
//using namespace golos::protocol;


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
    //std::unique_ptr<oh_plugin> _plg;
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
        const char *argv = "plugin_test";
        auto parsed_cmd_line = bpo::parse_command_line(1, &argv, all_opts);
        print_options(parsed_cmd_line.options);
        bpo::store(parsed_cmd_line, _vm_opts); 
        print_vmap(_vm_opts);
        
        //bfs::path cfg_file_name = _vm_opts["config"].as<bfs::path>();
        //bfs::path cfg_dir = _vm_opts["data-dir"].as<bfs::path>();
        //cfg_file_name = cfg_dir / cfg_file_name;
        //if (bfs::exists(cfg_file_name)) {
        //    bfs::remove(cfg_file_name);
        //}
        //std::ofstream out_cfg(bfs::path(cfg_file_name).make_preferred().string());
        //out_cfg << "history-whitelist-ops = " << opt_type::opt << "\n";
        //out_cfg << "history-blacklist-ops = " << opt_type::opt << "\n";
        //bfs::path cfg_path = cfg_file_name.make_preferred();
        
        std::stringstream ss_opts;
        ss_opts << "history-whitelist-ops = " << opt_type::opt << "\n";
        ss_opts << "history-blacklist-ops = " << opt_type::opt << "\n";
        std::istringstream iss_opts(ss_opts.str());
        _cfg_opts.add_options()
            (key_opt_type::key.c_str(), bpo::value<std::vector<std::string>>())
            ;
        auto parsed_cfg = bpo::parse_config_file<char>(iss_opts, _cfg_opts, true);

        //auto parsed_cfg = bpo::parse_config_file<char>(cfg_path.string().c_str(), _cfg_opts, true);
        BOOST_TEST_MESSAGE("# PARSE OPTS");
        print_options(parsed_cfg.options);
        bpo::store(parsed_cfg, _vm_opts);
        print_vmap(_vm_opts);
        //BOOST_TEST_MESSAGE((_vm_opts.count("history-whitelist-ops") ? "TRUE":"FALSE"));
        //BOOST_TEST_MESSAGE((_vm_opts.count("history-blacklist-ops") ? "TRUE":"FALSE"));
        
        //bpo::notify(_vm_opts);
        
        //if (bfs::exists(cfg_file_name)) {
        //    bfs::remove(cfg_file_name);
        //}
    }
    

    //test_options_postfix(oh_plugin *oh_plg) {
    test_options_postfix() {
        fill_default_options();
        fill_testing_options();
        //BOOST_TEST_REQUIRE(oh_plg);
        //BOOST_TEST_MESSAGE("# plugin: \"" + oh_plg->name() + "\"");
        //oh_plg->initialize(_vm_opts);
        
        //std::vector<applied_operation> opts_list = _plg->get_ops_in_block();
        //for(auto opts : opts_list) {
        //    BOOST_TEST_MESSAGE("# ops_in_block :" + opts);
        //}
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
//typedef std::unique_ptr<test_white_with_postfix> ptest_white_with_postfix;
//typedef std::unique_ptr<test_white_without_postfix> ptest_white_without_postfix;
//typedef std::unique_ptr<test_black_with_postfix> ptest_black_with_postfix;
//typedef std::unique_ptr<test_black_without_postfix> ptest_black_without_postfix;


//BOOST_AUTO_TEST_SUITE(options_postfix);
BOOST_FIXTURE_TEST_SUITE(options_postfix, database_fixture);

    BOOST_AUTO_TEST_CASE(options_white_with_postfix) {
        BOOST_TEST_MESSAGE("\n@ white options with postfix: ");
        try {
            test_white_with_postfix o;
            //appbase::app().add_program_options(o._cli_opts, o._cfg_opts);
            gpoh_plugin *plg = &appbase::app().register_plugin<gpoh_plugin>();
            
            //initialize();
            BOOST_TEST_REQUIRE(plg);
            BOOST_TEST_MESSAGE("# plugin: \"" + plg->name() + "\"");
            plg->initialize(o._vm_opts);
            //plg->initialize(getVmOptions());
            //ptest_white_with_postfix test_obj(new test_white_with_postfix(plg));
            //test_white_with_postfix(plg);
            //open_database();
            //close_database();
        } FC_LOG_AND_RETHROW();
    }

    //BOOST_AUTO_TEST_CASE(options_white_without_postfix) {
    //    BOOST_TEST_MESSAGE("\n@ white options without postfix: ");
    //    //test_white_with_postfix();
    //}
    //
    //BOOST_AUTO_TEST_CASE(options_black_with_postfix) {
    //    BOOST_TEST_MESSAGE("\n@ black options with postfix: ");
    //    //test_black_without_postfix();
    //}
    //
    //BOOST_AUTO_TEST_CASE(options_black_without_postfix) {
    //    BOOST_TEST_MESSAGE("\n@ black options without postfix: ");
    //    //test_black_with_postfix();
    //}

BOOST_AUTO_TEST_SUITE_END()


// - там есть настройка history-start-block - это номер блока до которого не хранить историю 
// - хранить историю только за последний день, неделю, месяц это более удобно потому, 
// что делегатов сейчас интересует только последняя история операций.
