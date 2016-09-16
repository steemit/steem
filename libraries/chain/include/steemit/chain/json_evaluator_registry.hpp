
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

      void apply_operations( const vector< CustomOperationType >& custom_operations, const operation& outer_o )
      {
         auto plugin_session = this->_db._undo_db.start_undo_session( true );

         flat_set<aname_type> outer_active;
         flat_set<aname_type> outer_owner;
         flat_set<aname_type> outer_posting;
         std::vector< authority > outer_other;

         flat_set<aname_type> inner_active;
         flat_set<aname_type> inner_owner;
         flat_set<aname_type> inner_posting;
         std::vector< authority > inner_other;

         operation_get_required_authorities( outer_o, outer_active, outer_owner, outer_posting, outer_other );

         for( const CustomOperationType& inner_o : custom_operations )
         {
            operation_validate( inner_o );
            operation_get_required_authorities( inner_o, inner_active, inner_owner, inner_posting, inner_other );
         }

         FC_ASSERT( inner_owner == outer_owner );
         FC_ASSERT( inner_active == outer_active );
         FC_ASSERT( inner_posting == outer_posting );
         FC_ASSERT( inner_other == outer_other );

         for( const CustomOperationType& inner_o : custom_operations )
         {
            // gcc errors if this-> is not here
            // error message is "declarations in dependent base are not found by unqualified lookup"
            this->get_evaluator( inner_o ).apply( inner_o );
         }

         plugin_session.merge();
      }

      virtual void apply( const custom_json_operation& outer_o ) override
      {
         try
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

            apply_operations( custom_operations, operation( outer_o ) );
         } FC_CAPTURE_AND_RETHROW( (outer_o) )
      }

      virtual void apply( const custom_binary_operation& outer_o ) override
      {
         try
         {
            vector< CustomOperationType > custom_operations;

            try
            {
               custom_operations = fc::raw::unpack< vector< CustomOperationType > >( outer_o.data );
            }
            catch ( fc::exception& )
            {
               custom_operations.push_back( fc::raw::unpack< CustomOperationType >( outer_o.data ) );
            }

            apply_operations( custom_operations, operation( outer_o ) );
         }
         FC_CAPTURE_AND_RETHROW( (outer_o) )
      }
};

} }
