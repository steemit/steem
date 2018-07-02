#include <sstream>

#include <boost/test/unit_test.hpp>

#include "options_fixture.hpp"


using namespace golos::test;

namespace bpo = boost::program_options;

typedef golos::chain::add_operations_database_fixture add_operations_database_fixture;
typedef golos::plugins::json_rpc::msg_pack msg_pack;

using namespace golos::protocol;
using namespace golos::chain;


struct operation_visitor {
    using result_type = std::string;
    template<class T>
    std::string operator()(const T&) const {
        return std::string(fc::get_typename<T>::name());
    }
} ovisit;


void operation_options_fixture::log_applied_options(const applied_operation &opts) const {
    std::stringstream ss;
    ss << "[" << opts.block << "] ";
    ss << opts.trx_id.str() << " : "; /// golos::protocol::transaction_id_type
    std::string op_name = opts.op.visit(ovisit);
    ss << "\"" << op_name << "\""; /// golos::protocol::operation
    ilog(ss.str());
}


void operation_options_fixture::check_operations() {
    uint32_t head_block_num = _db_init.db->head_block_num();
    ilog("Check history operations, block num is " + std::to_string(head_block_num));
    auto plg = _db_init._plg;
    if (plg) {
        for (uint32_t i = 0; i <= head_block_num; ++i) {
            msg_pack mo;
            mo.args = std::vector<fc::variant>({fc::variant(i), fc::variant(false)});
            auto ops = plg->get_ops_in_block(mo);
            for (auto o : ops) {
                auto iter = _founded_ops.find(o.trx_id.str());
                if (iter == _founded_ops.end()) {
                    _founded_ops.insert(std::make_pair(o.trx_id.str(), o.op.visit(ovisit)));
                    log_applied_options(o);
                }
            }
        }
    } else {
        ilog("Operation history plugin is not inited.");
    }
}


void account_options_fixture::check() {
    uint32_t head_block_num = _db_init.db->head_block_num();
    ilog("Check history accounts, block num is " + std::to_string(head_block_num));
    auto plg = _db_init._plg;
    if (plg) {
        fc::flat_map<std::string, std::string> accs = plg->tracked_accounts();
        for (auto a : accs) {
            ilog("{\"" + a.first + "\":\"" + a.second + "\"}");
        }
    }  else {
        ilog("Account history plugin is not inited.");
    }

    //for (uint32_t i = 0; i <= head_block_num; ++i) {
    //    msg_pack mp;
    //    std::string account = "";
    //    uint64_t from = 0;
    //    uint32_t limit = 0;
    //    mp.args = std::vector<fc::variant>({fc::variant(account), fc::variant(from), fc::variant(limit)});
    //}
}
