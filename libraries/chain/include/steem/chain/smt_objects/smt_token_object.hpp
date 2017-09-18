#pragma once

#include <steem/chain/steem_object_types.hpp>

#ifdef STEEM_ENABLE_SMT

namespace steem { namespace chain {

class smt_token_object : public object< smt_token_object_type, smt_token_object >
{
   smt_token_object() = delete;

   public:
      template< typename Constructor, typename Allocator >
      smt_token_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      // id_type is actually oid<smt_token_object>
      id_type           id;
};

typedef multi_index_container <
   smt_token_object,
   indexed_by <
      random_access< tag< chainbase::MasterIndexTag >>
   >,
   allocator< smt_token_object >
> smt_token_index;

} }

FC_REFLECT( steem::chain::smt_token_object,
   (id)
)
CHAINBASE_SET_INDEX_TYPE( steem::chain::smt_token_object, steem::chain::smt_token_index )

#endif
