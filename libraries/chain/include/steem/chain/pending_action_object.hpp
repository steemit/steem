#pragma once
#include <steem/protocol/automated_actions.hpp>

#include <steem/chain/steem_object_types.hpp>

#include <boost/multi_index/composite_key.hpp>

namespace steem { namespace chain {

using steem::protocol::automated_action;

struct required_action_visitor
{
   typedef bool result_type;

   required_action_visitor() {}

   template< typename Action >
   bool operator()( const Action& a )const
   {
      return a.is_required();
   }
};

class pending_action_object : public object< pending_action_object_type, pending_action_object >
{
   pending_action_object() = delete;

   public:
      template< typename Constructor, typename Allocator >
      pending_action_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      id_type           id;

      automated_action  action;

      bool is_required()const
      {
         static const required_action_visitor visitor;
         return action.visit( visitor );
      }
};

struct by_required;
typedef multi_index_container<
   pending_action_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< pending_action_object, pending_action_id_type, &pending_action_object::id > >,
      ordered_unique< tag< by_required >,
         composite_key< pending_action_object,
            const_mem_fun< pending_action_object, bool, &pending_action_object::is_required >,
            member< pending_action_object, pending_action_id_type, &pending_action_object::id >
         >
      >
   >,
   allocator< pending_action_object >
> pending_action_index;

} } //steem::chain

FC_REFLECT( steem::chain::pending_action_object,
            (id)(action) )
CHAINBASE_SET_INDEX_TYPE( steem::chain::pending_action_object, steem::chain::pending_action_index )
