#pragma once

#include <steem/chain/steem_object_types.hpp>

namespace steem { namespace chain {

   class database;

   /**Abstract interface implemented separately for STEEM and SMTs.
    *
    * Allows to keep common code of vote_operation evaluator hiding significant differences in voting between STEEM and SMTs:
    * 1. Different parameters influencing voting evaluation:
    *     common to all SMTs:
    *        STEEM_MIN_VOTE_INTERVAL_SEC   - SMT_MIN_VOTE_INTERVAL_SEC
    *     individually set by SMT creator:
    *        STEEM_VOTE_REGENERATION_SECONDS  - vote_regeneration_period_seconds
    *        vote_power_reserve_rate (dgpo)   - votes_per_regeneration_period
    * 2. Separately stored user/comment/comment vote info:
    *     Per-user voting data:
    *        last_vote_time - duplicated as last_smt_vote_time as dependent on ..._MIN_VOTE_INTERVAL_SEC
    *        voting_power   - common to both STEEM and SMT voting
    *     comment_object
    *        net_rshares
    *        abs_rshares
    *        vote_rshares
    *        total_vote_weight
    *     comment_vote_object
    *        rshares
    *        weight
    */
   class IVotingHelper
   {
      public:
      virtual uint32_t GetMinimalVoteInterval() const = 0;
      virtual uint32_t GetVoteRegenerationPeriod() const = 0;
      virtual uint32_t GetVotesPerRegenerationPeriod() const = 0;
      virtual uint32_t GetReverseAuctionWindowSeconds() = 0;

      virtual const share_type& GetCommentNetRshares( const comment_object& comment ) = 0;
      virtual const share_type& GetCommentVoteRshares( const comment_object& comment ) = 0;

      virtual void IncreaseCommentTotalVoteWeight( const comment_object& comment, const uint64_t& delta ) = 0;

      virtual const comment_vote_object& CreateCommentVoteObject( const account_id_type& voterId, const comment_id_type& commentId,
                                                                  const int16_t& opWeight ) = 0;

      virtual fc::uint128_t CalculateAvgCashoutSec( const comment_object& comment, const comment_object& root,
                                                   const int64_t& voter_abs_rshares, bool is_recast ) = 0;
      virtual uint64_t CalculateCommentVoteWeight(const comment_object& comment, const int64_t& rshares, 
                                                const share_type& old_vote_rshares ) = 0;

      virtual void UpdateVoterParams(const account_object& voter, const uint16_t& newVotingPower, const time_point_sec& newLastVoteTime) = 0;
      virtual void UpdateComment( const comment_object& comment, const int64_t& rshares, const int64_t& absRshares ) = 0;
      virtual void UpdateCommentRecast( const comment_object& comment, const comment_vote_object& vote, 
                                       const int64_t& recast_rshares, const int64_t& vote_absRshares ) = 0;
      virtual void UpdateRootComment( const comment_object& root, const int64_t& vote_absRshares, const fc::uint128_t& avg_cashout_sec ) = 0;
      virtual void UpdateCommentVoteObject( const comment_vote_object& cvo, const uint64_t& vote_weight, const int64_t& rshares ) = 0;
      virtual void UpdateCommentRshares2( const comment_object& c, const fc::uint128_t& old_rshares2, const fc::uint128_t& new_rshares2 ) = 0;
      virtual void UpdateVote( const comment_vote_object& vote, const int64_t& rshares, const int16_t& opWeight ) = 0;
   };

   class TSteemVotingHelper: public IVotingHelper
   {
      public:
      TSteemVotingHelper(database& db);

      virtual uint32_t GetMinimalVoteInterval() const override { return STEEM_MIN_VOTE_INTERVAL_SEC; }
      virtual uint32_t GetVoteRegenerationPeriod() const override { return STEEM_VOTE_REGENERATION_SECONDS; }
      virtual uint32_t GetVotesPerRegenerationPeriod() const override { return VotesPerRegenerationPeriod; }
      virtual uint32_t GetReverseAuctionWindowSeconds() override { return STEEM_REVERSE_AUCTION_WINDOW_SECONDS; }

      virtual const share_type& GetCommentNetRshares( const comment_object& comment ) override;
      virtual const share_type& GetCommentVoteRshares( const comment_object& comment ) override;
      virtual void IncreaseCommentTotalVoteWeight( const comment_object& comment, const uint64_t& delta ) override;
      virtual const comment_vote_object& CreateCommentVoteObject( const account_id_type& voterId, const comment_id_type& commentId,
                                                                  const int16_t& opWeight ) override;
      virtual fc::uint128_t CalculateAvgCashoutSec( const comment_object& comment, const comment_object& root,
                                                    const int64_t& voter_abs_rshares, bool is_recast ) override;
      virtual uint64_t CalculateCommentVoteWeight(const comment_object& comment, const int64_t& rshares, 
                                                  const share_type& old_vote_rshares ) override;
      virtual void UpdateVoterParams(const account_object& voter, const uint16_t& newVotingPower,
                                     const time_point_sec& newLastVoteTime) override;
      virtual void UpdateComment( const comment_object& comment, const int64_t& vote_rshares, const int64_t& vote_absRshares ) override;
      virtual void UpdateCommentRecast( const comment_object& comment, const comment_vote_object& vote, 
                                        const int64_t& recast_rshares, const int64_t& vote_absRshares ) override;
      virtual void UpdateRootComment( const comment_object& root, const int64_t& vote_absRshares, const fc::uint128_t& avg_cashout_sec ) override;
      virtual void UpdateCommentVoteObject( const comment_vote_object& cvo, const uint64_t& vote_weight, const int64_t& rshares ) override;
      virtual void UpdateCommentRshares2( const comment_object& c, const fc::uint128_t& old_rshares2, const fc::uint128_t& new_rshares2 ) override;
      virtual void UpdateVote( const comment_vote_object& vote, const int64_t& rshares, const int16_t& opWeight ) override;

      private:
      database&                  DB;
      uint32_t                   VotesPerRegenerationPeriod = 0;
      const comment_vote_object* CVO = nullptr;
   };

} } // steem::chain

