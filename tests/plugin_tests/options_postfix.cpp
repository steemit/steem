/// Runnung example:
///     ./plugin_test --log_level=test_suite --run_test=options_postfix

#include <memory>
#include <vector>
#include <sstream>
#include <fstream>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include <golos/plugins/operation_history/plugin.hpp>



namespace golos { namespace tests {

namespace bpo = boost::program_options;
namespace bfs = boost::filesystem;

typedef golos::plugins::operation_history::plugin test_plugin;
    

struct option_with_postfix {
    std::string opt =
        "account_create_operation " \
        "account_update_operation " \
        "comment_operation " \
        "delete_comment_operation";
};


struct option_without_postfix {
    std::string opt =
        "account_create " \
        "account_update " \
        "comment " \
        "delete_comment";
};


template<class opt_type>
class test_options_postfix : public opt_type {
    std::unique_ptr<test_plugin> _plg;
    bpo::options_description _cfg_opts;
    bpo::options_description _cli_opts;
    bpo::variables_map _vm_opts;
    
    void fill_default_options() {
        bpo::options_description cfg_opts("Application Config Options");
        cfg_opts.add_options()
            ("plugin", bpo::value< vector<string> >()->composing(), 
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
        int argc = 0;
        char **argv = nullptr;
        bpo::store(bpo::parse_command_line(argc, argv, all_opts), _vm_opts); 
        bfs::path cfg_file_name = _vm_opts["config"].as<bfs::path>();
        bfs::path cfg_dir = _vm_opts["data-dir"].as<bfs::path>();
        cfg_file_name = cfg_dir / cfg_file_name;
        if (bfs::exists(cfg_file_name)) {
            bfs::remove(cfg_file_name);
        }
        std::ofstream out_cfg(bfs::path(cfg_file_name).make_preferred().string());
        out_cfg << "history-whitelist-ops = " << opt_type::opt << "\n";
        out_cfg << "history-blacklist-ops = " << opt_type::opt << "\n";
        bfs::path cfg_path = cfg_file_name.make_preferred();
        auto parsed_cfg = bpo::parse_config_file<char>(cfg_path.string().c_str(), _cfg_opts, true);
        //_cli_opts.add_options()
        //    ("history-whitelist-ops", "")
        //    ("history-blacklist-ops", "")
        //    ;
        //std::string str_opts(
        //    "history-whitelist-ops = " + opt_type::opt + "\n" +
        //    "history-blacklist-ops = " + opt_type::opt + "\n");
        //std::istringstream iss_opts(str_opts);
        //auto parsed_cfg = bpo::parse_config_file(iss_opts, all_opts);
        //auto parsed_cfg = bpo::parse_command_line(argc, test_opt, all_options);
        BOOST_TEST_MESSAGE("# PARSE OPTS");
        std::vector<bpo::option> args = parsed_cfg.options;
        for (auto arg : args) {
            for (auto val : arg.value) {
                std::string msg("> {" + arg.string_key + " : " + val + "}");
                BOOST_TEST_MESSAGE(msg);
            }
        }
        bpo::store(parsed_cfg, _vm_opts);
        bpo::notify(_vm_opts);

        if (bfs::exists(cfg_file_name)) {
            bfs::remove(cfg_file_name);
        }
    }
    
public:
    test_options_postfix() {
        fill_default_options();
        fill_testing_options();
        _plg.reset(new test_plugin);
        BOOST_TEST_MESSAGE("# plugin :" + _plg->name());
        _plg->initialize(_vm_opts);
    }
    
    ~test_options_postfix() {
    }
};
}}


typedef golos::tests::option_with_postfix option_with_postfix;
typedef golos::tests::option_without_postfix option_without_postfix;
typedef golos::tests::test_options_postfix<option_with_postfix> test_with_postfix;
typedef golos::tests::test_options_postfix<option_without_postfix> test_without_postfix;


BOOST_AUTO_TEST_SUITE(options_postfix);

BOOST_AUTO_TEST_CASE(options_without_postfix) {
    BOOST_TEST_MESSAGE("@ options_without_postfix: ");
    test_without_postfix();
}

BOOST_AUTO_TEST_CASE(options_with_postfix) {
    BOOST_TEST_MESSAGE("@ options_with_postfix: ");
    test_with_postfix();
}

BOOST_AUTO_TEST_SUITE_END()
