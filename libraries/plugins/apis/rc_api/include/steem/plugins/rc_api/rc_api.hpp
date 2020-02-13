#pragma once
#include <steem/plugins/json_rpc/utility.hpp>
#include <steem/plugins/rc/rc_objects.hpp>

#include <steem/chain/comment_object.hpp>

#include <fc/optional.hpp>
#include <fc/variant.hpp>
#include <fc/vector.hpp>

#define RC_API_SINGLE_QUERY_LIMIT 1000

namespace steem { namespace plugins { namespace rc {

namespace detail
{
   class rc_api_impl;
}

enum sort_order_type
{
   by_name,
   by_edge,
   by_pool
};

struct list_object_args_type
{
   fc::variant       start;
   uint32_t          limit;
   sort_order_type   order;
};

using plugins::json_rpc::void_type;

typedef void_type get_resource_params_args;

struct get_resource_params_return
{
   vector< string >                                         resource_names;
   variant_object                                           resource_params;
   variant_object                                           size_info;
};

typedef void_type get_resource_pool_args;

struct resource_pool_api_object
{
   int64_t pool = 0;
};

struct get_resource_pool_return
{
   variant_object                                           resource_pool;
};

struct pool_delegation
{
   steem::chain::util::manabar rc_manabar;
   int64_t                     max_mana = 0;
};

struct rc_account_api_object
{
   rc_account_api_object( const rc_account_object& rca, const database& db ) :
      account( rca.account ),
      creator( rca.creator ),
      rc_manabar( rca.rc_manabar ),
      max_rc_creation_adjustment( rca.max_rc_creation_adjustment ),
      vests_delegated_to_pools( rca.vests_delegated_to_pools ),
      delegation_slots( rca.indel_slots ),
      out_delegation_total( rca.out_delegations )
   {
      max_rc = get_maximum_rc( db.get_account( account ), rca );

      db.get_index< chain::comment_index, chain::by_permlink >(); // works
      db.get_index< rc_outdel_drc_edge_index, by_edge >(); // does not work
      db.get_index< rc_outdel_drc_edge_index >().indices().get< by_edge >(); // works
      for( const account_name_type& pool : delegation_slots )
      {
         pool_delegation del;

         db.get< rc_outdel_drc_edge_object, by_edge >( boost::make_tuple( pool, account, VESTS_SYMBOL ) ); // does not work
         auto indel_edge = db.find< rc_outdel_drc_edge_object, by_edge >( boost::make_tuple( pool, account, VESTS_SYMBOL ) ); // does not work
         if( indel_edge != nullptr )
         {
            del.rc_manabar = indel_edge->drc_manabar;
            del.max_mana = indel_edge->drc_max_mana;
         }

         incoming_delegations[ pool ] = del;
      }
   }

   account_name_type             account;
   account_name_type             creator;
   steem::chain::util::manabar   rc_manabar;
   asset                         max_rc_creation_adjustment = asset( 0, VESTS_SYMBOL );
   int64_t                       max_rc = 0;
   asset                         vests_delegated_to_pools = asset( 0 , VESTS_SYMBOL );
   fc::array< account_name_type, STEEM_RC_MAX_SLOTS >
                                 delegation_slots;

   flat_map< account_name_type, pool_delegation >
                                 incoming_delegations;

   uint32_t                      out_delegation_total = 0;

   //
   // This is used for bug-catching, to match that the vesting shares in a
   // pre-op are equal to what they were at the last post-op.
   //
   // Don't return it in the API for now, because we don't want
   // API's to be built to assume its presence
   //
   // If it's needed it for debugging, we might think about un-commenting it
   //
   // asset                 last_vesting_shares = asset( 0, VESTS_SYMBOL );
};

struct find_rc_accounts_args
{
   std::vector< account_name_type > accounts;
};

struct find_rc_accounts_return
{
   std::vector< rc_account_api_object > rc_accounts;
};

typedef list_object_args_type list_rc_accounts_args;

typedef find_rc_accounts_return list_rc_accounts_return;

typedef rc_delegation_pool_object rc_delegation_pool_api_object;

struct find_rc_delegation_pools_args
{
   std::vector< account_name_type > accounts;
};

struct find_rc_delegation_pools_return
{
   std::vector< rc_delegation_pool_api_object > rc_delegation_pools;
};

typedef list_object_args_type list_rc_delegation_pools_args;

typedef find_rc_delegation_pools_return list_rc_delegation_pools_return;

typedef rc_indel_edge_object rc_indel_edge_api_object;

struct find_rc_delegations_args
{
   account_name_type account;
};

struct find_rc_delegations_return
{
   std::vector< rc_indel_edge_api_object > rc_delegations;
};

typedef list_object_args_type list_rc_delegations_args;

typedef find_rc_delegations_return list_rc_delegations_return;

class rc_api
{
   public:
      rc_api();
      ~rc_api();

      DECLARE_API(
         (get_resource_params)
         (get_resource_pool)
         (find_rc_accounts)
         (list_rc_accounts)
         (find_rc_delegation_pools)
         (list_rc_delegation_pools)
         (find_rc_delegations)
         (list_rc_delegations)
         )

   private:
      std::unique_ptr< detail::rc_api_impl > my;
};

} } } // steem::plugins::rc

FC_REFLECT_ENUM( steem::plugins::rc::sort_order_type,
   (by_name)
   (by_edge)
   (by_pool) )

FC_REFLECT( steem::plugins::rc::list_object_args_type,
   (start)
   (limit)
   (order) )

FC_REFLECT( steem::plugins::rc::get_resource_params_return,
   (resource_names)
   (resource_params)
   (size_info) )

FC_REFLECT( steem::plugins::rc::resource_pool_api_object,
   (pool) )

FC_REFLECT( steem::plugins::rc::get_resource_pool_return,
   (resource_pool) )

FC_REFLECT( steem::plugins::rc::pool_delegation,
   (rc_manabar)
   (max_mana) )

FC_REFLECT( steem::plugins::rc::rc_account_api_object,
   (account)
   (creator)
   (rc_manabar)
   (max_rc_creation_adjustment)
   (max_rc)
   (vests_delegated_to_pools)
   (delegation_slots)
   (incoming_delegations)
   (out_delegation_total) )

FC_REFLECT( steem::plugins::rc::find_rc_accounts_args,
   (accounts) )

FC_REFLECT( steem::plugins::rc::find_rc_accounts_return,
   (rc_accounts) )

FC_REFLECT( steem::plugins::rc::find_rc_delegation_pools_args,
   (accounts) )

FC_REFLECT( steem::plugins::rc::find_rc_delegation_pools_return,
   (rc_delegation_pools) )

FC_REFLECT( steem::plugins::rc::find_rc_delegations_args,
   (account) )

FC_REFLECT( steem::plugins::rc::find_rc_delegations_return,
   (rc_delegations) )
