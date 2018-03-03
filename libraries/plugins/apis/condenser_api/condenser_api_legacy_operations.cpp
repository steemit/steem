#include <steem/plugins/condenser_api/condenser_api_legacy_operations.hpp>

#include <steem/protocol/operation_util_impl.hpp>

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

} // fc

STEEM_DEFINE_OPERATION_TYPE_INTERNAL( steem::plugins::condenser_api::legacy_operation, fc::name_from_legacy_type )
