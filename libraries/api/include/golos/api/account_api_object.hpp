#ifndef GOLOS_ACCOUNT_API_OBJ_HPP
#define GOLOS_ACCOUNT_API_OBJ_HPP

#include <golos/chain/account_object.hpp>
#include <golos/chain/database.hpp>
#include <golos/protocol/types.hpp>
#include <golos/chain/steem_object_types.hpp>

namespace golos { namespace api {

using golos::chain::account_object;
using protocol::asset;
using protocol::share_type;
using protocol::authority;
using protocol::account_name_type;
using protocol::public_key_type;


struct account_api_object {
    account_api_object(const account_object&, const golos::chain::database&);
    account_api_object();

    account_object::id_type id;

    account_name_type name;
    authority owner;
    authority active;
    authority posting;
    public_key_type memo_key;
    std::string json_metadata;
    account_name_type proxy;

    time_point_sec last_owner_update;
    time_point_sec last_account_update;

    time_point_sec created;
    bool mined;
    bool owner_challenged;
    bool active_challenged;
    time_point_sec last_owner_proved;
    time_point_sec last_active_proved;
    account_name_type recovery_account;
    account_name_type reset_account;
    time_point_sec last_account_recovery;
    uint32_t comment_count;
    uint32_t lifetime_vote_count;
    uint32_t post_count;

    bool can_vote;
    uint16_t voting_power;
    time_point_sec last_vote_time;

    asset balance;
    asset savings_balance;

    asset sbd_balance;
    uint128_t sbd_seconds;
    time_point_sec sbd_seconds_last_update;
    time_point_sec sbd_last_interest_payment;

    asset savings_sbd_balance;
    uint128_t savings_sbd_seconds;
    time_point_sec savings_sbd_seconds_last_update;
    time_point_sec savings_sbd_last_interest_payment;

    uint8_t savings_withdraw_requests;

    protocol::share_type curation_rewards;
    share_type posting_rewards;

    asset vesting_shares;
    asset delegated_vesting_shares;
    asset received_vesting_shares;
    asset vesting_withdraw_rate;
    time_point_sec next_vesting_withdrawal;
    share_type withdrawn;
    share_type to_withdraw;
    uint16_t withdraw_routes;

    std::vector<share_type> proxied_vsf_votes;

    uint16_t witnesses_voted_for;

    share_type average_bandwidth = 0;
    share_type lifetime_bandwidth = 0;
    time_point_sec last_bandwidth_update;

    share_type average_market_bandwidth = 0;
    time_point_sec last_market_bandwidth_update;
    time_point_sec last_post;
    time_point_sec last_root_post;
    share_type post_bandwidth = STEEMIT_100_PERCENT;

    share_type new_average_bandwidth;
    share_type new_average_market_bandwidth;
    set<string> witness_votes;
    
    fc::optional<share_type> reputation;
};

} } // golos::api


FC_REFLECT((golos::api::account_api_object),
    (id)(name)(owner)(active)(posting)(memo_key)(json_metadata)(proxy)(last_owner_update)(last_account_update)
    (created)(mined)(owner_challenged)(active_challenged)(last_owner_proved)(last_active_proved)
    (recovery_account)(last_account_recovery)(reset_account)(comment_count)(lifetime_vote_count)
    (post_count)(can_vote)(voting_power)(last_vote_time)(balance)(savings_balance)(sbd_balance)
    (sbd_seconds)(sbd_seconds_last_update)(sbd_last_interest_payment)(savings_sbd_balance)
    (savings_sbd_seconds)(savings_sbd_seconds_last_update)(savings_sbd_last_interest_payment)
    (savings_withdraw_requests)(vesting_shares)(delegated_vesting_shares)(received_vesting_shares)
    (vesting_withdraw_rate)(next_vesting_withdrawal)(withdrawn)(to_withdraw)(withdraw_routes)
    (curation_rewards)(posting_rewards)(proxied_vsf_votes)(witnesses_voted_for)(average_bandwidth)
    (lifetime_bandwidth)(last_bandwidth_update)(average_market_bandwidth)(last_market_bandwidth_update)
    (last_post)(last_root_post)(post_bandwidth)(new_average_bandwidth)(new_average_market_bandwidth)
    (witness_votes)(reputation))

#endif //GOLOS_ACCOUNT_API_OBJ_HPP
