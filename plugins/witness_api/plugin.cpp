#include <golos/plugins/witness_api/plugin.hpp>
#include <golos/chain/operation_notification.hpp>

#include <fc/smart_ref_impl.hpp>
#include <boost/algorithm/string.hpp>

#define CHECK_ARG_SIZE(s) \
   FC_ASSERT( args.args->size() == s, "Expected #s argument(s), was ${n}", ("n", args.args->size()) );

namespace golos { namespace plugins { namespace witness_api {

using namespace golos::protocol;
using namespace golos::chain;

struct plugin::witness_plugin_impl {
public:
    witness_plugin_impl() : database_(appbase::app().get_plugin<chain::plugin>().db()) {
    }

    ~witness_plugin_impl() = default;

    golos::chain::database &database() const {
        return database_;
    }

    std::vector<account_name_type> get_miner_queue() const;
    std::vector<optional<witness_api_object>> get_witnesses(const std::vector<witness_object::id_type> &witness_ids) const;
    fc::optional<witness_api_object> get_witness_by_account(std::string account_name) const;
    std::vector<witness_api_object> get_witnesses_by_vote(std::string from, uint32_t limit) const;
    uint64_t get_witness_count() const;
    std::set<account_name_type> lookup_witness_accounts(const std::string &lower_bound_name, uint32_t limit) const;

