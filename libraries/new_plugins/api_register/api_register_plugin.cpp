#include <steemit/plugins/api_register/api_register_plugin.hpp>

#include <boost/algorithm/string.hpp>

#include <fc/log/logger_config.hpp>
#include <fc/exception/exception.hpp>

namespace steemit { namespace plugins { namespace api_register {

api_register_plugin::api_register_plugin() : appbase::plugin< api_register_plugin >( API_REGISTER_PLUGIN_NAME ) {}
api_register_plugin::~api_register_plugin() {}

void api_register_plugin::plugin_initialize( const variables_map& options ) {}
void api_register_plugin::plugin_startup() {}
void api_register_plugin::plugin_shutdown() {}

string api_register_plugin::call_api( string& body )
{
   try
   {
   idump( (body) );
   vector< std::string > request;
   boost::split( request, body, boost::is_any_of("/") );

   idump( (request) );

   if( request.size() == 3 )
   {
      auto api_itr = _registered_apis.find( request[0] );
      if( api_itr == _registered_apis.end() ) return "";

      auto call_itr = api_itr->second.find( request[1] );
      if( call_itr == api_itr->second.end() ) return "";

      return fc::json::to_string( call_itr->second( request[2] ) );
   }
   else
   {
      return "";
   }
   }
   FC_LOG_AND_RETHROW()
}

} } } // steemit::plugins::api_register