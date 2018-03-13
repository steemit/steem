#pragma once

#include <steem/chain/steem_object_types.hpp>
#include <steem/protocol/smt_operations.hpp>

#ifdef STEEM_ENABLE_SMT

namespace steem { namespace chain {

/**Note that the object represents both liquid and vesting variant of SMT.
 * The same object is returned by indices when searched by liquid/vesting symbol/nai.
 */
class smt_token_object : public object< smt_token_object_type, smt_token_object >
{
   smt_token_object() = delete;

public:
   enum class smt_phase : unsigned char
   {
      account_elevated,
      setup_completed,
   };

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

   uint32_t get_liquid_nai() const
   {
      return liquid_symbol.to_nai();
   }

   price    one_liquid_to_one_vesting() const
   {
      int64_t one_smt = std::pow(10, liquid_symbol.decimals());
      return price ( asset( one_smt, liquid_symbol ), asset( one_smt, liquid_symbol.get_paired_symbol() ) );
      // ^ On the assumption that liquid and vesting SMT have the same precision. See issue 2212
   }

   price    get_vesting_share_price() const
   {
      if ( total_vesting_fund_smt == 0 || total_vesting_shares == 0 )
         return one_liquid_to_one_vesting();

      return price( asset( total_vesting_shares, liquid_symbol.get_paired_symbol() ), asset( total_vesting_fund_smt, liquid_symbol ) );
   }

   price    get_reward_vesting_share_price() const
   {
      share_type reward_vesting_shares = total_vesting_shares + pending_rewarded_vesting_shares;
      share_type reward_vesting_smt = total_vesting_fund_smt + pending_rewarded_vesting_smt;

      if( reward_vesting_shares == 0 || reward_vesting_smt == 0 )
          return one_liquid_to_one_vesting();
      // ^ Additional check not found in original get_reward_vesting_share_price. See issue 2212

      return price( asset( reward_vesting_shares, liquid_symbol.get_paired_symbol() ), asset( reward_vesting_smt, liquid_symbol ) );
   }

   // id_type is actually oid<smt_token_object>
   id_type           id;

   /**The object represents both liquid and vesting variant of SMT
    * To get vesting symbol, call liquid_symbol.get_paired_symbol()
    */
   asset_symbol_type liquid_symbol;
   account_name_type control_account;
   smt_phase         phase = smt_phase::account_elevated;

   share_type  current_supply = 0;
   share_type  total_vesting_fund_smt = 0;
   share_type  total_vesting_shares = 0;
   share_type  pending_rewarded_vesting_shares = 0;
   share_type  pending_rewarded_vesting_smt = 0;

   smt_market_maker_state  market_maker;

   /// set_setup_parameters
   bool              allow_voting = false;
   bool              allow_vesting = false;

   /// set_runtime_parameters
   uint32_t cashout_window_seconds = 0;
   uint32_t reverse_auction_window_seconds = 0;

   uint32_t vote_regeneration_period_seconds = 0;
   uint32_t votes_per_regeneration_period = 0;

   uint128_t content_constant = 0;
   uint16_t percent_curation_rewards = 0;
   uint16_t percent_content_rewards = 0;
   protocol::curve_id author_reward_curve;
   protocol::curve_id curation_reward_curve;

   /// smt_setup_emissions
   time_point_sec       schedule_time = STEEM_GENESIS_TIME;
   steem::protocol::
   smt_emissions_unit   emissions_unit;
   uint32_t             interval_seconds = 0;
   uint32_t             interval_count = 0;
   time_point_sec       lep_time = STEEM_GENESIS_TIME;
   time_point_sec       rep_time = STEEM_GENESIS_TIME;
   asset                lep_abs_amount = asset( 0, STEEM_SYMBOL );
   asset                rep_abs_amount = asset( 0, STEEM_SYMBOL );
   uint32_t             lep_rel_amount_numerator = 0;
   uint32_t             rep_rel_amount_numerator = 0;
   uint8_t              rel_amount_denom_bits = 0;
};

struct by_symbol;
struct by_nai;
struct by_control_account;

/**Comparison operators that allow to return the same object representation
 * for both liquid and vesting symbol/nai.
 */
struct vesting_liquid_less
{
   bool operator ()( const asset_symbol_type& lhs, const asset_symbol_type& rhs ) const
   {
      // Compare as if both symbols represented liquid version.
      return ( lhs.is_vesting() ? lhs.get_paired_symbol() : lhs ) <
             ( rhs.is_vesting() ? rhs.get_paired_symbol() : rhs );
   }

   bool operator ()( const uint32_t& lhs, const uint32_t& rhs ) const
   {
      // Use the other operator, adding the same precision to both NAIs.
      return operator()( asset_symbol_type::from_nai( lhs, 0 ) , asset_symbol_type::from_nai( rhs, 0 ) );
   }
};

typedef multi_index_container <
   smt_token_object,
   indexed_by <
      ordered_unique< tag< by_id >,
         member< smt_token_object, smt_token_id_type, &smt_token_object::id > >,
      ordered_unique< tag< by_symbol >,
         member< smt_token_object, asset_symbol_type, &smt_token_object::liquid_symbol >, vesting_liquid_less >,
      ordered_unique< tag< by_nai >,
         const_mem_fun< smt_token_object, uint32_t, &smt_token_object::get_liquid_nai >, vesting_liquid_less >,
      ordered_non_unique< tag< by_control_account >,
         member< smt_token_object, account_name_type, &smt_token_object::control_account > >
   >,
   allocator< smt_token_object >
> smt_token_index;

} } // namespace steem::chain

FC_REFLECT_ENUM( steem::chain::smt_token_object::smt_phase,
                 (account_elevated)
                 (setup_completed)
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
   (market_maker)
   (allow_voting)
   (allow_vesting)
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

CHAINBASE_SET_INDEX_TYPE( steem::chain::smt_token_object, steem::chain::smt_token_index )

#endif
