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
#include <steemit/app/api.hpp>
#include <steemit/app/api_access.hpp>
#include <steemit/app/application.hpp>
#include <steemit/app/plugin.hpp>

#include <steemit/chain/steem_objects.hpp>
#include <steemit/chain/steem_object_types.hpp>
#include <steemit/chain/database_exceptions.hpp>

#include <graphene/net/core_messages.hpp>
#include <graphene/net/exceptions.hpp>

#include <graphene/time/time.hpp>

#include <graphene/utilities/key_conversion.hpp>

#include <fc/smart_ref_impl.hpp>

#include <fc/io/fstream.hpp>
#include <fc/rpc/api_connection.hpp>
#include <fc/rpc/websocket_api.hpp>
#include <fc/network/resolve.hpp>
#include <fc/string.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/signals2.hpp>
#include <boost/range/algorithm/reverse.hpp>

#include <iostream>

#include <fc/log/file_appender.hpp>
#include <fc/log/logger.hpp>
#include <fc/log/logger_config.hpp>

#include <boost/range/adaptor/reversed.hpp>

namespace steemit { namespace app {
using graphene::net::item_hash_t;
using graphene::net::item_id;
using graphene::net::message;
using graphene::net::block_message;
using graphene::net::trx_message;

using protocol::block_header;
using protocol::signed_block_header;
using protocol::signed_block;
using protocol::block_id_type;

using std::vector;

namespace bpo = boost::program_options;

api_context::api_context( application& _app, const std::string& _api_name, std::weak_ptr< api_session_data > _session )
   : app(_app), api_name(_api_name), session(_session) {}

namespace detail {

   class application_impl : public graphene::net::node_delegate
   {
   public:
      fc::optional<fc::temp_file> _lock_file;
      bool _is_block_producer = false;
      bool _force_validate = false;

      void reset_p2p_node(const fc::path& data_dir)
      { try {
         _p2p_network = std::make_shared<graphene::net::node>("Graphene Reference Implementation");

         _p2p_network->load_configuration(data_dir / "p2p");
         _p2p_network->set_node_delegate(this);

         if( _options->count("seed-node") )
         {
            auto seeds = _options->at("seed-node").as<vector<string>>();
            for( const string& endpoint_string : seeds )
            {
               try {
                  std::vector<fc::ip::endpoint> endpoints = resolve_string_to_ip_endpoints(endpoint_string);
                  for (const fc::ip::endpoint& endpoint : endpoints)
                  {
                     ilog("Adding seed node ${endpoint}", ("endpoint", endpoint));
                     _p2p_network->add_node(endpoint);
                     _p2p_network->connect_to_endpoint(endpoint);
                  }
               } catch( const fc::exception& e ) {
                  wlog( "caught exception ${e} while adding seed node ${endpoint}",
                           ("e", e.to_detail_string())("endpoint", endpoint_string) );
               }
            }
         }

         if( _options->count("p2p-endpoint") )
         {
            auto p2p_endpoint = _options->at("p2p-endpoint").as<string>();
            auto endpoints = resolve_string_to_ip_endpoints( p2p_endpoint );
            FC_ASSERT( endpoints.size(), "p2p-endpoint ${hostname} did not resolve", ("hostname", p2p_endpoint) );
            _p2p_network->listen_on_endpoint( endpoints[0], true);
         }
         else
            _p2p_network->listen_on_port(0, false);

         if( _options->count("p2p-max-connections") )
         {
            fc::variant_object node_param = fc::variant_object(
               "maximum_number_of_connections",
               fc::variant( _options->at("p2p-max-connections").as<uint32_t>() ) );
            _p2p_network->set_advanced_node_parameters( node_param );
            ilog("Setting p2p max connections to ${n}", ("n", node_param["maximum_number_of_connections"]));
         }

         _p2p_network->listen_to_p2p_network();
         ilog("Configured p2p node to listen on ${ip}", ("ip", _p2p_network->get_actual_listening_endpoint()));

         _p2p_network->connect_to_p2p_network();
         idump( (_chain_db->head_block_id()) );
         _p2p_network->sync_from(graphene::net::item_id(graphene::net::core_message_type_enum::block_message_type,
                                              _chain_db->head_block_id()),
                                 std::vector<uint32_t>());
      } FC_CAPTURE_AND_RETHROW() }

      std::vector<fc::ip::endpoint> resolve_string_to_ip_endpoints(const std::string& endpoint_string)
      {
         try
         {
            string::size_type colon_pos = endpoint_string.find(':');
            if (colon_pos == std::string::npos)
               FC_THROW("Missing required port number in endpoint string \"${endpoint_string}\"",
                        ("endpoint_string", endpoint_string));
            std::string port_string = endpoint_string.substr(colon_pos + 1);
            try
            {
               uint16_t port = boost::lexical_cast<uint16_t>(port_string);

               std::string hostname = endpoint_string.substr(0, colon_pos);
               std::vector<fc::ip::endpoint> endpoints = fc::resolve(hostname, port);
               if (endpoints.empty())
                  FC_THROW_EXCEPTION(fc::unknown_host_exception, "The host name can not be resolved: ${hostname}", ("hostname", hostname));
               return endpoints;
            }
            catch (const boost::bad_lexical_cast&)
            {
               FC_THROW("Bad port: ${port}", ("port", port_string));
            }
         }
         FC_CAPTURE_AND_RETHROW((endpoint_string))
      }

