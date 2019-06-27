#pragma once

#include <steem/chain/steem_object_types.hpp>

#ifndef STEEM_WITNESS_SPACE_ID
#define STEEM_WITNESS_SPACE_ID 19
#endif

namespace steem { namespace chain {
struct by_account;
} }

namespace steem { namespace plugins { namespace witness {

using namespace steem::chain;

enum witness_object_types
{
   witness_custom_op_object_type          = ( STEEM_WITNESS_SPACE_ID << 8 )
};

class witness_custom_op_object : public object< witness_custom_op_object_type, witness_custom_op_object >
{
   public:
      template< typename Constructor, typename Allocator >
      witness_custom_op_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      witness_custom_op_object() {}

      id_type               id;
      account_name_type     account;
};

typedef multi_index_container<
   witness_custom_op_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< witness_custom_op_object, witness_custom_op_object::id_type, &witness_custom_op_object::id > >,
      ordered_unique< tag< by_account >, member< witness_custom_op_object, account_name_type, &witness_custom_op_object::account > >
   >,
   allocator< witness_custom_op_object >
> witness_custom_op_index;

} } }

FC_REFLECT( steem::plugins::witness::witness_custom_op_object,
   (id)
   (account)
   )
CHAINBASE_SET_INDEX_TYPE( steem::plugins::witness::witness_custom_op_object, steem::plugins::witness::witness_custom_op_index )
