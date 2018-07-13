#include <golos/plugins/account_history/plugin.hpp>
#include <golos/plugins/account_history/history_object.hpp>

#include <golos/plugins/operation_history/history_object.hpp>

#include <golos/chain/operation_notification.hpp>

#include <boost/algorithm/string.hpp>
#define STEEM_NAMESPACE_PREFIX "golos::protocol::"

#define CHECK_ARG_SIZE(s) \
   FC_ASSERT( args.args->size() == s, "Expected #s argument(s), was ${n}", ("n", args.args->size()) );

namespace golos { namespace plugins { namespace account_history {

    struct operation_visitor_filter;
    void operation_get_impacted_accounts(const operation &op,
                                         flat_map<golos::chain::account_name_type, operation_direction_type> &result);

using namespace golos::protocol;
using namespace golos::chain;
//
template<typename T>
T dejsonify(const string &s) {
    return fc::json::from_string(s).as<T>();
}

#define DEFAULT_VALUE_VECTOR(value) default_value({fc::json::to_string(value)}, fc::json::to_string(value))
#define LOAD_VALUE_SET(options, name, container, type) \
if( options.count(name) ) { \
    const std::vector<std::string>& ops = options[name].as<std::vector<std::string>>(); \
    std::transform(ops.begin(), ops.end(), std::inserter(container, container.end()), &dejsonify<type>); \
}
//

std::string to_string(operation_direction_type dir) {
    switch (dir) {
        case operation_direction_type::sender:
            return "sender";
        case operation_direction_type::receiver:
            return "receaver";
        default:
            return "any";
    }
    return "any";
}

    struct operation_visitor final {
        operation_visitor(
            golos::chain::database& db,
            const golos::chain::operation_notification& op_note,
            std::string op_account,
            operation_direction_type dir)
            : database(db),
              note(op_note),
              account(op_account),
              dir(dir){
        }

        using result_type = void;

        golos::chain::database& database;
        const golos::chain::operation_notification& note;
        std::string account;
        operation_direction_type dir;

        template<typename Op>
        void operator()(Op &&) const {
            const auto& idx = database.get_index<account_history_index>().indices().get<by_account>();

            auto itr = idx.lower_bound(std::make_tuple(account, uint32_t(-1)));
            uint32_t sequence = 0;
            if (itr != idx.end() && itr->account == account) {
                sequence = itr->sequence + 1;
            }

            database.create<account_history_object>([&](account_history_object& history) {
                history.block = note.block;
                history.account = account;
                history.sequence = sequence;
                history.op = operation_history::operation_id_type(note.db_id);
                history.dir = dir;
            });
        }
    };

    struct plugin::plugin_impl final {
    public:
        plugin_impl( )
            : database(appbase::app().get_plugin<chain::plugin>().db()) {
        }

        ~plugin_impl() = default;

        void erase_old_blocks() {
            uint32_t head_block = database.head_block_num();
            if (history_blocks <= head_block) {
                uint32_t need_block = head_block - history_blocks;
                const auto& idx = database.get_index<account_history_index>().indices().get<by_location>();
                auto it = idx.begin();
                while (it != idx.end() && it->block <= need_block) {
                    auto next_it = it;
                    ++next_it;
                    database.remove(*it);
                    it = next_it;
                }
            }
        }

        void on_operation(const golos::chain::operation_notification& note) {
            if (!note.stored_in_db) {
                return;
            }

            fc::flat_map<golos::chain::account_name_type, operation_direction_type> impacted;
            operation_get_impacted_accounts(note.op, impacted);

            for (const auto& item : impacted) {
                auto itr = tracked_accounts.lower_bound(item.first);
                if (!tracked_accounts.size() ||
                    (itr != tracked_accounts.end() && itr->first <= item.first && item.first <= itr->second)
                ) {
                    note.op.visit(operation_visitor(database, note, item.first, item.second));
                }
            }
        }

        std::map<uint32_t, applied_operation> get_account_history(
            std::string account,
            uint64_t from,
            uint32_t limit,
            operation_direction_type dir
        ) {
            FC_ASSERT(limit <= 10000, "Limit of ${l} is greater than maxmimum allowed", ("l", limit));
            FC_ASSERT(from >= limit, "From must be greater than limit");
            FC_ASSERT(static_cast<uint8_t>(dir) <= static_cast<uint8_t>(operation_direction_type::any),
                      "Invalid direction ${l} type", ("l", static_cast<uint8_t>(dir)));
            //   idump((account)(from)(limit));
            const auto& idx = database.get_index<account_history_index>().indices().get<by_account>();
            auto itr = idx.lower_bound(std::make_tuple(account, from));
            //   if( itr != idx.end() ) idump((*itr));
            auto end = idx.upper_bound(std::make_tuple(account, std::max(int64_t(0), int64_t(itr->sequence) - limit)));
            //   if( end != idx.end() ) idump((*end));

            std::map<uint32_t, applied_operation> result;
            for (; itr != end; ++itr) {
                if (dir == itr->dir || dir == operation_direction_type::any) {
                    result[itr->sequence] = database.get(itr->op);
                }
            }
            return result;
        }

