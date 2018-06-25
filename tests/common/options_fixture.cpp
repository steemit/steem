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


void options_fixture::log_applied_options(const applied_operation &opts) const {
    std::stringstream ss;
    ss << "[" << opts.block << "] ";
    ss << opts.trx_id.str() << " : "; /// golos::protocol::transaction_id_type
    std::string op_name = opts.op.visit(ovisit);
    ss << "\"" << op_name << "\""; /// golos::protocol::operation
    ilog(ss.str());
}


void options_fixture::check_operations() {
    uint32_t head_block_num = _db_init.db->head_block_num();
    ilog("Check history operations, block num is " + std::to_string(head_block_num));
    for (uint32_t i = 0; i <= head_block_num; ++i) {
        msg_pack mo;
        mo.args = std::vector<fc::variant>({fc::variant(i), fc::variant(false)});
        auto ops = _db_init._plg->get_ops_in_block(mo);
        for (auto o : ops) {
            auto iter = _finded_ops.find(o.trx_id.str());
            if (iter == _finded_ops.end()) {
                _finded_ops.insert(std::make_pair(o.trx_id.str(), o.op.visit(ovisit)));
                log_applied_options(o);
            }
        }
    }
}
