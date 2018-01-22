#pragma once
#include <golos/plugins/account_by_key/account_by_key_plugin.hpp>
#include <golos/plugins/account_by_key/account_by_key_objects.hpp>

#include <golos/protocol/types.hpp>
#include <appbase/application.hpp>

#include <golos/chain/database.hpp>
#include <golos/plugins/chain/plugin.hpp>



namespace golos {
    namespace plugins {
        namespace account_by_key {

            using namespace golos::protocol;

            DEFINE_API_ARGS(get_key_references, json_rpc::msg_pack, key_references_r)

            class account_by_key_plugin : public appbase::plugin<account_by_key_plugin> {
            public:
                APPBASE_PLUGIN_REQUIRES((json_rpc::plugin))

                account_by_key_plugin();

                DECLARE_API((get_key_references))

                constexpr const static char *plugin_name = "account_by_key";

                static const std::string &name() {
                    static std::string name = plugin_name;
                    return name;
                }

                void set_program_options(
                        boost::program_options::options_description &cli,
                        boost::program_options::options_description &cfg) override;

                void plugin_initialize(const boost::program_options::variables_map &options) override;

                void plugin_startup() override;

                void plugin_shutdown() override;

                // Impl
                class account_by_key_plugin_impl {
                public:
                    account_by_key_plugin_impl(account_by_key_plugin &_plugin)
                            : _self(_plugin) ,
                              _db(appbase::app().get_plugin<golos::plugins::chain::plugin>().db()){
                    }

                    void pre_operation(const operation_notification &op_obj);

                    void post_operation(const operation_notification &op_obj);

                    void clear_cache();

                    void cache_auths(const account_authority_object &a);

                    void update_key_lookup(const account_authority_object &a);

                    key_references_r get_key_references(const key_references_a& val) const;

                    golos::chain::database &database() const {
                        return _db;
                    }

                    flat_set <public_key_type> cached_keys;
                    account_by_key_plugin &_self;

                    golos::chain::database &_db;
                };

                std::unique_ptr<account_by_key_plugin_impl> my;
            };

        }
    }
} // golos::plugins::account_by_key