        fc::flat_map<std::string, std::string> tracked_accounts;
        golos::chain::database& database;
        uint32_t history_blocks = UINT32_MAX;
    };

    DEFINE_API(plugin, get_account_history) {
        auto n_args = args.args->size();
        FC_ASSERT(3 <= n_args && n_args <= 4);
        auto account = args.args->at(0).as<std::string>();
        auto from = args.args->at(1).as<uint64_t>();
        auto limit = args.args->at(2).as<uint32_t>();
        auto dir = operation_direction_type::any;
        if (3 < n_args) {
            dir = static_cast<operation_direction_type>(args.args->at(3).as<uint8_t>());
        }

        return pimpl->database.with_weak_read_lock([&]() {
            return pimpl->get_account_history(account, from, limit, dir);
        });
    }

    struct get_impacted_account_visitor final {
        fc::flat_map<golos::chain::account_name_type, operation_direction_type>& impacted;

        get_impacted_account_visitor(fc::flat_map<golos::chain::account_name_type, operation_direction_type>& impact)
            : impacted(impact) {
        }

        using result_type = void;

        template<typename T>
        void operator()(const T& op) {
            fc::flat_set<golos::chain::account_name_type> impd;
            op.get_required_posting_authorities(impd);
            op.get_required_active_authorities(impd);
            op.get_required_owner_authorities(impd);
            for (auto i : impd) {
                impacted.insert(make_pair(i, operation_direction_type::any));
            }
        }

        void operator()(const account_create_operation& op) {
            impacted.insert(make_pair(op.new_account_name, operation_direction_type::receiver));
            impacted.insert(make_pair(op.creator, operation_direction_type::sender));
        }

        void operator()(const account_create_with_delegation_operation& op) {
            impacted.insert(make_pair(op.new_account_name, operation_direction_type::receiver));
            impacted.insert(make_pair(op.creator, operation_direction_type::sender));
        }

        void operator()(const account_update_operation& op) {
            impacted.insert(make_pair(op.account, operation_direction_type::any));
        }

        void operator()(const account_metadata_operation& op) {
            impacted.insert(make_pair(op.account, operation_direction_type::any));
        }

        void operator()(const comment_operation& op) {
            if (op.author == op.parent_author) {
                impacted.insert(make_pair(op.author, operation_direction_type::any));
            } else {
                if (op.parent_author.size()) {
                    impacted.insert(make_pair(op.author, operation_direction_type::receiver));
                } else {
                    impacted.insert(make_pair(op.author, operation_direction_type::sender));
                }
            }
        }

        void operator()(const delete_comment_operation& op) {
            impacted.insert(make_pair(op.author, operation_direction_type::any));
        }

        void operator()(const vote_operation& op) {
            if (op.voter == op.author) {
                impacted.insert(make_pair(op.voter, operation_direction_type::any));
                impacted.insert(make_pair(op.author, operation_direction_type::any));
            } else {
                impacted.insert(make_pair(op.voter, operation_direction_type::sender));
                impacted.insert(make_pair(op.author, operation_direction_type::receiver));
            }
        }

        void operator()(const author_reward_operation& op) {
            impacted.insert(make_pair(op.author, operation_direction_type::receiver));
        }

        void operator()(const curation_reward_operation& op) {
            impacted.insert(make_pair(op.curator, operation_direction_type::receiver));
        }

        void operator()(const liquidity_reward_operation& op) {
            impacted.insert(make_pair(op.owner, operation_direction_type::any));
        }

        void operator()(const interest_operation& op) {
            impacted.insert(make_pair(op.owner, operation_direction_type::any));
        }

        void operator()(const fill_convert_request_operation& op) {
            impacted.insert(make_pair(op.owner, operation_direction_type::any));
        }

        void operator()(const transfer_operation& op) {
            if (op.from == op.to) {
                impacted.insert(make_pair(op.from, operation_direction_type::any));
                impacted.insert(make_pair(op.to, operation_direction_type::any));
            } else {
                impacted.insert(make_pair(op.from, operation_direction_type::sender));
                impacted.insert(make_pair(op.to, operation_direction_type::receiver));
            }
        }

