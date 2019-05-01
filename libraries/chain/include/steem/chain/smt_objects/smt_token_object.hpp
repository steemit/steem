#pragma once

#include <steem/chain/steem_fwd.hpp>
#include <steem/chain/steem_object_types.hpp>
#include <steem/protocol/smt_operations.hpp>

#ifdef STEEM_ENABLE_SMT

namespace steem { namespace chain {

using protocol::curve_id;

enum class smt_phase : uint8_t
{
   account_elevated,
   setup_completed,
   contribution_begin_time_completed,
   contribution_end_time_completed,
   launch_time_completed,              /// launch window opened
   launch_failed,                      /// launch window closed with either not enough contributions or some cap not revealed
   launch_success                      /// enough contributions were declared and caps revealed before launch windows closed
};

/**Note that the object represents both liquid and vesting variant of SMT.
 * The same object is returned by indices when searched by liquid/vesting symbol/nai.
 */
class smt_token_object : public object< smt_token_object_type, smt_token_object >
{
   STEEM_STD_ALLOCATOR_CONSTRUCTOR( smt_token_object );

public:

   struct smt_market_maker_state
   {
      asset    steem_balance;
      asset    token_balance;
      uint32_t reserve_ratio = 0;
   };

public:

   template< typename Constructor, typename Allocator >
   smt_token_object( Constructor&& c, allocator< Allocator > a )
   {
      c( *this );
   }

   price    one_vesting_to_one_liquid() const
   {
      int64_t one_smt = std::pow(10, liquid_symbol.decimals());
      return price ( asset( one_smt, liquid_symbol.get_paired_symbol() ), asset( one_smt, liquid_symbol ) );
      // ^ On the assumption that liquid and vesting SMT have the same precision. See issue 2212
   }

   price    get_vesting_share_price() const
   {
      if ( total_vesting_fund_smt == 0 || total_vesting_shares == 0 )
         return one_vesting_to_one_liquid();
         // ^ In original method of globa_property_object it was one liquid to one vesting which seems to be a bug.

      return price( asset( total_vesting_shares, liquid_symbol.get_paired_symbol() ), asset( total_vesting_fund_smt, liquid_symbol ) );
   }

   price    get_reward_vesting_share_price() const
   {
      share_type reward_vesting_shares = total_vesting_shares + pending_rewarded_vesting_shares;
      share_type reward_vesting_smt = total_vesting_fund_smt + pending_rewarded_vesting_smt;

      if( reward_vesting_shares == 0 || reward_vesting_smt == 0 )
          return one_vesting_to_one_liquid();
      // ^ Additional check not found in original get_reward_vesting_share_price. See issue 2212

      return price( asset( reward_vesting_shares, liquid_symbol.get_paired_symbol() ), asset( reward_vesting_smt, liquid_symbol ) );
   }

   // id_type is actually oid<smt_token_object>
   id_type              id;

   /**The object represents both liquid and vesting variant of SMT
    * To get vesting symbol, call liquid_symbol.get_paired_symbol()
    */
   asset_symbol_type    liquid_symbol;
   account_name_type    control_account;
   smt_phase            phase = smt_phase::account_elevated;

   share_type           current_supply = 0;
   share_type           total_vesting_fund_smt = 0;
   share_type           total_vesting_shares = 0;
   share_type           pending_rewarded_vesting_shares = 0;
   share_type           pending_rewarded_vesting_smt = 0;

   smt_market_maker_state  market_maker;

   /// set_setup_parameters
   bool                 allow_voting = true;

   /// set_runtime_parameters
   uint32_t             cashout_window_seconds = STEEM_CASHOUT_WINDOW_SECONDS;
   uint32_t             reverse_auction_window_seconds = STEEM_REVERSE_AUCTION_WINDOW_SECONDS_HF20;

   uint32_t             vote_regeneration_period_seconds = STEEM_VOTING_MANA_REGENERATION_SECONDS;
   uint32_t             votes_per_regeneration_period = SMT_DEFAULT_VOTES_PER_REGEN_PERIOD;

