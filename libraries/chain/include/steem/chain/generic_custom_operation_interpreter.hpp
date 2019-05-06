
#pragma once

#include <steem/protocol/schema_types.hpp>
#include <steem/chain/schema_types.hpp>
#include <steem/schema/schema.hpp>

#include <steem/protocol/steem_operations.hpp>
#include <steem/protocol/operation_util_impl.hpp>
#include <steem/protocol/types.hpp>

#include <steem/chain/evaluator.hpp>
#include <steem/chain/evaluator_registry.hpp>
#include <steem/chain/custom_operation_interpreter.hpp>

#include <fc/variant.hpp>

#include <string>
#include <vector>

namespace steem { namespace chain {

using protocol::operation;
using protocol::authority;
using protocol::account_name_type;
using protocol::custom_id_type;

class database;

std::string legacy_custom_name_from_type( const std::string& type_name );

struct get_legacy_custom_operation_name
{
   string& name;
   get_legacy_custom_operation_name( string& dv )
      : name( dv ) {}

   typedef void result_type;
   template< typename T > void operator()( const T& v )const
   {
      name = legacy_custom_name_from_type( fc::get_typename< T >::name() );
   }
};

struct get_custom_operation_name
{
   string& name;
   get_custom_operation_name( string& n )
      : name( n ) {}

   typedef void result_type;

   template< typename T > void operator()( const T& v )const
   {
      name = fc::trim_typename_namespace( fc::get_typename< T >::name() );
   }
};

template< typename CustomOperationType >
void custom_op_from_variant( const fc::variant& var, CustomOperationType& vo )
{
   static std::map<string,int64_t> to_legacy_tag = []()
   {
      std::map<string,int64_t> name_map;
      for( int i = 0; i < CustomOperationType::count(); ++i )
      {
         CustomOperationType tmp;
         tmp.set_which(i);
         string n;
         tmp.visit( get_legacy_custom_operation_name(n) );
         name_map[n] = i;
      }
      return name_map;
   }();

   static std::map< string, int64_t > to_tag = []()
   {
      std::map< string, int64_t > name_map;
      for( int i = 0; i < CustomOperationType::count(); ++i )
      {
         CustomOperationType tmp;
         tmp.set_which(i);
         string n;
         tmp.visit( get_custom_operation_name( n ) );
         name_map[n] = i;
      }
      return name_map;
   }();

   if( var.is_array() ) // legacy serialization
   {
      auto ar = var.get_array();
      if( ar.size() < 2 ) return;
      if( ar[0].is_uint64() )
         vo.set_which( ar[0].as_uint64() );
      else
      {
         auto itr = to_legacy_tag.find(ar[0].as_string());
         FC_ASSERT( itr != to_legacy_tag.end(), "Invalid operation name: ${n}", ("n", ar[0]) );
         vo.set_which( itr->second );
      }
      vo.visit( fc::to_static_variant( ar[1] ) );
   }
   else // new serialization
   {
      FC_ASSERT( var.is_object(), "Input data have to treated as object." );
      auto v_object = var.get_object();

      FC_ASSERT( v_object.contains( "type" ), "Type field doesn't exist." );
      FC_ASSERT( v_object.contains( "value" ), "Value field doesn't exist." );

      int64_t which = -1;

      if( v_object[ "type" ].is_integer() )
      {
         which = v_object[ "type" ].as_int64();
      }
      else
      {
         auto itr = to_tag.find( v_object[ "type" ].as_string() );
         FC_ASSERT( itr != to_tag.end(), "Invalid object name: ${n}", ("n", v_object[ "type" ]) );
         which = itr->second;
      }

      vo.set_which( which );
      vo.visit( fc::to_static_variant( v_object[ "value" ] ) );
   }
}

template< typename CustomOperationType >
class generic_custom_operation_interpreter
   : public custom_operation_interpreter, public evaluator_registry< CustomOperationType >
{
   public:
      generic_custom_operation_interpreter( database& db, const custom_id_type& cid )
         : evaluator_registry< CustomOperationType >(db), custom_id(cid) {}
      virtual ~generic_custom_operation_interpreter() = default;

      void apply_operations( const vector< CustomOperationType >& custom_operations, const operation& outer_o )
      {
         auto plugin_session = this->_db.start_undo_session();

         flat_set<account_name_type> outer_active;
         flat_set<account_name_type> outer_owner;
         flat_set<account_name_type> outer_posting;
         std::vector< authority >    outer_other;

         flat_set<account_name_type> inner_active;
         flat_set<account_name_type> inner_owner;
         flat_set<account_name_type> inner_posting;
         std::vector< authority >    inner_other;

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

         plugin_session.squash();
      }

      virtual void apply( const protocol::custom_json_operation& outer_o ) override
      {
         try
         {
            FC_TODO( "Should we hardfork out old serialization?" )
            fc::variant v = fc::json::from_string( outer_o.json );

            std::vector< CustomOperationType > custom_operations;
            if( v.is_array() && v.size() > 0 && v.get_array()[0].is_array() )
            {
               // it looks like a list
               for( auto& o : v.get_array() )
               {
                  custom_operations.emplace_back();
                  custom_op_from_variant( o, custom_operations.back() );
               }
            }
            else
            {
               custom_operations.emplace_back();
               custom_op_from_variant( v, custom_operations[0] );
            }

            apply_operations( custom_operations, operation( outer_o ) );
         } FC_CAPTURE_AND_RETHROW( (outer_o) )
      }

      virtual void apply( const protocol::custom_binary_operation& outer_o ) override
      {
         try
         {
            vector< CustomOperationType > custom_operations;

            try
            {
               custom_operations = fc::raw::unpack_from_vector< vector< CustomOperationType > >( outer_o.data );
            }
            catch ( fc::exception& )
            {
               custom_operations.push_back( fc::raw::unpack_from_vector< CustomOperationType >( outer_o.data ) );
            }

            apply_operations( custom_operations, operation( outer_o ) );
         }
         FC_CAPTURE_AND_RETHROW( (outer_o) )
      }

      virtual std::shared_ptr< steem::schema::abstract_schema > get_operation_schema() override
      {
         return steem::schema::get_schema_for_type< CustomOperationType >();
      }

      virtual custom_id_type get_custom_id() override
      {
         return custom_id;
      }

   private:
      custom_id_type custom_id;
};

} }
