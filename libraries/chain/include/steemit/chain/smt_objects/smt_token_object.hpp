#pragma once

#include <steemit/chain/steem_object_types.hpp>

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

      account_name_type control_account;
};

struct by_control_account;

typedef multi_index_container <
   smt_token_object,
   indexed_by <
      ordered_unique< tag< by_id >,
         member< smt_token_object, smt_token_id_type, &smt_token_object::id > >,
      ordered_unique< tag< by_control_account >,
         member< smt_token_object, account_name_type, &smt_token_object::control_account > >
   >,
   allocator< smt_token_object >
> smt_token_index;

} }

FC_REFLECT( steem::chain::smt_token_object,
   (id)
   (control_account)
)
CHAINBASE_SET_INDEX_TYPE( steem::chain::smt_token_object, steem::chain::smt_token_index )
