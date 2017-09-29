#pragma once

#include <steem/chain/steem_object_types.hpp>

#ifdef STEEM_ENABLE_SMT

namespace steem { namespace chain {

class smt_token_object : public object< smt_token_object_type, smt_token_object >
{
   smt_token_object() = delete;

   public:
      enum class smt_phase : unsigned char
      {
         account_elevated,
         setup_completed,
      };

   public:
      template< typename Constructor, typename Allocator >
      smt_token_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      // id_type is actually oid<smt_token_object>
      id_type           id;

      account_name_type control_account;

      uint32_t cashout_window_seconds = 0;
      uint32_t reverse_auction_window_seconds = 0;

      uint32_t vote_regeneration_period_seconds = 0;
      uint32_t votes_per_regeneration_period = 0;

      uint128_t content_constant = 0;
      uint16_t percent_curation_rewards = 0;
      uint16_t percent_content_rewards = 0;
      utilities::curve_id author_reward_curve;
      utilities::curve_id curation_reward_curve;

      smt_phase         phase = smt_phase::account_elevated;

      /// set_setup_parameters
      bool              allow_voting = false;
      bool              allow_vesting = false;
   };


struct by_control_account;

typedef multi_index_container <
   smt_token_object,
   indexed_by <
      ordered_unique< tag< by_id >,
         member< smt_token_object, smt_token_id_type, &smt_token_object::id > >,
      ordered_unique< tag< by_control_account >,
         member< smt_token_object, account_name_type, &smt_token_object::control_account > >
   >,
   allocator< smt_token_object >
> smt_token_index;

} } // namespace steem::chain

FC_REFLECT_ENUM( steem::chain::smt_token_object::smt_phase,
                 (account_elevated)
                 (setup_completed)
)

FC_REFLECT( steem::chain::smt_token_object,
   (id)
   (control_account)
   (phase)
   (allow_voting)
   (allow_vesting)
)

CHAINBASE_SET_INDEX_TYPE( steem::chain::smt_token_object, steem::chain::smt_token_index )

#endif