      void reset_websocket_server()
      { try {
         if( !_options->count("rpc-endpoint") )
            return;

         _websocket_server = std::make_shared<fc::http::websocket_server>();

         _websocket_server->on_connection([&]( const fc::http::websocket_connection_ptr& c ){ on_connection(c); } );
         auto rpc_endpoint = _options->at("rpc-endpoint").as<string>();
         ilog("Configured websocket rpc to listen on ${ip}", ("ip", rpc_endpoint));
         auto endpoints = resolve_string_to_ip_endpoints( rpc_endpoint );
         FC_ASSERT( endpoints.size(), "rpc-endpoint ${hostname} did not resolve", ("hostname", rpc_endpoint) );
         _websocket_server->listen( endpoints[0] );
         _websocket_server->start_accept();
      } FC_CAPTURE_AND_RETHROW() }


      void reset_websocket_tls_server()
      { try {
         if( !_options->count("rpc-tls-endpoint") )
            return;
         if( !_options->count("server-pem") )
         {
            wlog( "Please specify a server-pem to use rpc-tls-endpoint" );
            return;
         }

         string password = _options->count("server-pem-password") ? _options->at("server-pem-password").as<string>() : "";
         _websocket_tls_server = std::make_shared<fc::http::websocket_tls_server>( _options->at("server-pem").as<string>(), password );

         _websocket_tls_server->on_connection([this]( const fc::http::websocket_connection_ptr& c ){ on_connection(c); } );
         auto rpc_tls_endpoint = _options->at("rpc-tls-endpoint").as<string>();
         ilog("Configured websocket TLS rpc to listen on ${ip}", ("ip", rpc_tls_endpoint));
         auto endpoints = resolve_string_to_ip_endpoints( rpc_tls_endpoint );
         FC_ASSERT( endpoints.size(), "rpc-tls-endpoint ${hostname} did not resolve", ("hostname", rpc_tls_endpoint) );
         _websocket_tls_server->listen( endpoints[0] );
         _websocket_tls_server->start_accept();
      } FC_CAPTURE_AND_RETHROW() }

      void on_connection( const fc::http::websocket_connection_ptr& c )
      {
         std::shared_ptr< api_session_data > session = std::make_shared<api_session_data>();
         session->wsc = std::make_shared<fc::rpc::websocket_api_connection>(*c);

         for( const std::string& name : _public_apis )
         {
            api_context ctx( *_self, name, session );
            fc::api_ptr api = create_api_by_name( ctx );
            if( !api )
            {
               elog( "Couldn't create API ${name}", ("name", name) );
               continue;
            }
            session->api_map[name] = api;
            api->register_api( *session->wsc );
         }
         c->set_session_data( session );
      }

      application_impl(application* self)
         : _self(self),
           //_pending_trx_db(std::make_shared<graphene::db::object_database>()),
           _chain_db(std::make_shared<chain::database>())
      {
      }

      ~application_impl(){}

      void register_builtin_apis()
      {
         _self->register_api_factory< login_api >( "login_api" );
         _self->register_api_factory< database_api >( "database_api" );
         _self->register_api_factory< network_node_api >( "network_node_api" );
         _self->register_api_factory< network_broadcast_api >( "network_broadcast_api" );
      }

