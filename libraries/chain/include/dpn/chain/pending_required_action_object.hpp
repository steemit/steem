#pragma once
#include <dpn/chain/dpn_fwd.hpp>

#include <dpn/protocol/required_automated_actions.hpp>

#include <dpn/chain/dpn_object_types.hpp>

namespace dpn { namespace chain {

using dpn::protocol::required_automated_action;

class pending_required_action_object : public object< pending_required_action_object_type, pending_required_action_object >
{
   DPN_STD_ALLOCATOR_CONSTRUCTOR( pending_required_action_object )

   public:
      template< typename Constructor, typename Allocator >
      pending_required_action_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      id_type                    id;

      time_point_sec             execution_time;
      required_automated_action  action;
};

struct by_execution;

typedef multi_index_container<
   pending_required_action_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< pending_required_action_object, pending_required_action_id_type, &pending_required_action_object::id > >,
      ordered_unique< tag< by_execution >,
         composite_key< pending_required_action_object,
            member< pending_required_action_object, time_point_sec, &pending_required_action_object::execution_time >,
            member< pending_required_action_object, pending_required_action_id_type, &pending_required_action_object::id >
         >
      >
   >,
   allocator< pending_required_action_object >
> pending_required_action_index;

} } //dpn::chain

FC_REFLECT( dpn::chain::pending_required_action_object,
            (id)(execution_time)(action) )
CHAINBASE_SET_INDEX_TYPE( dpn::chain::pending_required_action_object, dpn::chain::pending_required_action_index )
