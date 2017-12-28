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

#include <steemit/app/api_context.hpp>
#include <steemit/app/database_api.hpp>
#include <steemit/protocol/types.hpp>

#include <graphene/net/node.hpp>

#include <fc/api.hpp>
#include <fc/optional.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/network/ip.hpp>

#include <boost/container/flat_set.hpp>

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace steemit { namespace app {
   using namespace steemit::chain;
   using namespace fc::ecc;
   using namespace std;

   class application;
   struct api_context;

   /**
    * @brief The network_broadcast_api class allows broadcasting of transactions.
    */
   class network_broadcast_api : public std::enable_shared_from_this<network_broadcast_api>
   {
      public:
         network_broadcast_api(const api_context& a);

         struct transaction_confirmation
         {
            transaction_confirmation( transaction_id_type txid, int32_t bn, int32_t tn, bool ex )
            : id(txid), block_num(bn), trx_num(tn), expired(ex) {}

            transaction_id_type   id;
            int32_t               block_num = 0;
            int32_t               trx_num   = 0;
            bool                  expired   = false;
         };

         typedef std::function<void(variant/*transaction_confirmation*/)> confirmation_callback;

         /**
          * @brief Broadcast a transaction to the network
          * @param trx The transaction to broadcast
          *
          * The transaction will be checked for validity in the local database prior to broadcasting. If it fails to
          * apply locally, an error will be thrown and the transaction will not be broadcast.
          */
         void broadcast_transaction(const signed_transaction& trx);

         /** this version of broadcast transaction registers a callback method that will be called when the transaction is
          * included into a block.  The callback method includes the transaction id, block number, and transaction number in the
          * block.
          */
         void broadcast_transaction_with_callback( confirmation_callback cb, const signed_transaction& trx);

         /**
          * This call will not return until the transaction is included in a block.
          */
         fc::variant broadcast_transaction_synchronous( const signed_transaction& trx);

         void broadcast_block( const signed_block& block );

         void set_max_block_age( int32_t max_block_age );

         // implementation detail, not reflected
         bool check_max_block_age( int32_t max_block_age );

         /**
          * @brief Not reflected, thus not accessible to API clients.
          *
          * This function is registered to receive the applied_block
          * signal from the chain database when a block is received.
          * It then dispatches callbacks to clients who have requested
          * to be notified when a particular txid is included in a block.
          */
         void on_applied_block( const signed_block& b );

         /// internal method, not exposed via JSON RPC
         void on_api_startup();

      private:
         boost::signals2::scoped_connection             _applied_block_connection;

         map<transaction_id_type,confirmation_callback>     _callbacks;
         map<time_point_sec, vector<transaction_id_type> >  _callbacks_expirations;

         int32_t                                        _max_block_age = -1;

         application&                                   _app;
   };

   /**
    * @brief The network_node_api class allows maintenance of p2p connections.
    */
   class network_node_api
   {
      public:
         network_node_api(const api_context& a);

         /**
          * @brief Return general network information, such as p2p port
          */
         fc::variant_object get_info() const;

         /**
          * @brief add_node Connect to a new peer
          * @param ep The IP/Port of the peer to connect to
          */
         void add_node(const fc::ip::endpoint& ep);

         /**
          * @brief Get status of all current connections to peers
          */
         std::vector<graphene::net::peer_status> get_connected_peers() const;

         /**
          * @brief Get advanced node parameters, such as desired and max
          *        number of connections
          */
         graphene::net::node_configuration get_advanced_node_parameters() const;

         /**
          * @brief Set advanced node parameters, such as desired and max
          *        number of connections
          * @param params a JSON object containing the name/value pairs for the parameters to set
          */
         void set_advanced_node_parameters(const fc::variant_object& params);

         /**
          * @brief Return list of potential peers
          */
         std::vector<graphene::net::potential_peer_record> get_potential_peers() const;

         /// internal method, not exposed via JSON RPC
         void on_api_startup();

      private:
         application& _app;
   };

   struct steem_version_info
   {
      steem_version_info() {}
      steem_version_info( fc::string bc_v, fc::string s_v, fc::string fc_v )
         :blockchain_version( bc_v ), steem_revision( s_v ), fc_revision( fc_v ) {}

      fc::string blockchain_version;
      fc::string steem_revision;
      fc::string fc_revision;
   };

   /**
    * @brief The login_api class implements the bottom layer of the RPC API
    *
    * All other APIs must be requested from this API.
    */
   class login_api
   {
      public:
         login_api(const api_context& ctx);
         virtual ~login_api();

         /**
          * @brief Authenticate to the RPC server
          * @param user Username to login with
          * @param password Password to login with
          * @return True if logged in successfully; false otherwise
          *
          * @note This must be called prior to requesting other APIs. Other APIs may not be accessible until the client
          * has sucessfully authenticated.
          */
         bool login(const string& user, const string& password);

         fc::api_ptr get_api_by_name( const string& api_name )const;

         steem_version_info get_version();

         /// internal method, not exposed via JSON RPC
         void on_api_startup();

      private:
         api_context _ctx;
   };

}}  // steemit::app

FC_REFLECT( steemit::app::network_broadcast_api::transaction_confirmation,
        (id)(block_num)(trx_num)(expired) )
FC_REFLECT( steemit::app::steem_version_info, (blockchain_version)(steem_revision)(fc_revision) )
//FC_REFLECT_TYPENAME( fc::ecc::compact_signature );
//FC_REFLECT_TYPENAME( fc::ecc::commitment_type );

FC_API(steemit::app::network_broadcast_api,
       (broadcast_transaction)
       (broadcast_transaction_with_callback)
       (broadcast_transaction_synchronous)
       (broadcast_block)
       (set_max_block_age)
     )
FC_API(steemit::app::network_node_api,
       (get_info)
       (add_node)
       (get_connected_peers)
       (get_potential_peers)
       (get_advanced_node_parameters)
       (set_advanced_node_parameters)
     )
FC_API(steemit::app::login_api,
       (login)
       (get_api_by_name)
       (get_version)
     )