        void operator()(const transfer_to_vesting_operation& op) {
            if (op.from == op.to) {
                impacted.insert(make_pair(op.from, operation_direction_type::any));
                impacted.insert(make_pair(op.to, operation_direction_type::any));
            } else {
                impacted.insert(make_pair(op.from, operation_direction_type::sender));
                if (op.to != golos::chain::account_name_type() && op.to != op.from) {
                    impacted.insert(make_pair(op.from, operation_direction_type::receiver));
                }
            }
        }

        void operator()(const withdraw_vesting_operation& op) {
            impacted.insert(make_pair(op.account, operation_direction_type::any));
        }

        void operator()(const witness_update_operation& op) {
            impacted.insert(make_pair(op.owner, operation_direction_type::any));
        }

        void operator()(const account_witness_vote_operation& op) {
            impacted.insert(make_pair(op.account, operation_direction_type::sender));
            impacted.insert(make_pair(op.witness, operation_direction_type::receiver));
        }

        void operator()(const account_witness_proxy_operation& op) {
            impacted.insert(make_pair(op.account, operation_direction_type::sender));
            impacted.insert(make_pair(op.proxy, operation_direction_type::receiver));
        }

        void operator()(const feed_publish_operation& op) {
            impacted.insert(make_pair(op.publisher, operation_direction_type::any));
        }

        void operator()(const limit_order_create_operation& op) {
            impacted.insert(make_pair(op.owner, operation_direction_type::any));
        }

        void operator()(const fill_order_operation& op) {
            impacted.insert(make_pair(op.current_owner, operation_direction_type::sender));
            impacted.insert(make_pair(op.open_owner, operation_direction_type::receiver));
        }

        void operator()(const limit_order_cancel_operation& op) {
            impacted.insert(make_pair(op.owner, operation_direction_type::any));
        }

        void operator()(const pow_operation& op) {
            impacted.insert(make_pair(op.worker_account, operation_direction_type::any));
        }

        void operator()(const fill_vesting_withdraw_operation& op) {
            if (op.from_account == op.to_account) {
                impacted.insert(make_pair(op.from_account, operation_direction_type::any));
                impacted.insert(make_pair(op.to_account, operation_direction_type::any));
            } else {
                impacted.insert(make_pair(op.from_account, operation_direction_type::sender));
                impacted.insert(make_pair(op.to_account, operation_direction_type::receiver));
            }
        }

        void operator()(const shutdown_witness_operation& op) {
            impacted.insert(make_pair(op.owner, operation_direction_type::any));
        }

        void operator()(const custom_operation& op) {
            for (auto& s: op.required_auths) {
                impacted.insert(make_pair(s, operation_direction_type::any));
            }
        }

        void operator()(const request_account_recovery_operation& op) {
            impacted.insert(make_pair(op.account_to_recover, operation_direction_type::any));
        }

        void operator()(const recover_account_operation& op) {
            impacted.insert(make_pair(op.account_to_recover, operation_direction_type::any));
        }

        void operator()(const change_recovery_account_operation& op) {
            impacted.insert(make_pair(op.account_to_recover, operation_direction_type::any));
        }

        void operator()(const escrow_transfer_operation& op) {
            impacted.insert(make_pair(op.from, operation_direction_type::sender));
            impacted.insert(make_pair(op.to, operation_direction_type::receiver));
            impacted.insert(make_pair(op.agent, operation_direction_type::receiver));
        }

        void operator()(const escrow_approve_operation& op) {
            impacted.insert(make_pair(op.from, operation_direction_type::sender));
            impacted.insert(make_pair(op.to, operation_direction_type::receiver));
            impacted.insert(make_pair(op.agent, operation_direction_type::receiver));
        }

        void operator()(const escrow_dispute_operation& op) {
            impacted.insert(make_pair(op.from, operation_direction_type::sender));
            impacted.insert(make_pair(op.to, operation_direction_type::receiver));
            impacted.insert(make_pair(op.agent, operation_direction_type::receiver));
        }

        void operator()(const escrow_release_operation& op) {
            impacted.insert(make_pair(op.from, operation_direction_type::sender));
            impacted.insert(make_pair(op.to, operation_direction_type::receiver));
            impacted.insert(make_pair(op.agent, operation_direction_type::receiver));
        }

        void operator()(const transfer_to_savings_operation& op) {
            if (op.from == op.to) {
                impacted.insert(make_pair(op.from, operation_direction_type::any));
                impacted.insert(make_pair(op.to, operation_direction_type::any));
            } else {
                impacted.insert(make_pair(op.from, operation_direction_type::sender));
                impacted.insert(make_pair(op.to, operation_direction_type::receiver));
            }
        }