    golos::chain::database &database_;
};


DEFINE_API(plugin, get_current_median_history_price) {
    return my->database().with_weak_read_lock([&]() {
        return my->database().get_feed_history().current_median_history;
    });
}

DEFINE_API(plugin, get_feed_history) {
    return my->database().with_weak_read_lock([&]() {
        return feed_history_api_object(my->database().get_feed_history());
    });
}

std::vector<account_name_type> plugin::witness_plugin_impl::get_miner_queue() const {
    std::vector<account_name_type> result;
    const auto &pow_idx = database().get_index<witness_index>().indices().get<by_pow>();

    auto itr = pow_idx.upper_bound(0);
    while (itr != pow_idx.end()) {
        if (itr->pow_worker) {
            result.push_back(itr->owner);
        }
        ++itr;
    }
    return result;
}

DEFINE_API(plugin, get_miner_queue) {
    return my->database().with_weak_read_lock([&]() {
        return my->get_miner_queue();
    });
}


DEFINE_API(plugin, get_active_witnesses) {
    return my->database().with_weak_read_lock([&]() {
        const auto &wso = my->database().get_witness_schedule_object();
        size_t n = wso.current_shuffled_witnesses.size();
        vector<account_name_type> result;
        result.reserve(n);
        for (size_t i = 0; i < n; i++) {
            if (wso.current_shuffled_witnesses[i] != "") {
                result.push_back(wso.current_shuffled_witnesses[i]);
            }
        }
        return result;
    });
}

DEFINE_API(plugin, get_witness_schedule) {
    return my->database().with_weak_read_lock([&]() {
        return my->database().get(witness_schedule_object::id_type());
    });
}

std::vector<optional<witness_api_object>> plugin::witness_plugin_impl::get_witnesses(
        const std::vector<witness_object::id_type> &witness_ids) const {
    std::vector<optional<witness_api_object>> result;
    result.reserve(witness_ids.size());
    std::transform(witness_ids.begin(), witness_ids.end(), std::back_inserter(result),
                   [this](witness_object::id_type id) -> optional<witness_api_object> {
                       if (auto o = database().find(id)) {
                           return *o;
                       }
                       return {};
                   });
    return result;
}

DEFINE_API(plugin, get_witnesses) {
    CHECK_ARG_SIZE(1)
    auto witness_ids = args.args->at(0).as<vector<witness_object::id_type> >();
    return my->database().with_weak_read_lock([&]() {
        return my->get_witnesses(witness_ids);
    });
}

DEFINE_API(plugin, get_witness_by_account) {
    CHECK_ARG_SIZE(1)
    auto account_name = args.args->at(0).as<std::string>();
    return my->database().with_weak_read_lock([&]() {
        return my->get_witness_by_account(account_name);
    });
}


fc::optional<witness_api_object> plugin::witness_plugin_impl::get_witness_by_account(std::string account_name) const {
    const auto &idx = database().get_index<witness_index>().indices().get<by_name>();
    auto itr = idx.find(account_name);
    if (itr != idx.end()) {
        return witness_api_object(*itr);
    }
    return {};
}

DEFINE_API(plugin, get_witnesses_by_vote) {
    CHECK_ARG_SIZE(2)
    auto from = args.args->at(0).as<std::string>();
    auto limit = args.args->at(1).as<uint32_t>();
    return my->database().with_weak_read_lock([&]() {
        return my->get_witnesses_by_vote(from, limit);
    });
}

std::vector<witness_api_object> plugin::witness_plugin_impl::get_witnesses_by_vote(
        std::string from, uint32_t limit) const {
    FC_ASSERT(limit <= 100);

    std::vector<witness_api_object> result;
    result.reserve(limit);

    const auto &name_idx = database().get_index<witness_index>().indices().get<by_name>();
    const auto &vote_idx = database().get_index<witness_index>().indices().get<by_vote_name>();

    auto itr = vote_idx.begin();
    if (from.size()) {
        auto nameitr = name_idx.find(from);
        FC_ASSERT(nameitr != name_idx.end(), "invalid witness name ${n}", ("n", from));
        itr = vote_idx.iterator_to(*nameitr);
    }

    while (itr != vote_idx.end() && result.size() < limit && itr->votes > 0) {
        result.push_back(witness_api_object(*itr));
        ++itr;
    }
    return result;
}

DEFINE_API(plugin, get_witness_count) {
    return my->database().with_weak_read_lock([&]() {
        return my->get_witness_count();
    });
}

uint64_t plugin::witness_plugin_impl::get_witness_count() const {
    return database().get_index<witness_index>().indices().size();
}

DEFINE_API(plugin, lookup_witness_accounts) {
    CHECK_ARG_SIZE(2)
    auto lower_bound_name = args.args->at(0).as<std::string>();
    auto limit = args.args->at(1).as<uint32_t>();
    return my->database().with_weak_read_lock([&]() {
        return my->lookup_witness_accounts(lower_bound_name, limit);
    });
}

std::set<account_name_type> plugin::witness_plugin_impl::lookup_witness_accounts(
        const std::string &lower_bound_name,
        uint32_t limit) const {
    FC_ASSERT(limit <= 1000);
    const auto &witnesses_by_id = database().get_index<witness_index>().indices().get<by_id>();

    // get all the names and look them all up, sort them, then figure out what
    // records to return.  This could be optimized, but we expect the
    // number of witnesses to be few and the frequency of calls to be rare
    std::set<account_name_type> witnesses_by_account_name;
    for (const witness_api_object &witness : witnesses_by_id) {
        if (witness.owner >= lower_bound_name) { // we can ignore anything below lower_bound_name
            witnesses_by_account_name.insert(witness.owner);
        }
    }

    auto end_iter = witnesses_by_account_name.begin();
    while (end_iter != witnesses_by_account_name.end() && limit--) {
        ++end_iter;
    }
    witnesses_by_account_name.erase(end_iter, witnesses_by_account_name.end());
    return witnesses_by_account_name;
}

void plugin::set_program_options(
        boost::program_options::options_description &cli,
        boost::program_options::options_description &cfg) {
}

void plugin::plugin_initialize(const boost::program_options::variables_map &options) {
    ilog("witness_api plugin: plugin_initialize() begin");

    my.reset(new witness_plugin_impl);

    ilog("witness_api plugin: plugin_initialize() end");
}

plugin::plugin() {

}

plugin::~plugin() {

}

void plugin::plugin_startup() {
    ilog("account_history plugin: plugin_startup() begin");

    ilog("account_history plugin: plugin_startup() end");
}


} } } // golos::plugins::witness_api
