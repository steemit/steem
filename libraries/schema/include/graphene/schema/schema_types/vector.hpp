
#pragma once

#include <graphene/schema/abstract_schema.hpp>
#include <graphene/schema/schema_impl.hpp>

#include <vector>

namespace graphene { namespace schema { namespace detail {

//////////////////////////////////////////////
// vector                                   //
//////////////////////////////////////////////

template< typename E >
struct schema_vector_impl
   : public abstract_schema
{
   GRAPHENE_SCHEMA_CLASS_BODY( schema_vector_impl )
};

template< typename E >
void schema_vector_impl< E >::get_deps( std::vector< std::shared_ptr< abstract_schema > >& deps )
{
   deps.push_back( get_schema_for_type<E>() );
}

template< typename E >
void schema_vector_impl< E >::get_str_schema( std::string& s )
{
   if( str_schema != "" )
   {
      s = str_schema;
      return;
   }

   std::vector< std::shared_ptr< abstract_schema > > deps;
   get_deps( deps );
   std::string e_name;
   deps[0]->get_name(e_name);

   std::string my_name;
   get_name( my_name );
   fc::mutable_variant_object mvo;
   mvo("name", my_name)
      ("type", "list")
      ("etype", e_name)
      ;

   str_schema = fc::json::to_string( mvo );
   s = str_schema;
   return;
}

}

template< typename E >
struct schema_reflect< std::vector< E > >
{
   typedef detail::schema_vector_impl< E >        schema_impl_type;
};

} }
