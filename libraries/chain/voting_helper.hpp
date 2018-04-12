#pragma once

#include <steem/chain/steem_object_types.hpp>

namespace steem { namespace protocol {
   class asset_symbol_type;
} }

namespace steem { namespace chain {

   class database;

   using steem::protocol::asset_symbol_type;

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
    *        last_vote_time - separate value per SMT (and separate for STEEM too)
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
   class i_voting_helper
   {
      public:
      virtual uint32_t get_minimal_vote_interval() const = 0;
      virtual uint32_t get_vote_regeneration_period() const = 0;
      virtual uint32_t get_votes_per_regeneration_period() const = 0;
      virtual uint32_t get_reverse_auction_window_seconds() = 0;

      virtual const time_point_sec& get_last_vote_time( const account_object& voter ) const = 0;

      virtual const share_type& get_comment_net_rshares( const comment_object& comment ) = 0;
      virtual const share_type& get_comment_vote_rshares( const comment_object& comment ) = 0;

      virtual void increase_comment_total_vote_weight( const comment_object& comment, uint64_t delta ) = 0;

      virtual const comment_vote_object& create_comment_vote_object( const account_id_type& voterId, const comment_id_type& commentId,
                                                                     int16_t opWeight ) = 0;

      virtual fc::uint128_t calculate_avg_cashout_sec( const comment_object& comment, const comment_object& root,
                                                       int64_t voter_abs_rshares, bool is_recast ) = 0;
      virtual uint64_t calculate_comment_vote_weight( const comment_object& comment, int64_t rshares, const share_type& old_vote_rshares ) = 0;

      virtual void update_voter_params( const account_object& voter, uint16_t newVotingPower, const time_point_sec& newLastVoteTime ) = 0;
      virtual void update_comment( const comment_object& comment, int64_t rshares, int64_t absRshares ) = 0;
      virtual void update_comment_recast( const comment_object& comment, const comment_vote_object& vote, 
                                          int64_t recast_rshares, int64_t vote_absRshares ) = 0;
      virtual void update_root_comment( const comment_object& root, int64_t vote_absRshares, const fc::uint128_t& avg_cashout_sec ) = 0;
      virtual void update_comment_vote_object( const comment_vote_object& cvo, uint64_t vote_weight, int64_t rshares ) = 0;
      virtual void update_comment_rshares2( const comment_object& c, const fc::uint128_t& old_rshares2,
                                            const fc::uint128_t& new_rshares2 ) = 0;
      virtual void update_vote( const comment_vote_object& vote, int64_t rshares, int16_t opWeight ) = 0;
   };

   /// Temporary implementation of i_voting_helper for STEEM.
   class t_steem_voting_helper: public i_voting_helper
   {
      public:
      t_steem_voting_helper(database& db);

      virtual uint32_t get_minimal_vote_interval() const override { return STEEM_MIN_VOTE_INTERVAL_SEC; }
      virtual uint32_t get_vote_regeneration_period() const override { return STEEM_VOTE_REGENERATION_SECONDS; }
      virtual uint32_t get_votes_per_regeneration_period() const override { return _votes_per_regeneration_period; }
      virtual uint32_t get_reverse_auction_window_seconds() override { return STEEM_REVERSE_AUCTION_WINDOW_SECONDS; }

      virtual const time_point_sec& get_last_vote_time( const account_object& voter ) const override;

