#pragma once
#include <fc/uint128.hpp>

#include <steemit/chain/protocol/asset.hpp>
#include <steemit/chain/protocol/types.hpp>
#include <steemit/chain/database.hpp>
#include <graphene/db/object.hpp>

namespace steemit { namespace chain {


   /**
    * @class dynamic_global_property_object
    * @brief Maintains global state information
    * @ingroup object
    * @ingroup implementation
    *
    * This is an implementation detail. The values here are calculated during normal chain operations and reflect the
    * current values of global blockchain properties.
    */
   class dynamic_global_property_object : public abstract_object<dynamic_global_property_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_dynamic_global_property_object_type;

         uint32_t          head_block_number = 0;
         block_id_type     head_block_id;
         time_point_sec    time;
         string            current_witness;


         /**
          *  The total POW accumulated, aka the sum of num_pow_witness at the time new POW is added
          */
         uint64_t total_pow = -1;

         /**
          * The current count of how many pending POW witnesses there are, determines the difficulty
          * of doing pow
          */
         uint32_t num_pow_witnesses = 0;

         asset       virtual_supply             = asset( 0, STEEM_SYMBOL );
         asset       current_supply             = asset( 0, STEEM_SYMBOL );
         asset       confidential_supply        = asset( 0, STEEM_SYMBOL ); ///< total asset held in confidential balances
         asset       current_sbd_supply         = asset( 0, SBD_SYMBOL );
         asset       confidential_sbd_supply    = asset( 0, SBD_SYMBOL ); ///< total asset held in confidential balances
         asset       total_vesting_fund_steem   = asset( 0, STEEM_SYMBOL );
         asset       total_vesting_shares       = asset( 0, VESTS_SYMBOL );
         asset       total_reward_fund_steem    = asset( 0, STEEM_SYMBOL );
         fc::uint128 total_reward_shares2; ///< the running total of REWARD^2

         /**
          *  These fields are used to reward users who remain active. Every block 15% of
          *  content + curation rewards steem is placed into the activity fund steem.
          *
          *  Every time a user votes they earn activity_fund_shares which are calculated as
          *  min(time_since_last_active,24h) * VESTS.  This is designed to reward those who
          *  check in daily.  Activity rewards can only be earned by accounts with trival
          *  posting authorities consisting of a single key.
          */
         //@{
         asset       total_activity_fund_steem  = asset( 0, STEEM_SYMBOL );
         fc::uint128 total_activity_fund_shares = 0;
         //@}

         price       get_vesting_share_price() const
         {
            if ( total_vesting_fund_steem.amount == 0 || total_vesting_shares.amount == 0 )
               return price ( asset( 1000, STEEM_SYMBOL ), asset( 1000000, VESTS_SYMBOL ) );

            return price( total_vesting_shares, total_vesting_fund_steem );
         }

         /**
          *  This property defines the interest rate that SBD deposits receive.
          */
         uint16_t sbd_interest_rate = 0;

         /**
          *  Average block size is updated every block to be:
          *
          *     average_block_size = (99 * average_block_size + new_block_size) / 100
          *
          *  This property is used to update the current_reserve_ratio to maintain approximately
          *  50% or less utilization of network capacity.
          */
         uint32_t     average_block_size = 0;

         /**
          *  Maximum block size is decided by the set of active witnesses which change every round.
          *  Each witness posts what they think the maximum size should be as part of their witness
          *  properties, the median size is chosen to be the maximum block size for the round.
          *
          *  @note the minimum value for maximum_block_size is defined by the protocol to prevent the
          *  network from getting stuck by witnesses attempting to set this too low.
          */
         uint32_t     maximum_block_size = 0;

         /**
          * The current absolute slot number.  Equal to the total
          * number of slots since genesis.  Also equal to the total
          * number of missed slots plus head_block_number.
          */
         uint64_t      current_aslot = 0;

         /**
          * used to compute witness participation.
          */
         fc::uint128_t recent_slots_filled;
         uint8_t       participation_count; ///< Divide by 128 to compute participation percentage

         uint32_t last_irreversible_block_num = 0;

         /**
          * The maximum bandwidth the blockchain can support is:
          *
          *    max_bandwidth = maximum_block_size * STEEMIT_BANDWIDTH_AVERAGE_WINDOW_SECONDS / STEEMIT_BLOCK_INTERVAL
          *
          * The maximum virtual bandwidth is:
          *
          *    max_bandwidth * current_reserve_ratio
          */
         uint64_t max_virtual_bandwidth = 0;

         /**
          *   Any time average_block_size <= 50% maximum_block_size this value grows by 1 until it
          *   reaches STEEMIT_MAX_RESERVE_RATIO.  Any time average_block_size is greater than
          *   50% it falls by 1%.  Upward adjustments happen once per round, downward adjustments
          *   happen every block.
          */
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
                    (confidential_supply)
                    (current_sbd_supply)
                    (confidential_sbd_supply)
                    (total_vesting_fund_steem)
                    (total_vesting_shares)
                    (total_reward_fund_steem)
                    (total_reward_shares2)
                    (total_activity_fund_steem)
                    (total_activity_fund_shares)
                    (sbd_interest_rate)
                    (average_block_size)
                    (maximum_block_size)
                    (current_aslot)
                    (recent_slots_filled)
                    (participation_count)
                    (last_irreversible_block_num)
                    (max_virtual_bandwidth)
                    (current_reserve_ratio)
                  )

