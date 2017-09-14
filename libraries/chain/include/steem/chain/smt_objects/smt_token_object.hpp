#pragma once

#include <steem/chain/steem_object_types.hpp>

#ifdef STEEM_ENABLE_SMT

namespace steem { namespace chain {

class smt_token_object : public object< smt_token_object_type, smt_token_object >
{
   smt_token_object() = delete;

   public:
      enum class smt_phase
      {
         account_elevated,
         setup_completed,
      };

   public:
      template< typename Constructor, typename Allocator >
      smt_token_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      // id_type is actually oid<smt_token_object>
      id_type           id;

      account_name_type control_account;
      smt_phase         phase = smt_phase::account_elevated;

      /// set_setup_parameters
      bool              allow_voting = false;
      bool              allow_vesting = false;
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

#endif
