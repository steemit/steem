/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <steemit/delayed_node/delayed_node_plugin.hpp>

#include <steemit/protocol/types.hpp>

#include <steemit/chain/database.hpp>
#include <steemit/app/api.hpp>
#include <steemit/app/database_api.hpp>

#include <fc/network/http/websocket.hpp>
#include <fc/rpc/websocket_api.hpp>
#include <fc/api.hpp>
#include <fc/smart_ref_impl.hpp>


namespace steemit { namespace delayed_node {
namespace bpo = boost::program_options;

namespace detail {
struct delayed_node_plugin_impl {
   std::string remote_endpoint;
   fc::http::websocket_client client;
   std::shared_ptr<fc::rpc::websocket_api_connection> client_connection;
   fc::api<steemit::app::database_api> database_api;
   boost::signals2::scoped_connection client_connection_closed;
   steemit::chain::block_id_type last_received_remote_head;
   steemit::chain::block_id_type last_processed_remote_head;
};
}

delayed_node_plugin::delayed_node_plugin( application* app )
   : plugin( app ), my(new detail::delayed_node_plugin_impl)
{}

delayed_node_plugin::~delayed_node_plugin()
{}

void delayed_node_plugin::plugin_set_program_options(bpo::options_description& cli, bpo::options_description& cfg)
{
   cli.add_options()
         ("trusted-node", boost::program_options::value<std::string>(), "RPC endpoint of a trusted validating node (required)")
         ;
   cfg.add(cli);
}

void delayed_node_plugin::connect()
{
   my->client_connection = std::make_shared<fc::rpc::websocket_api_connection>(*my->client.connect(my->remote_endpoint));
   my->database_api = my->client_connection->get_remote_api<steemit::app::database_api>(0);
   my->client_connection_closed = my->client_connection->closed.connect([this] {
      connection_failed();
   });
}

void delayed_node_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
   FC_ASSERT( options.count( "trusted-node" ) > 0 );
   my->remote_endpoint = "ws://" + options.at("trusted-node").as<std::string>();
}

void delayed_node_plugin::sync_with_trusted_node()
{
   auto& db = database();
   uint32_t synced_blocks = 0;
   uint32_t pass_count = 0;
   while( true )
   {
      steemit::chain::dynamic_global_property_object remote_dpo = my->database_api->get_dynamic_global_properties();
      if( remote_dpo.last_irreversible_block_num <= db.head_block_num() )
      {
         if( remote_dpo.last_irreversible_block_num < db.head_block_num() )
         {
            wlog( "Trusted node seems to be behind delayed node" );
         }
         if( synced_blocks > 1 )
         {
            ilog( "Delayed node finished syncing ${n} blocks in ${k} passes", ("n", synced_blocks)("k", pass_count) );
         }
         break;
      }
      pass_count++;
      while( remote_dpo.last_irreversible_block_num > db.head_block_num() )
      {
         fc::optional<steemit::chain::signed_block> block = my->database_api->get_block( db.head_block_num()+1 );
         FC_ASSERT(block, "Trusted node claims it has blocks it doesn't actually have.");
         ilog("Pushing block #${n}", ("n", block->block_num()));
         db.push_block(*block);
         synced_blocks++;
      }
   }
}

void delayed_node_plugin::mainloop()
{
   while( true )
   {
      try
      {
         fc::usleep( fc::microseconds( 296645 ) );  // wake up a little over 3Hz

         if( my->last_received_remote_head == my->last_processed_remote_head )
            continue;

         sync_with_trusted_node();
         my->last_processed_remote_head = my->last_received_remote_head;
      }
      catch( const fc::exception& e )
      {
         elog("Error during connection: ${e}", ("e", e.to_detail_string()));
      }
   }
}

void delayed_node_plugin::plugin_startup()
{
   fc::async([this]()
   {
      mainloop();
   });

   try
   {
      connect();
      my->database_api->set_block_applied_callback([this]( const fc::variant& block_id )
      {
         fc::from_variant( block_id, my->last_received_remote_head );
      } );
      return;
   }
   catch (const fc::exception& e)
   {
      elog("Error during connection: ${e}", ("e", e.to_detail_string()));
   }
   fc::async([this]{connection_failed();});
}

void delayed_node_plugin::connection_failed()
{
   elog("Connection to trusted node failed; retrying in 5 seconds...");
   fc::schedule([this]{connect();}, fc::time_point::now() + fc::seconds(5));
}

} }

STEEMIT_DEFINE_PLUGIN( delayed_node, steemit::delayed_node::delayed_node_plugin )
