#include <steem/plugins/network_broadcast_api/network_broadcast_api.hpp>
#include <steem/plugins/network_broadcast_api/network_broadcast_api_plugin.hpp>

#include <appbase/application.hpp>

#include <boost/thread/future.hpp>
#include <boost/thread/lock_guard.hpp>

namespace steem { namespace plugins { namespace network_broadcast_api {

namespace detail
{
   class network_broadcast_api_impl
   {
      public:
         network_broadcast_api_impl() :
            _p2p( appbase::app().get_plugin< steem::plugins::p2p::p2p_plugin >() ),
            _chain( appbase::app().get_plugin< steem::plugins::chain::chain_plugin >() )
         {
            _on_applied_block_connection = _chain.db().applied_block_proxy(
               [&]( const signed_block& b ){ on_applied_block( b ); }, 0,
               STEEM_NETWORK_BROADCAST_API_PLUGIN_NAME );
         }

         DECLARE_API_IMPL(
            (broadcast_transaction)
            (broadcast_transaction_synchronous)
            (broadcast_block)
         )

         bool check_max_block_age( int32_t max_block_age ) const;

         void on_applied_block( const signed_block& b );

         steem::plugins::p2p::p2p_plugin&                      _p2p;
         steem::plugins::chain::chain_plugin&                  _chain;
         map< transaction_id_type, confirmation_callback >     _callbacks;
         map< time_point_sec, vector< transaction_id_type > >  _callback_expirations;
         boost::signals2::connection                           _on_applied_block_connection;

         boost::mutex                                          _mtx;
   };

   DEFINE_API_IMPL( network_broadcast_api_impl, broadcast_transaction )
   {
      FC_ASSERT( !check_max_block_age( args.max_block_age ) );
      _chain.db().push_transaction( args.trx );
      _p2p.broadcast_transaction( args.trx );

      return broadcast_transaction_return();
   }

   DEFINE_API_IMPL( network_broadcast_api_impl, broadcast_transaction_synchronous )
   {
      FC_ASSERT( !check_max_block_age( args.max_block_age ) );

      boost::promise< broadcast_transaction_synchronous_return > p;

      {
         boost::lock_guard< boost::mutex > guard( _mtx );
         _callbacks[ args.trx.id() ] = [&p]( const broadcast_transaction_synchronous_return& r )
         {
            p.set_value( r );
         };
         _callback_expirations[ args.trx.expiration ].push_back( args.trx.id() );
      }

      _chain.db().push_transaction( args.trx );
      _p2p.broadcast_transaction( args.trx );

      return p.get_future().get();
   }

   DEFINE_API_IMPL( network_broadcast_api_impl, broadcast_block )
   {
      _chain.db().push_block( args.block );
      _p2p.broadcast_block( args.block );
      return broadcast_block_return();
   }

   bool network_broadcast_api_impl::check_max_block_age( int32_t max_block_age ) const
   {
      if( max_block_age < 0 )
         return false;

      return _chain.db().with_read_lock( [&]()
      {
         fc::time_point_sec now = fc::time_point::now();
         const auto& dgpo = _chain.db().get_dynamic_global_properties();

         return ( dgpo.time < now - fc::seconds( max_block_age ) );
      });
   }

   void network_broadcast_api_impl::on_applied_block( const signed_block& b )
   { try {
      boost::lock_guard< boost::mutex > guard( _mtx );
      int32_t block_num = int32_t(b.block_num());
      if( _callbacks.size() )
      {
         for( size_t trx_num = 0; trx_num < b.transactions.size(); ++trx_num )
         {
            const auto& trx = b.transactions[trx_num];
            auto id = trx.id();
            auto itr = _callbacks.find( id );
            if( itr == _callbacks.end() ) continue;
            itr->second( broadcast_transaction_synchronous_return( id, block_num, int32_t( trx_num ), false ) );
            _callbacks.erase( itr );
         }
      }

      /// clear all expirations
      while( true )
      {
         auto exp_it = _callback_expirations.begin();
         if( exp_it == _callback_expirations.end() )
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
            callback( broadcast_transaction_synchronous_return( txid_byval, block_num, -1, true ) );

            _callbacks.erase( cb_it );
         }
         _callback_expirations.erase( exp_it );
      }
   } FC_LOG_AND_RETHROW() }
   #pragma message( "Remove FC_LOG_AND_RETHROW here before appbase release. It exists to help debug a rare lock exception" )

} // detail

network_broadcast_api::network_broadcast_api() : my( new detail::network_broadcast_api_impl() )
{
   JSON_RPC_REGISTER_API( STEEM_NETWORK_BROADCAST_API_PLUGIN_NAME );
}

network_broadcast_api::~network_broadcast_api() {}

DEFINE_LOCKLESS_APIS( network_broadcast_api,
   (broadcast_transaction)
   (broadcast_transaction_synchronous)
   (broadcast_block)
)

} } } // steem::plugins::network_broadcast_api
