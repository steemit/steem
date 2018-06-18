/// Runnung example:
///     ./plugin_test --log_level=test_suite --run_test=options_postfix

#include <memory>
#include <vector>
#include <sstream>
#include <fstream>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include <golos/plugins/operation_history/plugin.hpp>
#include <golos/chain/operation_notification.hpp>

#include "database_fixture.hpp"
#include "comment_reward.hpp"


namespace golos { namespace test {
    
namespace bpo = boost::program_options;
namespace bfs = boost::filesystem;

typedef golos::chain::clean_database_fixture clean_database_fixture;
typedef golos::chain::database_fixture database_fixture;
typedef golos::chain::database database;
typedef golos::plugins::operation_history::plugin gpoh_plugin;
typedef golos::plugins::operation_history::applied_operation applied_operation; 
typedef golos::plugins::chain::plugin chain_plugin;
typedef golos::plugins::json_rpc::plugin json_rpc_plugin;
typedef golos::plugins::json_rpc::msg_pack msg_pack;


struct app_initialise {
    gpoh_plugin *_plg;
    
    app_initialise() {
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
        _plg = &appbase::app().register_plugin<gpoh_plugin>();
        BOOST_TEST_REQUIRE(_plg);
        appbase::app().initialize<gpoh_plugin>(argc, argv);
    }
};


struct key_option_whitelist {
    std::string key = "history-whitelist-ops";
};


struct key_option_blacklist {
    std::string key = "history-blacklist-ops";
};


struct option_with_postfix {
    std::string opt =
        "account_create_operation," \
        "vote_operation," \
        "comment_operation," \
        "delete_comment_operation";
};


struct option_without_postfix {
    std::string opt =
        "account_create," \
        "vote," \
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
        //print_vmap(_vm_opts);
        
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
        //print_vmap(_vm_opts);
    }
    

    test_options_postfix() {
        fill_default_options();
        fill_testing_options();
    }
    
    operator bpo::variables_map () const {
        return _vm_opts;
    }
    
    struct operation_visitor {   
        using result_type = std::string;   
        template<class T>
        std::string operator()(const T&) const { 
            return std::string(fc::get_typename<T>::name());   
        }
    } ovisit;


    bool check_applied_options(const applied_operation &opts) const {
        std::stringstream ss;
        ss << opts.trx_id << ", "; /// golos::protocol::transaction_id_type
        ss << opts.block << ", ";
        ss << opts.trx_in_block << ", ";
        ss << opts.op_in_trx << ", ";
        ss << opts.virtual_op << ", ";
        ss << opts.timestamp.to_iso_string() << ", "; /// fc::time_point_sec
        ss << "which is [" << opts.op.visit(ovisit) << "]"; /// golos::protocol::operation
        BOOST_TEST_MESSAGE(ss.str());
        return true;
    }

};


using namespace golos::protocol;
using namespace golos::chain;


//struct transaction_fixture {
template<class test_type>
//struct transaction_fixture : clean_database_fixture {
struct transaction_fixture : database_fixture {
    test_type _tt;
    gpoh_plugin *_plg;

    transaction_fixture(const test_type &tt) 
        : _tt(tt) { 
        BOOST_TEST_MESSAGE("Create transactions");
        try {
            database_fixture::initialize();
            database_fixture::open_database();
            database_fixture::startup();

            _plg = app_initialise()._plg;
            _plg->plugin_initialize(_tt);
            _plg->plugin_startup();
            
            add_operations();
            check_operations();
        } FC_LOG_AND_RETHROW();
    }

