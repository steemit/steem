#include <fc/reflect/typename.hpp>

#include <fc/log/logger.hpp>

namespace fc{

std::string trim_typename_namespace( const std::string& name )
{
   auto start = name.find_last_of( ':' );
   start = ( start == std::string::npos ) ? 0 : start + 1;
   return name.substr( start );
}

}
