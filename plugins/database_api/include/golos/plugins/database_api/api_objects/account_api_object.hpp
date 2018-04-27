#ifndef GOLOS_ACCOUNT_API_OBJ_HPP
#define GOLOS_ACCOUNT_API_OBJ_HPP

#include <golos/chain/account_object.hpp>
#include <golos/chain/database.hpp>
#include <golos/protocol/types.hpp>
#include <golos/chain/steem_object_types.hpp>

namespace golos { namespace plugins { namespace database_api {

            using protocol::asset;
            using protocol::share_type;
            using golos::chain::account_bandwidth_object;
            using golos::chain::by_account;
            using golos::chain::account_object;
            using protocol::authority;
            using protocol::account_name_type;
            using protocol::public_key_type;
            using golos::chain::by_account_bandwidth_type;
            using golos::chain::account_authority_object;
            using golos::chain::account_metadata_object;
            using golos::chain::bandwidth_type;


            struct account_api_object {
                account_api_object(const golos::chain::account_object &a, const golos::chain::database &db) : id(a.id), name(a.name),
                        memo_key(a.memo_key), proxy(a.proxy),
                        last_account_update(a.last_account_update), created(a.created), mined(a.mined),
                        owner_challenged(a.owner_challenged), active_challenged(a.active_challenged),
                        last_owner_proved(a.last_owner_proved), last_active_proved(a.last_active_proved),
                        recovery_account(a.recovery_account), reset_account(a.reset_account),
                        last_account_recovery(a.last_account_recovery), comment_count(a.comment_count),
                        lifetime_vote_count(a.lifetime_vote_count), post_count(a.post_count), can_vote(a.can_vote),
                        voting_power(a.voting_power), last_vote_time(a.last_vote_time), balance(a.balance),
                        savings_balance(a.savings_balance), sbd_balance(a.sbd_balance), sbd_seconds(a.sbd_seconds),
                        sbd_seconds_last_update(a.sbd_seconds_last_update),
                        sbd_last_interest_payment(a.sbd_last_interest_payment),
                        savings_sbd_balance(a.savings_sbd_balance), savings_sbd_seconds(a.savings_sbd_seconds),
                        savings_sbd_seconds_last_update(a.savings_sbd_seconds_last_update),
                        savings_sbd_last_interest_payment(a.savings_sbd_last_interest_payment),
                        savings_withdraw_requests(a.savings_withdraw_requests), curation_rewards(a.curation_rewards),
                        posting_rewards(a.posting_rewards), vesting_shares(a.vesting_shares),
                        delegated_vesting_shares(a.delegated_vesting_shares), received_vesting_shares(a.received_vesting_shares),
                        vesting_withdraw_rate(a.vesting_withdraw_rate),
                        next_vesting_withdrawal(a.next_vesting_withdrawal), withdrawn(a.withdrawn),
                        to_withdraw(a.to_withdraw), withdraw_routes(a.withdraw_routes),
                        witnesses_voted_for(a.witnesses_voted_for), last_post(a.last_post) {
                    size_t n = a.proxied_vsf_votes.size();
                    proxied_vsf_votes.reserve(n);
                    for (size_t i = 0; i < n; i++) {
                        proxied_vsf_votes.push_back(a.proxied_vsf_votes[i]);
                    }

                    const auto& auth = db.get<account_authority_object, by_account>(name);
                    owner = authority(auth.owner);
                    active = authority(auth.active);
                    posting = authority(auth.posting);
                    last_owner_update = auth.last_owner_update;

                    const auto& meta = db.get<account_metadata_object, by_account>(name);
                    json_metadata = golos::chain::to_string(meta.json_metadata);

                    auto old_forum = db.find<account_bandwidth_object, by_account_bandwidth_type>(
                            std::make_tuple(name, bandwidth_type::old_forum));
                    if (old_forum != nullptr) {
                        average_bandwidth = old_forum->average_bandwidth;
                        lifetime_bandwidth = old_forum->lifetime_bandwidth;
                        last_bandwidth_update = old_forum->last_bandwidth_update;
                    }

                    auto old_market = db.find<account_bandwidth_object, by_account_bandwidth_type>(
                            std::make_tuple(name, bandwidth_type::old_market));
                    if (old_market != nullptr) {
                        average_market_bandwidth = old_market->average_bandwidth;
                        last_market_bandwidth_update = old_market->last_bandwidth_update;
                    }

                    auto post = db.find<account_bandwidth_object, by_account_bandwidth_type>(
                            std::make_tuple(name, bandwidth_type::post));
                    if (post != nullptr) {
                        last_root_post = post->last_bandwidth_update;
                        post_bandwidth = post->average_bandwidth;
                    }

                    auto forum = db.find<account_bandwidth_object, by_account_bandwidth_type>(
                            std::make_tuple(name, bandwidth_type::forum));
                    if (forum != nullptr) {
                        new_average_bandwidth = forum->average_bandwidth;
                    }

                    auto market = db.find<account_bandwidth_object, by_account_bandwidth_type>(
                            std::make_tuple(name, bandwidth_type::market));
                    if (market != nullptr) {
                        new_average_market_bandwidth = market->average_bandwidth;
                    }
                }


                account_api_object() {
                }

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
            };
} } } // golos::plugins::database_api


FC_REFLECT((golos::plugins::database_api::account_api_object),
           (id)(name)(owner)(active)(posting)(memo_key)(json_metadata)(proxy)(last_owner_update)(last_account_update)(
                   created)(mined)(owner_challenged)(active_challenged)(last_owner_proved)(last_active_proved)(
                   recovery_account)(last_account_recovery)(reset_account)(comment_count)(lifetime_vote_count)(
                   post_count)(can_vote)(voting_power)(last_vote_time)(balance)(savings_balance)(sbd_balance)(
                   sbd_seconds)(sbd_seconds_last_update)(sbd_last_interest_payment)(savings_sbd_balance)(
                   savings_sbd_seconds)(savings_sbd_seconds_last_update)(savings_sbd_last_interest_payment)(
                   savings_withdraw_requests)(vesting_shares)(delegated_vesting_shares)(received_vesting_shares)(
                   vesting_withdraw_rate)(next_vesting_withdrawal)(withdrawn)(to_withdraw)(withdraw_routes)(
                   curation_rewards)(posting_rewards)(proxied_vsf_votes)(witnesses_voted_for)(average_bandwidth)(
                   lifetime_bandwidth)(last_bandwidth_update)(average_market_bandwidth)(last_market_bandwidth_update)(
                   last_post)(last_root_post)(post_bandwidth)(new_average_bandwidth)(new_average_market_bandwidth))
#endif //GOLOS_ACCOUNT_API_OBJ_HPP
