#include <fc/api.hpp>
#include <fc/log/logger.hpp>
#include <fc/rpc/api_connection.hpp>
#include <fc/rpc/websocket_api.hpp>

class calculator
{
   public:
      int32_t add( int32_t a, int32_t b ); // not implemented
      int32_t sub( int32_t a, int32_t b ); // not implemented
      void    on_result( const std::function<void(int32_t)>& cb );
      void    on_result2( const std::function<void(int32_t)>& cb, int test );
};

FC_API( calculator, (add)(sub)(on_result)(on_result2) )


class login_api
{
   public:
      fc::api<calculator> get_calc()const
      {
         FC_ASSERT( calc );
         return *calc;
      }
      fc::optional<fc::api<calculator>> calc;
      std::set<std::string> test( const std::string&, const std::string& ) { return std::set<std::string>(); }
};
FC_API( login_api, (get_calc)(test) );

using namespace fc;

class some_calculator
{
   public:
      int32_t add( int32_t a, int32_t b ) { wlog("."); if( _cb ) _cb(a+b); return a+b; }
      int32_t sub( int32_t a, int32_t b ) {  wlog(".");if( _cb ) _cb(a-b); return a-b; }
      void    on_result( const std::function<void(int32_t)>& cb ) { wlog( "set callback" ); _cb = cb;  return ; }
      void    on_result2(  const std::function<void(int32_t)>& cb, int test ){}
      std::function<void(int32_t)> _cb;
};
class variant_calculator
{
   public:
      double add( fc::variant a, fc::variant b ) { return a.as_double()+b.as_double(); }
      double sub( fc::variant a, fc::variant b ) { return a.as_double()-b.as_double(); }
      void    on_result( const std::function<void(int32_t)>& cb ) { wlog("set callback"); _cb = cb; return ; }
      void    on_result2(  const std::function<void(int32_t)>& cb, int test ){}
      std::function<void(int32_t)> _cb;
};

using namespace fc::http;
using namespace fc::rpc;

int main( int argc, char** argv )
{
   try {
      fc::api<calculator> calc_api( std::make_shared<some_calculator>() );

      fc::http::websocket_server server;
      server.on_connection([&]( const websocket_connection_ptr& c ){
               auto wsc = std::make_shared<websocket_api_connection>(*c);
               auto login = std::make_shared<login_api>();
               login->calc = calc_api;
               wsc->register_api(fc::api<login_api>(login));
               c->set_session_data( wsc );
          });

      server.listen( 8090 );
      server.start_accept();

      for( uint32_t i = 0; i < 5000; ++i )
      {
         try { 
            fc::http::websocket_client client;
            auto con  = client.connect( "ws://localhost:8090" );
            auto apic = std::make_shared<websocket_api_connection>(*con);
            auto remote_login_api = apic->get_remote_api<login_api>();
            auto remote_calc = remote_login_api->get_calc();
            remote_calc->on_result( []( uint32_t r ) { elog( "callback result ${r}", ("r",r) ); } );
            wdump((remote_calc->add( 4, 5 )));
         } catch ( const fc::exception& e )
         {
            edump((e.to_detail_string()));
         }
      }
      wlog( "exit scope" );
   } 
   catch( const fc::exception& e )
   {
      edump((e.to_detail_string()));
   }
   wlog( "returning now..." );
   
   return 0;

   some_calculator calc;
   variant_calculator vcalc;

   fc::api<calculator> api_calc( &calc );
   fc::api<calculator> api_vcalc( &vcalc );
   fc::api<calculator> api_nested_calc( api_calc );

   wdump( (api_calc->add(5,4)) );
   wdump( (api_calc->sub(5,4)) );
   wdump( (api_vcalc->add(5,4)) );
   wdump( (api_vcalc->sub(5,4)) );
   wdump( (api_nested_calc->sub(5,4)) );
   wdump( (api_nested_calc->sub(5,4)) );

   /*
   variants v = { 4, 5 };
   auto g = to_generic( api_calc->add );
   auto r = call_generic( api_calc->add, v.begin(), v.end() );
   wdump((r));
   wdump( (g(v)) );
   */

   /*
   try {
      fc::api_server server;
      auto api_id = server.register_api( api_calc );
      wdump( (api_id) );
      auto result = server.call( api_id, "add", {4, 5} );
      wdump( (result) );
   } catch ( const fc::exception& e )
   {
      elog( "${e}", ("e",e.to_detail_string() ) );
   }

   ilog( "------------------ NESTED TEST --------------" );
   try {
      login_api napi_impl;
      napi_impl.calc = api_calc;
      fc::api<login_api>  napi(&napi_impl);

      fc::api_server server;
      auto api_id = server.register_api( napi );
      wdump( (api_id) );
      auto result = server.call( api_id, "get_calc" );
      wdump( (result) );
      result = server.call( result.as_uint64(), "add", {4,5} );
      wdump( (result) );


      fc::api<api_server> serv( &server );

      fc::api_client<login_api> apic( serv );

      fc::api<login_api> remote_api = apic;


      auto remote_calc = remote_api->get_calc();
      int r = remote_calc->add( 4, 5 );
      idump( (r) );

   } catch ( const fc::exception& e )
   {
      elog( "${e}", ("e",e.to_detail_string() ) );
   }
   */

   ilog( "------------------ NESTED TEST --------------" );
   try {
      login_api napi_impl;
      napi_impl.calc = api_calc;
      fc::api<login_api>  napi(&napi_impl);


      auto client_side = std::make_shared<local_api_connection>();
      auto server_side = std::make_shared<local_api_connection>();
      server_side->set_remote_connection( client_side );
      client_side->set_remote_connection( server_side );

      server_side->register_api( napi );

      fc::api<login_api> remote_api = client_side->get_remote_api<login_api>();

      auto remote_calc = remote_api->get_calc();
      int r = remote_calc->add( 4, 5 );
      idump( (r) );

   } catch ( const fc::exception& e )
   {
      elog( "${e}", ("e",e.to_detail_string() ) );
   }

   return 0;
}
