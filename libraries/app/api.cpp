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
#include <cctype>

#include <steemit/app/api.hpp>
#include <steemit/app/api_access.hpp>
#include <steemit/app/application.hpp>
#include <steemit/app/impacted.hpp>

#include <steemit/protocol/get_config.hpp>

#include <steemit/chain/database.hpp>
#include <steemit/chain/steem_objects.hpp>
#include <steemit/chain/transaction_object.hpp>
#include <fc/time.hpp>

#include <graphene/utilities/key_conversion.hpp>

#include <fc/crypto/hex.hpp>
#include <fc/smart_ref_impl.hpp>

#include <graphene/utilities/git_revision.hpp>
#include <fc/git_revision.hpp>

namespace steemit { namespace app {

    login_api::login_api(const api_context& ctx)
    :_ctx(ctx)
    {
    }

    login_api::~login_api()
    {
    }

    void login_api::on_api_startup() {
    }

    bool login_api::login(const string& user, const string& password)
    {
       idump((user)(password));
       optional< api_access_info > acc = _ctx.app.get_api_access_info( user );
       if( !acc.valid() )
          return false;
       if( acc->password_hash_b64 != "*" )
       {
          std::string password_salt = fc::base64_decode( acc->password_salt_b64 );
          std::string acc_password_hash = fc::base64_decode( acc->password_hash_b64 );

          fc::sha256 hash_obj = fc::sha256::hash( password + password_salt );
          if( hash_obj.data_size() != acc_password_hash.length() )
             return false;
          if( memcmp( hash_obj.data(), acc_password_hash.c_str(), hash_obj.data_size() ) != 0 )
             return false;
       }

       idump((acc->allowed_apis));
       std::shared_ptr< api_session_data > session = _ctx.session.lock();
       FC_ASSERT( session );

       std::map< std::string, api_ptr >& _api_map = session->api_map;

       for( const std::string& api_name : acc->allowed_apis )
       {
          auto it = _api_map.find( api_name );
          if( it != _api_map.end() ) {
             wlog( "known api: ${api}", ("api",api_name) );
             continue;
          }
          idump((api_name));
          api_context new_ctx( _ctx.app, api_name, _ctx.session );
          _api_map[ api_name ] = _ctx.app.create_api_by_name( new_ctx );
       }
       return true;
    }

    fc::api_ptr login_api::get_api_by_name( const string& api_name )const
    {
       std::shared_ptr< api_session_data > session = _ctx.session.lock();
       FC_ASSERT( session );

       const std::map< std::string, api_ptr >& _api_map = session->api_map;
       auto it = _api_map.find( api_name );
       if( it == _api_map.end() )
       {
          wlog( "unknown api: ${api}", ("api",api_name) );
          return fc::api_ptr();
       }
       if( it->second )
       {
          ilog( "found api: ${api}", ("api",api_name) );
       }
       FC_ASSERT( it->second != nullptr );
       return it->second;
    }

    steem_version_info login_api::get_version()
    {
       return steem_version_info(
         fc::string( STEEMIT_BLOCKCHAIN_VERSION ),
         fc::string( graphene::utilities::git_revision_sha ),
         fc::string( fc::git_revision_sha ) );
    }

    network_broadcast_api::network_broadcast_api(const api_context& a):_app(a.app)
    {
       /// NOTE: cannot register callbacks in constructor because shared_from_this() is not valid.
       _app.get_max_block_age( _max_block_age );
    }

    void network_broadcast_api::on_api_startup()
    {
       /// note cannot capture shared pointer here, because _applied_block_connection will never
       /// be freed if the lambda holds a reference to it.
       _applied_block_connection = connect_signal( _app.chain_database()->applied_block, *this, &network_broadcast_api::on_applied_block );
    }

    bool network_broadcast_api::check_max_block_age( int32_t max_block_age )
    {
       return _app.chain_database()->with_read_lock( [&]()
       {
          if( max_block_age < 0 )
             return false;

          fc::time_point_sec now = fc::time_point::now();
          std::shared_ptr< database > db = _app.chain_database();
          const dynamic_global_property_object& dgpo = db->get_dynamic_global_properties();

          return ( dgpo.time < now - fc::seconds( max_block_age ) );
       });
    }

    void network_broadcast_api::set_max_block_age( int32_t max_block_age )
    {
       _max_block_age = max_block_age;
    }