    void add_operations() {
        BOOST_TEST_MESSAGE("Init transactions");

        ACTORS((alice)(bob)(sam))
        fund("alice", 10000);            
        vest("alice", 10000);            
        fund("bob", 7500);            
        vest("bob", 7500);            
        fund("sam", 8000);            
        vest("sam", 8000);            
        
        db->set_clear_votes(0xFFFFFFFF);
        
        signed_transaction tx;            
        
        BOOST_TEST_MESSAGE("Creating comments.");   
        comment_operation com;            
        com.author = "bob";            
        com.permlink = "test";            
        com.parent_author = "";            
        com.parent_permlink = "test";            
        com.title = "foo";            
        com.body = "bar";            
        tx.set_expiration(db->head_block_time() + STEEMIT_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(com); 
        tx.sign(bob_private_key, db->get_chain_id());
        db->push_transaction(tx, 0);
        generate_block();
        tx.operations.clear();
        tx.signatures.clear();
        
        BOOST_TEST_MESSAGE("Voting for comments.");
        vote_operation vote;            
        vote.voter = "alice";            
        vote.author = "bob";            
        vote.permlink = "test";            
        vote.weight = -1; ///< Nessary for the posiblity of delet_comment_operation.            
        tx.operations.push_back(vote);            
        vote.voter = "bob";            
        tx.operations.push_back(vote);            
        vote.voter = "sam";            
        tx.operations.push_back(vote);            
        tx.sign(alice_private_key, db->get_chain_id());            
        tx.sign(bob_private_key, db->get_chain_id());            
        tx.sign(sam_private_key, db->get_chain_id());            
        db->push_transaction(tx, 0);
        generate_block();
        tx.operations.clear();
        tx.signatures.clear();
        
        BOOST_TEST_MESSAGE("Deleting comment.");
        delete_comment_operation dco;
        dco.author = "bob";
        dco.permlink = "test";
        tx.set_expiration(db->head_block_time() + STEEMIT_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(dco); 
        tx.sign(bob_private_key, db->get_chain_id());
        db->push_transaction(tx, 0);
        generate_block();
        tx.operations.clear();
        tx.signatures.clear();
        
        BOOST_TEST_MESSAGE("Generate accaunt.");
        account_create_operation aco;
        aco.new_account_name = "dave";
        aco.creator = STEEMIT_INIT_MINER_NAME;
        aco.owner = authority(1, init_account_pub_key, 1);
        aco.active = aco.owner;
        tx.set_expiration(db->head_block_time() + STEEMIT_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(aco);
        tx.sign(init_account_priv_key, db->get_chain_id());
        db->push_transaction(tx, 0);
        generate_block();
        tx.operations.clear();
        tx.signatures.clear();
        
        validate_database();
    }
        
    void check_operations() {
        BOOST_TEST_MESSAGE("Check history operations.");
        //msg_pack mt;
        //mt.args = std::vector<fc::variant>({fc::variant(1)});
        //auto trans = _plg->get_transaction(mt);
        //const auto& idx = database.get_index<operation_index>().indices().get<by_location>();
        
        msg_pack mo;
        //mo.args = std::vector<fc::variant>({fc::variant(trans.block_num), fc::variant(true)});
        mo.args = std::vector<fc::variant>({fc::variant(1), fc::variant(true)});
        auto ops = _plg->get_ops_in_block(mo);
        BOOST_TEST_MESSAGE("Checked perations is " + std::to_string(ops.size()) + " {");
        BOOST_TEST_MESSAGE("\t[" + std::to_string(operation::tag<account_create_operation>::value) + 
                           "] account_create_operation");
        BOOST_TEST_MESSAGE("\t[" + std::to_string(operation::tag<vote_operation>::value) + 
                           "] vote_operation");
        BOOST_TEST_MESSAGE("\t[" + std::to_string(operation::tag<comment_operation>::value) + 
                           "] comment_operation");
        BOOST_TEST_MESSAGE("\t[" + std::to_string(operation::tag<delete_comment_operation>::value) + 
                           "] delete_comment_operation");
        BOOST_TEST_MESSAGE("}");

        //size_t count = 0;
        for (auto o : ops) {
            _tt.check_applied_options(o);
            
            //fc::variant v(o.op);
            //fc::variant v;
            //fc::to_variant(o.op, v);
          
            //BOOST_TEST_MESSAGE("type name[" + std::to_string(++count) + "]: " + o.op.visit(ovisit));
            //BOOST_TEST_MESSAGE("type name[" + std::to_string(++count) + "]: " + v.get_type().name_from_type);
            
            //BOOST_TEST_MESSAGE(std::string("is marcet: ") + (o.is_market_operation() ? "TRUE":"FALSE"));
            //fc::variant v;
            //fc::to_variant(o.op, v);
            //BOOST_TEST_MESSAGE("witch = " + std::to_string(v.which()));
            //fc::get_operation_name on;
            //std::string op_name = on(o).name;
            //BOOST_TEST_MESSAGE("validate: " + (is_market_operation(o) ? "TRUE":"FALSE"));
            //if (o.op.which() == operation::tag<comment_operation>::value) {}
            //
            //    BOOST_TEST_MESSAGE("comment_operation " + std::to_string(++count) + " " 
            //        + std::to_string(operation::tag<comment_operation>::value));
                
                //auto &top = o.get<comment_operation>();
                //top.memo = decrypt_memo(top.memo);
            //}
        }
    }
};


template<class test_type>
struct execute_fixture {
    execute_fixture() {
        BOOST_TEST_MESSAGE("Execute");
        typedef transaction_fixture<test_type> transaction;
        std::unique_ptr<transaction> trans(new transaction(test_type()));
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


BOOST_AUTO_TEST_SUITE(options_postfix)

    BOOST_FIXTURE_TEST_CASE(white_options_with_postfix, execute_fixture<test_white_with_postfix>) {}

    BOOST_FIXTURE_TEST_CASE(white_options_without_postfix, execute_fixture<test_white_without_postfix>) {}
    
    BOOST_FIXTURE_TEST_CASE(black_options_with_postfix, execute_fixture<test_black_with_postfix>) {}
    
    BOOST_FIXTURE_TEST_CASE(black_options_without_postfix, execute_fixture<test_black_without_postfix>) {}

BOOST_AUTO_TEST_SUITE_END()


// - там есть настройка history-start-block - это номер блока до которого не хранить историю 
// - хранить историю только за последний день, неделю, месяц это более удобно потому, 
// что делегатов сейчас интересует только последняя история операций.