      void startup()
      { try {
         _shared_file_size = fc::parse_size( _options->at( "shared-file-size" ).as< string >() );
         ilog( "shared_file_size is ${n} bytes", ("n", _shared_file_size) );
         bool read_only = _options->count( "read-only" );
         register_builtin_apis();

         if( _options->count("shared-file-dir") )
            _shared_dir = fc::path( _options->at("shared-file-dir").as<string>() );
         else
            _shared_dir = _data_dir / "blockchain";

         if( !read_only )
         {
            _self->_read_only = false;
            ilog( "Starting Steem node in write mode." );
            _max_block_age =_options->at("max-block-age").as<int32_t>();

            if( _options->count("resync-blockchain") )
               _chain_db->wipe(_data_dir / "blockchain", _shared_dir, true);

            _chain_db->set_flush_interval( _options->at("flush").as<uint32_t>() );

            flat_map<uint32_t,block_id_type> loaded_checkpoints;
            if( _options->count("checkpoint") )
            {
               auto cps = _options->at("checkpoint").as<vector<string>>();
               loaded_checkpoints.reserve( cps.size() );
               for( auto cp : cps )
               {
                  auto item = fc::json::from_string(cp).as<std::pair<uint32_t,block_id_type> >();
                  loaded_checkpoints[item.first] = item.second;
               }
            }
            _chain_db->add_checkpoints( loaded_checkpoints );

            if( _options->count("replay-blockchain") )
            {
               ilog("Replaying blockchain on user request.");
               _chain_db->reindex( _data_dir / "blockchain", _shared_dir, _shared_file_size );
            }
            else
            {
               try
               {
                  _chain_db->open(_data_dir / "blockchain", _shared_dir, 0, _shared_file_size, chainbase::database::read_write );\
               }
               catch( fc::assert_exception& )
               {
                  wlog( "Error when opening database. Attempting reindex..." );

                  try
                  {
                     _chain_db->reindex( _data_dir / "blockchain", _shared_dir, _shared_file_size );
                  }
                  catch( chain::block_log_exception& )
                  {
                     wlog( "Error opening block log. Having to resync from network..." );
                     _chain_db->open( _data_dir / "blockchain", _shared_dir, 0, _shared_file_size, chainbase::database::read_write );
                  }
               }
            }

            if( _options->count("force-validate") )
            {
               ilog( "All transaction signatures will be validated" );
               _force_validate = true;
            }

            graphene::time::now();
         }
         else
         {
            ilog( "Starting Steem node in read mode." );
            _chain_db->open( _data_dir / "blockchain", _shared_dir, 0, _shared_file_size, chainbase::database::read_only );

            if( _options->count( "read-forward-rpc" ) )
            {
               try
               {
                  auto ws_ptr = _self->_client.connect( _options->at( "read-forward-rpc" ).as< string >() );
                  auto apic = std::make_shared< fc::rpc::websocket_api_connection >( *ws_ptr );
                  auto login = apic->get_remote_api< login_api >( 1 );
                  FC_ASSERT( login->login( "", "" ) );
                  _self->_remote_net_api = login->get_api_by_name( "network_broadcast_api" )->as< network_broadcast_api >();
               }
               catch( fc::exception& e )
               {
                  wlog( "Error connecting to remote RPC, network api forwarding disabled.  ${e}", ("e", e.to_detail_string()) );
               }
            }
         }

         if( _options->count("api-user") )
         {
            for( const std::string& api_access_str : _options->at("api-user").as< std::vector<std::string> >() )
            {
               api_access_info info = fc::json::from_string( api_access_str ).as<api_access_info>();
               FC_ASSERT( info.username != "" );
               _apiaccess.permission_map[info.username] = info;
            }
         }
         else
         {
            // TODO:  Remove this generous default access policy
            // when the UI logs in properly
            _apiaccess = api_access();
            api_access_info wild_access;
            wild_access.username = "*";
            wild_access.password_hash_b64 = "*";
            wild_access.password_salt_b64 = "*";
            wild_access.allowed_apis.push_back( "database_api" );
            wild_access.allowed_apis.push_back( "network_broadcast_api" );
            wild_access.allowed_apis.push_back( "tag_api" );
            _apiaccess.permission_map["*"] = wild_access;
         }

         for( const std::string& arg : _options->at("public-api").as< std::vector< std::string > >() )
         {
            vector<string> names;
            boost::split(names, arg, boost::is_any_of(" \t,"));
            for( const std::string& name : names )
            {
               ilog( "API ${name} enabled publicly", ("name", name) );
               _public_apis.push_back( name );
            }
         }
         _running = true;

         if( !read_only )
         {
            reset_p2p_node(_data_dir);
         }

         reset_websocket_server();
         reset_websocket_tls_server();
      } FC_LOG_AND_RETHROW() }

      optional< api_access_info > get_api_access_info(const string& username)const
      {
         optional< api_access_info > result;
         auto it = _apiaccess.permission_map.find(username);
         if( it == _apiaccess.permission_map.end() )
         {
            it = _apiaccess.permission_map.find("*");
            if( it == _apiaccess.permission_map.end() )
               return result;
         }
         return it->second;
      }

      void set_api_access_info(const string& username, api_access_info&& permissions)
      {
         _apiaccess.permission_map.insert(std::make_pair(username, std::move(permissions)));
      }

      void register_api_factory( const string& name, std::function< fc::api_ptr( const api_context& ) > factory )
      {
         _api_factories_by_name[name] = factory;
      }

      fc::api_ptr create_api_by_name( const api_context& ctx )
      {
         auto it = _api_factories_by_name.find(ctx.api_name);
         if( it == _api_factories_by_name.end() )
         {
            wlog( "unknown api: ${api}", ("api",ctx.api_name) );
            return nullptr;
         }
         return it->second(ctx);
      }

      /**
       * If delegate has the item, the network has no need to fetch it.
       */
      virtual bool has_item(const graphene::net::item_id& id) override
      {
         // If the node is shutting down we don't care about fetching items
         if( !_running )
            return true;

         try
         {
            if( id.item_type == graphene::net::block_message_type )
               return _chain_db->is_known_block(id.item_hash);
            else
               return _chain_db->is_known_transaction(id.item_hash);
         }
         FC_CAPTURE_AND_RETHROW( (id) )
      }

