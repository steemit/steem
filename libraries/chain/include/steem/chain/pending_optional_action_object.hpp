#pragma once
#include <steem/chain/steem_fwd.hpp>

#include <steem/protocol/required_automated_actions.hpp>

#include <steem/chain/steem_object_types.hpp>

namespace steem { namespace chain {

using steem::protocol::optional_automated_action;

class pending_optional_action_object : public object< pending_optional_action_object_type, pending_optional_action_object >
{
   STEEM_STD_ALLOCATOR_CONSTRUCTOR( pending_optional_action_object )

   public:
      template< typename Constructor, typename Allocator >
      pending_optional_action_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      id_type                    id;

      time_point_sec             execution_time;
      optional_automated_action  action;
};

typedef multi_index_container<
   pending_optional_action_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< pending_optional_action_object, pending_optional_action_id_type, &pending_optional_action_object::id > >,
      ordered_unique< tag< by_execution >,
         composite_key< pending_optional_action_object,
            member< pending_optional_action_object, time_point_sec, &pending_optional_action_object::execution_time >,
            member< pending_optional_action_object, pending_optional_action_id_type, &pending_optional_action_object::id >
         >
      >
   >,
   allocator< pending_optional_action_object >
> pending_optional_action_index;

} } //steem::chain

FC_REFLECT( steem::chain::pending_optional_action_object,
            (id)(execution_time)(action) )
CHAINBASE_SET_INDEX_TYPE( steem::chain::pending_optional_action_object, steem::chain::pending_optional_action_index )