      virtual const share_type& get_comment_net_rshares( const comment_object& comment ) override;
      virtual const share_type& get_comment_vote_rshares( const comment_object& comment ) override;
      virtual void increase_comment_total_vote_weight( const comment_object& comment, uint64_t delta ) override;
      virtual const comment_vote_object& create_comment_vote_object( const account_id_type& voterId, const comment_id_type& commentId,
                                                                     int16_t opWeight ) override;
      virtual fc::uint128_t calculate_avg_cashout_sec( const comment_object& comment, const comment_object& root,
                                                       int64_t voter_abs_rshares, bool is_recast ) override;
      virtual uint64_t calculate_comment_vote_weight( const comment_object& comment, int64_t rshares, 
                                                      const share_type& old_vote_rshares ) override;
      virtual void update_voter_params( const account_object& voter, uint16_t newVotingPower, const time_point_sec& newLastVoteTime ) override;
      virtual void update_comment( const comment_object& comment, int64_t vote_rshares, int64_t vote_absRshares ) override;
      virtual void update_comment_recast( const comment_object& comment, const comment_vote_object& vote, 
                                          int64_t recast_rshares, int64_t vote_absRshares ) override;
      virtual void update_root_comment( const comment_object& root, int64_t vote_absRshares, const fc::uint128_t& avg_cashout_sec ) override;
      virtual void update_comment_vote_object( const comment_vote_object& cvo, uint64_t vote_weight, int64_t rshares ) override;
      virtual void update_comment_rshares2( const comment_object& c, const fc::uint128_t& old_rshares2, const fc::uint128_t& new_rshares2 ) override;
      virtual void update_vote( const comment_vote_object& vote, int64_t rshares, int16_t opWeight ) override;

      private:
      database&                  _db;
      uint32_t                   _votes_per_regeneration_period = 0;
      const comment_vote_object* _comment_vote_object = nullptr;
   };

   /// Empty implementation of i_voting_helper for SMT. \warning Throws exception at every attempt of using.
   class t_smt_voting_helper: public i_voting_helper
   {
      public:
      t_smt_voting_helper( const comment_vote_object& cvo, database& db, const asset_symbol_type& symbol,
                           const share_type& max_accepted_payout, bool allowed_curation_awards );

      virtual uint32_t get_minimal_vote_interval() const override;
      virtual uint32_t get_vote_regeneration_period() const override;
      virtual uint32_t get_votes_per_regeneration_period() const override;
      virtual uint32_t get_reverse_auction_window_seconds() override;

      virtual const time_point_sec& get_last_vote_time( const account_object& voter ) const override;

      virtual const share_type& get_comment_net_rshares( const comment_object& comment ) override;
      virtual const share_type& get_comment_vote_rshares( const comment_object& comment ) override;
      virtual void increase_comment_total_vote_weight( const comment_object& comment, uint64_t delta ) override;
      virtual const comment_vote_object& create_comment_vote_object( const account_id_type& voterId, const comment_id_type& commentId,
                                                                     int16_t opWeight ) override;
      virtual fc::uint128_t calculate_avg_cashout_sec( const comment_object& comment, const comment_object& root,
                                                       int64_t voter_abs_rshares, bool is_recast ) override;
      virtual uint64_t calculate_comment_vote_weight( const comment_object& comment, int64_t rshares, 
                                                      const share_type& old_vote_rshares ) override;
      virtual void update_voter_params( const account_object& voter, uint16_t newVotingPower,
                                        const time_point_sec& newLastVoteTime) override;
      virtual void update_comment( const comment_object& comment, int64_t vote_rshares, int64_t vote_absRshares ) override;
      virtual void update_comment_recast( const comment_object& comment, const comment_vote_object& vote, 
                                          int64_t recast_rshares, int64_t vote_absRshares ) override;
      virtual void update_root_comment( const comment_object& root, int64_t vote_absRshares, const fc::uint128_t& avg_cashout_sec ) override;
      virtual void update_comment_vote_object( const comment_vote_object& cvo, uint64_t vote_weight, int64_t rshares ) override;
      virtual void update_comment_rshares2( const comment_object& c, const fc::uint128_t& old_rshares2,
                                            const fc::uint128_t& new_rshares2 ) override;
      virtual void update_vote( const comment_vote_object& vote, int64_t rshares, int16_t opWeight ) override;

      private:
      //database&                  _db;
      //uint32_t                   _votes_per_regeneration_period = 0;
      const comment_vote_object& _comment_vote_object;
   };

} } // steem::chain

