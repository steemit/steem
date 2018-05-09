#include <fc/asio.hpp>
#include <fc/network/ip.hpp>
#include <fc/log/logger.hpp>

namespace fc
{
  std::vector<fc::ip::endpoint> resolve( const fc::string& host, uint16_t port )
  {
    auto ep = fc::asio::tcp::resolve( host, std::to_string(uint64_t(port)) );
    std::vector<fc::ip::endpoint> eps;
    eps.reserve(ep.size());
    for( auto itr = ep.begin(); itr != ep.end(); ++itr )
    {
      if( itr->address().is_v4() )
      {
       eps.push_back( fc::ip::endpoint(itr->address().to_v4().to_ulong(), itr->port()) );
      }
      // TODO: add support for v6
    }
    return eps;
  }

  std::vector< fc::ip::endpoint > resolve_string_to_ip_endpoints( const string& endpoint_string )
  {
    try
    {
      string::size_type colon_pos = endpoint_string.find( ':' );
      if( colon_pos == std::string::npos )
        FC_THROW( "Missing required port number in endpoint string \"${endpoint_string}\"",
                  ("endpoint_string", endpoint_string) );

      string port_string = endpoint_string.substr( colon_pos + 1 );

      try
      {
        uint16_t port = boost::lexical_cast< uint16_t >( port_string );
        string hostname = endpoint_string.substr( 0, colon_pos );
        std::vector< fc::ip::endpoint > endpoints = resolve( hostname, port );

        if( endpoints.empty() )
          FC_THROW_EXCEPTION( unknown_host_exception, "The host name can not be resolved: ${hostname}", ("hostname", hostname) );

        return endpoints;
      }
      catch( const boost::bad_lexical_cast& )
      {
        FC_THROW("Bad port: ${port}", ("port", port_string) );
      }
    }
    FC_CAPTURE_AND_RETHROW( (endpoint_string) )
  }
}
