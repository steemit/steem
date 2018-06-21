#pragma once

#include <cstdint>

#include <fc/saturation.hpp>
#include <fc/uint128.hpp>
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
   {
      FC_ASSERT( max_mana >= 0 );
   }
};

struct manabar
{
   int64_t    current_mana     = 0;
   uint32_t   last_update_time = 0;

   manabar() {}
   manabar( int64_t m, uint32_t t )
      : current_mana(m), last_update_time(t) {}

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

      dt = (dt > params.regen_time) ? params.regen_time : dt;

      uint128_t max_mana_dt = uint64_t( params.max_mana >= 0 ? params.max_mana : 0 );
      max_mana_dt *= dt;
      uint64_t u_regen = (max_mana_dt / params.regen_time).to_uint64();
      FC_ASSERT( u_regen <= std::numeric_limits< int64_t >::max() );
      int64_t new_current_mana = fc::signed_sat_add( current_mana, int64_t( u_regen ) );
      current_mana = (new_current_mana > params.max_mana) ? params.max_mana : new_current_mana;

      last_update_time = now;
   }

   bool has_mana( int64_t mana_needed )const
   {
      return (mana_needed <= 0) || (current_mana >= mana_needed);
   }

   void use_mana( int64_t mana_used )
   {
      current_mana = fc::signed_sat_sub( current_mana, mana_used );
   }
};

} } } // steem::chain::util

FC_REFLECT( steem::chain::util::manabar,
   (current_mana)
   (last_update_time)
   )
