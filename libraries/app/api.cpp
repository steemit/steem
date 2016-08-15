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
#include <steemit/chain/database.hpp>
#include <steemit/chain/get_config.hpp>
#include <steemit/chain/steem_objects.hpp>
#include <steemit/chain/transaction_object.hpp>

#include <graphene/time/time.hpp>
#include <graphene/utilities/key_conversion.hpp>

#include <fc/crypto/hex.hpp>
#include <fc/smart_ref_impl.hpp>

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

    network_broadcast_api::network_broadcast_api(const api_context& a):_app(a.app)
    {
       /// NOTE: cannot register callbacks in constructor because shared_from_this() is not valid.
       _app.get_bcd_trigger( _bcd_trigger );
    }

    void network_broadcast_api::on_api_startup()
    {
       /// note cannot capture shared pointer here, because _applied_block_connection will never
       /// be freed if the lambda holds a reference to it.
       _applied_block_connection = connect_signal( _app.chain_database()->applied_block, *this, &network_broadcast_api::on_applied_block );
    }

    bool network_broadcast_api::check_bcd_trigger( const std::vector< std::pair< uint32_t, uint32_t > >& bcd_trigger )
    {
       FC_ASSERT( bcd_trigger.size() < 16 );
       std::shared_ptr< database > db = _app.chain_database();
       const dynamic_global_property_object& dgpo = db->get_dynamic_global_properties();
       fc::uint128_t slots = db->get_dynamic_global_properties().recent_slots_filled;
       fc::time_point_sec now = graphene::time::now();

       ilog( "Entering check_bcd_trigger: now=${now}", ("now", now) );

       for( const std::pair< uint32_t, uint32_t >& trig : bcd_trigger )
       {
          // trig_slots is the number of slots which will be checked by the trigger.
          uint32_t trig_slots = (trig.second + STEEMIT_BLOCK_INTERVAL - 1) / STEEMIT_BLOCK_INTERVAL;
          ilog( "Testing bcd trigger: ${trig}  trig_slots=${trig_slots}", ("trig", trig)("trig_slots", trig_slots) );
          if( trig_slots > 128 )
          {
             elog( "Bad --bcd-trigger parameter specified (trig_slots=${s})", ("s", trig_slots) );
             continue;
          }

          fc::uint128_t temp_slots = slots;

          fc::time_point_sec db_now = db->head_block_time();
          int64_t delta = (now - db_now).to_seconds();
          ilog( "delta: ${delta}", ("delta", delta) );
          if( delta > 0 )
             delta = std::max( delta-STEEMIT_BLOCK_INTERVAL*2, int64_t(0) );
          int64_t delta_slots = delta / STEEMIT_BLOCK_INTERVAL;
          if( delta_slots < 0 )
          {
             uint64_t discarded_slots = uint64_t( -delta_slots );
             // now is in the past, so rewind temp_slots by discarding the slots that will happen in the future
             if( delta_slots >= 128 - trig_slots )
             {
                elog( "Bailed in check_bcd_trigger because the blockchain extends too far into the future" );
                continue;
             }
             temp_slots >>= discarded_slots;
          }
          else
          {
             // now is in the future, so add temp_slots by shifting in new empty places
             uint64_t empty_slots = delta_slots;
             // if now is so far in the future that all slots are empty, all triggers should trip
             if( empty_slots >= 128 )
                return true;
             temp_slots <<= empty_slots;
          }
          fc::uint128_t mask = 1;
          mask <<= trig_slots;
          mask -= 1;
          temp_slots &= mask;
          if( temp_slots.popcount() <= trig.first )
             return true;
       }
       return false;
    }

    void network_broadcast_api::set_bcd_trigger( const std::vector< std::pair< uint32_t, uint32_t > >  bcd_trigger )
    {
       _bcd_trigger = bcd_trigger;
    }

    void network_broadcast_api::on_applied_block( const signed_block& b )
    {
       int32_t block_num = int32_t(b.block_num());
       if( _callbacks.size() )
       {
          /// we need to ensure the database_api is not deleted for the life of the async operation
          auto capture_this = shared_from_this();
          for( int32_t trx_num = 0; trx_num < b.transactions.size(); ++trx_num )
          {
             const auto& trx = b.transactions[trx_num];
             auto id = trx.id();
             auto itr = _callbacks.find(id);
             if( itr != _callbacks.end() )
             {
                auto callback = _callbacks.find(id)->second;
                fc::async( [capture_this,this,id,block_num,trx_num,callback](){ callback( fc::variant(transaction_confirmation{ id, block_num, trx_num, false}) ); } );
                itr->second = []( const variant& ){};
             }
          }
       }

       /// clear all expirations

       if( _callbacks_expirations.size() ) {
          auto end = _callbacks_expirations.upper_bound( b.timestamp );
          auto itr = _callbacks_expirations.begin();
          while( itr != end ) {
             for( const auto trx_id : itr->second ) {
               auto cb_itr = _callbacks.find( trx_id );
               if( cb_itr != _callbacks.end() ) {
                   auto capture_this = shared_from_this();
                   auto callback = _callbacks.find(trx_id)->second; 
                   fc::async( [capture_this,this,block_num,trx_id,callback](){ callback( fc::variant(transaction_confirmation{ trx_id, block_num, -1, true}) ); } );
                   _callbacks.erase(cb_itr);
               }
             }
             _callbacks_expirations.erase( itr );
             itr = _callbacks_expirations.begin();
          }
       }
    }

    void network_broadcast_api::broadcast_transaction(const signed_transaction& trx)
    {
       trx.validate();
       FC_ASSERT( !check_bcd_trigger( _bcd_trigger ) );
       _app.chain_database()->push_transaction(trx);
       _app.p2p_node()->broadcast_transaction(trx);
    }
    fc::variant network_broadcast_api::broadcast_transaction_synchronous(const signed_transaction& trx)
    {
       promise<fc::variant>::ptr prom( new fc::promise<fc::variant>() );
       broadcast_transaction_with_callback( [=]( const fc::variant& v ){ 
          prom->set_value(v);
       }, trx );
       return future<fc::variant>(prom).wait();
    }

    void network_broadcast_api::broadcast_block( const signed_block& b )
    {
       _app.chain_database()->push_block(b);
       _app.p2p_node()->broadcast( graphene::net::block_message( b ));
    }

    void network_broadcast_api::broadcast_transaction_with_callback(confirmation_callback cb, const signed_transaction& trx)
    {
       FC_ASSERT( !check_bcd_trigger( _bcd_trigger ) );
       trx.validate();
       _callbacks[trx.id()] = cb;
       _callbacks_expirations[trx.expiration].push_back(trx.id());

       _app.chain_database()->push_transaction(trx);
       _app.p2p_node()->broadcast_transaction(trx);
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

    vector< string > get_relevant_accounts( const object* obj )
    {
       vector< string > result;
       if( obj->id.space() == protocol_ids )
       {
          switch( (object_type)obj->id.type() )
          {
            case null_object_type:
            case base_object_type:
            case OBJECT_TYPE_COUNT:
               return result;
            /*case account_object_type:{
               result.push_back( obj->id );
               break;*/ // TODO: Add back in when steem objects are done.
            /*} case witness_object_type:{
               const auto& aobj = dynamic_cast<const witness_object*>(obj);
               assert( aobj != nullptr );
               result.push_back( aobj->witness_account );
               break;
            }*/ // TODO: Add back in when new witness is complete.
            case impl_comment_object_type:{
               const auto& aobj = dynamic_cast<const comment_object*>(obj);
               assert( aobj != nullptr );
               result.push_back( aobj->author );
               break;
            } /* case vote_object_type:{  -- this is commented out until we reflect vote_object in object_type enum in types.hpp
               const auto& aobj = dynamic_cast<const vote_object*>(obj);
               assert( aobj != nullptr );
               result.push_back( aobj->voter );
               break;
            } */
          }
       }
       else if( obj->id.space() == implementation_ids )
       {
          /*
          switch( (impl_object_type)obj->id.type() )
          {
          }
          */
       }
       return result;
    } // end get_relevant_accounts( obj )

} } // steemit::app
