#pragma once

#include <golos/protocol/block.hpp>

#include <golos/plugins/p2p/p2p_plugin.hpp>
#include <golos/plugins/chain/plugin.hpp>
#include <golos/plugins/json_rpc/utility.hpp>

#include <fc/optional.hpp>
#include <fc/variant.hpp>
#include <fc/vector.hpp>

#include <boost/thread/mutex.hpp>

namespace golos {
    namespace plugins {
        namespace network_broadcast_api {

            using json_rpc::msg_pack;
            using json_rpc::void_type;

            using golos::protocol::signed_block;
            using golos::protocol::transaction_id_type;

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


            ///               API,                                    args,                return
            DEFINE_API_ARGS(broadcast_transaction, msg_pack, void_type)
            DEFINE_API_ARGS(broadcast_transaction_synchronous, msg_pack, broadcast_transaction_synchronous_t)
            DEFINE_API_ARGS(broadcast_block, msg_pack, void_type)

            class network_broadcast_api final {
            public:
                network_broadcast_api();

                ~network_broadcast_api();

                DECLARE_API((broadcast_transaction)(broadcast_transaction_synchronous)(broadcast_block))

                bool check_max_block_age(int32_t max_block_age) const;

                void on_applied_block(const signed_block &b);


            private:
                struct impl;
                std::unique_ptr<impl> pimpl;
            };

        }
    }
}


FC_REFLECT((golos::plugins::network_broadcast_api::broadcast_transaction_synchronous_t),
           (id)(block_num)(trx_num)(expired))