#ifndef GOLOS_WITNESS_API_OBJ_HPP
#define GOLOS_WITNESS_API_OBJ_HPP

#include <golos/chain/witness_objects.hpp>

namespace golos {
    namespace plugins {
        namespace database_api {
            using golos::protocol::asset;
            using golos::protocol::price;
            using golos::protocol::account_name_type;


            struct witness_api_object {
                witness_api_object(const golos::chain::witness_object &w) : id(w.id), owner(w.owner), created(w.created),
                        url(to_string(w.url)), total_missed(w.total_missed), last_aslot(w.last_aslot),
                        last_confirmed_block_num(w.last_confirmed_block_num), pow_worker(w.pow_worker),
                        signing_key(w.signing_key), props(w.props), sbd_exchange_rate(w.sbd_exchange_rate),
                        last_sbd_exchange_update(w.last_sbd_exchange_update), votes(w.votes),
                        virtual_last_update(w.virtual_last_update), virtual_position(w.virtual_position),
                        virtual_scheduled_time(w.virtual_scheduled_time), last_work(w.last_work),
                        running_version(w.running_version), hardfork_version_vote(w.hardfork_version_vote),
                        hardfork_time_vote(w.hardfork_time_vote) {
                }

                witness_api_object() {
                }

                witness_object::id_type id;
                account_name_type owner;
                time_point_sec created;
                std::string url;
                uint32_t total_missed;
                uint64_t last_aslot;
                uint64_t last_confirmed_block_num;
                uint64_t pow_worker;
                public_key_type signing_key;
                chain_properties props;
                price sbd_exchange_rate;
                time_point_sec last_sbd_exchange_update;
                share_type votes;
                fc::uint128_t virtual_last_update;
                fc::uint128_t virtual_position;
                fc::uint128_t virtual_scheduled_time;
                digest_type last_work;
                version running_version;
                hardfork_version hardfork_version_vote;
                time_point_sec hardfork_time_vote;
            };

        }
    }
} // golos::application






FC_REFLECT((golos::plugins::database_api::witness_api_object),
           (id)(owner)(created)(url)(votes)(virtual_last_update)(virtual_position)(virtual_scheduled_time)(
                   total_missed)(last_aslot)(last_confirmed_block_num)(pow_worker)(signing_key)(props)(
                   sbd_exchange_rate)(last_sbd_exchange_update)(last_work)(running_version)(hardfork_version_vote)(
                   hardfork_time_vote))

#endif //GOLOS_WITNESS_API_OBJ_HPP
