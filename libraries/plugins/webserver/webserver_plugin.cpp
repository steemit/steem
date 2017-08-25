#include <steemit/plugins/webserver/webserver_plugin.hpp>

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

#define WEBSERVER_THREAD_POOL_SIZE 256

namespace steemit { namespace plugins { namespace webserver {

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

         typedef base::alog_type alog_type;
         typedef base::elog_type elog_type;
         //typedef websocketpp::log::stub elog_type;
         //typedef websocketpp::log::stub alog_type;

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

using websocket_server_type = websocketpp::server< detail::asio_with_stub_log >;

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

class webserver_plugin_impl
{
   public:
      webserver_plugin_impl() :
         api_work( this->api_ios )
      {
         for( uint32_t i = 0; i < WEBSERVER_THREAD_POOL_SIZE; i++ )
            api_thread_pool.create_thread( boost::bind( &asio::io_service::run, &api_ios ) );
      }

      shared_ptr< std::thread >  http_thread;
      asio::io_service           http_ios;
      optional< tcp::endpoint >  http_endpoint;
      websocket_server_type      http_server;

      shared_ptr< std::thread >  ws_thread;
      asio::io_service           ws_ios;
      optional< tcp::endpoint >  ws_endpoint;
      websocket_server_type      ws_server;

      boost::thread_group        api_thread_pool;
      asio::io_service           api_ios;
      asio::io_service::work     api_work;

