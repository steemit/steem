#pragma once

#include <cstdint>

#include <fc/saturation.hpp>
#include <fc/uint128.hpp>
#include <fc/time.hpp>
#include <fc/exception/exception.hpp>
#include <fc/reflect/reflect.hpp>

namespace steem { namespace chain { namespace util {

struct manabar_params
{
   int64_t    max_mana         = 0;
   uint32_t   regen_time       = 0;

   manabar_params() {}
   manabar_params( int64_t mm, uint32_t rt )
      : max_mana(mm), regen_time(rt) {}

   void validate()const
   { try{
      FC_ASSERT( max_mana >= 0 );
   } FC_CAPTURE_AND_RETHROW( (max_mana) ) }
};

struct manabar
{
   int64_t    current_mana     = 0;
   uint32_t   last_update_time = 0;

   manabar() {}
   manabar( int64_t m, uint32_t t )
      : current_mana(m), last_update_time(t) {}

   template< bool skip_cap_regen = false >
   void regenerate_mana( const manabar_params& params, uint32_t now )
   {
      params.validate();

      FC_ASSERT( now >= last_update_time );
      uint32_t dt = now - last_update_time;
      if( current_mana >= params.max_mana )
      {
         current_mana = params.max_mana;
         last_update_time = now;
         return;
      }

      if( !skip_cap_regen )
         dt = (dt > params.regen_time) ? params.regen_time : dt;

      fc::uint128_t max_mana_dt = uint64_t( params.max_mana >= 0 ? params.max_mana : 0 );
      max_mana_dt *= dt;
      uint64_t u_regen = (max_mana_dt / params.regen_time).to_uint64();
      FC_ASSERT( u_regen <= std::numeric_limits< int64_t >::max() );
      int64_t new_current_mana = fc::signed_sat_add( current_mana, int64_t( u_regen ) );
      current_mana = (new_current_mana > params.max_mana) ? params.max_mana : new_current_mana;

      last_update_time = now;
   }

   template< bool skip_cap_regen = false >
   void regenerate_mana( const manabar_params& params, fc::time_point_sec now )
   {
      regenerate_mana< skip_cap_regen >( params, now.sec_since_epoch() );
   }

   bool has_mana( int64_t mana_needed )const
   {
      return (mana_needed <= 0) || (current_mana >= mana_needed);
   }

   bool has_mana( uint64_t mana_needed )const
   {
      FC_ASSERT( mana_needed <= std::numeric_limits< int64_t >::max() );
      return has_mana( (int64_t) mana_needed );
   }

   void use_mana( int64_t mana_used, int64_t min_mana = std::numeric_limits< uint64_t >::min() )
   {
      current_mana = fc::signed_sat_sub( current_mana, mana_used );

      if( current_mana < min_mana )
      {
         current_mana = min_mana;
      }
   }

   void use_mana( uint64_t mana_used, int64_t min_mana = std::numeric_limits< uint64_t >::min() )
   {
      FC_ASSERT( mana_used <= std::numeric_limits< int64_t >::max() );
      use_mana( (int64_t) mana_used, min_mana );
   }
};

template< typename T >
int64_t get_effective_vesting_shares( const T& account )
{
   int64_t effective_vesting_shares =
        account.vesting_shares.amount.value              // base vesting shares
      + account.received_vesting_shares.amount.value     // incoming delegations
      - account.delegated_vesting_shares.amount.value;   // outgoing delegations

   // If there is a power down occuring, also reduce effective vesting shares by this week's power down amount
   if( account.next_vesting_withdrawal != fc::time_point_sec::maximum() )
   {
      effective_vesting_shares -=
         std::min(
            account.vesting_withdraw_rate.amount.value,           // Weekly amount
            account.to_withdraw.value - account.withdrawn.value   // Or remainder
            );
   }

   return effective_vesting_shares;
}

template< typename PropType, typename AccountType >
void update_manabar( const PropType& gpo, AccountType& account, int32_t mana_regen_seconds, bool downvote_mana = false, int64_t new_mana = 0 )
{
   auto effective_vests = util::get_effective_vesting_shares( account );
   try {
   manabar_params params( effective_vests, mana_regen_seconds );
   account.voting_manabar.regenerate_mana( params, gpo.time );
   account.voting_manabar.use_mana( -new_mana );
   } FC_CAPTURE_LOG_AND_RETHROW( (account)(effective_vests) )

   FC_TODO( "This hardfork check should not be needed. Remove after HF21 if that is the case." );
   // This is used as a hardfork check. Can be replaced with if( gpo.downvote_pool_percent ). Leaving as a hard check to be safe until after HF 21
   try{
   if( downvote_mana )
   {
      manabar_params params;
      params.regen_time = mana_regen_seconds;
      params.max_mana = ( ( fc::uint128_t( effective_vests ) * gpo.downvote_pool_percent ) / STEEM_100_PERCENT ).to_int64();
      account.downvote_manabar.regenerate_mana( params, gpo.time );
      account.downvote_manabar.use_mana( ( -new_mana * gpo.downvote_pool_percent ) / STEEM_100_PERCENT );
   }
   } FC_CAPTURE_LOG_AND_RETHROW( (account)(effective_vests) )
}

} } } // steem::chain::util

FC_REFLECT( steem::chain::util::manabar,
   (current_mana)
   (last_update_time)
   )
