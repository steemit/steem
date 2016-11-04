
#pragma once

#include <graphene/schema/abstract_schema.hpp>
#include <graphene/schema/schema_impl.hpp>

#include <fc/static_variant.hpp>

namespace graphene { namespace schema { namespace detail {

//////////////////////////////////////////////
// static_variant                           //
//////////////////////////////////////////////

template< typename... Types >
struct schema_static_variant_impl
   : public abstract_schema
{
   GRAPHENE_SCHEMA_CLASS_BODY( schema_static_variant_impl )
};

template< typename... Types > struct get_schemas_for_types_impl;

template<>
struct get_schemas_for_types_impl<>
{
   static void get( std::vector< std::shared_ptr< abstract_schema > >& schemas ) {}
};

template< typename T, typename... Types >
struct get_schemas_for_types_impl< T, Types... >
{
   static void get( std::vector< std::shared_ptr< abstract_schema > >& schemas )
   {
      schemas.push_back( get_schema_for_type<T>() );
      get_schemas_for_types_impl<Types...>::get( schemas );
   }
};

template< typename... Types >
void get_schemas_for_types( std::vector< std::shared_ptr< abstract_schema > >& schemas )
{
   get_schemas_for_types_impl<Types...>::get( schemas );
   return;
}

template< typename... Types >
void schema_static_variant_impl< Types... >::get_deps( std::vector< std::shared_ptr< abstract_schema > >& deps )
{
   get_schemas_for_types<Types...>( deps );
}

template< typename... Types >
void schema_static_variant_impl< Types... >::get_str_schema( std::string& s )
{
   if( str_schema != "" )
   {
      s = str_schema;
      return;
   }

   std::vector< std::shared_ptr< abstract_schema > > deps;
   get_deps( deps );
   std::vector< std::string > e_types;
   for( std::shared_ptr< abstract_schema >& schema : deps )
   {
      e_types.emplace_back();
      schema->get_name( e_types.back() );
   }

   std::string my_name;
   get_name( my_name );
   fc::mutable_variant_object mvo;
   mvo("name", my_name)
      ("type", "static_variant")
      ("etypes", e_types)
      ;

   str_schema = fc::json::to_string( mvo );
   s = str_schema;
   return;
}

}

template< typename... Types >
struct schema_reflect< fc::static_variant< Types... > >
{
   typedef detail::schema_static_variant_impl< Types... >     schema_impl_type;
};

} }
