#pragma once
#include <dpn/plugins/block_api/block_api_objects.hpp>

#include <dpn/protocol/types.hpp>
#include <dpn/protocol/transaction.hpp>
#include <dpn/protocol/block_header.hpp>

#include <dpn/plugins/json_rpc/utility.hpp>

namespace dpn { namespace plugins { namespace block_api {

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

} } } // dpn::block_api

FC_REFLECT( dpn::plugins::block_api::get_block_header_args,
   (block_num) )

FC_REFLECT( dpn::plugins::block_api::get_block_header_return,
   (header) )

FC_REFLECT( dpn::plugins::block_api::get_block_args,
   (block_num) )

FC_REFLECT( dpn::plugins::block_api::get_block_return,
   (block) )

