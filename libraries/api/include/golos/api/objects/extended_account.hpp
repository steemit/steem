#include <golos/api/objects/account_api_object.hpp>

namespace golos { namespace api { namespace objects {

    /**
    *  Convert's vesting shares
    */
    struct extended_account : public account_api_object {
        extended_account() {
        }

        extended_account(const account_object &a, const golos::chain::database &db)
                : account_api_object(a, db) {
        }

        asset vesting_balance; /// convert vesting_shares to vesting steem
        set<string> witness_votes;
    };

} } } // golos::api::objects

FC_REFLECT_DERIVED((golos::api::objects::extended_account),
        ((golos::plugins::database_api::account_api_object)),
        (vesting_balance)
        (witness_votes)
   )
