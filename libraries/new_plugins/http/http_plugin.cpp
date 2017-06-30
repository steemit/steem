#include <steemit/plugins/http/http_plugin.hpp>

#include <fc/network/ip.hpp>
#include <fc/log/logger_config.hpp>
#include <fc/io/json.hpp>
#include <fc/network/resolve.hpp>

#include <boost/asio.hpp>
#include <boost/optional.hpp>
#include <boost/bind.hpp>

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/config/asio.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/logger/stub.hpp>
#include <websocketpp/logger/syslog.hpp>

#include <thread>
#include <memory>
#include <iostream>

#define HTTP_THREAD_POOL_SIZE 64

namespace steemit { namespace plugins { namespace http {

namespace asio = boost::asio;

using std::map;
using std::string;
using boost::optional;
using boost::asio::ip::tcp;
using std::shared_ptr;
using websocketpp::connection_hdl;


namespace detail {

   struct asio_with_stub_log : public websocketpp::config::asio
   {
         typedef asio_with_stub_log type;
         typedef asio base;

         typedef base::concurrency_type concurrency_type;

         typedef base::request_type request_type;
         typedef base::response_type response_type;

         typedef base::message_type message_type;
         typedef base::con_msg_manager_type con_msg_manager_type;
         typedef base::endpoint_msg_manager_type endpoint_msg_manager_type;

         //typedef base::alog_type alog_type;
         //typedef base::elog_type elog_type;
         typedef websocketpp::log::stub elog_type;
         typedef websocketpp::log::stub alog_type;

         typedef base::rng_type rng_type;

         struct transport_config : public base::transport_config
         {
            typedef type::concurrency_type concurrency_type;
            typedef type::alog_type alog_type;
            typedef type::elog_type elog_type;
            typedef type::request_type request_type;
            typedef type::response_type response_type;
            typedef websocketpp::transport::asio::basic_socket::endpoint
               socket_type;
         };

         typedef websocketpp::transport::asio::endpoint< transport_config >
            transport_type;

         static const long timeout_open_handshake = 0;
   };
}

using websocket_server_type = websocketpp::server<detail::asio_with_stub_log>;

std::vector<fc::ip::endpoint> resolve_string_to_ip_endpoints( const std::string& endpoint_string )
{
   try
   {
      string::size_type colon_pos = endpoint_string.find( ':' );
      if( colon_pos == std::string::npos )
         FC_THROW( "Missing required port number in endpoint string \"${endpoint_string}\"",
                  ("endpoint_string", endpoint_string) );

      std::string port_string = endpoint_string.substr( colon_pos + 1 );

      try
      {
         uint16_t port = boost::lexical_cast< uint16_t >( port_string );
         std::string hostname = endpoint_string.substr( 0, colon_pos );
         std::vector< fc::ip::endpoint > endpoints = fc::resolve( hostname, port );

         if( endpoints.empty() )
            FC_THROW_EXCEPTION( fc::unknown_host_exception, "The host name can not be resolved: ${hostname}", ("hostname", hostname) );

         return endpoints;
      }
      catch( const boost::bad_lexical_cast& )
      {
         FC_THROW("Bad port: ${port}", ("port", port_string) );
      }
   }
   FC_CAPTURE_AND_RETHROW( (endpoint_string) )
}

class http_plugin_impl
{
   public:
      http_plugin_impl() : api_work( this->api_ios )
      {
         for( uint32_t i = 0; i < HTTP_THREAD_POOL_SIZE; i++ )
            api_thread_pool.create_thread( boost::bind( &asio::io_service::run, &api_ios ) );
      }

      shared_ptr< std::thread >  http_thread;
      asio::io_service           http_ios;

      boost::thread_group        api_thread_pool;
      asio::io_service           api_ios;
      asio::io_service::work     api_work;

      optional< tcp::endpoint >  listen_endpoint;

      websocket_server_type      server;

      plugins::api_register::api_register_plugin* api;
};

http_plugin::http_plugin() : appbase::plugin< http_plugin >( HTTP_PLUGIN_NAME ), _my( new http_plugin_impl() ) {}
http_plugin::~http_plugin(){}

void http_plugin::set_program_options( options_description&, options_description& cfg )
{
   cfg.add_options()
         ("http-server-endpoint", bpo::value<string>()->default_value("127.0.0.1:8888"), "The local IP and port to listen for incoming http connections.")
         ;
   //cli.add( cfg );
}

void http_plugin::plugin_initialize( const variables_map& options )
{
   if( options.count( "http-server-endpoint" ) )
   {
      auto http_endpoint = options.at( "http-server-endpoint" ).as< string >();
      auto endpoints = resolve_string_to_ip_endpoints( http_endpoint );
      FC_ASSERT( endpoints.size(), "http-server-endpoint ${hostname} did not resolve", ("hostname", http_endpoint) );
      _my->listen_endpoint = tcp::endpoint( boost::asio::ip::address_v4::from_string( ( string )endpoints[0].get_address() ), endpoints[0].port() );
      ilog( "configured http to listen on ${ep}", ("ep", endpoints[0]) );
   }
}

void http_plugin::plugin_startup()
{
   if( _my->listen_endpoint )
   {
      _my->api = appbase::app().find_plugin< plugins::api_register::api_register_plugin >();
      FC_ASSERT( _my->api != nullptr, "Could not find API Register Plugin" );

      _my->http_thread = std::make_shared<std::thread>( [&]()
      {
         ilog( "start processing http thread" );
         try
         {
            _my->server.clear_access_channels( websocketpp::log::alevel::all );
            _my->server.clear_error_channels( websocketpp::log::elevel::all );
            _my->server.init_asio( &_my->http_ios );
            _my->server.set_reuse_addr( true );

            _my->server.set_http_handler( [&]( connection_hdl hdl )
            {
               auto con = _my->server.get_con_from_hdl( hdl );
               con->defer_http_response();

               _my->api_ios.post( [con, this]()
               {
                  auto body = con->get_request_body();

                  try
                  {
                     con->set_body( _my->api->call_api( body ) );
                     con->set_status( websocketpp::http::status_code::ok );
                  }
                  catch( fc::exception& e )
                  {
                     con->set_body( "Could not call API" );
                     con->set_status( websocketpp::http::status_code::not_found );
                  }

                  con->send_http_response();
               });
            });

            ilog( "start listening for http requests" );
            _my->server.listen( *_my->listen_endpoint );
            _my->server.start_accept();

            _my->http_ios.run();
            ilog( "http io service exit" );
         }
         catch( ... )
         {
            elog( "error thrown from http io service" );
         }
      });

   }
}

void http_plugin::plugin_shutdown()
{
   if( _my->http_thread )
   {
      if( _my->server.is_listening() )
         _my->server.stop_listening();

      _my->http_ios.stop();
      _my->http_thread->join();
      _my->http_thread.reset();
   }
}

} } } // steemit::plugins::http
