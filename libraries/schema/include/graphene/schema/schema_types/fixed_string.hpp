
#pragma once

#include <graphene/schema/abstract_schema.hpp>
#include <graphene/schema/schema_impl.hpp>

#include <fc/fixed_string.hpp>

namespace graphene { namespace schema { namespace detail {

//////////////////////////////////////////////
// fixed_string                             //
//////////////////////////////////////////////

template<typename Storage>
struct schema_fixed_string_impl
   : public abstract_schema
{
   GRAPHENE_SCHEMA_CLASS_BODY( schema_fixed_string_impl )
};

template<typename Storage>
void schema_fixed_string_impl<Storage>::get_deps( std::vector< std::shared_ptr< abstract_schema > >& deps )
{
   return;
}

template<typename Storage>
void schema_fixed_string_impl<Storage>::get_str_schema( std::string& s )
{
   if( str_schema != "" )
   {
      s = str_schema;
      return;
   }

   std::string my_name;
   get_name( my_name );
   fc::mutable_variant_object mvo;
   mvo("name", my_name)
      ("type", "prim");

   str_schema = fc::json::to_string( mvo );
   s = str_schema;
   return;
}

}

template<typename Storage>
struct schema_reflect< typename fc::fixed_string<Storage> >
{
   typedef detail::schema_fixed_string_impl< Storage >        schema_impl_type;
};

} }