        void operator()(const transfer_from_savings_operation& op) {
            if (op.from == op.to) {
                impacted.insert(make_pair(op.from, operation_direction_type::any));
                impacted.insert(make_pair(op.to, operation_direction_type::any));
            } else {
                impacted.insert(make_pair(op.from, operation_direction_type::sender));
                impacted.insert(make_pair(op.to, operation_direction_type::receiver));
            }
        }

        void operator()(const cancel_transfer_from_savings_operation& op) {
            impacted.insert(make_pair(op.from, operation_direction_type::any));
        }

        void operator()(const decline_voting_rights_operation& op) {
            impacted.insert(make_pair(op.account, operation_direction_type::any));
        }

        void operator()(const comment_benefactor_reward_operation& op) {
            impacted.insert(make_pair(op.benefactor, operation_direction_type::receiver));
            impacted.insert(make_pair(op.author, operation_direction_type::sender));
        }

        void operator()(const delegate_vesting_shares_operation& op) {
            impacted.insert(make_pair(op.delegator, operation_direction_type::sender));
            impacted.insert(make_pair(op.delegatee, operation_direction_type::receiver));
        }

        void operator()(const return_vesting_delegation_operation& op) {
            impacted.insert(make_pair(op.account, operation_direction_type::receiver));
        }

        void operator()(const proposal_create_operation& op) {
            impacted.insert(make_pair(op.author, operation_direction_type::any));
        }

        void operator()(const proposal_update_operation& op) {
            auto insert_fn = [this](const fc::flat_set<golos::chain::account_name_type>& impd) {
                for (auto i : impd) {
                    impacted.insert(make_pair(i, operation_direction_type::any));
                }
            };
            insert_fn(op.active_approvals_to_add);
            insert_fn(op.owner_approvals_to_add);
            insert_fn(op.posting_approvals_to_add);
            insert_fn(op.active_approvals_to_remove);
            insert_fn(op.owner_approvals_to_remove);
            insert_fn(op.posting_approvals_to_remove);
        }

        void operator()(const proposal_delete_operation& op) {
            impacted.insert(make_pair(op.requester, operation_direction_type::any));
        }
        //void operator()( const operation& op ){}
    };

    void operation_get_impacted_accounts(
        const operation& op, fc::flat_map<golos::chain::account_name_type, operation_direction_type>& result
    ) {
        get_impacted_account_visitor vtor = get_impacted_account_visitor(result);
        op.visit(vtor);
    }

    void plugin::set_program_options(
        boost::program_options::options_description& cli,
        boost::program_options::options_description& cfg
    ) {
        cli.add_options()(
            "track-account-range",
            boost::program_options::value<std::vector<std::string>>()->composing()->multitoken(),
            "Defines a range of accounts to track as a json pair [\"from\",\"to\"] [from,to]. "
            "Can be specified multiple times"
        );
        cfg.add(cli);
    }

    void plugin::plugin_initialize(const boost::program_options::variables_map& options) {
        ilog("account_history plugin: plugin_initialize() begin");
        pimpl = std::make_unique<plugin_impl>();

        if (options.count("history-blocks")) {
            uint32_t history_blocks = options.at("history-blocks").as<uint32_t>();
            pimpl->history_blocks = history_blocks;
            pimpl->database.applied_block.connect([&](const signed_block& block){
                pimpl->erase_old_blocks();
            });
        } else {
            pimpl->history_blocks = UINT32_MAX;
        }
        ilog("account_history: history-blocks ${s}", ("s", pimpl->history_blocks));

        // this is worked, because the appbase initialize required plugins at first
        pimpl->database.pre_apply_operation.connect([&](golos::chain::operation_notification& note){
            pimpl->on_operation(note);
        });

        golos::chain::add_plugin_index<account_history_index>(pimpl->database);

        using pairstring = std::pair<std::string, std::string>;
        LOAD_VALUE_SET(options, "track-account-range", pimpl->tracked_accounts, pairstring);

        ilog("account_history: tracked_accounts ${s}", ("s", pimpl->tracked_accounts));

        JSON_RPC_REGISTER_API(name());
        ilog("account_history plugin: plugin_initialize() end");
    }

    plugin::plugin() = default;

    plugin::~plugin() = default;

    const std::string& plugin::name() {
        static std::string name = "account_history";
        return name;
    }

    void plugin::plugin_startup() {
        ilog("account_history plugin: plugin_startup() begin");
        ilog("account_history plugin: plugin_startup() end");
    }

    void plugin::plugin_shutdown() {
    }

    fc::flat_map<std::string, std::string> plugin::tracked_accounts() const {
        return pimpl->tracked_accounts;
    }

} } } // golos::plugins::account_history
