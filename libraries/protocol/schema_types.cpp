
#include <steem/schema/abstract_schema.hpp>
#include <steem/schema/schema_impl.hpp>

#include <steem/protocol/schema_types.hpp>

namespace steem { namespace schema { namespace detail {

//////////////////////////////////////////////
// account_name_type                        //
//////////////////////////////////////////////

STEEM_SCHEMA_DEFINE_CLASS_METHODS( schema_account_name_type_impl )

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

//////////////////////////////////////////////
// asset_symbol_type                        //
//////////////////////////////////////////////

STEEM_SCHEMA_DEFINE_CLASS_METHODS( schema_asset_symbol_type_impl )

void schema_asset_symbol_type_impl::get_deps( std::vector< std::shared_ptr< abstract_schema > >& deps )
{
}

void schema_asset_symbol_type_impl::get_str_schema( std::string& s )
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
      ("type", "asset_symbol_type")
      ;

   str_schema = fc::json::to_string( mvo );
   s = str_schema;
   return;
}

} } }