   uint128_t            content_constant = STEEM_CONTENT_CONSTANT_HF0;
   uint16_t             percent_curation_rewards = SMT_DEFAULT_PERCENT_CURATION_REWARDS;
   protocol::curve_id   author_reward_curve = curve_id::linear;
   protocol::curve_id   curation_reward_curve = curve_id::square_root;

   bool                 allow_downvotes = true;

   ///parameters for 'smt_setup_operation'
   int64_t                       max_supply = 0;
   steem::protocol::
   smt_capped_generation_policy  capped_generation_policy;
   time_point_sec                generation_begin_time;
   time_point_sec                generation_end_time;
   time_point_sec                announced_launch_time;
   time_point_sec                launch_expiration_time;

   // smt_cap_reveal
   share_type  steem_units_min_cap = -1;
   share_type  steem_units_hard_cap = -1;
};

class smt_token_emissions_object : public object< smt_token_emissions_object_type, smt_token_emissions_object >
{
   STEEM_STD_ALLOCATOR_CONSTRUCTOR( smt_token_emissions_object );

public:
   template< typename Constructor, typename Allocator >
   smt_token_emissions_object( Constructor&& c, allocator< Allocator > a )
   {
      c( *this );
   }

   id_type                               id;
   asset_symbol_type                     symbol;
   time_point_sec                        schedule_time = STEEM_GENESIS_TIME;
   steem::protocol::smt_emissions_unit   emissions_unit;
   uint32_t                              interval_seconds = 0;
   uint32_t                              interval_count = 0;
   time_point_sec                        lep_time = STEEM_GENESIS_TIME;
   time_point_sec                        rep_time = STEEM_GENESIS_TIME;
   asset                                 lep_abs_amount = asset();
   asset                                 rep_abs_amount = asset();
   uint32_t                              lep_rel_amount_numerator = 0;
   uint32_t                              rep_rel_amount_numerator = 0;
   uint8_t                               rel_amount_denom_bits = 0;
};

class smt_event_token_object : public object< smt_event_token_object_type, smt_event_token_object >
{
   STEEM_STD_ALLOCATOR_CONSTRUCTOR( smt_event_token_object );

public:
   template< typename Constructor, typename Allocator >
   smt_event_token_object( Constructor&& c, allocator< Allocator > a )
   {
      c( *this );
   }

   // id_type is actually oid<smt_event_token_object>
   id_type           id;

   smt_token_id_type             parent;

   smt_phase                     phase = smt_phase::setup_completed;

   time_point_sec                generation_begin_time;
   time_point_sec                generation_end_time;
   time_point_sec                announced_launch_time;
   time_point_sec                launch_expiration_time;
};

struct by_symbol;
struct by_control_account;

typedef multi_index_container <
   smt_token_object,
   indexed_by <
      ordered_unique< tag< by_id >,
         member< smt_token_object, smt_token_id_type, &smt_token_object::id > >,
      ordered_unique< tag< by_symbol >,
         member< smt_token_object, asset_symbol_type, &smt_token_object::liquid_symbol > >,
      ordered_unique< tag< by_control_account >,
         composite_key< smt_token_object,
            member< smt_token_object, account_name_type, &smt_token_object::control_account >,
            member< smt_token_object, asset_symbol_type, &smt_token_object::liquid_symbol >
         >
      >
   >,
   allocator< smt_token_object >
> smt_token_index;

struct by_symbol_time;

typedef multi_index_container <
   smt_token_emissions_object,
   indexed_by <
      ordered_unique< tag< by_id >,
         member< smt_token_emissions_object, smt_token_emissions_object_id_type, &smt_token_emissions_object::id > >,
      ordered_unique< tag< by_symbol_time >,
         composite_key< smt_token_emissions_object,
            member< smt_token_emissions_object, asset_symbol_type, &smt_token_emissions_object::symbol >,
            member< smt_token_emissions_object, time_point_sec, &smt_token_emissions_object::schedule_time >
         >
      >
   >,
   allocator< smt_token_emissions_object >
> smt_token_emissions_index;

struct by_interval_gen_begin;
struct by_interval_gen_end;
struct by_interval_launch;
struct by_interval_launch_exp;
typedef multi_index_container <
   smt_event_token_object,
   indexed_by <
      ordered_unique< tag< by_id >,
         member< smt_event_token_object, smt_event_token_id_type, &smt_event_token_object::id > >,

