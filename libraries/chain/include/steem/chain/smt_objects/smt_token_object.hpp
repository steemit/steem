#pragma once

#include <steem/chain/steem_object_types.hpp>

#ifdef STEEM_ENABLE_SMT

namespace steem { namespace chain {

class smt_token_object : public object< smt_token_object_type, smt_token_object >
{
   public:
      template< typename Constructor, typename Allocator >
      smt_token_object( Constructor&& c, size_t assignedId, allocator<Allocator> a ) :
         id(assignedId)
      {
         c( *this );
      }

      // id_type is actually oid<smt_token_object>
      id_type id;
};

typedef chainbase::indexed_container<smt_token_object> smt_token_index;

} }

FC_REFLECT( steem::chain::smt_token_object,
   (id)
)
CHAINBASE_SET_INDEX_TYPE( steem::chain::smt_token_object, steem::chain::smt_token_index )

#endif
