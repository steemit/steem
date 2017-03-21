#pragma once

#include <steemit/witness/witness_plugin.hpp>

#include <steemit/chain/steem_objects_type.hpp>

namespace steemit { namespace witness_plugin {

using namespace std;
using namespace steemit::chain;

#ifndef WITNESS_SPACE_ID
#define WITNESS_SPACE_ID 12
#endif

enum witness_plugin_object_type
{
   account_bandwidth_object   = ( WITNESS_SPACE_ID << 8 )
};

enum bandwidth_type
{
   post,    ///< Rate limiting posting reward eligibility over time
   forum,   ///< Rate limiting for all forum related actins
   market   ///< Rate limiting for all other actions
};

class account_bandwidth_object : public object< account_bandwidth_object_type, account_bandwidth_object >
{
   public:
      template< typename Constructor, typename Allocator >
      account_bandwidth_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      account_bandwidth_object() {}

      id_type           id;

      account_name_type account;
      bandwidth_type    type;
      share_type        average_bandwidth;
      share_type        lifetime_bandwidth;
      time_point_sec    last_bandwidth_update;
};

typedef oid< account_bandwidth_object > account_bandwidth_id_type;

struct by_account_bandwidth_type;

typedef multi_index_container <
   account_bandwidth_object,
   indexed_by <
      ordered_unique< tag< by_id >,
         member< account_bandwidth_object, account_bandwidth_id_type, &account_bandwidth_object::id > >,
      ordered_unique< tag< by_account_bandwidth_type >,
         composite_key< account_bandwidth_object,
            member< account_bandwidth_object, account_name_type, &account_bandwidth_object::account >,
            member< account_bandwidth_object, bandwidth_type, &account_bandwidth_object::type >
         >
      >
   >,
   allocator< account_bandwidth_object >
> account_bandwidth_index;

} } // steemit::witness_plugin

FC_REFLECT_ENUM( steemit::witness_plugin::bandwidth_type, (post)(forum)(market) )

FC_REFLECT( steemit::witness_plugin::account_bandwidth_object,
            (id)(account)(type)(average_bandwidth)(lifetime_bandwidth)(last_bandwidth_update) )
CHAINBASE_SET_INDEX_TYPE( steemit::witness_plugin::account_bandwidth_object, steemit::witness_plugin::account_bandwidth_index )