      ordered_unique< tag< by_interval_gen_begin >,
         composite_key< smt_event_token_object,
            member< smt_event_token_object, smt_phase, &smt_event_token_object::phase >,
            member< smt_event_token_object, time_point_sec, &smt_event_token_object::generation_begin_time >,
            member< smt_event_token_object, smt_event_token_id_type, &smt_event_token_object::id >
         >
      >,
      ordered_unique< tag< by_interval_gen_end >,
         composite_key< smt_event_token_object,
            member< smt_event_token_object, smt_phase, &smt_event_token_object::phase >,
            member< smt_event_token_object, time_point_sec, &smt_event_token_object::generation_end_time >,
            member< smt_event_token_object, smt_event_token_id_type, &smt_event_token_object::id >
         >
      >,
      ordered_unique< tag< by_interval_launch >,
         composite_key< smt_event_token_object,
            member< smt_event_token_object, smt_phase, &smt_event_token_object::phase >,
            member< smt_event_token_object, time_point_sec, &smt_event_token_object::announced_launch_time >,
            member< smt_event_token_object, smt_event_token_id_type, &smt_event_token_object::id >
         >
      >,
      ordered_unique< tag< by_interval_launch_exp >,
         composite_key< smt_event_token_object,
            member< smt_event_token_object, smt_phase, &smt_event_token_object::phase >,
            member< smt_event_token_object, time_point_sec, &smt_event_token_object::launch_expiration_time >,
            member< smt_event_token_object, smt_event_token_id_type, &smt_event_token_object::id >
         >
      >
   >,
   allocator< smt_event_token_object >
> smt_event_token_index;

} } // namespace steem::chain

FC_REFLECT_ENUM( steem::chain::smt_phase,
                  (account_elevated)
                  (setup_completed)
                  (contribution_begin_time_completed)
                  (contribution_end_time_completed)
                  (launch_time_completed)
                  (launch_failed)
                  (launch_success)
)

FC_REFLECT( steem::chain::smt_token_object::smt_market_maker_state,
   (steem_balance)
   (token_balance)
   (reserve_ratio)
)

FC_REFLECT( steem::chain::smt_token_object,
   (id)
   (liquid_symbol)
   (control_account)
   (phase)
   (current_supply)
   (total_vesting_fund_smt)
   (total_vesting_shares)
   (pending_rewarded_vesting_shares)
   (pending_rewarded_vesting_smt)
   (allow_downvotes)
   (market_maker)
   (allow_voting)
   (cashout_window_seconds)
   (reverse_auction_window_seconds)
   (vote_regeneration_period_seconds)
   (votes_per_regeneration_period)
   (content_constant)
   (percent_curation_rewards)
   (author_reward_curve)
   (curation_reward_curve)
   (max_supply)
   (capped_generation_policy)
   (generation_begin_time)
   (generation_end_time)
   (announced_launch_time)
   (launch_expiration_time)
   (steem_units_min_cap)
   (steem_units_hard_cap)
)

FC_REFLECT( steem::chain::smt_token_emissions_object,
   (id)
   (symbol)
   (schedule_time)
   (emissions_unit)
   (interval_seconds)
   (interval_count)
   (lep_time)
   (rep_time)
   (lep_abs_amount)
   (rep_abs_amount)
   (lep_rel_amount_numerator)
   (rep_rel_amount_numerator)
   (rel_amount_denom_bits)
)

FC_REFLECT( steem::chain::smt_event_token_object,
   (id)
   (parent)
   (phase)
   (generation_begin_time)
   (generation_end_time)
   (announced_launch_time)
   (launch_expiration_time)
)

CHAINBASE_SET_INDEX_TYPE( steem::chain::smt_token_object, steem::chain::smt_token_index )
CHAINBASE_SET_INDEX_TYPE( steem::chain::smt_token_emissions_object, steem::chain::smt_token_emissions_index )
CHAINBASE_SET_INDEX_TYPE( steem::chain::smt_event_token_object, steem::chain::smt_event_token_index )

#endif
