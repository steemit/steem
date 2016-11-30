
#pragma once

#include <graphene/schema/abstract_schema.hpp>
#include <graphene/schema/schema_impl.hpp>

#include <boost/container/flat_map.hpp>

namespace graphene { namespace schema { namespace detail {

//////////////////////////////////////////////
// flat_map                                 //
//////////////////////////////////////////////

template< typename K, typename V >
struct schema_flat_map_impl
   : public abstract_schema
{
   GRAPHENE_SCHEMA_CLASS_BODY( schema_flat_map_impl )
};

template< typename K, typename V >
void schema_flat_map_impl< K, V >::get_deps( std::vector< std::shared_ptr< abstract_schema > >& deps )
{
   deps.push_back( get_schema_for_type<K>() );
   deps.push_back( get_schema_for_type<V>() );
}

template< typename K, typename V >
void schema_flat_map_impl< K, V >::get_str_schema( std::string& s )
{
   if( str_schema != "" )
   {
      s = str_schema;
      return;
   }

   std::vector< std::shared_ptr< abstract_schema > > deps;
   get_deps( deps );
   std::string k_name, v_name;
   deps[0]->get_name(k_name);
   deps[1]->get_name(v_name);

   std::string my_name;
   get_name( my_name );
   fc::mutable_variant_object mvo;
   mvo("name", my_name)
      ("type", "map")
      ("ktype", k_name)
      ("vtype", v_name)
      ;

   str_schema = fc::json::to_string( mvo );
   s = str_schema;
   return;
}

}

template< typename K, typename V, typename Compare, typename Allocator >
struct schema_reflect< boost::container::flat_map< K, V, Compare, Allocator > >
{
   typedef detail::schema_flat_map_impl< K, V >        schema_impl_type;
};

} }