      /**
       * @brief allows the application to validate an item prior to broadcasting to peers.
       *
       * @param sync_mode true if the message was fetched through the sync process, false during normal operation
       * @returns true if this message caused the blockchain to switch forks, false if it did not
       *
       * @throws exception if error validating the item, otherwise the item is safe to broadcast on.
       */
      virtual bool handle_block(const graphene::net::block_message& blk_msg, bool sync_mode,
                                std::vector<fc::uint160_t>& contained_transaction_message_ids) override
      { try {
         if( _running )
         {
            if (sync_mode)
               fc_ilog(fc::logger::get("sync"),
                     "chain pushing sync block #${block_num} ${block_hash}, head is ${head}",
                     ("block_num", blk_msg.block.block_num())
                     ("block_hash", blk_msg.block_id)
                     ("head", _chain_db->head_block_num()));
            else
               fc_ilog(fc::logger::get("sync"),
                     "chain pushing block #${block_num} ${block_hash}, head is ${head}",
                     ("block_num", blk_msg.block.block_num())
                     ("block_hash", blk_msg.block_id)
                     ("head", _chain_db->head_block_num()));
            if (sync_mode && blk_msg.block.block_num() % 10000 == 0)
            {
               ilog("Syncing Blockchain --- Got block: #${n} time: ${t}",
                  ("t",blk_msg.block.timestamp)
                  ("n", blk_msg.block.block_num()) );
            }

            time_point_sec now = graphene::time::now();

            uint64_t max_accept_time = now.sec_since_epoch();
            max_accept_time += allow_future_time;
            FC_ASSERT( blk_msg.block.timestamp.sec_since_epoch() <= max_accept_time );

            try {
               // TODO: in the case where this block is valid but on a fork that's too old for us to switch to,
               // you can help the network code out by throwing a block_older_than_undo_history exception.
               // when the net code sees that, it will stop trying to push blocks from that chain, but
               // leave that peer connected so that they can get sync blocks from us
               bool result = _chain_db->push_block(blk_msg.block, (_is_block_producer | _force_validate) ? database::skip_nothing : database::skip_transaction_signatures);

               if( !sync_mode && blk_msg.block.transactions.size() )
               {
                  ilog( "Got ${t} transactions from network on block ${b}",
                     ("t", blk_msg.block.transactions.size())
                     ("b", blk_msg.block.block_num()) );
               }

               return result;
            } catch ( const steemit::chain::unlinkable_block_exception& e ) {
               // translate to a graphene::net exception
               fc_elog(fc::logger::get("sync"),
                     "Error when pushing block, current head block is ${head}:\n${e}",
                     ("e", e.to_detail_string())
                     ("head", _chain_db->head_block_num()));
               elog("Error when pushing block:\n${e}", ("e", e.to_detail_string()));
               FC_THROW_EXCEPTION(graphene::net::unlinkable_block_exception, "Error when pushing block:\n${e}", ("e", e.to_detail_string()));
            } catch( const fc::exception& e ) {
               fc_elog(fc::logger::get("sync"),
                     "Error when pushing block, current head block is ${head}:\n${e}",
                     ("e", e.to_detail_string())
                     ("head", _chain_db->head_block_num()));
               elog("Error when pushing block:\n${e}", ("e", e.to_detail_string()));
               throw;
            }
         }
         return false;
      } FC_CAPTURE_AND_RETHROW( (blk_msg)(sync_mode) ) }

      virtual void handle_transaction(const graphene::net::trx_message& transaction_message) override
      { try {
         if( _running )
            _chain_db->push_transaction( transaction_message.trx );
      } FC_CAPTURE_AND_RETHROW( (transaction_message) ) }

      virtual void handle_message(const message& message_to_process) override
      {
         // not a transaction, not a block
         FC_THROW( "Invalid Message Type" );
      }

      bool is_included_block(const block_id_type& block_id)
      {
        uint32_t block_num = block_header::num_from_id(block_id);
        block_id_type block_id_in_preferred_chain = _chain_db->get_block_id_for_num(block_num);
        return block_id == block_id_in_preferred_chain;
      }

      /**
       * Assuming all data elements are ordered in some way, this method should
       * return up to limit ids that occur *after* the last ID in synopsis that
       * we recognize.
       *
       * On return, remaining_item_count will be set to the number of items
       * in our blockchain after the last item returned in the result,
       * or 0 if the result contains the last item in the blockchain
       */
      virtual std::vector<item_hash_t> get_block_ids(const std::vector<item_hash_t>& blockchain_synopsis,
                                                     uint32_t& remaining_item_count,
                                                     uint32_t limit) override
      { try {
         vector<block_id_type> result;
         remaining_item_count = 0;
         if( _chain_db->head_block_num() == 0 )
            return result;

         result.reserve(limit);
         block_id_type last_known_block_id;

         if (blockchain_synopsis.empty() ||
             (blockchain_synopsis.size() == 1 && blockchain_synopsis[0] == block_id_type()))
         {
           // peer has sent us an empty synopsis meaning they have no blocks.
           // A bug in old versions would cause them to send a synopsis containing block 000000000
           // when they had an empty blockchain, so pretend they sent the right thing here.

           // do nothing, leave last_known_block_id set to zero
         }
         else
         {
           bool found_a_block_in_synopsis = false;
           for (const item_hash_t& block_id_in_synopsis : boost::adaptors::reverse(blockchain_synopsis))
             if (block_id_in_synopsis == block_id_type() ||
                 (_chain_db->is_known_block(block_id_in_synopsis) && is_included_block(block_id_in_synopsis)))
             {
               last_known_block_id = block_id_in_synopsis;
               found_a_block_in_synopsis = true;
               break;
             }
           if (!found_a_block_in_synopsis)
             FC_THROW_EXCEPTION(graphene::net::peer_is_on_an_unreachable_fork, "Unable to provide a list of blocks starting at any of the blocks in peer's synopsis");
         }
         for( uint32_t num = block_header::num_from_id(last_known_block_id);
              num <= _chain_db->head_block_num() && result.size() < limit;
              ++num )
            if( num > 0 )
               result.push_back(_chain_db->get_block_id_for_num(num));

         if( !result.empty() && block_header::num_from_id(result.back()) < _chain_db->head_block_num() )
            remaining_item_count = _chain_db->head_block_num() - block_header::num_from_id(result.back());

         return result;
      } FC_CAPTURE_AND_RETHROW( (blockchain_synopsis)(remaining_item_count)(limit) ) }

