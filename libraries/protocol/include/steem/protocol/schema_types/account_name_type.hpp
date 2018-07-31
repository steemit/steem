
#pragma once

#include <steem/schema/abstract_schema.hpp>
#include <steem/schema/schema_impl.hpp>

#include <steem/protocol/types.hpp>

namespace steem { namespace schema { namespace detail {

//////////////////////////////////////////////
// account_name_type                        //
//////////////////////////////////////////////

struct schema_account_name_type_impl
   : public abstract_schema
{
   GRAPHENE_SCHEMA_CLASS_BODY( schema_account_name_type_impl )
};

void schema_account_name_type_impl::get_deps( std::vector< std::shared_ptr< abstract_schema > >& deps )
{
}

void schema_account_name_type_impl::get_str_schema( std::string& s )
{
   if( str_schema != "" )
   {
      s = str_schema;
      return;
   }

   std::vector< std::shared_ptr< abstract_schema > > deps;
   get_deps( deps );

   std::string my_name;
   get_name( my_name );
   fc::mutable_variant_object mvo;
   mvo("name", my_name)
      ("type", "account_name_type")
      ;

   str_schema = fc::json::to_string( mvo );
   s = str_schema;
   return;
}

}

template<>
struct schema_reflect< steem::protocol::account_name_type >
{
   typedef detail::schema_account_name_type_impl           schema_impl_type;
};

} }

namespace fc {

template<>
struct get_typename< steem::protocol::account_name_type >
{
   static const char* name()
   {
      return "steem::protocol::account_name_type";
   }
};

}
