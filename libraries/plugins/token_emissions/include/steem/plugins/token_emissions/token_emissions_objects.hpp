#pragma once
#include <steem/chain/steem_object_types.hpp>

namespace steem { namespace plugins { namespace token_emissions {

using namespace steem::chain;

#ifndef STEEM_TOKEN_EMISSIONS_SPACE_ID
#define STEEM_TOKEN_EMISSIONS_SPACE_ID 19
#endif

enum token_emissions_object_type
{
   token_emissions_object_type = ( STEEM_TOKEN_EMISSIONS_SPACE_ID << 8 )
};

class token_emissions_object : public object< token_emissions_object_type, token_emissions_object >
{
public:
   template< typename Constructor, typename Allocator >
   token_emissions_object( Constructor&& c, allocator< Allocator > a )
   {
      c( *this );
   }

   token_emissions_object() {}

   id_type                     id;
};

typedef oid< token_emissions_object > token_emissions_object_id_type;

typedef multi_index_container<
   token_emissions_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< token_emissions_object, token_emissions_object_id_type, &token_emissions_object::id > >
   >,
   allocator< token_emissions_object >
> token_emissions_index;



} } } // steem::plugins::token_emissions


FC_REFLECT( steem::plugins::token_emissions::token_emissions_object, (id) )
CHAINBASE_SET_INDEX_TYPE( steem::plugins::token_emissions::token_emissions_object, steem::plugins::token_emissions::token_emissions_index )
