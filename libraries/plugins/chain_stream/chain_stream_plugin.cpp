
#include <steemit/chain/chain_enodes.hpp>
#include <steemit/chain/enode.hpp>

#include <steemit/plugins/chain_stream/chain_stream_api.hpp>
#include <steemit/plugins/chain_stream/chain_stream_plugin.hpp>

#include <fc/network/ip.hpp>
#include <fc/network/tcp_socket.hpp>

#include <fc/variant.hpp>

#include <string>

namespace steemit { namespace plugin { namespace chain_stream {

namespace detail {

using steemit::chain::block_enode;
using steemit::chain::enode;

class chain_stream_plugin_impl
{
   public:
      chain_stream_plugin_impl( steemit::app::application& app );
      virtual ~chain_stream_plugin_impl();

      virtual std::string plugin_name()const;
      virtual void plugin_set_program_options(
         boost::program_options::options_description& cli,
         boost::program_options::options_description& cfg);
      virtual void plugin_initialize( const boost::program_options::variables_map& options );
      virtual void plugin_startup();
      virtual void plugin_shutdown();
      void on_applied_block( const chain::signed_block& b );
      void on_push_enode( const enode& node );
      void on_pop_enode( const enode& node );

      std::vector< fc::variant > _nodes;
      steemit::app::application& _app;
      boost::signals2::scoped_connection _applied_block_conn;
      boost::signals2::scoped_connection _push_enode_conn;
      boost::signals2::scoped_connection _pop_enode_conn;
      fc::tcp_socket _stream_socket;
};

chain_stream_plugin_impl::chain_stream_plugin_impl( steemit::app::application& app )
  : _app(app) {}

chain_stream_plugin_impl::~chain_stream_plugin_impl() {}

std::string chain_stream_plugin_impl::plugin_name()const
{
   return "chain_stream";
}

void chain_stream_plugin_impl::plugin_set_program_options(
   boost::program_options::options_description& cli,
   boost::program_options::options_description& cfg)
{
   cli.add_options()
         ("chain-stream", boost::program_options::value<std::string>(), "TCP server which will receive newline-delimited JSON")
         ;
   cfg.add(cli);
}

void chain_stream_plugin_impl::plugin_initialize( const boost::program_options::variables_map& options )
{
   if( options.count("chain-stream") == 0 )
   {
      wlog( "chain_stream plugin is loaded, but will be disabled because --chain-stream was not specified" );
      wlog( "to resolve this issue, remove chain_stream from plugins or add --chain-stream option" );
      return;
   }
   fc::ip::endpoint ep = fc::ip::endpoint::from_string( options.at("chain-stream").as<std::string>() );
   _stream_socket.connect_to( ep );

   _applied_block_conn = _app.chain_database()->applied_block.connect(
      [this](const chain::signed_block& b){ on_applied_block(b); });

   _push_enode_conn = _app.chain_database()->on_push_enode.connect(
      [this](const chain::enode& n ){ on_push_enode(n); });

   _pop_enode_conn = _app.chain_database()->on_pop_enode.connect(
      [this](const chain::enode& n ){ on_pop_enode(n); });
}

void chain_stream_plugin_impl::plugin_startup()
{
   _app.register_api_factory< chain_stream_api >( "chain_stream_api" );
}

void chain_stream_plugin_impl::plugin_shutdown()
{
}

void chain_stream_plugin_impl::on_applied_block( const chain::signed_block& b )
{
   ilog( "on_applied_block()" );
   const chain::database& db = *_app.chain_database();

   fc::mutable_variant_object line;
   line("bid", b.id())
       ("b", b)
       ("dgp", db.get_dynamic_global_properties())
       ("en", _nodes)
       ;
   std::string json_line = fc::json::to_string( line );
   json_line += "\n";
   _stream_socket.write( json_line.c_str(), json_line.size() );
   _stream_socket.flush();
}

void chain_stream_plugin_impl::on_push_enode( const enode& node )
{
   ilog( "on_push_enode ${t}", ("t", node.type->get_name()) );
   if( node.is< block_enode >() )
   {
      ilog( "clearing node list" );
      _nodes.clear();
   }
}

void chain_stream_plugin_impl::on_pop_enode( const enode& node )
{
   ilog( "on_pop_enode ${t}", ("t", node.type->get_name()) );
   try
   {
      chain::enode_type* et = node.type;
      FC_ASSERT( et != nullptr );
      while( _nodes.size() <= size_t( node.enid ) )
         _nodes.emplace_back();
      fc::variant& v = _nodes[node.enid];
      FC_ASSERT( v.is_null() );
      et->to_variant( node, v );
   }
   FC_LOG_AND_RETHROW()
}

}

chain_stream_plugin::chain_stream_plugin( steemit::app::application* app )
   : plugin(app)
{
   FC_ASSERT( app != nullptr );
   my = std::make_shared< detail::chain_stream_plugin_impl >( *app );
}

chain_stream_plugin::~chain_stream_plugin() {}

std::string chain_stream_plugin::plugin_name()const
{
   return my->plugin_name();
}

void chain_stream_plugin::plugin_set_program_options(
   boost::program_options::options_description& cli,
   boost::program_options::options_description& cfg)
{
   my->plugin_set_program_options( cli, cfg );
}

void chain_stream_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   my->plugin_initialize( options );
}

void chain_stream_plugin::plugin_startup()
{
   my->plugin_startup();
}

void chain_stream_plugin::plugin_shutdown()
{
   my->plugin_shutdown();
}

} } } // steemit::plugin::chain_stream

STEEMIT_DEFINE_PLUGIN( chain_stream, steemit::plugin::chain_stream::chain_stream_plugin )
