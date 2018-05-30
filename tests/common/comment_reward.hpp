#pragma once

#include <map>

#include <fc/uint128_t.hpp>
#include <golos/chain/comment_object.hpp>
#include <golos/chain/database.hpp>
#include <golos/chain/account_object.hpp>

namespace golos { namespace chain {

    class comment_fund final {
    public:
        comment_fund(database& db):
            db_(db)
        {
            auto& gpo = db.get_dynamic_global_properties();
            reward_shares_ = gpo.total_reward_shares2;
            reward_fund_ = gpo.total_reward_fund_steem;
            vesting_shares_ = gpo.total_vesting_shares;
            vesting_fund_ = gpo.total_vesting_fund_steem;

            process_funds();
        }

        const fc::uint128_t& reward_shares() const {
            return reward_shares_;
        }

        const asset& reward_fund() const {
            return reward_fund_;
        }

        const asset& vesting_shares() const {
            return vesting_shares_;
        }

        const asset& vesting_fund() const {
            return vesting_fund_;
        }

        asset claim_comment_reward(const comment_object& comment) {
            auto vshares = db_.calculate_vshares(comment.net_rshares.value);
            BOOST_REQUIRE_LE(vshares.to_uint64(), reward_shares_.to_uint64());

            auto reward = vshares * reward_fund_.amount.value / reward_shares_;
            BOOST_REQUIRE_LE(reward.to_uint64(), reward_fund_.amount.value);

            reward_shares_ -= vshares;
            reward_fund_ -= asset(reward.to_uint64(), reward_fund_.symbol);

            return asset(reward.to_uint64(), STEEM_SYMBOL);
        }

        asset create_vesting(asset value) {
            asset vesting = value * price(vesting_shares_, vesting_fund_);
            vesting_shares_ += vesting;
            vesting_fund_ += value;
            return vesting;
        }

    private:
        void process_funds() {
            int64_t inflation_rate = STEEMIT_INFLATION_RATE_START_PERCENT;
            int64_t supply = db_.get_dynamic_global_properties().virtual_supply.amount.value;

            auto total_reward = (supply * inflation_rate) / (int64_t(STEEMIT_100_PERCENT) * STEEMIT_BLOCKS_PER_YEAR);
            auto content_reward = (total_reward * STEEMIT_CONTENT_REWARD_PERCENT) / STEEMIT_100_PERCENT;
            auto vesting_reward = total_reward * STEEMIT_VESTING_FUND_PERCENT / STEEMIT_100_PERCENT;
            auto witness_reward = total_reward - content_reward - vesting_reward;
            auto witness_normalize = 25;

            witness_reward = witness_reward * STEEMIT_MAX_WITNESSES / witness_normalize;

            vesting_fund_ += asset(vesting_reward, STEEM_SYMBOL);
            reward_fund_ += asset(content_reward, STEEM_SYMBOL);

            create_vesting(asset(witness_reward, STEEM_SYMBOL));
        }
        
        database& db_;
        fc::uint128_t reward_shares_;
        asset reward_fund_;
        asset vesting_shares_;
        asset vesting_fund_;
    };

    class comment_reward final {
    public:
        comment_reward(
            database& db,
            comment_fund& fund,
            const account_name_type& author,
            const std::string& permlink
        ): comment_reward(db, fund, comment_reward::get_comment(db, author, permlink)) {
        }

        comment_reward(
            database& db,
            comment_fund& fund,
            const comment_object& comment
        ):
            fund_(fund),
            db_(db),
            comment_(comment)
        {
            calculate_rewards();
            calculate_vote_payouts();
            calculate_beneficiaries_payout();
            calculate_comment_payout();
        }

        asset total_vote_payouts() const {
            return total_vote_payouts_;
        }

        asset vote_payout(const account_object& voter) const {
            auto itr = vote_payout_map_.find(voter.id);
            BOOST_REQUIRE(vote_payout_map_.end() != itr);
            return itr->second;
        }

        asset vote_payout(const std::string& voter) const {
            auto account = db_.find_account(voter);
            BOOST_REQUIRE(account != nullptr);
            return vote_payout(*account);
        }

        asset total_beneficiary_payouts() const {
            return total_beneficiary_payouts_;
        }

