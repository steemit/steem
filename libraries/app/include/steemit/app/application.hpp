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
#pragma once

#include <steemit/app/api_access.hpp>
#include <steemit/app/api_context.hpp>
#include <steemit/chain/database.hpp>

#include <graphene/net/node.hpp>

#include <fc/api.hpp>
#include <fc/rpc/api_connection.hpp>
#include <fc/rpc/websocket_api.hpp>

#include <boost/program_options.hpp>

namespace steemit { namespace app {
   namespace detail { class application_impl; }
   using std::string;

   class abstract_plugin;
   class plugin;
   class application;

   class network_broadcast_api;
   class login_api;

   class application
   {
      public:
         application();
         ~application();

         void set_program_options( boost::program_options::options_description& command_line_options,
                                   boost::program_options::options_description& configuration_file_options )const;
         void initialize(const fc::path& data_dir, const boost::program_options::variables_map&options);
         void initialize_plugins( const boost::program_options::variables_map& options );
         void startup();
         void shutdown();
         void startup_plugins();
         void shutdown_plugins();

         template<typename PluginType>
         std::shared_ptr<PluginType> register_plugin()
         {
            auto plug = std::make_shared<PluginType>( this );
            register_abstract_plugin( plug );
            return plug;
         }

         void register_abstract_plugin( std::shared_ptr< abstract_plugin > plug );
         void enable_plugin( const std::string& name );
         std::shared_ptr<abstract_plugin> get_plugin( const string& name )const;

         template<typename PluginType>
         std::shared_ptr<PluginType> get_plugin( const string& name ) const
         {
            std::shared_ptr<abstract_plugin> abs_plugin = get_plugin( name );
            std::shared_ptr<PluginType> result = std::dynamic_pointer_cast<PluginType>( abs_plugin );
            FC_ASSERT( result != std::shared_ptr<PluginType>() );
            return result;
         }

         graphene::net::node_ptr                    p2p_node();
         std::shared_ptr<chain::database> chain_database()const;
         //std::shared_ptr<graphene::db::object_database> pending_trx_database() const;

         void set_block_production(bool producing_blocks);
         fc::optional< api_access_info > get_api_access_info( const string& username )const;
         void set_api_access_info(const string& username, api_access_info&& permissions);

         /**
          * Register a way to instantiate the named API with the application.
          */
         void register_api_factory( const string& name, std::function< fc::api_ptr( const api_context& ) > factory );

         /**
          * Convenience method to build an API factory from a type which only requires a reference to the application.
          */
         template< typename Api >
         void register_api_factory( const string& name )
         {
#ifndef IS_TEST_NET
            idump((name));
#endif
            register_api_factory( name, []( const api_context& ctx ) -> fc::api_ptr
            {
               // apparently the compiler is smart enough to downcast shared_ptr< api<Api> > to shared_ptr< api_base > automatically
               // see http://en.cppreference.com/w/cpp/memory/shared_ptr/pointer_cast for example
               std::shared_ptr< Api > api = std::make_shared< Api >( ctx );
               api->on_api_startup();
               return std::make_shared< fc::api< Api > >( api );
            } );
         }

         /**
          * Instantiate the named API.  Currently this simply calls the previously registered factory method.
          */
         fc::api_ptr create_api_by_name( const api_context& ctx );

         void get_max_block_age( int32_t& result );

         void connect_to_write_node();

         bool _read_only = true;
         bool _disable_get_block = false;
         fc::optional< string > _remote_endpoint;
         fc::optional< fc::api< network_broadcast_api > > _remote_net_api;
         fc::optional< fc::api< login_api > > _remote_login;
         fc::http::websocket_connection_ptr _ws_ptr;
         std::shared_ptr< fc::rpc::websocket_api_connection > _ws_apic;
         fc::http::websocket_client _client;

      private:
         std::shared_ptr<detail::application_impl> my;

         boost::program_options::options_description _cli_options;
         boost::program_options::options_description _cfg_options;

         const std::shared_ptr< plugin > null_plugin;
   };

   template< class C, typename... Args >
   boost::signals2::scoped_connection connect_signal( boost::signals2::signal< void(Args...) >& sig, C& c, void(C::* f)(Args...) )
   {
      std::weak_ptr<C> weak_c = c.shared_from_this();
      return sig.connect(
         [weak_c,f](Args... args)
         {
            std::shared_ptr<C> shared_c = weak_c.lock();
            if( !shared_c )
               return;
            ((*shared_c).*f)(args...);
      } );
   }

} } // steemit::app
