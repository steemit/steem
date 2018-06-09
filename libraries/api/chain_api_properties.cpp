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
            create_account_min_golos_fee = src.create_account_min_golos_fee;
            create_account_min_delegation = src.create_account_min_delegation;
            create_account_delegation_time = src.create_account_delegation_time;
            min_delegation = src.min_delegation;
        }
    }

} } // golos::api
