#include <golos/api/chain_api_properties.hpp>

namespace golos { namespace api {

    chain_api_properties::chain_api_properties(
        const chain_properties& src,
        const database& db
    ) : account_creation_fee(src.account_creation_fee),
        maximum_block_size(src.maximum_block_size),
        sbd_interest_rate(src.sbd_interest_rate)
    {
        if (db.has_hardfork(STEEMIT_HARDFORK_0_18__673)) {
            create_account_with_golos_modifier = src.create_account_with_golos_modifier;
            create_account_delegation_ratio = src.create_account_delegation_ratio;
            create_account_delegation_time = src.create_account_delegation_time;
            min_delegation_multiplier = src.min_delegation_multiplier;
        }
    }

} } // golos::api