      /**
       * Given the hash of the requested data, fetch the body.
       */
      virtual message get_item(const item_id& id) override
      { try {
        // ilog("Request for item ${id}", ("id", id));
         if( id.item_type == graphene::net::block_message_type )
         {
            auto opt_block = _chain_db->fetch_block_by_id(id.item_hash);
            if( !opt_block )
               elog("Couldn't find block ${id} -- corresponding ID in our chain is ${id2}",
                    ("id", id.item_hash)("id2", _chain_db->get_block_id_for_num(block_header::num_from_id(id.item_hash))));
            FC_ASSERT( opt_block.valid() );
            // ilog("Serving up block #${num}", ("num", opt_block->block_num()));
            return block_message(std::move(*opt_block));
         }
         return trx_message( _chain_db->get_recent_transaction( id.item_hash ) );
      } FC_CAPTURE_AND_RETHROW( (id) ) }

      /**
       * Returns a synopsis of the blockchain used for syncing.  This consists of a list of
       * block hashes at intervals exponentially increasing towards the genesis block.
       * When syncing to a peer, the peer uses this data to determine if we're on the same
       * fork as they are, and if not, what blocks they need to send us to get us on their
       * fork.
       *
       * In the over-simplified case, this is a straighforward synopsis of our current
       * preferred blockchain; when we first connect up to a peer, this is what we will be sending.
       * It looks like this:
       *   If the blockchain is empty, it will return the empty list.
       *   If the blockchain has one block, it will return a list containing just that block.
       *   If it contains more than one block:
       *     the first element in the list will be the hash of the highest numbered block that
       *         we cannot undo
       *     the second element will be the hash of an item at the half way point in the undoable
       *         segment of the blockchain
       *     the third will be ~3/4 of the way through the undoable segment of the block chain
       *     the fourth will be at ~7/8...
       *       &c.
       *     the last item in the list will be the hash of the most recent block on our preferred chain
       * so if the blockchain had 26 blocks labeled a - z, the synopsis would be:
       *    a n u x z
       * the idea being that by sending a small (<30) number of block ids, we can summarize a huge
       * blockchain.  The block ids are more dense near the end of the chain where because we are
       * more likely to be almost in sync when we first connect, and forks are likely to be short.
       * If the peer we're syncing with in our example is on a fork that started at block 'v',
       * then they will reply to our synopsis with a list of all blocks starting from block 'u',
       * the last block they know that we had in common.
       *
       * In the real code, there are several complications.
       *
       * First, as an optimization, we don't usually send a synopsis of the entire blockchain, we
       * send a synopsis of only the segment of the blockchain that we have undo data for.  If their
       * fork doesn't build off of something in our undo history, we would be unable to switch, so there's
       * no reason to fetch the blocks.
       *
       * Second, when a peer replies to our initial synopsis and gives us a list of the blocks they think
       * we are missing, they only send a chunk of a few thousand blocks at once.  After we get those
       * block ids, we need to request more blocks by sending another synopsis (we can't just say "send me
       * the next 2000 ids" because they may have switched forks themselves and they don't track what
       * they've sent us).  For faster performance, we want to get a fairly long list of block ids first,
       * then start downloading the blocks.
       * The peer doesn't handle these follow-up block id requests any different from the initial request;
       * it treats the synopsis we send as our blockchain and bases its response entirely off that.  So to
       * get the response we want (the next chunk of block ids following the last one they sent us, or,
       * failing that, the shortest fork off of the last list of block ids they sent), we need to construct
       * a synopsis as if our blockchain was made up of:
       *    1. the blocks in our block chain up to the fork point (if there is a fork) or the head block (if no fork)
       *    2. the blocks we've already pushed from their fork (if there's a fork)
       *    3. the block ids they've previously sent us
       * Segment 3 is handled in the p2p code, it just tells us the number of blocks it has (in
       * number_of_blocks_after_reference_point) so we can leave space in the synopsis for them.
       * We're responsible for constructing the synopsis of Segments 1 and 2 from our active blockchain and
       * fork database.  The reference_point parameter is the last block from that peer that has been
       * successfully pushed to the blockchain, so that tells us whether the peer is on a fork or on
       * the main chain.
       */
      virtual std::vector<item_hash_t> get_blockchain_synopsis(const item_hash_t& reference_point,
                                                               uint32_t number_of_blocks_after_reference_point) override
      { try {
          std::vector<item_hash_t> synopsis;
          synopsis.reserve(30);
          uint32_t high_block_num;
          uint32_t non_fork_high_block_num;
          uint32_t low_block_num = _chain_db->last_non_undoable_block_num();
          std::vector<block_id_type> fork_history;

          if (reference_point != item_hash_t())
          {
            // the node is asking for a summary of the block chain up to a specified
            // block, which may or may not be on a fork
            // for now, assume it's not on a fork
            if (is_included_block(reference_point))
            {
              // reference_point is a block we know about and is on the main chain
              uint32_t reference_point_block_num = block_header::num_from_id(reference_point);
              assert(reference_point_block_num > 0);
              high_block_num = reference_point_block_num;
              non_fork_high_block_num = high_block_num;

              if (reference_point_block_num < low_block_num)
              {
                // we're on the same fork (at least as far as reference_point) but we've passed
                // reference point and could no longer undo that far if we diverged after that
                // block.  This should probably only happen due to a race condition where
                // the network thread calls this function, and then immediately pushes a bunch of blocks,
                // then the main thread finally processes this function.
                // with the current framework, there's not much we can do to tell the network
                // thread what our current head block is, so we'll just pretend that
                // our head is actually the reference point.
                // this *may* enable us to fetch blocks that we're unable to push, but that should
                // be a rare case (and correctly handled)
                low_block_num = reference_point_block_num;
              }
            }
            else
            {
              // block is a block we know about, but it is on a fork
              try
              {
                fork_history = _chain_db->get_block_ids_on_fork(reference_point);
                // returns a vector where the last element is the common ancestor with the preferred chain,
                // and the first element is the reference point you passed in
                assert(fork_history.size() >= 2);

                if( fork_history.front() != reference_point )
                {
                   edump( (fork_history)(reference_point) );
                   assert(fork_history.front() == reference_point);
                }
                block_id_type last_non_fork_block = fork_history.back();
                fork_history.pop_back();  // remove the common ancestor
                boost::reverse(fork_history);

                if (last_non_fork_block == block_id_type()) // if the fork goes all the way back to genesis (does graphene's fork db allow this?)
                  non_fork_high_block_num = 0;
                else
                  non_fork_high_block_num = block_header::num_from_id(last_non_fork_block);

                high_block_num = non_fork_high_block_num + fork_history.size();
                assert(high_block_num == block_header::num_from_id(fork_history.back()));
              }
              catch (const fc::exception& e)
              {
                // unable to get fork history for some reason.  maybe not linked?
                // we can't return a synopsis of its chain
                elog("Unable to construct a blockchain synopsis for reference hash ${hash}: ${exception}", ("hash", reference_point)("exception", e));
                throw;
              }
              if (non_fork_high_block_num < low_block_num)
              {
                wlog("Unable to generate a usable synopsis because the peer we're generating it for forked too long ago "
                     "(our chains diverge after block #${non_fork_high_block_num} but only undoable to block #${low_block_num})",
                     ("low_block_num", low_block_num)
                     ("non_fork_high_block_num", non_fork_high_block_num));
                FC_THROW_EXCEPTION(graphene::net::block_older_than_undo_history, "Peer is are on a fork I'm unable to switch to");
              }
            }
          }
          else
          {
            // no reference point specified, summarize the whole block chain
            high_block_num = _chain_db->head_block_num();
            non_fork_high_block_num = high_block_num;
            if (high_block_num == 0)
              return synopsis; // we have no blocks
          }

          if( low_block_num == 0)
             low_block_num = 1;

          // at this point:
          // low_block_num is the block before the first block we can undo,
          // non_fork_high_block_num is the block before the fork (if the peer is on a fork, or otherwise it is the same as high_block_num)
          // high_block_num is the block number of the reference block, or the end of the chain if no reference provided

          // true_high_block_num is the ending block number after the network code appends any item ids it
          // knows about that we don't
          uint32_t true_high_block_num = high_block_num + number_of_blocks_after_reference_point;
          do
          {
            // for each block in the synopsis, figure out where to pull the block id from.
            // if it's <= non_fork_high_block_num, we grab it from the main blockchain;
            // if it's not, we pull it from the fork history
            if (low_block_num <= non_fork_high_block_num)
              synopsis.push_back(_chain_db->get_block_id_for_num(low_block_num));
            else
              synopsis.push_back(fork_history[low_block_num - non_fork_high_block_num - 1]);
            low_block_num += (true_high_block_num - low_block_num + 2) / 2;
          }
          while (low_block_num <= high_block_num);

          //idump((synopsis));
          return synopsis;
      } FC_CAPTURE_AND_RETHROW() }

