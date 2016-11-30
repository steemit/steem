
#pragma once

#include <graphene/schema/abstract_schema.hpp>
#include <graphene/schema/schema_impl.hpp>

#include <utility>

namespace graphene { namespace schema { namespace detail {

//////////////////////////////////////////////
// pair                                     //
//////////////////////////////////////////////

template< typename K, typename V >
struct schema_pair_impl
   : public abstract_schema
{
   GRAPHENE_SCHEMA_CLASS_BODY( schema_pair_impl )
};

template< typename K, typename V >
void schema_pair_impl< K, V >::get_deps( std::vector< std::shared_ptr< abstract_schema > >& deps )
{
   deps.push_back( get_schema_for_type<K>() );
   deps.push_back( get_schema_for_type<V>() );
}

template< typename K, typename V >
void schema_pair_impl< K, V >::get_str_schema( std::string& s )
{
   if( str_schema != "" )
   {
      s = str_schema;
      return;
   }

   std::vector< std::shared_ptr< abstract_schema > > deps;
   get_deps( deps );
   std::vector< std::string > e_types;
   for( size_t i=0; i<2; i++ )
   {
      e_types.emplace_back();
      deps[i]->get_name(e_types.back());
   }

   std::string my_name;
   get_name( my_name );
   fc::mutable_variant_object mvo;
   mvo("name", my_name)
      ("type", "tuple")
      ("etypes", e_types)
      ;

   str_schema = fc::json::to_string( mvo );
   s = str_schema;
   return;
}

}

template< typename K, typename V >
struct schema_reflect< std::pair< K, V > >
{
   typedef detail::schema_pair_impl< K, V >        schema_impl_type;
};

} }
