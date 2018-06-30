#include <golos/plugins/database_api/plugin.hpp>

#include <golos/plugins/follow/plugin.hpp>

#include <golos/protocol/get_config.hpp>

#include <fc/bloom_filter.hpp>
#include <fc/smart_ref_impl.hpp>

#include <boost/range/iterator_range.hpp>
#include <boost/algorithm/string.hpp>
#include <memory>
#include <golos/plugins/json_rpc/plugin.hpp>

#define GET_REQUIRED_FEES_MAX_RECURSION 4

#define CHECK_ARG_SIZE(s) \
   FC_ASSERT( args.args->size() == s, "Expected #s argument(s), was ${n}", ("n", args.args->size()) );
#define CHECK_ARGS_COUNT(min, max) \
   FC_ASSERT(n_args >= min && n_args <= max, "Expected #min-#max arguments, got ${n}", ("n", n_args));


namespace golos { namespace plugins { namespace database_api {

struct block_applied_callback_info {
    using ptr = std::shared_ptr<block_applied_callback_info>;
    using cont = std::list<ptr>;

    block_applied_callback callback;
    boost::signals2::connection connection;
    cont::iterator it;

    void connect(
        boost::signals2::signal<void(const signed_block &)> &sig,
        cont &free_cont,
        block_applied_callback cb
    ) {
        callback = cb;

        connection = sig.connect([this, &free_cont](const signed_block &block) {
            try {
                this->callback(fc::variant(block));
            } catch (...) {
                free_cont.push_back(*this->it);
                this->connection.disconnect();
            }
        });
    }
};


struct plugin::api_impl final {
public:
    api_impl();
    ~api_impl();

    void startup() {
    }

    // Subscriptions
    void set_subscribe_callback(std::function<void(const variant &)> cb, bool clear_filter);
    void set_pending_transaction_callback(std::function<void(const variant &)> cb);
    void set_block_applied_callback(block_applied_callback cb);
    void clear_block_applied_callback();
    void cancel_all_subscriptions();

    // Blocks and transactions
    optional<block_header> get_block_header(uint32_t block_num) const;
    optional<signed_block> get_block(uint32_t block_num) const;

    // Globals
    fc::variant_object get_config() const;
    dynamic_global_property_api_object get_dynamic_global_properties() const;

    // Accounts
    std::vector<account_api_object> get_accounts(std::vector<std::string> names) const;
    std::vector<optional<account_api_object>> lookup_account_names(const std::vector<std::string> &account_names) const;
    std::set<std::string> lookup_accounts(const std::string &lower_bound_name, uint32_t limit) const;
    uint64_t get_account_count() const;

    // Authority / validation
    std::string get_transaction_hex(const signed_transaction &trx) const;
    std::set<public_key_type> get_required_signatures(const signed_transaction &trx, const flat_set<public_key_type> &available_keys) const;
    std::set<public_key_type> get_potential_signatures(const signed_transaction &trx) const;
    bool verify_authority(const signed_transaction &trx) const;
    bool verify_account_authority(const std::string &name_or_id, const flat_set<public_key_type> &signers) const;

    std::vector<withdraw_route> get_withdraw_routes(std::string account, withdraw_route_type type) const;
    std::vector<proposal_api_object> get_proposed_transactions(const std::string&, uint32_t, uint32_t) const;

    template<typename T>
    void subscribe_to_item(const T &i) const {
        auto vec = fc::raw::pack(i);
        if (!_subscribe_callback) {
            return;
        }

        if (!is_subscribed_to_item(i)) {
            idump((i));
            _subscribe_filter.insert(vec.data(), vec.size());
        }
    }

    template<typename T>
    bool is_subscribed_to_item(const T &i) const {
        if (!_subscribe_callback) {
            return false;
        }

        return _subscribe_filter.contains(i);
    }

    mutable fc::bloom_filter _subscribe_filter;
    std::function<void(const fc::variant &)> _subscribe_callback;
    std::function<void(const fc::variant &)> _pending_trx_callback;