      /**
       * Call this after the call to handle_message succeeds.
       *
       * @param item_type the type of the item we're synchronizing, will be the same as item passed to the sync_from() call
       * @param item_count the number of items known to the node that haven't been sent to handle_item() yet.
       *                   After `item_count` more calls to handle_item(), the node will be in sync
       */
      virtual void sync_status(uint32_t item_type, uint32_t item_count) override
      {
         // any status reports to GUI go here
      }

      /**
       * Call any time the number of connected peers changes.
       */
      virtual void connection_count_changed(uint32_t c) override
      {
        // any status reports to GUI go here
      }

      virtual uint32_t get_block_number(const item_hash_t& block_id) override
      { try {
         return block_header::num_from_id(block_id);
      } FC_CAPTURE_AND_RETHROW( (block_id) ) }

      /**
       * Returns the time a block was produced (if block_id = 0, returns genesis time).
       * If we don't know about the block, returns time_point_sec::min()
       */
      virtual fc::time_point_sec get_block_time(const item_hash_t& block_id) override
      { try {
         auto opt_block = _chain_db->fetch_block_by_id( block_id );
         if( opt_block.valid() ) return opt_block->timestamp;
         return fc::time_point_sec::min();
      } FC_CAPTURE_AND_RETHROW( (block_id) ) }

