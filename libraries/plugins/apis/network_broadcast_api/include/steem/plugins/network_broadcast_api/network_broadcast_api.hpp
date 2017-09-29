#pragma once
#include <steem/protocol/block.hpp>

#include <steem/plugins/p2p/p2p_plugin.hpp>
#include <steem/plugins/chain/chain_plugin.hpp>
#include <steem/plugins/json_rpc/utility.hpp>

#include <fc/optional.hpp>
#include <fc/variant.hpp>
#include <fc/vector.hpp>

#include <boost/thread/mutex.hpp>

namespace steem { namespace plugins { namespace network_broadcast_api {

using std::vector;
using fc::variant;
using fc::optional;
using steem::plugins::json_rpc::void_type;

using steem::protocol::signed_transaction;
using steem::protocol::transaction_id_type;
using steem::protocol::signed_block;

struct broadcast_transaction_args
{
   signed_transaction   trx;
   int32_t              max_block_age = -1;
};

typedef void_type broadcast_transaction_return;

typedef broadcast_transaction_args broadcast_transaction_synchronous_args;

struct broadcast_transaction_synchronous_return
{
   broadcast_transaction_synchronous_return() {}
   broadcast_transaction_synchronous_return( transaction_id_type txid, int32_t bn, int32_t tn, bool ex )
   : id(txid), block_num(bn), trx_num(tn), expired(ex) {}

   transaction_id_type   id;
   int32_t               block_num = 0;
   int32_t               trx_num   = 0;
   bool                  expired   = false;
};

struct broadcast_block_args
{
   signed_block   block;
};

typedef void_type broadcast_block_return;

typedef std::function< void( const broadcast_transaction_synchronous_return& ) > confirmation_callback;

class network_broadcast_api
{
public:
   network_broadcast_api();
   ~network_broadcast_api();

   DECLARE_API(
      (broadcast_transaction)
      (broadcast_transaction_synchronous)
      (broadcast_block)
   )

   bool check_max_block_age( int32_t max_block_age ) const;

   void on_applied_block( const signed_block& b );


private:
   steem::plugins::p2p::p2p_plugin&                    _p2p;
   steem::plugins::chain::chain_plugin&                _chain;
   map< transaction_id_type, confirmation_callback >     _callbacks;
   map< time_point_sec, vector< transaction_id_type > >  _callback_expirations;

   boost::mutex                                          _mtx;
};

} } } // steem::plugins::network_broadcast_api

FC_REFLECT( steem::plugins::network_broadcast_api::broadcast_transaction_args,
   (trx)(max_block_age) )

FC_REFLECT( steem::plugins::network_broadcast_api::broadcast_block_args,
   (block) )

FC_REFLECT( steem::plugins::network_broadcast_api::broadcast_transaction_synchronous_return,
   (id)(block_num)(trx_num)(expired) )
