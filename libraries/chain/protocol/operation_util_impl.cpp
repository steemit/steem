#include <string>

namespace fc {

std::string name_from_type( const std::string& type_name )
{
   auto start = type_name.find_last_of( ':' ) + 1;
   auto end   = type_name.find_last_of( '_' );
   return type_name.substr( start, end-start );
}

} // fc