      /** returns graphene::time::now() */
      virtual fc::time_point_sec get_blockchain_now() override
      {
         return graphene::time::now();
      }

      virtual item_hash_t get_head_block_id() const override
      {
         return _chain_db->head_block_id();
      }

      virtual uint32_t estimate_last_known_fork_from_git_revision_timestamp(uint32_t unix_timestamp) const override
      {
         return 0; // there are no forks in graphene
      }

      virtual void error_encountered(const std::string& message, const fc::oexception& error) override
      {
         // notify GUI or something cool
      }

      void get_max_block_age( int32_t& result )
      {
         result = _max_block_age;
         return;
      }

      void shutdown()
      {
         _running = false;
         fc::usleep( fc::seconds( 1 ) );
         if( _p2p_network )
         {
            _p2p_network->close();
            fc::usleep( fc::seconds( 1 ) ); // p2p node has some calls to the database, give it a second to shutdown before invalidating the chain db pointer
         }
         if( _chain_db )
            _chain_db->close();
      }

      application* _self;

      fc::path _data_dir;
      fc::path _shared_dir;
      const bpo::variables_map* _options = nullptr;
      api_access _apiaccess;

      //std::shared_ptr<graphene::db::object_database>   _pending_trx_db;
      std::shared_ptr<steemit::chain::database>        _chain_db;
      std::shared_ptr<graphene::net::node>             _p2p_network;
      std::shared_ptr<fc::http::websocket_server>      _websocket_server;
      std::shared_ptr<fc::http::websocket_tls_server>  _websocket_tls_server;

      std::map<string, std::shared_ptr<abstract_plugin> > _plugins_available;
      std::map<string, std::shared_ptr<abstract_plugin> > _plugins_enabled;
      flat_map< std::string, std::function< fc::api_ptr( const api_context& ) > >   _api_factories_by_name;
      std::vector< std::string >                       _public_apis;
      int32_t                                          _max_block_age = -1;
      uint64_t                                         _shared_file_size;

      bool                                             _running;

