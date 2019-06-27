#pragma once
#include <steem/plugins/block_api/block_api_objects.hpp>

#include <steem/protocol/types.hpp>
#include <steem/protocol/transaction.hpp>
#include <steem/protocol/block_header.hpp>

#include <steem/plugins/json_rpc/utility.hpp>

namespace steem { namespace plugins { namespace block_api {

/* get_block_header */

struct get_block_header_args
{
   uint32_t block_num;
};

struct get_block_header_return
{
   optional< block_header > header;
};

/* get_block */
struct get_block_args
{
   uint32_t block_num;
};

struct get_block_return
{
   optional< api_signed_block_object > block;
};

} } } // steem::block_api

FC_REFLECT( steem::plugins::block_api::get_block_header_args,
   (block_num) )

FC_REFLECT( steem::plugins::block_api::get_block_header_return,
   (header) )

FC_REFLECT( steem::plugins::block_api::get_block_args,
   (block_num) )

FC_REFLECT( steem::plugins::block_api::get_block_return,
   (block) )

