
#include <steem/plugins/network_broadcast_api/network_broadcast_api.hpp>
#include <steem/plugins/network_broadcast_api/network_broadcast_api_plugin.hpp>

#include <appbase/application.hpp>

#include <steem/chain/block_notification.hpp>
#include <steem/chain/database.hpp>

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
            _post_apply_block_conn = _chain.db().add_post_apply_block_handler(
               [&]( const steem::chain::block_notification& note ){ on_post_apply_block( note ); }, _chain, 0 );
         }

         DECLARE_API_IMPL(
            (broadcast_transaction)
            (broadcast_transaction_synchronous)
            (broadcast_block)
         )

         bool check_max_block_age( int32_t max_block_age ) const;

         void on_post_apply_block( const steem::chain::block_notification& note );

         steem::plugins::p2p::p2p_plugin&                      _p2p;
         steem::plugins::chain::chain_plugin&                  _chain;
         map< transaction_id_type, confirmation_callback >     _callbacks;
         map< time_point_sec, vector< transaction_id_type > >  _callback_expirations;
         boost::signals2::connection                           _post_apply_block_conn;

         boost::mutex                                          _mtx;
   };

   DEFINE_API_IMPL( network_broadcast_api_impl, broadcast_transaction )
   {
      FC_ASSERT( !check_max_block_age( args.max_block_age ) );
      _chain.accept_transaction( args.trx );
      _p2p.broadcast_transaction( args.trx );

      return broadcast_transaction_return();
   }

   DEFINE_API_IMPL( network_broadcast_api_impl, broadcast_transaction_synchronous )
   {
      FC_ASSERT( !check_max_block_age( args.max_block_age ) );

      auto txid = args.trx.id();
      boost::promise< broadcast_transaction_synchronous_return > p;

      {
         boost::lock_guard< boost::mutex > guard( _mtx );
         FC_ASSERT( _callbacks.find( txid ) == _callbacks.end(), "Transaction is a duplicate" );
         _callbacks[ txid ] = [&p]( const broadcast_transaction_synchronous_return& r )
         {
            p.set_value( r );
         };
         _callback_expirations[ args.trx.expiration ].push_back( txid );
      }

      try
      {
         /* It may look strange to call these without the lock and then lock again in the case of an exception,
          * but it is correct and avoids deadlock. accept_transaction is trained along with all other writes, including
          * pushing blocks. Pushing blocks do not originate from this code path and will never have this lock.
          * However, it will trigger the on_post_apply_block callback and then attempt to acquire the lock. In this case,
          * this thread will be waiting on accept_block so it can write and the block thread will be waiting on this
          * thread for the lock.
          */
         _chain.accept_transaction( args.trx );
         _p2p.broadcast_transaction( args.trx );
      }
      catch( fc::exception& e )
      {
         boost::lock_guard< boost::mutex > guard( _mtx );

         // The callback may have been cleared in the meantine, so we need to check for existence.
         auto c_itr = _callbacks.find( txid );
         if( c_itr != _callbacks.end() ) _callbacks.erase( c_itr );

         // We do not need to clean up _callback_expirations because on_post_apply_block handles this case.

         throw e;
      }
      catch( ... )
      {
         boost::lock_guard< boost::mutex > guard( _mtx );

         // The callback may have been cleared in the meantine, so we need to check for existence.
         auto c_itr = _callbacks.find( txid );
         if( c_itr != _callbacks.end() ) _callbacks.erase( c_itr );

         throw fc::unhandled_exception(
            FC_LOG_MESSAGE( warn, "Unknown error occured when pushing transaction" ),
            std::current_exception() );
      }

      return p.get_future().get();
   }

   DEFINE_API_IMPL( network_broadcast_api_impl, broadcast_block )
   {
      _chain.accept_block( args.block, /*currently syncing*/ false, /*skip*/ chain::database::skip_nothing );
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

   void network_broadcast_api_impl::on_post_apply_block( const steem::chain::block_notification& note )
   { try {
      const signed_block& b = note.block;
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