    void network_broadcast_api::on_applied_block( const signed_block& b )
    {
       /// we need to ensure the database_api is not deleted for the life of the async operation
       auto capture_this = shared_from_this();

       fc::async( [this,capture_this,b]() {
          int32_t block_num = int32_t(b.block_num());
          if( _callbacks.size() )
          {
             for( size_t trx_num = 0; trx_num < b.transactions.size(); ++trx_num )
             {
                const auto& trx = b.transactions[trx_num];
                auto id = trx.id();
                auto itr = _callbacks.find(id);
                if( itr == _callbacks.end() ) continue;
                confirmation_callback callback = itr->second;
                itr->second = [](variant){};
                callback( fc::variant(transaction_confirmation( id, block_num, int32_t(trx_num), false )) );
             }
          }

          /// clear all expirations
          while( true )
          {
             auto exp_it = _callbacks_expirations.begin();
             if( exp_it == _callbacks_expirations.end() )
                break;
             if( exp_it->first >= b.timestamp )
                break;
             for( const transaction_id_type& txid : exp_it->second )
             {
                auto cb_it = _callbacks.find( txid );
                // If it's empty, that means the transaction has been confirmed and has been deleted by the above check.
                if( cb_it == _callbacks.end() )
                   continue;

                confirmation_callback callback = cb_it->second;
                transaction_id_type txid_byval = txid;    // can't pass in by reference as it's going to be deleted
                callback( fc::variant(transaction_confirmation{ txid_byval, block_num, -1, true}) );

                _callbacks.erase( cb_it );
             }
             _callbacks_expirations.erase( exp_it );
          }
       }); /// fc::async

    }

    void network_broadcast_api::broadcast_transaction(const signed_transaction& trx)
    {
       trx.validate();

       if( _app._read_only )
       {
          // If we are not connected, attempt to connect once and then fail
          if( !_app._remote_net_api )
          {
            _app.connect_to_write_node();
            FC_ASSERT( _app._remote_net_api, "Write node RPC not configured properly or not currently connected." );
          }

          (*_app._remote_net_api)->broadcast_transaction( trx );
       }
       else
       {
          FC_ASSERT( !check_max_block_age( _max_block_age ) );
          _app.chain_database()->push_transaction(trx);
          _app.p2p_node()->broadcast_transaction(trx);
       }
    }

    fc::variant network_broadcast_api::broadcast_transaction_synchronous(const signed_transaction& trx)
    {
       if( _app._read_only )
       {
         // If we are not connected, attempt to connect once and then fail
          if( !_app._remote_net_api )
          {
             _app.connect_to_write_node();
             FC_ASSERT( _app._remote_net_api, "Write node RPC not configured properly or not currently connected." );
          }

          return (*_app._remote_net_api)->broadcast_transaction_synchronous( trx );
       }
       else
       {
          promise<fc::variant>::ptr prom( new fc::promise<fc::variant>() );
          broadcast_transaction_with_callback( [=]( const fc::variant& v ){
             prom->set_value(v);
          }, trx );
          return future<fc::variant>(prom).wait();
       }
    }

    void network_broadcast_api::broadcast_block( const signed_block& b )
    {
       if( _app._read_only )
       {
          // If we are not connected, attempt to connect once and then fail
          if( !_app._remote_net_api )
          {
             _app.connect_to_write_node();
             FC_ASSERT( _app._remote_net_api, "Write node RPC not configured properly or not currently connected." );
          }

          (*_app._remote_net_api)->broadcast_block( b );
       }
       else
       {
          _app.chain_database()->push_block(b);
          _app.p2p_node()->broadcast( graphene::net::block_message( b ));
       }
    }

    void network_broadcast_api::broadcast_transaction_with_callback(confirmation_callback cb, const signed_transaction& trx)
    {
       if( _app._read_only )
       {
          // If we are not connected, attempt to connect once and then fail
          if( !_app._remote_net_api )
          {
             _app.connect_to_write_node();
             FC_ASSERT( _app._remote_net_api, "Write node RPC not configured properly or not currently connected." );
          }

          (*_app._remote_net_api)->broadcast_transaction_with_callback( cb, trx );
       }
       else
       {
          FC_ASSERT( !check_max_block_age( _max_block_age ) );
          trx.validate();
          _callbacks[trx.id()] = cb;
          _callbacks_expirations[trx.expiration].push_back(trx.id());

          _app.chain_database()->push_transaction(trx);
          _app.p2p_node()->broadcast_transaction(trx);
       }
    }

    network_node_api::network_node_api( const api_context& a ) : _app( a.app )
    {
    }

    void network_node_api::on_api_startup() {}

    fc::variant_object network_node_api::get_info() const
    {
       fc::mutable_variant_object result = _app.p2p_node()->network_get_info();
       result["connection_count"] = _app.p2p_node()->get_connection_count();
       return result;
    }

    void network_node_api::add_node(const fc::ip::endpoint& ep)
    {
       _app.p2p_node()->add_node(ep);
    }

    std::vector<graphene::net::peer_status> network_node_api::get_connected_peers() const
    {
       return _app.p2p_node()->get_connected_peers();
    }

    std::vector<graphene::net::potential_peer_record> network_node_api::get_potential_peers() const
    {
       return _app.p2p_node()->get_potential_peers();
    }

    fc::variant_object network_node_api::get_advanced_node_parameters() const
    {
       return _app.p2p_node()->get_advanced_node_parameters();
    }

    void network_node_api::set_advanced_node_parameters(const fc::variant_object& params)
    {
       return _app.p2p_node()->set_advanced_node_parameters(params);
    }

} } // steemit::app
