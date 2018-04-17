#pragma once

#include <steem/plugins/block_data_export/exportable_block_data.hpp>
#include <steem/plugins/witness/witness_objects.hpp>

namespace steem { namespace plugins { namespace witness {

using steem::plugins::block_data_export::exportable_block_data;

struct exp_bandwidth_update_object
{
   exp_bandwidth_update_object();
   exp_bandwidth_update_object( const account_bandwidth_object& bwo, uint32_t tsize );

   account_name_type account;
   bandwidth_type    type;
   share_type        average_bandwidth;
   share_type        lifetime_bandwidth;
   time_point_sec    last_bandwidth_update;
   uint32_t          tx_size = 0;
};

struct exp_reserve_ratio_object
{
   exp_reserve_ratio_object();
   exp_reserve_ratio_object( const reserve_ratio_object& rr, int32_t block_size );

   int32_t    average_block_size = 0;
   int64_t    current_reserve_ratio = 1;
   uint128_t  max_virtual_bandwidth = 0;
   int32_t    block_size = 0;
};

class exp_witness_data_object
   : public exportable_block_data
{
   public:
      exp_witness_data_object();
      virtual ~exp_witness_data_object();

      virtual void to_variant( fc::variant& v )const override
      {
         fc::to_variant( *this, v );
      }

      std::vector< exp_bandwidth_update_object >            bandwidth_updates;
      exp_reserve_ratio_object                              reserve_ratio;
};

} } }

FC_REFLECT( steem::plugins::witness::exp_bandwidth_update_object,
   (account)
   (type)
   (average_bandwidth)
   (lifetime_bandwidth)
   (last_bandwidth_update)
   (tx_size)
   )

FC_REFLECT( steem::plugins::witness::exp_reserve_ratio_object,
   (average_block_size)
   (current_reserve_ratio)
   (max_virtual_bandwidth)
   (block_size)
   )

FC_REFLECT( steem::plugins::witness::exp_witness_data_object,
   (bandwidth_updates)
   (reserve_ratio)
   )
