
#pragma once

#include <steemit/chain/steem_object_types.hpp>

namespace steemit { namespace plugin { namespace block_info {

struct block_info
{
   chain::block_id_type      block_id;
   uint32_t                  block_size                  = 0;
   uint64_t                  aslot                       = 0;
   uint32_t                  last_irreversible_block_num = 0;
   uint32_t                  num_pow_witnesses           = 0;
};

struct block_with_info
{
   chain::signed_block       block;
   block_info                info;
};

} } }

FC_REFLECT( steemit::plugin::block_info::block_info,
   (block_id)
   (block_size)
   (aslot)
   (last_irreversible_block_num)
   (num_pow_witnesses)
   )

FC_REFLECT( steemit::plugin::block_info::block_with_info,
   (block)
   (info)
   )
