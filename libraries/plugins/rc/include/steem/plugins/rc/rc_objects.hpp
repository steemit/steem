#pragma once
#include <steem/chain/util/manabar.hpp>

#include <steem/plugins/rc/rc_utility.hpp>
#include <steem/plugins/rc/resource_count.hpp>

#include <steem/chain/steem_object_types.hpp>

#include <fc/int_array.hpp>

#include <boost/multi_index/composite_key.hpp>

namespace steem { namespace plugins { namespace rc {

using namespace std;
using namespace steem::chain;

#ifndef STEEM_RC_SPACE_ID
#define STEEM_RC_SPACE_ID 16
#endif

enum rc_object_types
{
   rc_resource_param_object_type   = ( STEEM_RC_SPACE_ID << 8 ),
   rc_pool_object_type             = ( STEEM_RC_SPACE_ID << 8 ) + 1,
   rc_account_object_type          = ( STEEM_RC_SPACE_ID << 8 ) + 2
};

class rc_resource_param_object : public object< rc_resource_param_object_type, rc_resource_param_object >
{
   public:
      template< typename Constructor, typename Allocator >
      rc_resource_param_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      id_type               id;
      fc::int_array< rc_resource_params, STEEM_NUM_RESOURCE_TYPES >
                            resource_param_array;
};

class rc_pool_object : public object< rc_pool_object_type, rc_pool_object >
{
   public:
      template< typename Constructor, typename Allocator >
      rc_pool_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      id_type               id;
      fc::int_array< int64_t, STEEM_NUM_RESOURCE_TYPES >
                            pool_array;
};

class rc_account_object : public object< rc_account_object_type, rc_account_object >
{
   public:
      template< typename Constructor, typename Allocator >
      rc_account_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      id_type               id;

      account_name_type     account;
      steem::chain::util::manabar   rc_manabar;
      asset                 max_rc_creation_adjustment = asset( 0, VESTS_SYMBOL );
      int64_t               max_rc = 0;

      // This is used for bug-catching, to match that the vesting shares in a
      // pre-op are equal to what they were at the last post-op.
      int64_t               last_max_rc = 0;
};

using namespace boost::multi_index;

typedef multi_index_container<
   rc_resource_param_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< rc_resource_param_object, rc_resource_param_object::id_type, &rc_resource_param_object::id > >
   >,
   allocator< rc_resource_param_object >
> rc_resource_param_index;

typedef multi_index_container<
   rc_pool_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< rc_pool_object, rc_pool_object::id_type, &rc_pool_object::id > >
   >,
   allocator< rc_pool_object >
> rc_pool_index;

typedef multi_index_container<
   rc_account_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< rc_account_object, rc_account_object::id_type, &rc_account_object::id > >,
      ordered_unique< tag< by_name >, member< rc_account_object, account_name_type, &rc_account_object::account > >
   >,
   allocator< rc_account_object >
> rc_account_index;

} } } // steem::plugins::rc

FC_REFLECT( steem::plugins::rc::rc_resource_param_object, (id)(resource_param_array) )
CHAINBASE_SET_INDEX_TYPE( steem::plugins::rc::rc_resource_param_object, steem::plugins::rc::rc_resource_param_index )

FC_REFLECT( steem::plugins::rc::rc_pool_object, (id)(pool_array) )
CHAINBASE_SET_INDEX_TYPE( steem::plugins::rc::rc_pool_object, steem::plugins::rc::rc_pool_index )

FC_REFLECT( steem::plugins::rc::rc_account_object,
   (id)
   (account)
   (rc_manabar)
   (max_rc_creation_adjustment)
   (max_rc)
   (last_max_rc)
   )
CHAINBASE_SET_INDEX_TYPE( steem::plugins::rc::rc_account_object, steem::plugins::rc::rc_account_index )
