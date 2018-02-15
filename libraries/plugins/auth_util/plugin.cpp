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
     std::vector<protocol::public_key_type> check_authority_signature(
             const std::string& account_name,
             const std::string& level,
             fc::sha256 dig,
             const std::vector<protocol::signature_type>& sigs);

    // HELPING METHODS
    golos::chain::database &database() {
        return db_;
    }
private:
    golos::chain::database & db_;
};

    std::vector<protocol::public_key_type> plugin::plugin_impl::check_authority_signature(
        const std::string& account_name,
        const std::string& level,
        fc::sha256 dig,
        const std::vector<protocol::signature_type>& sigs) {
    auto & db = database();
    std::vector<protocol::public_key_type> result;

    const golos::chain::account_authority_object &acct =
            db.get<golos::chain::account_authority_object, golos::chain::by_account>(account_name);
    protocol::authority auth;
    if ((level == "posting") || (level == "p")) {
        auth = protocol::authority(acct.posting);
    } else if ((level == "active") ||
               (level == "a") || (level == "")) {
        auth = protocol::authority(acct.active);
    } else if ((level == "owner") || (level == "o")) {
        auth = protocol::authority(acct.owner);
    } else {
        FC_ASSERT(false, "invalid level specified");
    }
    flat_set<protocol::public_key_type> signing_keys;
    for (const protocol::signature_type &sig : sigs) {
        result.emplace_back(fc::ecc::public_key(sig, dig, true));
        signing_keys.insert(result.back());
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
    auto arg0 = args.args->at(0).as<std::string>();
    auto arg1 = args.args->at(1).as<std::string>();
    auto arg2 = args.args->at(2).as<fc::sha256>();
    auto arg3 = args.args->at(3).as<std::vector<protocol::signature_type>>();
    auto &db = my->database();
    return db.with_read_lock([&]() {
        return my->check_authority_signature(arg0, arg1, arg2, arg3);;
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