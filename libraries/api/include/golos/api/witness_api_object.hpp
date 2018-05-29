#pragma once

#include <golos/chain/witness_objects.hpp>
#include <golos/chain/steem_object_types.hpp>
#include <golos/api/chain_api_properties.hpp>

namespace golos { namespace api {
    using namespace golos::chain;

    struct witness_api_object {
        witness_api_object(const witness_object &w, const database& db);

        witness_api_object() = default;

        witness_object::id_type id;
        account_name_type owner;
        time_point_sec created;
        std::string url;
        uint32_t total_missed;
        uint64_t last_aslot;
        uint64_t last_confirmed_block_num;
        uint64_t pow_worker;
        public_key_type signing_key;
        api::chain_api_properties props;
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

} } // golos::api


FC_REFLECT(
    (golos::api::witness_api_object),
    (id)(owner)(created)(url)(votes)(virtual_last_update)(virtual_position)(virtual_scheduled_time)
    (total_missed)(last_aslot)(last_confirmed_block_num)(pow_worker)(signing_key)(props)
    (sbd_exchange_rate)(last_sbd_exchange_update)(last_work)(running_version)(hardfork_version_vote)
    (hardfork_time_vote))