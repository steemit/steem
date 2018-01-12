#pragma once

#include <appbase/application.hpp>

#include <boost/signals2.hpp>
#include <golos/protocol/types.hpp>
#include <golos/chain/database.hpp>
#include <golos/protocol/block.hpp>

#include <golos/plugins/json_rpc/utility.hpp>
#include <golos/plugins/json_rpc/plugin.hpp>
// for api
#include <fc/optional.hpp>

namespace golos {
    namespace plugins {
        namespace chain {

            using golos::plugins::json_rpc::msg_pack;

            struct push_block_a {
                golos::chain::signed_block block;
                bool currently_syncing = false;
            };


            struct push_block_r {
                bool success;
                optional <std::string> error;
            };

            typedef golos::chain::signed_transaction push_transaction_a;

            struct push_transaction_r {
                bool success;
                optional <std::string> error;
            };

            DEFINE_API_ARGS ( push_block, msg_pack, push_block_r) 
            //
            DEFINE_API_ARGS ( push_transaction, msg_pack, push_transaction_r)
            //

            class plugin final : public appbase::plugin<plugin> {
            public:
                APPBASE_PLUGIN_REQUIRES((json_rpc::plugin))

                plugin();

                ~plugin();

                DECLARE_API((push_block)(push_transaction))


                constexpr const static char *plugin_name = "chain";

                static const std::string &name() {
                    static std::string name = plugin_name;
                    return name;
                }

                void set_program_options(boost::program_options::options_description &cli, boost::program_options::options_description &cfg) override;

                void plugin_initialize(const boost::program_options::variables_map &options) override;

                void plugin_startup() override;

                void plugin_shutdown() override;

                bool accept_block(const protocol::signed_block &block, bool currently_syncing, uint32_t skip);

                void accept_transaction(const protocol::signed_transaction &trx);

                bool block_is_on_preferred_chain(const protocol::block_id_type &block_id);

                void check_time_in_block(const protocol::signed_block &block);

                template<typename MultiIndexType>
                bool has_index() const {
                    return db().has_index<MultiIndexType>();
                }

                template<typename MultiIndexType>
                const chainbase::generic_index<MultiIndexType> &get_index() const {
                    return db().get_index<MultiIndexType>();
                }

                template<typename ObjectType, typename IndexedByType, typename CompatibleKey>
                const ObjectType *find(CompatibleKey &&key) const {
                    return db().find<ObjectType, IndexedByType, CompatibleKey>(key);
                }

                template<typename ObjectType>
                const ObjectType *find(chainbase::object_id<ObjectType> key = chainbase::object_id<ObjectType>()) {
                    return db().find<ObjectType>(key);
                }

                template<typename ObjectType, typename IndexedByType, typename CompatibleKey>
                const ObjectType &get(CompatibleKey &&key) const {
                    return db().get<ObjectType, IndexedByType, CompatibleKey>(key);
                }

                template<typename ObjectType>
                const ObjectType &get(
                        const chainbase::object_id<ObjectType> &key = chainbase::object_id<ObjectType>()) {
                    return db().get<ObjectType>(key);
                }

                // Exposed for backwards compatibility. In the future, plugins should manage their own internal database
                golos::chain::database &db();

                const golos::chain::database &db() const;

                // Emitted when the blockchain is syncing/live.
                // This is to synchronize plugins that have the chain plugin as an optional dependency.
                boost::signals2::signal<void()> on_sync;

            private:
                class plugin_impl;

                std::unique_ptr<plugin_impl> my;
            };
        }
    }
} // golos::plugins::chain

FC_REFLECT((golos::plugins::chain::push_block_a), (block)(currently_syncing) )
FC_REFLECT((golos::plugins::chain::push_block_r), (success)(error) )
FC_REFLECT((golos::plugins::chain::push_transaction_r), (success)(error) )
