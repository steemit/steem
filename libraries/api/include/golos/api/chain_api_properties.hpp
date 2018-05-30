#pragma once

#include <golos/chain/witness_objects.hpp>
#include <golos/chain/database.hpp>

namespace golos { namespace api {
    using golos::protocol::asset;
    using golos::chain::chain_properties;
    using golos::chain::database;

    struct chain_api_properties {
        chain_api_properties(const chain_properties&, const database&);
        chain_api_properties() = default;

        asset account_creation_fee;
        uint32_t maximum_block_size;
        uint16_t sbd_interest_rate;

        fc::optional<uint32_t> create_account_with_golos_modifier;
        fc::optional<uint32_t> create_account_delegation_ratio;
        fc::optional<fc::microseconds> create_account_delegation_time;
        fc::optional<uint32_t> min_delegation_multiplier;
    };

} } // golos::api

FC_REFLECT(
    (golos::api::chain_api_properties),
    (account_creation_fee)(maximum_block_size)(maximum_block_size)
    (create_account_with_golos_modifier)(create_account_delegation_ratio)
    (create_account_delegation_time)(min_delegation_multiplier))