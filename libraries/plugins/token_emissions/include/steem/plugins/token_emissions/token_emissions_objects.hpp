#pragma once
#include <steem/chain/steem_object_types.hpp>

namespace steem { namespace plugins { namespace token_emissions {

using namespace steem::chain;

#ifndef STEEM_TOKEN_EMISSIONS_SPACE_ID
#define STEEM_TOKEN_EMISSIONS_SPACE_ID 20
#endif

enum token_emission_schedule_object_type
{
   token_emission_schedule_object_type = ( STEEM_TOKEN_EMISSIONS_SPACE_ID << 8 )
};

class token_emission_schedule_object : public object< token_emission_schedule_object_type, token_emission_schedule_object >
{
public:
   template< typename Constructor, typename Allocator >
   token_emission_schedule_object( Constructor&& c, allocator< Allocator > a )
   {
      c( *this );
   }

   token_emission_schedule_object() {}

   id_type                     id;
   asset_symbol_type           symbol;
   fc::time_point_sec          next_consensus_emission = fc::time_point_sec();
   fc::time_point_sec          next_scheduled_emission = fc::time_point_sec();
};

typedef oid< token_emission_schedule_object > token_emission_schedule_object_id_type;

struct by_symbol;
struct by_next_emission_symbol;

typedef multi_index_container<
   token_emission_schedule_object,
   indexed_by<
      ordered_unique< tag< by_id >,     member< token_emission_schedule_object, token_emission_schedule_object_id_type, &token_emission_schedule_object::id > >,
      ordered_unique< tag< by_symbol >, member< token_emission_schedule_object, asset_symbol_type, &token_emission_schedule_object::symbol > >,
      ordered_unique< tag< by_next_emission_symbol >,
         composite_key< token_emission_schedule_object,
            member< token_emission_schedule_object, fc::time_point_sec, &token_emission_schedule_object::next_scheduled_emission >,
            member< token_emission_schedule_object, asset_symbol_type, &token_emission_schedule_object::symbol >
         >,
         composite_key_compare< std::less< fc::time_point_sec >, std::less< asset_symbol_type > >
      >
   >,
   allocator< token_emission_schedule_object >
> token_emission_schedule_index;



} } } // steem::plugins::token_emissions


FC_REFLECT( steem::plugins::token_emissions::token_emission_schedule_object, (id)(symbol)(next_consensus_emission)(next_scheduled_emission) )
CHAINBASE_SET_INDEX_TYPE( steem::plugins::token_emissions::token_emission_schedule_object, steem::plugins::token_emissions::token_emission_schedule_index )
