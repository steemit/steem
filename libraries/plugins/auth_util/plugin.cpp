#include <golos/plugins/auth_util/plugin.hpp>
#include <golos/chain/database.hpp>
#include <golos/chain/account_object.hpp>
#include <golos/protocol/types.hpp>
#include <golos/plugins/json_rpc/utility.hpp>
#include <golos/plugins/json_rpc/plugin.hpp>

namespace golos {
namespace plugins {
namespace auth_util {

struct plugin::plugin_impl {
public:
    plugin_impl() : db_(appbase::app().get_plugin<plugins::chain::plugin>().db()) {
    }
     // API
    check_authority_signature_r check_authority_signature(const check_authority_signature_a & args);

    // HELPING METHODS
    golos::chain::database &database() {
        return db_;
    }
private:
    golos::chain::database & db_;
};

check_authority_signature_r plugin::plugin_impl::check_authority_signature(const check_authority_signature_a & args) {
    auto & db = database();
    check_authority_signature_r result;

    const golos::chain::account_authority_object &acct = db.get<golos::chain::account_authority_object, golos::chain::by_account>(args.account_name);
    protocol::authority auth;
    if ((args.level == "posting") || (args.level == "p")) {
        auth = protocol::authority(acct.posting);
    } else if ((args.level == "active") ||
               (args.level == "a") || (args.level == "")) {
        auth = protocol::authority(acct.active);
    } else if ((args.level == "owner") || (args.level == "o")) {
        auth = protocol::authority(acct.owner);
    } else {
        FC_ASSERT(false, "invalid level specified");
    }
    flat_set<protocol::public_key_type> signing_keys;
    for (const protocol::signature_type &sig : args.sigs) {
        result.keys.emplace_back(fc::ecc::public_key(sig, args.dig, true));
        signing_keys.insert(result.keys.back());
    }

    flat_set<protocol::public_key_type> avail;
    protocol::sign_state ss(signing_keys, [&db](const std::string &account_name) -> const protocol::authority {
        return protocol::authority(db.get<golos::chain::account_authority_object, golos::chain::by_account>(account_name).active);
    }, avail);

    bool has_authority = ss.check_authority(auth);
    FC_ASSERT(has_authority);

    return result;
}

DEFINE_API ( plugin, check_authority_signature ) {
    auto tmp = args.args->at(0).as<check_authority_signature_a>();
    auto &db = my->database();
    return db.with_read_lock([&]() {
        check_authority_signature_r result;
        result = my->check_authority_signature(tmp);
        return result;
    });
}

plugin::plugin() {
}

plugin::~plugin() {
}

void plugin::plugin_initialize(const boost::program_options::variables_map &options) {
    my.reset(new plugin_impl());

    JSON_RPC_REGISTER_API ( name() ) ;
}

void plugin::plugin_startup() {
}
void plugin::set_program_options(boost::program_options::options_description &cli,
                             boost::program_options::options_description &cfg) {
}

} } } // golos::plugin::auth_util