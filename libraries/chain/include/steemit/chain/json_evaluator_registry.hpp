
#pragma once

#include <steemit/chain/evaluator.hpp>
#include <steemit/chain/generic_json_evaluator_registry.hpp>
#include <steemit/chain/protocol/steem_operations.hpp>
#include <steemit/chain/protocol/operation_util_impl.hpp>

#include <fc/variant.hpp>

#include <string>
#include <vector>

namespace steemit { namespace chain {

class database;

template< typename CustomOperationType >
class json_evaluator_registry
   : public generic_json_evaluator_registry, public evaluator_registry< CustomOperationType >
{
   public:
      json_evaluator_registry( database& db ) : evaluator_registry< CustomOperationType >(db) {}

      virtual void apply( const custom_json_operation& outer_o ) override
      {
         fc::variant v = fc::json::from_string( outer_o.json );

         std::vector< CustomOperationType > custom_operations;
         if( v.is_array() && v.size() > 0 && v.get_array()[0].is_array() )
         {
            // it looks like a list
            from_variant( v, custom_operations );
         }
         else
         {
            custom_operations.emplace_back();
            from_variant( v, custom_operations[0] );
         }

         flat_set< string > inner_active;
         flat_set< string > inner_owner;
         flat_set< string > inner_posting;
         std::vector< authority > inner_other;

         for( const CustomOperationType& inner_o : custom_operations )
         {
            operation_validate( inner_o );
            operation_get_required_authorities( inner_o, inner_active, inner_owner, inner_posting, inner_other );
         }

         FC_ASSERT( inner_active == outer_o.required_auths );
         FC_ASSERT( inner_owner.size() == 0 );
         FC_ASSERT( inner_posting == outer_o.required_posting_auths );
         FC_ASSERT( inner_other.size() == 0 );

         for( const CustomOperationType& inner_o : custom_operations )
         {
            // gcc errors if this-> is not here
            // error message is "declarations in dependent base are not found by unqualified lookup"
            this->get_evaluator( inner_o ).apply( inner_o );
         }
      }
};

} }