      uint32_t allow_future_time = 5;
   };

}

application::application()
   : my(new detail::application_impl(this))
{}

application::~application()
{
   if( my->_p2p_network )
   {
      my->_p2p_network->close();
      my->_p2p_network.reset();
   }
   if( my->_chain_db )
   {
      my->_chain_db->close();
   }
   /*if( my->_pending_trx_db )
   {
      my->_pending_trx_db->close();
   }*/
}

void application::set_program_options(boost::program_options::options_description& command_line_options,
                                      boost::program_options::options_description& configuration_file_options) const
{
   std::vector< std::string > default_apis;
   default_apis.push_back( "database_api" );
   default_apis.push_back( "login_api" );
   default_apis.push_back( "account_by_key_api" );
   std::string str_default_apis = boost::algorithm::join( default_apis, " " );

   std::vector< std::string > default_plugins;
   default_plugins.push_back( "witness" );
   default_plugins.push_back( "account_history" );
   default_plugins.push_back( "account_by_key" );
   std::string str_default_plugins = boost::algorithm::join( default_plugins, " " );

   configuration_file_options.add_options()
         ("p2p-endpoint", bpo::value<string>(), "Endpoint for P2P node to listen on")
         ("p2p-max-connections", bpo::value<uint32_t>(), "Maxmimum number of incoming connections on P2P endpoint")
         ("seed-node,s", bpo::value<vector<string>>()->composing(), "P2P nodes to connect to on startup (may specify multiple times)")
         ("checkpoint,c", bpo::value<vector<string>>()->composing(), "Pairs of [BLOCK_NUM,BLOCK_ID] that should be enforced as checkpoints.")
         ("shared-file-dir", bpo::value<string>(), "Location of the shared memory file. Defaults to data_dir/blockchain")
         ("shared-file-size", bpo::value<string>()->default_value("32G"), "Size of the shared memory file. Default: 32G")
         ("rpc-endpoint", bpo::value<string>()->implicit_value("127.0.0.1:8090"), "Endpoint for websocket RPC to listen on")
         ("rpc-tls-endpoint", bpo::value<string>()->implicit_value("127.0.0.1:8089"), "Endpoint for TLS websocket RPC to listen on")
         ("read-forward-rpc", bpo::value<string>(), "Endpoint to forward write API calls to for a read node" )
         ("server-pem,p", bpo::value<string>()->implicit_value("server.pem"), "The TLS certificate file for this server")
         ("server-pem-password,P", bpo::value<string>()->implicit_value(""), "Password for this certificate")
         ("api-user", bpo::value< vector<string> >()->composing(), "API user specification, may be specified multiple times")
         ("public-api", bpo::value< vector<string> >()->composing()->default_value(default_apis, str_default_apis), "Set an API to be publicly available, may be specified multiple times")
         ("enable-plugin", bpo::value< vector<string> >()->composing()->default_value(default_plugins, str_default_plugins), "Plugin(s) to enable, may be specified multiple times")
         ("max-block-age", bpo::value< int32_t >()->default_value(200), "Maximum age of head block when broadcasting tx via API")
         ("flush", bpo::value< uint32_t >()->default_value(100000), "Flush shared memory file to disk this many blocks")
         ;
   command_line_options.add(configuration_file_options);
   command_line_options.add_options()
         ("replay-blockchain", "Rebuild object graph by replaying all blocks")
         ("resync-blockchain", "Delete all blocks and re-sync with network from scratch")
         ("force-validate", "Force validation of all transactions")
         ("read-only", "Node will not connect to p2p network and can only read from the chain state" )
         ;
   command_line_options.add(_cli_options);
   configuration_file_options.add(_cfg_options);
}

void application::initialize(const fc::path& data_dir, const boost::program_options::variables_map& options)
{
   my->_data_dir = data_dir;
   my->_options = &options;
}

void application::startup()
{
   try {
      my->startup();
   } catch ( const fc::exception& e ) {
      elog( "${e}", ("e",e.to_detail_string()) );
      throw;
   } catch ( ... ) {
      elog( "unexpected exception" );
      throw;
   }
}

std::shared_ptr<abstract_plugin> application::get_plugin(const string& name) const
{
   try
   {
      return my->_plugins_enabled.at( name );
   }
   catch ( ... ) {}

   return null_plugin;
}

graphene::net::node_ptr application::p2p_node()
{
   return my->_p2p_network;
}

std::shared_ptr<chain::database> application::chain_database() const
{
   return my->_chain_db;
}
/*std::shared_ptr<graphene::db::object_database> application::pending_trx_database() const
{
   return my->_pending_trx_db;
}*/

void application::set_block_production(bool producing_blocks)
{
   my->_is_block_producer = producing_blocks;
}

optional< api_access_info > application::get_api_access_info( const string& username )const
{
   return my->get_api_access_info( username );
}

void application::set_api_access_info(const string& username, api_access_info&& permissions)
{
   my->set_api_access_info(username, std::move(permissions));
}

void application::register_api_factory( const string& name, std::function< fc::api_ptr( const api_context& ) > factory )
{
   return my->register_api_factory( name, factory );
}

fc::api_ptr application::create_api_by_name( const api_context& ctx )
{
   return my->create_api_by_name( ctx );
}

void application::get_max_block_age( int32_t& result )
{
   my->get_max_block_age( result );
}

void application::shutdown_plugins()
{
   for( auto& entry : my->_plugins_enabled )
      entry.second->plugin_shutdown();
   return;
}
void application::shutdown()
{
   my->shutdown();
}

void application::register_abstract_plugin( std::shared_ptr< abstract_plugin > plug )
{
   boost::program_options::options_description plugin_cli_options("Options for plugin " + plug->plugin_name()), plugin_cfg_options;
   plug->plugin_set_program_options(plugin_cli_options, plugin_cfg_options);
   if( !plugin_cli_options.options().empty() )
      _cli_options.add(plugin_cli_options);
   if( !plugin_cfg_options.options().empty() )
      _cfg_options.add(plugin_cfg_options);

   my->_plugins_available[plug->plugin_name()] = plug;
}

void application::enable_plugin( const std::string& name )
{
   auto it = my->_plugins_available.find(name);
   if( it == my->_plugins_available.end() )
   {
      elog( "can't enable plugin ${name}", ("name", name) );
   }
   FC_ASSERT( it != my->_plugins_available.end() );
   my->_plugins_enabled[name] = it->second;
}

void application::initialize_plugins( const boost::program_options::variables_map& options )
{
   if( options.count("enable-plugin") > 0 )
   {
      for( auto& arg : options.at("enable-plugin").as< std::vector< std::string > >() )
      {
         vector<string> names;
         boost::split(names, arg, boost::is_any_of(" \t,"));
         for( const std::string& name : names )
         {
            enable_plugin( name );
         }
      }
   }
   for( auto& entry : my->_plugins_enabled )
   {
      ilog( "Initializing plugin ${name}", ("name", entry.first) );
      entry.second->plugin_initialize( options );
   }
   return;
}

void application::startup_plugins()
{
   for( auto& entry : my->_plugins_enabled )
      entry.second->plugin_startup();
   return;
}

// namespace detail
} }
