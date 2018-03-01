#include <steem/plugins/condenser_api/condenser_api_legacy_operations.hpp>

#define LEGACY_PREFIX "legacy_"
#define LEGACY_PREFIX_OFFSET (7)

namespace fc {

std::string name_from_legacy_type( const std::string& type_name )
{
   auto start = type_name.find( LEGACY_PREFIX );
   if( start == std::string::npos )
   {
      start = type_name.find_last_of( ':' ) + 1;
   }
   else
   {
      start += LEGACY_PREFIX_OFFSET;
   }
   auto end   = type_name.find_last_of( '_' );

   return type_name.substr( start, end-start );
}

struct from_operation
{
   variant& var;
   from_operation( variant& dv )
      : var( dv ) {}

   typedef void result_type;
   template<typename T> void operator()( const T& v )const
   {
      auto name = name_from_legacy_type( fc::get_typename< T >::name() );
      var = mutable_variant_object( "type", name )( "value", v );
   }
};

struct get_operation_name
{
   string& name;
   get_operation_name( string& dv )
      : name( dv ) {}

   typedef void result_type;
   template< typename T > void operator()( const T& v )const
   {
      name = name_from_legacy_type( fc::get_typename< T >::name() );
   }
};

void to_variant( const steem::plugins::condenser_api::legacy_operation& var,  fc::variant& vo )
{
   var.visit( from_operation( vo ) );
}

void from_variant( const fc::variant& var, steem::plugins::condenser_api::legacy_operation& vo )
{
   static std::map<string,uint32_t> to_tag = []()
   {
      std::map<string,uint32_t> name_map;
      for( int i = 0; i < steem::plugins::condenser_api::legacy_operation::count(); ++i )
      {
         steem::plugins::condenser_api::legacy_operation tmp;
         tmp.set_which(i);
         string n;
         tmp.visit( get_operation_name(n) );
         name_map[n] = i;
      }
      return name_map;
   }();

   FC_ASSERT( var.is_object(), "Operation has to treated as object." );
   auto v_object = var.get_object();
   FC_ASSERT( v_object.contains( "type" ), "Type field doesn't exist." );
   FC_ASSERT( v_object.contains( "value" ), "Value field doesn't exist." );

   auto itr = to_tag.find( v_object[ "type" ].as_string() );
   FC_ASSERT( itr != to_tag.end(), "Invalid operation name: ${n}", ("n", v_object[ "type" ]) );
   vo.set_which( to_tag[ v_object[ "type" ].as_string() ] );
   vo.visit( fc::to_static_variant( v_object[ "value" ] ) );  
}

} // fc
