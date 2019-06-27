
#pragma once

#include <steem/schema/abstract_schema.hpp>
#include <steem/schema/schema_impl.hpp>

#include <chainbase/util/object_id.hpp>

namespace steem { namespace schema { namespace detail {

//////////////////////////////////////////////
// oid                                      //
//////////////////////////////////////////////

template< typename T >
struct schema_oid_impl
   : public abstract_schema
{
   STEEM_SCHEMA_TEMPLATE_CLASS_BODY( schema_oid_impl )
};

template< typename T >
void schema_oid_impl< T >::get_deps( std::vector< std::shared_ptr< abstract_schema > >& deps )
{
   deps.push_back( get_schema_for_type<T>() );
}

template< typename T >
void schema_oid_impl< T >::get_str_schema( std::string& s )
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
      ("type", "oid")
      ("etype", e_name)
      ;

   str_schema = fc::json::to_string( mvo );
   s = str_schema;
   return;
}

}

template< typename T >
struct schema_reflect< chainbase::oid< T > >
{
   typedef detail::schema_oid_impl< T >        schema_impl_type;
};

} }
