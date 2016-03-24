#pragma once
#include <fc/uint128.hpp>

#include <steemit/chain/protocol/types.hpp>
#include <steemit/chain/database.hpp>
#include <graphene/db/object.hpp>

namespace steemit { namespace chain {
   class dynamic_global_property_object : public abstract_object<dynamic_global_property_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_dynamic_global_property_object_type;

         uint32_t          head_block_number = 0;
         block_id_type     head_block_id;
         time_point_sec    time;
         string            current_witness;


         uint64_t total_pow = 0;

         uint32_t num_pow_witnesses = 0;

         asset       virtual_supply             = asset( 0, STEEM_SYMBOL );
         asset       current_supply             = asset( 0, STEEM_SYMBOL );
         asset       current_sbd_supply         = asset( 0, SBD_SYMBOL );
         asset       total_vesting_fund_steem   = asset( 0, STEEM_SYMBOL );
         asset       total_vesting_shares       = asset( 0, VESTS_SYMBOL );
         asset       total_reward_fund_steem    = asset( 0, STEEM_SYMBOL);
         fc::uint128 total_reward_shares2; ///< the running total of REWARD^2

         price       get_vesting_share_price() const
         {
            if ( total_vesting_fund_steem.amount == 0 || total_vesting_shares.amount == 0 )
               return price ( asset( 1000, STEEM_SYMBOL ), asset( 1000000, VESTS_SYMBOL ) );

            return price( total_vesting_shares, total_vesting_fund_steem );
         }

         uint16_t sbd_interest_rate = 0;

         uint32_t     average_block_size = 0;

         uint32_t     maximum_block_size = 0;

         uint64_t      current_aslot = 0;

         fc::uint128_t recent_slots_filled;

         uint32_t last_irreversible_block_num = 0;

         uint64_t max_virtual_bandwidth = 0;

         uint64_t current_reserve_ratio = 1;
   };
}}

FC_REFLECT_DERIVED( steemit::chain::dynamic_global_property_object, (graphene::db::object),
                    (head_block_number)
                    (head_block_id)
                    (time)
                    (current_witness)
                    (total_pow)
                    (num_pow_witnesses)
                    (virtual_supply)
                    (current_supply)
                    (current_sbd_supply)
                    (total_vesting_fund_steem)
                    (total_vesting_shares)
                    (total_reward_fund_steem)
                    (total_reward_shares2)
                    (sbd_interest_rate)
                    (average_block_size)
                    (maximum_block_size)
                    (current_aslot)
                    (recent_slots_filled)
                    (last_irreversible_block_num)
                    (max_virtual_bandwidth)
                    (current_reserve_ratio)
                  )

