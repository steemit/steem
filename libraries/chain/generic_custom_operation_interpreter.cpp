#include <steem/chain/generic_custom_operation_interpreter.hpp>

namespace steem { namespace chain {

std::string legacy_custom_name_from_type( const std::string& type_name )
{
   auto start = type_name.find_last_of( ':' ) + 1;
   auto end   = type_name.find_last_of( '_' );
   return type_name.substr( start, end-start );
}

} } // steem::chain
