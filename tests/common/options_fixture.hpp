#pragma once

#include <memory>
#include <vector>
#include <map>


#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <golos/plugins/operation_history/plugin.hpp>
#include <golos/plugins/account_history/plugin.hpp>
#include <golos/chain/operation_notification.hpp>

#include "database_fixture.hpp"
#include "comment_reward.hpp"


namespace bpo = boost::program_options;
namespace bfs = boost::filesystem;

namespace golos { namespace test {

using namespace golos::protocol;
using namespace golos::chain;

typedef golos::plugins::operation_history::applied_operation applied_operation;
typedef golos::plugins::account_history::operation_direction_type operation_direction_type;

typedef std::map<std::string, std::string> chacked_operations_map; ///<  pair { [itx_id], [operation name] }
typedef std::map<uint32_t, std::set<std::string>> chacked_accounts_map; ///<  pair { [block], [accaunt names] }


template<class key_type_option>
struct combine_postfix : key_type_option {
    typedef std::vector<std::string> opt_type;
    std::string opt =
        "account_create_operation," \
        "delete_comment_operation," \
        "vote," \
        "comment";
};


template<class opt_type>
struct test_options : public opt_type {
    bpo::options_description _cfg_opts;
    bpo::options_description _cli_opts;
    bpo::variables_map _vm_opts;

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

        bpo::options_description all_opts;
        all_opts
            .add(_cli_opts)
            .add(_cfg_opts)
            ;
        const char *argv = "plugin";
        auto parsed_cmd_line = bpo::parse_command_line(1, &argv, all_opts);
        bpo::store(parsed_cmd_line, _vm_opts);
    }

    template<class custom_opt_type>
    void fill_options() {
        custom_opt_type cot;
        std::stringstream ss_opts;
        ss_opts << cot.key << " = " << cot.opt << "\n";
        std::istringstream iss_opts(ss_opts.str());
        _cfg_opts.add_options()
            (cot.key.c_str(), bpo::value<typename custom_opt_type::opt_type>())
            ;
        auto parsed_cfg = bpo::parse_config_file<char>(iss_opts, _cfg_opts, true);
        bpo::store(parsed_cfg, _vm_opts);
    }

    test_options() {
        fill_default_options();
        fill_options<opt_type>();
    }

    operator bpo::variables_map () const {
        return _vm_opts;
    }
};


struct operation_options_fixture {
    add_operations_database_fixture _db_init;
    chacked_operations_map _founded_ops;

    operation_options_fixture() = default;
    ~operation_options_fixture() = default;

    template<class test_type>
    void init_plugin(const test_type& tt) {
        ilog(std::string("init_plugin(") + typeid(test_type).name() + ")");
        _db_init._plg->plugin_initialize(tt);
        _db_init._plg->plugin_startup();
        _db_init.startup();
        _db_init.add_operations();
        check_operations();
    }

    void log_applied_options(const applied_operation &opts) const;

    void check_operations();
};


struct account_options_fixture {
    add_accounts_database_fixture _db_init;
    chacked_accounts_map _founded_accs;

    account_options_fixture() = default;
    ~account_options_fixture() = default;

    template<class test_type>
    void init_plugin(const test_type& tt) {
        ilog(std::string("init_plugin(") + typeid(test_type).name() + ")");
        _db_init._plg->plugin_initialize(tt);
        _db_init._plg->plugin_startup();
        _db_init.startup();
        _db_init.add_accounts();
        check();
    }

    void check();
};


struct account_direction_fixture {
    accounts_direction_database_fixture _db_init;
    chacked_accounts_map _founded_accs;

    account_direction_fixture() = default;
    ~account_direction_fixture() = default;

    template<class test_type>
    void init_plugin(const test_type& tt) {
        ilog(std::string("init_plugin(") + typeid(test_type).name() + ")");
        _db_init._plg->plugin_initialize(tt);
        _db_init._plg->plugin_startup();
        _db_init.startup();
        _db_init.add_accounts();
    }

    void check(operation_direction_type dir);
};
}}
