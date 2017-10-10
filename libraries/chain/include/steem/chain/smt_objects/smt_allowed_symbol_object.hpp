#pragma once

#include <steem/chain/steem_object_types.hpp>
#include <steem/protocol/asset_symbol.hpp>
#include <steem/protocol/smt_operations.hpp>

#ifdef STEEM_ENABLE_SMT

namespace steem { namespace chain {

using steem::protocol::asset_symbol_type;

class smt_allowed_symbol_object : public object< smt_allowed_symbol_object_type, smt_allowed_symbol_object >
{
      smt_allowed_symbol_object() = delete;

   public:
      template< typename Constructor, typename Allocator >
      smt_allowed_symbol_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      id_type               id;

      asset_symbol_type     symbol;
};

class smt_allowed_symbol_qitem_object : public object< smt_allowed_symbol_qitem_object_type, smt_allowed_symbol_qitem_object >
{
      smt_allowed_symbol_qitem_object() = delete;

   public:
      template< typename Constructor, typename Allocator >
      smt_allowed_symbol_qitem_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      id_type               id;

      block_id_type         start_block_id;
      uint32_t              block_num = 0;
      uint32_t              index_in_block = 0;
};

struct by_symbol;
struct by_block_num;

typedef multi_index_container <
   smt_allowed_symbol_object,
   indexed_by <
      ordered_unique< tag< by_id >,
         member< smt_allowed_symbol_object, smt_allowed_symbol_id_type, &smt_allowed_symbol_object::id > >,
      ordered_unique< tag< by_symbol >,
         member< smt_allowed_symbol_object, asset_symbol_type, &smt_allowed_symbol_object::symbol > >
   >,
   allocator< smt_allowed_symbol_object >
> smt_allowed_symbol_index;

typedef multi_index_container <
   smt_allowed_symbol_qitem_object,
   indexed_by <
      ordered_unique< tag< by_id >,
         member< smt_allowed_symbol_qitem_object, smt_allowed_symbol_qitem_id_type, &smt_allowed_symbol_qitem_object::id > >
/*,
      ordered_unique< tag< by_block_num >,
         member< smt_allowed_symbol_qitem_object, uint32_t, &smt_allowed_symbol_qitem_object::block_num >,
         member< smt_allowed_symbol_qitem_object, uint32_t, &smt_allowed_symbol_qitem_object::index_in_block >
      >
*/
   >,
   allocator< smt_allowed_symbol_qitem_object >
> smt_allowed_symbol_qitem_index;

} } // namespace steem::chain

FC_REFLECT( steem::chain::smt_allowed_symbol_object,
   (id)
   (symbol)
)

FC_REFLECT( steem::chain::smt_allowed_symbol_qitem_object,
   (id)
   (start_block_id)
   (block_num)
   (index_in_block)
)

CHAINBASE_SET_INDEX_TYPE( steem::chain::smt_allowed_symbol_object,       steem::chain::smt_allowed_symbol_index       )
CHAINBASE_SET_INDEX_TYPE( steem::chain::smt_allowed_symbol_qitem_object, steem::chain::smt_allowed_symbol_qitem_index )

#endif