    golos::chain::database &database() const {
        return _db;
    }


    std::map<std::pair<asset_symbol_type, asset_symbol_type>, std::function<void(const variant &)>> _market_subscriptions;

    block_applied_callback_info::cont active_block_applied_callback;
    block_applied_callback_info::cont free_block_applied_callback;

private:

    golos::chain::database &_db;
};


//void find_accounts(std::set<std::string> &accounts, const discussion &d) {
//    accounts.insert(d.author);
//}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Subscriptions                                                    //
//                                                                  //
//////////////////////////////////////////////////////////////////////

void plugin::set_subscribe_callback(std::function<void(const variant &)> cb, bool clear_filter) {
    my->database().with_weak_read_lock([&]() {
        my->set_subscribe_callback(cb, clear_filter);
    });
}

void plugin::api_impl::set_subscribe_callback(
    std::function<void(const variant &)> cb,
    bool clear_filter
) {
    _subscribe_callback = cb;
    if (clear_filter || !cb) {
        static fc::bloom_parameters param;
        param.projected_element_count = 10000;
        param.false_positive_probability = 1.0 / 10000;
        param.maximum_size = 1024 * 8 * 8 * 2;
        param.compute_optimal_parameters();
        _subscribe_filter = fc::bloom_filter(param);
    }
}

void plugin::set_pending_transaction_callback(std::function<void(const variant &)> cb) {
    my->database().with_weak_read_lock([&]() {
        my->set_pending_transaction_callback(cb);
    });
}

void plugin::api_impl::set_pending_transaction_callback(std::function<void(const variant &)> cb) {
    _pending_trx_callback = cb;
}

void plugin::cancel_all_subscriptions() {
    my->database().with_weak_read_lock([&]() {
        my->cancel_all_subscriptions();
    });
}

void plugin::api_impl::cancel_all_subscriptions() {
    set_subscribe_callback(std::function<void(const fc::variant &)>(), true);
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Constructors                                                     //
//                                                                  //
//////////////////////////////////////////////////////////////////////

plugin::plugin()  {
}

plugin::~plugin() {
}

plugin::api_impl::api_impl() : _db(appbase::app().get_plugin<chain::plugin>().db())
{
    wlog("creating database plugin ${x}", ("x", int64_t(this)));
}

plugin::api_impl::~api_impl() {
    elog("freeing database plugin ${x}", ("x", int64_t(this)));
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Blocks and transactions                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////

DEFINE_API(plugin, get_block_header) {
    CHECK_ARG_SIZE(1)
    return my->database().with_weak_read_lock([&]() {
        return my->get_block_header(args.args->at(0).as<uint32_t>());
    });
}

optional<block_header> plugin::api_impl::get_block_header(uint32_t block_num) const {
    auto result = database().fetch_block_by_number(block_num);
    if (result) {
        return *result;
    }
    return {};
}

DEFINE_API(plugin, get_block) {
    CHECK_ARG_SIZE(1)
    return my->database().with_weak_read_lock([&]() {
        return my->get_block(args.args->at(0).as<uint32_t>());
    });
}

optional<signed_block> plugin::api_impl::get_block(uint32_t block_num) const {
    return database().fetch_block_by_number(block_num);
}

DEFINE_API(plugin, set_block_applied_callback) {
    CHECK_ARG_SIZE(1)

    // Delegate connection handlers to callback
    msg_pack_transfer transfer(args);

    my->database().with_weak_read_lock([&]{
        my->set_block_applied_callback([msg = transfer.msg()](const fc::variant & block_header) {
            msg->unsafe_result(fc::variant(block_header));
        });
    });

    transfer.complete();

    return {};
}

void plugin::api_impl::set_block_applied_callback(std::function<void(const variant &block_header)> callback) {
    auto info_ptr = std::make_shared<block_applied_callback_info>();

    active_block_applied_callback.push_back(info_ptr);
    info_ptr->it = std::prev(active_block_applied_callback.end());

    info_ptr->connect(database().applied_block, free_block_applied_callback, callback);
}

void plugin::api_impl::clear_block_applied_callback() {
    for (auto &info: free_block_applied_callback) {
        active_block_applied_callback.erase(info->it);
    }
    free_block_applied_callback.clear();
}

void plugin::clear_block_applied_callback() {
    my->clear_block_applied_callback();
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Globals                                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////

DEFINE_API(plugin, get_config) {
    return my->database().with_weak_read_lock([&]() {
        return my->get_config();
    });
}

fc::variant_object plugin::api_impl::get_config() const {
    return golos::protocol::get_config();
}

DEFINE_API(plugin, get_dynamic_global_properties) {
    return my->database().with_weak_read_lock([&]() {
        return my->get_dynamic_global_properties();
    });
}

DEFINE_API(plugin, get_chain_properties) {
    return my->database().with_weak_read_lock([&]() {
        return chain_api_properties(my->database().get_witness_schedule_object().median_props, my->database());
    });
}

dynamic_global_property_api_object plugin::api_impl::get_dynamic_global_properties() const {
    return database().get(dynamic_global_property_object::id_type());
}

DEFINE_API(plugin, get_hardfork_version) {
    return my->database().with_weak_read_lock([&]() {
        return my->database().get(hardfork_property_object::id_type()).current_hardfork_version;
    });
}

DEFINE_API(plugin, get_next_scheduled_hardfork) {
    return my->database().with_weak_read_lock([&]() {
        scheduled_hardfork shf;
        const auto &hpo = my->database().get(hardfork_property_object::id_type());
        shf.hf_version = hpo.next_hardfork;
        shf.live_time = hpo.next_hardfork_time;
        return shf;
    });
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Accounts                                                         //
//                                                                  //
//////////////////////////////////////////////////////////////////////

DEFINE_API(plugin, get_accounts) {
    CHECK_ARG_SIZE(1)
    return my->database().with_weak_read_lock([&]() {
        return my->get_accounts(args.args->at(0).as<vector<std::string> >());
    });
}

std::vector<account_api_object> plugin::api_impl::get_accounts(std::vector<std::string> names) const {
    const auto &idx = _db.get_index<account_index>().indices().get<by_name>();
    const auto &vidx = _db.get_index<witness_vote_index>().indices().get<by_account_witness>();
    std::vector<account_api_object> results;

    for (auto name: names) {
        auto itr = idx.find(name);
        if (itr != idx.end()) {
            results.push_back(account_api_object(*itr, _db));
            follow::fill_account_reputation(_db, itr->name, results.back().reputation);
            auto vitr = vidx.lower_bound(boost::make_tuple(itr->id, witness_id_type()));
            while (vitr != vidx.end() && vitr->account == itr->id) {
                results.back().witness_votes.insert(_db.get(vitr->witness).owner);
                ++vitr;
            }
        }
    }

    return results;
}

DEFINE_API(plugin, lookup_account_names) {
    CHECK_ARG_SIZE(1)
    return my->database().with_weak_read_lock([&]() {
        return my->lookup_account_names(args.args->at(0).as<vector<std::string> >());
    });
}

std::vector<optional<account_api_object>> plugin::api_impl::lookup_account_names(
    const std::vector<std::string> &account_names
) const {
    std::vector<optional<account_api_object>> result;
    result.reserve(account_names.size());

    for (auto &name : account_names) {
        auto itr = database().find<account_object, by_name>(name);

        if (itr) {
            result.push_back(account_api_object(*itr, database()));
        } else {
            result.push_back(optional<account_api_object>());
        }
    }

    return result;
}

DEFINE_API(plugin, lookup_accounts) {
    CHECK_ARG_SIZE(2)
    account_name_type lower_bound_name = args.args->at(0).as<account_name_type>();
    uint32_t limit = args.args->at(1).as<uint32_t>();
    return my->database().with_weak_read_lock([&]() {
        return my->lookup_accounts(lower_bound_name, limit);
    });
}

std::set<std::string> plugin::api_impl::lookup_accounts(
    const std::string &lower_bound_name,
        uint32_t limit
) const {
    FC_ASSERT(limit <= 1000);
    const auto &accounts_by_name = database().get_index<account_index>().indices().get<by_name>();
    std::set<std::string> result;

    for (auto itr = accounts_by_name.lower_bound(lower_bound_name);
            limit-- && itr != accounts_by_name.end(); ++itr) {
        result.insert(itr->name);
    }

    return result;
}

DEFINE_API(plugin, get_account_count) {
    return my->database().with_weak_read_lock([&]() {
        return my->get_account_count();
    });
}

uint64_t plugin::api_impl::get_account_count() const {
    return database().get_index<account_index>().indices().size();
}

DEFINE_API(plugin, get_owner_history) {
    CHECK_ARG_SIZE(1)
    auto account = args.args->at(0).as<string>();
    return my->database().with_weak_read_lock([&]() {
        std::vector<owner_authority_history_api_object> results;
        const auto &hist_idx = my->database().get_index<owner_authority_history_index>().indices().get<
                by_account>();
        auto itr = hist_idx.lower_bound(account);

        while (itr != hist_idx.end() && itr->account == account) {
            results.push_back(owner_authority_history_api_object(*itr));
            ++itr;
        }

        return results;
    });
}

DEFINE_API(plugin, get_recovery_request) {
    CHECK_ARG_SIZE(1)
    auto account = args.args->at(0).as<account_name_type>();
    return my->database().with_weak_read_lock([&]() {
        optional<account_recovery_request_api_object> result;

        const auto &rec_idx = my->database().get_index<account_recovery_request_index>().indices().get<
                by_account>();
        auto req = rec_idx.find(account);

        if (req != rec_idx.end()) {
            result = account_recovery_request_api_object(*req);
        }

        return result;
    });
}

DEFINE_API(plugin, get_escrow) {
    CHECK_ARG_SIZE(2)
    auto from = args.args->at(0).as<account_name_type>();
    auto escrow_id = args.args->at(1).as<uint32_t>();
    return my->database().with_weak_read_lock([&]() {
        optional<escrow_api_object> result;

        try {
            result = my->database().get_escrow(from, escrow_id);
        } catch (...) {
        }

        return result;
    });
}

std::vector<withdraw_route> plugin::api_impl::get_withdraw_routes(
    std::string account,
    withdraw_route_type type
) const {
    std::vector<withdraw_route> result;

    const auto &acc = database().get_account(account);

    if (type == outgoing || type == all) {
        const auto &by_route = database().get_index<withdraw_vesting_route_index>().indices().get<
                by_withdraw_route>();
        auto route = by_route.lower_bound(acc.id);

        while (route != by_route.end() && route->from_account == acc.id) {
            withdraw_route r;
            r.from_account = account;
            r.to_account = database().get(route->to_account).name;
            r.percent = route->percent;
            r.auto_vest = route->auto_vest;

            result.push_back(r);

            ++route;
        }
    }

    if (type == incoming || type == all) {
        const auto &by_dest = database().get_index<withdraw_vesting_route_index>().indices().get<by_destination>();
        auto route = by_dest.lower_bound(acc.id);

        while (route != by_dest.end() && route->to_account == acc.id) {
            withdraw_route r;
            r.from_account = database().get(route->from_account).name;
            r.to_account = account;
            r.percent = route->percent;
            r.auto_vest = route->auto_vest;

            result.push_back(r);

            ++route;
        }
    }

    return result;
}


DEFINE_API(plugin, get_withdraw_routes) {
    FC_ASSERT(args.args->size() == 1 || args.args->size() == 2, "Expected 1-2 arguments, was ${n}",
                ("n", args.args->size()));
    auto account = args.args->at(0).as<string>();
    auto type = args.args->at(1).as<withdraw_route_type>();
    return my->database().with_weak_read_lock([&]() {
        return my->get_withdraw_routes(account, type);
    });
}

DEFINE_API(plugin, get_account_bandwidth) {
    CHECK_ARG_SIZE(2)
    auto account = args.args->at(0).as<string>();
    auto type = args.args->at(1).as<bandwidth_type>();
    optional<account_bandwidth_api_object> result;
    auto band = my->database().find<account_bandwidth_object, by_account_bandwidth_type>(
            boost::make_tuple(account, type));
    if (band != nullptr) {
        result = *band;
    }

    return result;
}


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Authority / validation                                           //
//                                                                  //
//////////////////////////////////////////////////////////////////////

DEFINE_API(plugin, get_transaction_hex) {
    CHECK_ARG_SIZE(1)
    auto trx = args.args->at(0).as<signed_transaction>();
    return my->database().with_weak_read_lock([&]() {
        return my->get_transaction_hex(trx);
    });
}

std::string plugin::api_impl::get_transaction_hex(const signed_transaction &trx) const {
    return fc::to_hex(fc::raw::pack(trx));
}

DEFINE_API(plugin, get_required_signatures) {
    CHECK_ARG_SIZE(2)
    auto trx = args.args->at(0).as<signed_transaction>();
    auto available_keys = args.args->at(1).as<flat_set<public_key_type>>();
    return my->database().with_weak_read_lock([&]() {
        return my->get_required_signatures(trx, available_keys);
    });
}

std::set<public_key_type> plugin::api_impl::get_required_signatures(
    const signed_transaction &trx,
    const flat_set<public_key_type> &available_keys
) const {
    //   wdump((trx)(available_keys));
    auto result = trx.get_required_signatures(
        STEEMIT_CHAIN_ID, available_keys,
        [&](std::string account_name) {
            return authority(database().get<account_authority_object, by_account>(account_name).active);
        },
        [&](std::string account_name) {
            return authority(database().get<account_authority_object, by_account>(account_name).owner);
        },
        [&](std::string account_name) {
            return authority(database().get<account_authority_object, by_account>(account_name).posting);
        },
        STEEMIT_MAX_SIG_CHECK_DEPTH
    );
    //   wdump((result));
    return result;
}

DEFINE_API(plugin, get_potential_signatures) {
    CHECK_ARG_SIZE(1)
    return my->database().with_weak_read_lock([&]() {
        return my->get_potential_signatures(args.args->at(0).as<signed_transaction>());
    });
}

std::set<public_key_type> plugin::api_impl::get_potential_signatures(const signed_transaction &trx) const {
    //   wdump((trx));
    std::set<public_key_type> result;
    trx.get_required_signatures(STEEMIT_CHAIN_ID, flat_set<public_key_type>(),
        [&](account_name_type account_name) {
            const auto &auth = database().get<account_authority_object, by_account>(account_name).active;
            for (const auto &k : auth.get_keys()) {
                result.insert(k);
            }
            return authority(auth);
        },
        [&](account_name_type account_name) {
            const auto &auth = database().get<account_authority_object, by_account>(account_name).owner;
            for (const auto &k : auth.get_keys()) {
                result.insert(k);
            }
            return authority(auth);
        },
        [&](account_name_type account_name) {
            const auto &auth = database().get<account_authority_object, by_account>(account_name).posting;
            for (const auto &k : auth.get_keys()) {
                result.insert(k);
            }
            return authority(auth);
        },
        STEEMIT_MAX_SIG_CHECK_DEPTH
    );

    //   wdump((result));
    return result;
}

DEFINE_API(plugin, verify_authority) {
    CHECK_ARG_SIZE(1)
    return my->database().with_weak_read_lock([&]() {
        return my->verify_authority(args.args->at(0).as<signed_transaction>());
    });
}

bool plugin::api_impl::verify_authority(const signed_transaction &trx) const {
    trx.verify_authority(STEEMIT_CHAIN_ID, [&](std::string account_name) {
        return authority(database().get<account_authority_object, by_account>(account_name).active);
    }, [&](std::string account_name) {
        return authority(database().get<account_authority_object, by_account>(account_name).owner);
    }, [&](std::string account_name) {
        return authority(database().get<account_authority_object, by_account>(account_name).posting);
    }, STEEMIT_MAX_SIG_CHECK_DEPTH);
    return true;
}

DEFINE_API(plugin, verify_account_authority) {
    CHECK_ARG_SIZE(2)
    return my->database().with_weak_read_lock([&]() {
        return my->verify_account_authority(args.args->at(0).as<account_name_type>(),
                                            args.args->at(1).as<flat_set<public_key_type> >());
    });
}

bool plugin::api_impl::verify_account_authority(
    const std::string &name,
    const flat_set<public_key_type> &keys
) const {
    FC_ASSERT(name.size() > 0);
    auto account = database().find<account_object, by_name>(name);
    FC_ASSERT(account, "no such account");

    /// reuse trx.verify_authority by creating a dummy transfer
    signed_transaction trx;
    transfer_operation op;
    op.from = account->name;
    trx.operations.emplace_back(op);

    return verify_authority(trx);
}

DEFINE_API(plugin, get_conversion_requests) {
    CHECK_ARG_SIZE(1)
    auto account = args.args->at(0).as<std::string>();
    return my->database().with_weak_read_lock([&]() {
        const auto &idx = my->database().get_index<convert_request_index>().indices().get<by_owner>();
        std::vector<convert_request_api_object> result;
        auto itr = idx.lower_bound(account);
        while (itr != idx.end() && itr->owner == account) {
            result.emplace_back(*itr);
            ++itr;
        }
        return result;
    });
}


DEFINE_API(plugin, get_savings_withdraw_from) {
    CHECK_ARG_SIZE(1)
    auto account = args.args->at(0).as<string>();
    return my->database().with_weak_read_lock([&]() {
        std::vector<savings_withdraw_api_object> result;

        const auto &from_rid_idx = my->database().get_index<savings_withdraw_index>().indices().get<by_from_rid>();
        auto itr = from_rid_idx.lower_bound(account);
        while (itr != from_rid_idx.end() && itr->from == account) {
            result.push_back(savings_withdraw_api_object(*itr));
            ++itr;
        }
        return result;
    });
}

DEFINE_API(plugin, get_savings_withdraw_to) {
    CHECK_ARG_SIZE(1)
    auto account = args.args->at(0).as<string>();
    return my->database().with_weak_read_lock([&]() {
        std::vector<savings_withdraw_api_object> result;

        const auto &to_complete_idx = my->database().get_index<savings_withdraw_index>().indices().get<by_to_complete>();
        auto itr = to_complete_idx.lower_bound(account);
        while (itr != to_complete_idx.end() && itr->to == account) {
            result.push_back(savings_withdraw_api_object(*itr));
            ++itr;
        }
        return result;
    });
}

//vector<vesting_delegation_api_obj> get_vesting_delegations(string account, string from, uint32_t limit, delegations_type type = delegated) const;
DEFINE_API(plugin, get_vesting_delegations) {
    size_t n_args = args.args->size();
    CHECK_ARGS_COUNT(2, 4);
    auto account = args.args->at(0).as<string>();
    auto from = args.args->at(1).as<string>();
    auto limit = n_args >= 3 ? args.args->at(2).as<uint32_t>() : 100;
    auto type = n_args >= 4 ? args.args->at(3).as<delegations_type>() : delegated;
    bool sent = type == delegated;
    FC_ASSERT(limit <= 1000);

    vector<vesting_delegation_api_object> result;
    result.reserve(limit);
    return my->database().with_weak_read_lock([&]() {
        auto fill_result = [&](const auto& idx) {
            auto i = idx.lower_bound(std::make_tuple(account, from));
            while (result.size() < limit && i != idx.end() && account == (sent ? i->delegator : i->delegatee)) {
                result.push_back(*i);
                ++i;
            }
        };
        if (sent)
            fill_result(my->database().get_index<vesting_delegation_index, by_delegation>());
        else
            fill_result(my->database().get_index<vesting_delegation_index, by_received>());
        return result;
    });
}

//vector<vesting_delegation_expiration_api_obj> get_expiring_vesting_delegations(string account, time_point_sec from, uint32_t limit = 100) const;
DEFINE_API(plugin, get_expiring_vesting_delegations) {
    size_t n_args = args.args->size();
    CHECK_ARGS_COUNT(2, 3);
    auto account = args.args->at(0).as<string>();
    auto from = args.args->at(1).as<time_point_sec>();
    uint32_t limit = n_args >= 3 ? args.args->at(2).as<uint32_t>() : 100;
    FC_ASSERT(limit <= 1000);

    return my->database().with_weak_read_lock([&]() {
        vector<vesting_delegation_expiration_api_object> result;
        result.reserve(limit);
        const auto& idx = my->database().get_index<vesting_delegation_expiration_index, by_account_expiration>();
        auto itr = idx.lower_bound(std::make_tuple(account, from));
        while (result.size() < limit && itr != idx.end() && itr->delegator == account) {
            result.push_back(*itr);
            ++itr;
        }
        return result;
    });
}

DEFINE_API(plugin, get_database_info) {
    CHECK_ARG_SIZE(0);

    // read lock doesn't seem needed...

    database_info info;
    auto& db = my->database();

    info.free_size = db.free_memory();
    info.total_size = db.max_memory();
    info.reserved_size = db.reserved_memory();
    info.used_size = info.total_size - info.free_size - info.reserved_size;

    info.index_list.reserve(db.index_list_size());

    for (auto it = db.index_list_begin(), et = db.index_list_end(); et != it; ++it) {
        info.index_list.push_back({(*it)->name(), (*it)->size()});
    }

    return info;
}

std::vector<proposal_api_object> plugin::api_impl::get_proposed_transactions(
    const std::string& a, uint32_t from, uint32_t limit
) const {
    std::vector<proposal_api_object> result;
    std::set<proposal_object_id_type> id_set;
    result.reserve(limit);

    // list of published proposals
    {
        auto& idx = database().get_index<proposal_index>().indices().get<by_account>();
        auto itr = idx.lower_bound(a);

        for (; idx.end() != itr && itr->author == a && result.size() < limit; ++itr) {
            id_set.insert(itr->id);
            if (id_set.size() >= from) {
                result.emplace_back(*itr);
            }
        }
    }

    // list of requested proposals
    if (result.size() < limit) {
        auto& idx = database().get_index<required_approval_index>().indices().get<by_account>();
        auto& pidx = database().get_index<proposal_index>().indices().get<by_id>();
        auto itr = idx.lower_bound(a);

        for (; idx.end() != itr && itr->account == a && result.size() < limit; ++itr) {
            if (!id_set.count(itr->proposal)) {
                id_set.insert(itr->proposal);
                auto pitr = pidx.find(itr->proposal);
                if (pidx.end() != pitr && id_set.size() >= from) {
                    result.emplace_back(*pitr);
                }
            }
        }
    }

    return result;
}

DEFINE_API(plugin, get_proposed_transactions) {
    CHECK_ARG_SIZE(3);
    auto account = args.args->at(0).as<string>();
    auto from = args.args->at(1).as<uint32_t>();
    auto limit = args.args->at(2).as<uint32_t>();
    FC_ASSERT(limit <= 100);

    return my->database().with_weak_read_lock([&]() {
        return my->get_proposed_transactions(account, from, limit);
    });
}

void plugin::plugin_initialize(const boost::program_options::variables_map &options) {
    ilog("database_api plugin: plugin_initialize() begin");
    my = std::make_unique<api_impl>();
    JSON_RPC_REGISTER_API(plugin_name)
    my->database().applied_block.connect([this](const protocol::signed_block &) {
        this->clear_block_applied_callback();
    });
    ilog("database_api plugin: plugin_initialize() end");
}

void plugin::plugin_startup() {
    my->startup();
}

} } } // golos::plugins::database_api