        asset beneficiary_payout(const account_object& voter) const {
            auto itr = beneficiary_payout_map_.find(voter.id);
            BOOST_REQUIRE(beneficiary_payout_map_.end() != itr);
            return itr->second;
        }

        asset benefeciary_payout(const std::string& voter) const {
            auto account = db_.find_account(voter);
            BOOST_REQUIRE(account != nullptr);
            return beneficiary_payout(*account);
        }

        asset sbd_payout() const {
            return sbd_payout_;
        }

        asset vesting_payout() const {
            return vesting_payout_;
        }

        asset total_payout() const {
            return total_payout_;
        }

    private:
        static const comment_object& get_comment(
            database& db,
            const account_name_type& author,
            const std::string& permlink
        ) {
            auto comment = db.find_comment(author, permlink);
            BOOST_REQUIRE(comment != nullptr);
            return *comment;
        }

        void calculate_rewards() {
            comment_rewards_ = fund_.claim_comment_reward(comment_).amount.value;
            vote_rewards_fund_ =  comment_rewards_ * STEEMIT_1_PERCENT * 25 / STEEMIT_100_PERCENT;
        }

        void calculate_vote_payouts() {
            auto& vote_idx = db_.get_index<comment_vote_index>().indices().get<by_comment_weight_voter>();
            auto itr = vote_idx.lower_bound(comment_.id);
            auto total_weight = comment_.total_vote_weight;

            total_vote_rewards_ = 0;
            total_vote_payouts_ = asset(0, VESTS_SYMBOL);
            for (; itr != vote_idx.end() && itr->comment == comment_.id; ++itr) {
                BOOST_REQUIRE(vote_payout_map_.find(itr->voter) == vote_payout_map_.end());
                auto weight = u256(itr->weight);
                int64_t reward = static_cast<int64_t>(weight * vote_rewards_fund_ / total_weight);

                total_vote_rewards_ += reward;
                BOOST_REQUIRE_LE(total_vote_rewards_, vote_rewards_fund_);

                auto payout = fund_.create_vesting(asset(reward, STEEM_SYMBOL));
                vote_payout_map_.emplace(itr->voter, payout);
                total_vote_payouts_ += payout;
            }

            comment_rewards_ -= total_vote_rewards_;
        }

        void calculate_beneficiaries_payout() {
            total_beneficiary_rewards_ = 0;
            total_beneficiary_payouts_ = asset(0, VESTS_SYMBOL);
            for (auto& route: comment_.beneficiaries) {
                auto beneficiary = db_.find_account(route.account);
                BOOST_REQUIRE(beneficiary != nullptr);
                BOOST_REQUIRE(beneficiary_payout_map_.find(beneficiary->id) == beneficiary_payout_map_.end());

                int64_t reward = (comment_rewards_ * route.weight) / STEEMIT_100_PERCENT;

                total_beneficiary_rewards_ += reward;
                BOOST_REQUIRE_LE(total_beneficiary_rewards_, comment_rewards_);
                
                auto payout = fund_.create_vesting(asset(reward, STEEM_SYMBOL));
                beneficiary_payout_map_.emplace(beneficiary->id, payout);
                total_beneficiary_payouts_ += payout;
            }

            comment_rewards_ -= total_beneficiary_rewards_;
        }

        void calculate_comment_payout() {
            auto sbd_payout_value = comment_rewards_ / 2;
            auto vesting_payout_value = comment_rewards_ - sbd_payout_value;

            total_payout_ = db_.to_sbd(asset(sbd_payout_value + vesting_payout_value, STEEM_SYMBOL));
            sbd_payout_ = asset(sbd_payout_value, SBD_SYMBOL);
            vesting_payout_ = fund_.create_vesting(asset(vesting_payout_value, STEEM_SYMBOL));
        }

        comment_fund& fund_;
        database& db_;

        const comment_object& comment_;
        int64_t comment_rewards_;

        int64_t vote_rewards_fund_;
        int64_t total_vote_rewards_;
        asset total_vote_payouts_;
        std::map<account_id_type, asset> vote_payout_map_;

        int64_t total_beneficiary_rewards_;
        asset total_beneficiary_payouts_;
        std::map<account_id_type, asset> beneficiary_payout_map_;

        asset sbd_payout_;
        asset vesting_payout_;
        asset total_payout_;
    };

} } // namespace golos::chain