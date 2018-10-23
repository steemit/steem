
#include <base58.h>

#include <fc/crypto/base58.hpp>
#include <fc/exception/exception.hpp>
#include <fc/log/logger.hpp>

namespace fc {

std::string to_base58( const char* d, size_t s ) {
  return EncodeBase58( (const unsigned char*)d, (const unsigned char*)d+s ).c_str();
}

std::string to_base58( const std::vector<char>& d )
{
  if( d.size() )
     return to_base58( d.data(), d.size() );
  return std::string();
}
std::vector<char> from_base58( const std::string& base58_str ) {
   std::vector<unsigned char> out;
   if( !DecodeBase58( base58_str.c_str(), out ) ) {
     FC_THROW_EXCEPTION( parse_error_exception, "Unable to decode base58 string ${base58_str}", ("base58_str",base58_str) );
   }
   return std::vector<char>((const char*)out.data(), ((const char*)out.data())+out.size() );
}
/**
 *  @return the number of bytes decoded
 */
size_t from_base58( const std::string& base58_str, char* out_data, size_t out_data_len ) {
  //slog( "%s", base58_str.c_str() );
  std::vector<unsigned char> out;
  if( !DecodeBase58( base58_str.c_str(), out ) ) {
    FC_THROW_EXCEPTION( parse_error_exception, "Unable to decode base58 string ${base58_str}", ("base58_str",base58_str) );
  }
  FC_ASSERT( out.size() <= out_data_len );
  memcpy( out_data, out.data(), out.size() );
  return out.size();
}

}
