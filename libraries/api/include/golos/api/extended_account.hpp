#pragma once
#include <golos/api/account_api_object.hpp>

namespace golos { namespace api {

    /**
    *  Convert's vesting shares
    */
    struct extended_account : public account_api_object {
        extended_account() = default;

        extended_account(const account_object &a, const golos::chain::database &db)
                : account_api_object(a, db) {
        }

        share_type reputation = 0;
    };

} } // golos::api

FC_REFLECT_DERIVED(
    (golos::api::extended_account),
    ((golos::api::account_api_object)),
    (reputation))
