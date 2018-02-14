#pragma once

#include <golos/plugins/json_rpc/plugin.hpp>
#include <golos/plugins/chain/plugin.hpp>
#include <golos/plugins/p2p/p2p_plugin.hpp>
#include <memory>
#include <appbase/application.hpp>

#define STEEM_NETWORK_BROADCAST_API_PLUGIN_NAME "network_broadcast_api"

namespace golos {
    namespace plugins {
        namespace network_broadcast_api {


            using json_rpc::msg_pack;
            using json_rpc::void_type;

            using golos::protocol::signed_block;
            using golos::protocol::transaction_id_type;
            using golos::protocol::signed_transaction;

            struct broadcast_transaction_synchronous_t {
                broadcast_transaction_synchronous_t() {
                }

                broadcast_transaction_synchronous_t(transaction_id_type txid, int32_t bn, int32_t tn, bool ex) : id(
                        txid), block_num(bn), trx_num(tn), expired(ex) {
                }

                transaction_id_type id;
                int32_t block_num = 0;
                int32_t trx_num = 0;
                bool expired = false;
            };

            ///               API,                               args,     return
            DEFINE_API_ARGS(broadcast_transaction,               msg_pack, void_type)
            DEFINE_API_ARGS(broadcast_transaction_synchronous,   msg_pack, void_type)
            DEFINE_API_ARGS(broadcast_block,                     msg_pack, void_type)
            DEFINE_API_ARGS(broadcast_transaction_with_callback, msg_pack, void_type)


            using namespace appbase;

            class network_broadcast_api_plugin final : public appbase::plugin<network_broadcast_api_plugin> {
            public:
                APPBASE_PLUGIN_REQUIRES((json_rpc::plugin) (chain::plugin) (p2p::p2p_plugin))

                network_broadcast_api_plugin();

                ~network_broadcast_api_plugin();

                static const std::string &name() {
                    static std::string name = STEEM_NETWORK_BROADCAST_API_PLUGIN_NAME;
                    return name;
                }

                DECLARE_API(
                        (broadcast_transaction)
                        (broadcast_transaction_synchronous)
                        (broadcast_block)
                        (broadcast_transaction_with_callback)
                )

                bool check_max_block_age(int32_t max_block_age) const;

                void on_applied_block(const signed_block &b);

                void set_program_options(boost::program_options::options_description &cli, boost::program_options::options_description &cfg) override;

                void plugin_initialize(const boost::program_options::variables_map &options) override;

                void plugin_startup() override;

                void plugin_shutdown() override;

                boost::signals2::connection on_applied_block_connection;
            private:
                struct impl;
                std::unique_ptr<impl> pimpl;
            };

        }
    }
}

FC_REFLECT((golos::plugins::network_broadcast_api::broadcast_transaction_synchronous_t),
           (id)(block_num)(trx_num)(expired))