      plugins::json_rpc::json_rpc_plugin* api;
};

} // detail

webserver_plugin::webserver_plugin() : _my( new detail::webserver_plugin_impl() ) {}
webserver_plugin::~webserver_plugin(){}

void webserver_plugin::set_program_options( options_description&, options_description& cfg )
{
   cfg.add_options()
         ("webserver-http-endpoint", bpo::value< string >(), "The local IP and port to listen for incoming http connections.")
         ("webserver-ws-endpoint", bpo::value< string >(), "The local IP and port to listen for incoming websocket connections.")
         ;
}

void webserver_plugin::plugin_initialize( const variables_map& options )
{
   if( options.count( "webserver-http-endpoint" ) )
   {
      auto http_endpoint = options.at( "webserver-http-endpoint" ).as< string >();
      auto endpoints = detail::resolve_string_to_ip_endpoints( http_endpoint );
      FC_ASSERT( endpoints.size(), "webserver-http-endpoint ${hostname} did not resolve", ("hostname", http_endpoint) );
      _my->http_endpoint = tcp::endpoint( boost::asio::ip::address_v4::from_string( ( string )endpoints[0].get_address() ), endpoints[0].port() );
      ilog( "configured http to listen on ${ep}", ("ep", endpoints[0]) );
   }

   if( options.count( "webserver-ws-endpoint" ) )
   {
      auto ws_endpoint = options.at( "webserver-ws-endpoint" ).as< string >();
      auto endpoints = detail::resolve_string_to_ip_endpoints( ws_endpoint );
      FC_ASSERT( endpoints.size(), "ws-server-endpoint ${hostname} did not resolve", ("hostname", ws_endpoint) );
      _my->ws_endpoint = tcp::endpoint( boost::asio::ip::address_v4::from_string( ( string )endpoints[0].get_address() ), endpoints[0].port() );
      ilog( "configured ws to listen on ${ep}", ("ep", endpoints[0]) );
   }
}

void webserver_plugin::plugin_startup()
{
   _my->api = appbase::app().find_plugin< plugins::json_rpc::json_rpc_plugin >();
   FC_ASSERT( _my->api != nullptr, "Could not find API Register Plugin" );

   if( _my->ws_endpoint )
   {
      _my->ws_thread = std::make_shared<std::thread>( [&]()
      {
         ilog( "start processing ws thread" );
         try
         {
            _my->ws_server.clear_access_channels( websocketpp::log::alevel::all );
            _my->ws_server.clear_error_channels( websocketpp::log::elevel::all );
            _my->ws_server.init_asio( &_my->ws_ios );
            _my->ws_server.set_reuse_addr( true );

            _my->ws_server.set_message_handler( [&]( connection_hdl hdl, detail::websocket_server_type::message_ptr msg )
            {
               auto con = _my->ws_server.get_con_from_hdl( hdl );

               _my->api_ios.post( [con, msg, this]()
               {
                  try
                  {
                     if( msg->get_opcode() == websocketpp::frame::opcode::text )
                        con->send( _my->api->call( msg->get_payload() ) );
                     else
                        con->send( "error: string payload expected" );
                  }
                  catch( fc::exception& e )
                  {
                     con->send( "error calling API " + e.to_string() );
                  }
               });
            });

            if( _my->http_endpoint && _my->http_endpoint == _my->ws_endpoint )
            {
               _my->ws_server.set_http_handler( [&]( connection_hdl hdl )
               {
                  auto con = _my->ws_server.get_con_from_hdl( hdl );
                  con->defer_http_response();

                  _my->api_ios.post( [con, this]()
                  {
                     auto body = con->get_request_body();

                     try
                     {
                        con->set_body( _my->api->call( body ) );
                        con->set_status( websocketpp::http::status_code::ok );
                     }
                     catch( fc::exception& e )
                     {
                        edump( (e) );
                        con->set_body( "Could not call API" );
                        con->set_status( websocketpp::http::status_code::not_found );
                     }

                     con->send_http_response();
                  });
               });

               ilog( "start listending for http requests" );
            }

            ilog( "start listening for ws requests" );
            _my->ws_server.listen( *_my->ws_endpoint );
            _my->ws_server.start_accept();

            _my->ws_ios.run();
            ilog( "ws io service exit" );
         }
         catch( ... )
         {
            elog( "error thrown from http io service" );
         }
      });
   }

   if( _my->http_endpoint && ( ( _my->ws_endpoint && _my->ws_endpoint != _my->http_endpoint ) || !_my->ws_endpoint ) )
   {
      _my->http_thread = std::make_shared<std::thread>( [&]()
      {
         ilog( "start processing http thread" );
         try
         {
            _my->http_server.clear_access_channels( websocketpp::log::alevel::all );
            _my->http_server.clear_error_channels( websocketpp::log::elevel::all );
            _my->http_server.init_asio( &_my->http_ios );
            _my->http_server.set_reuse_addr( true );

            _my->http_server.set_http_handler( [&]( connection_hdl hdl )
            {
               auto con = _my->http_server.get_con_from_hdl( hdl );
               con->defer_http_response();

               _my->api_ios.post( [con, this]()
               {
                  auto body = con->get_request_body();

                  try
                  {
                     con->set_body( _my->api->call( body ) );
                     con->set_status( websocketpp::http::status_code::ok );
                  }
                  catch( fc::exception& e )
                  {
                     edump( (e) );
                     con->set_body( "Could not call API" );
                     con->set_status( websocketpp::http::status_code::not_found );
                  }

                  con->send_http_response();
               });
            });

            ilog( "start listening for http requests" );
            _my->http_server.listen( *_my->http_endpoint );
            _my->http_server.start_accept();

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

void webserver_plugin::plugin_shutdown()
{
   if( _my->ws_server.is_listening() )
      _my->ws_server.stop_listening();

   if( _my->http_server.is_listening() )
      _my->http_server.stop_listening();

   _my->api_ios.stop();
   _my->api_thread_pool.join_all();

   if( _my->ws_thread )
   {
      _my->ws_ios.stop();
      _my->ws_thread->join();
      _my->ws_thread.reset();
   }

   if( _my->http_thread )
   {
      _my->http_ios.stop();
      _my->http_thread->join();
      _my->http_thread.reset();
   }
}

} } } // steemit::plugins::webserver
