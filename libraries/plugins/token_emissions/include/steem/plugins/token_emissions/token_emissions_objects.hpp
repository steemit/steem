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
   asset_symbol_type           symbol;
   fc::time_point_sec          last_emission = fc::time_point_sec();
   fc::time_point_sec          next_emission = fc::time_point_sec();
};

typedef oid< token_emissions_object > token_emissions_object_id_type;

struct by_symbol;
struct by_next_emission_symbol;

typedef multi_index_container<
   token_emissions_object,
   indexed_by<
      ordered_unique< tag< by_id >,     member< token_emissions_object, token_emissions_object_id_type, &token_emissions_object::id > >,
      ordered_unique< tag< by_symbol >, member< token_emissions_object, asset_symbol_type, &token_emissions_object::symbol > >,
      ordered_unique< tag< by_next_emission_symbol >,
         composite_key< token_emissions_object,
            member< token_emissions_object, fc::time_point_sec, &token_emissions_object::next_emission >,
            member< token_emissions_object, asset_symbol_type, &token_emissions_object::symbol >
         >,
         composite_key_compare< std::less< fc::time_point_sec >, std::less< asset_symbol_type > >
      >
   >,
   allocator< token_emissions_object >
> token_emissions_index;



} } } // steem::plugins::token_emissions


FC_REFLECT( steem::plugins::token_emissions::token_emissions_object, (id)(symbol)(last_emission)(next_emission) )
CHAINBASE_SET_INDEX_TYPE( steem::plugins::token_emissions::token_emissions_object, steem::plugins::token_emissions::token_emissions_index )
