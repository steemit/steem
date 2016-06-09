#include <steemit/blockchain_statistics/blockchain_statistics_api.hpp>

#include <steemit/app/impacted.hpp>
#include <steemit/chain/account_object.hpp>
#include <steemit/chain/comment_object.hpp>
#include <steemit/chain/history_object.hpp>

#include <steemit/chain/database.hpp>

namespace steemit { namespace blockchain_statistics {

namespace detail
{

class blockchain_statistics_plugin_impl
{
   public:
      blockchain_statistics_plugin_impl( blockchain_statistics_plugin& plugin )
         :_self( plugin ) {}
      virtual ~blockchain_statistics_plugin_impl() {}

      void on_block( const signed_block& b );
      void on_transaction( const signed_transaction& t );
      void on_operation( const operation_object& o );

      blockchain_statistics_plugin&       _self;
      flat_set< uint32_t >                _tracked_buckets = { 60, 3600, 21600, 86400, 604800, 2592000 };
      flat_set< bucket_object_id_type >   _current_buckets;
      uint32_t                            _maximum_history_per_bucket_size = 100;
      uint32_t                            _account_activity_api_limit = 1000;
};

struct operation_process
{
   const blockchain_statistics_plugin& _plugin;
   const bucket_object&                _bucket;
   chain::database&                    _db;

   operation_process( blockchain_statistics_plugin& bsp, const bucket_object& b )
      :_plugin( bsp ), _bucket( b ), _db( bsp.database() ) {}

   void update_account_forum_activity( const string& account )const
   {
      //ilog( "update account forum activity" );
      const auto& activity_idx = _db.get_index_type< account_activity_index >().indices().get< by_name >();
      auto itr = activity_idx.find( account );

      _db.modify( *itr, [&]( account_activity_object& a )
      {
         if( a.num_forum_ops == 0 )
         {
            a.last_forum_activity = _db.head_block_time();
            a.num_forum_ops = 1;
            a.forum_ops_per_day = 1;
         }
         else
         {
            a.num_forum_ops++;
            a.forum_ops_per_day = std::min( double( a.num_forum_ops ), 86400 * ( double( a.num_forum_ops ) / ( std::max( _db.head_block_time().sec_since_epoch() - _db.get_account( account ).created.sec_since_epoch(), 1u ) ) ) );
            a.last_forum_activity = _db.head_block_time();
         }
      });
   }

   typedef void result_type;

   template< typename T >
   void operator()( const T& )const {}

   void operator()( const transfer_operation& op )const
   {
      _db.modify( _bucket, [&]( bucket_object& b )
      {
         b.transfers++;

         if( op.amount.symbol == STEEM_SYMBOL )
            b.steem_transferred += op.amount.amount;
         else
            b.sbd_transferred += op.amount.amount;
      });
   }

   void operator()( const interest_operation& op )const
   {
      _db.modify( _bucket, [&]( bucket_object& b )
      {
         b.sbd_paid_as_interest += op.interest.amount;
      });
   }

   void operator()( const account_create_operation& op )const
   {
      _db.modify( _bucket, [&]( bucket_object& b )
      {
         b.accounts_created++;
      });
   }

   void operator()( const pow_operation& op )const
   {
      _db.modify( _bucket, [&]( bucket_object& b )
      {
         auto& worker = _db.get_account( op.worker_account );

         if( worker.created == _db.head_block_time() )
            b.accounts_created++;

         b.total_pow++;

         uint64_t bits = ( _db.get_dynamic_global_properties().num_pow_witnesses / 4 ) + 4;
         uint128_t estimated_hashes = ( 1 << bits );
         uint32_t delta_t;

         if( b.seconds == 0 )
            delta_t = _db.head_block_time().sec_since_epoch() - b.open.sec_since_epoch();
         else
         	delta_t = b.seconds;

         b.estimated_hashpower = ( b.estimated_hashpower * delta_t + estimated_hashes ) / delta_t;
      });
   }

   void operator()( const comment_operation& op )const
   {
      bool update_activity = false;

      _db.modify( _bucket, [&]( bucket_object& b )
      {
         auto& comment = _db.get_comment( op.author, op.permlink );

         if( comment.created == _db.head_block_time() )
         {
            b.total_posts++;

            if( comment.parent_author.length() )
               b.top_level_posts++;
            else
               b.replies++;

            update_activity = true;
         }
         else
         {
            b.posts_modified++;
         }
      });

      if( update_activity )
         update_account_forum_activity( op.author );
   }

   void operator()( const delete_comment_operation& op )const
   {
      _db.modify( _bucket, [&]( bucket_object& b )
      {
         b.posts_deleted++;
      });
   }

   void operator()( const vote_operation& op )const
   {
      bool update_activity = false;

      _db.modify( _bucket, [&]( bucket_object& b )
      {
         b.total_votes++;

         const auto& cv_idx = _db.get_index_type< comment_vote_index >().indices().get< by_comment_voter >();
         auto& comment = _db.get_comment( op.author, op.permlink );
         auto& voter = _db.get_account( op.voter );
         auto itr = cv_idx.find( boost::make_tuple( comment.id, voter.id ) );

         if( itr->num_changes )
            b.changed_votes++;
         else
         {
            b.new_votes++;
            update_activity = true;
         }
      });

      if( update_activity )
         update_account_forum_activity( op.voter );
   }

   void operator()( const comment_reward_operation& op )const
   {
      _db.modify( _bucket, [&]( bucket_object& b )
      {
         if( op.author == op.originating_author
            && op.permlink == op.originating_permlink )
            b.payouts++;

         b.sbd_paid_to_comments += op.payout.amount;
         b.vests_paid_to_comments += op.vesting_payout.amount;
      });
   }

   void operator()( const curate_reward_operation& op )const
   {
      _db.modify( _bucket, [&]( bucket_object& b )
      {
         b.vests_paid_to_curators += op.reward.amount;
      });
   }

   void operator()( const liquidity_reward_operation& op )const
   {
      _db.modify( _bucket, [&]( bucket_object& b )
      {
         b.liquidity_rewards_paid += op.payout.amount;
      });
   }

   void operator()( const transfer_to_vesting_operation& op )const
   {
      _db.modify( _bucket, [&]( bucket_object& b )
      {
         b.transfers_to_vesting++;
         b.steem_vested += op.amount.amount;
      });
   }

   void operator()( const fill_vesting_withdraw_operation& op )const
   {
      _db.modify( _bucket, [&]( bucket_object& b )
      {
         b.vesting_withdrawals_processed++;
         b.vests_withdrawn += op.vesting_shares.amount;
      });
   }

   void operator()( const limit_order_create_operation& op )const
   {
      _db.modify( _bucket, [&]( bucket_object& b )
      {
         b.limit_orders_created++;
      });
   }

   void operator()( const fill_order_operation& op )const
   {
      _db.modify( _bucket, [&]( bucket_object& b )
      {
         b.limit_orders_filled++;
      });
   }

   void operator()( const limit_order_cancel_operation& op )const
   {
      _db.modify( _bucket, [&]( bucket_object& b )
      {
         b.limit_orders_cancelled++;
      });
   }
};

void blockchain_statistics_plugin_impl::on_block( const signed_block& b )
{
   auto& db = _self.database();

   if( b.block_num() == 1 )
   {
      db.create< bucket_object >( [&]( bucket_object& bo )
      {
         bo.open = b.timestamp;
         bo.seconds = 0;
         bo.blocks = 1;
      });
   }
   else
   {
      db.modify( bucket_object_id_type()( db ), [&]( bucket_object& bo )
      {
         bo.blocks++;
      });
   }

   _current_buckets.clear();
   _current_buckets.insert( bucket_object_id_type() );

   const auto& bucket_idx = db.get_index_type< bucket_index >().indices().get< by_bucket >();

   for( auto bucket : _tracked_buckets )
   {
      auto open = fc::time_point_sec( ( db.head_block_time().sec_since_epoch() / bucket ) * bucket );
      auto itr = bucket_idx.find( boost::make_tuple( bucket, open ) );

      if( itr == bucket_idx.end() )
      {
         _current_buckets.insert(
            db.create< bucket_object >( [&]( bucket_object& bo )
            {
               bo.open = open;
               bo.seconds = bucket;
               bo.blocks = 1;
            }).id );

         if( _maximum_history_per_bucket_size > 0 )
         {
            try
            {
               auto cutoff = fc::time_point_sec( ( safe< uint32_t >( db.head_block_time().sec_since_epoch() ) - safe< uint32_t >( bucket ) * safe< uint32_t >( _maximum_history_per_bucket_size ) ).value );

               itr = bucket_idx.lower_bound( boost::make_tuple( bucket, fc::time_point_sec() ) );

               while( itr->seconds == bucket && itr->open < cutoff )
               {
                  auto old_itr = itr;
                  ++itr;
                  db.remove( *old_itr );
               }
            }
            catch( fc::overflow_exception& e ) {}
            catch( fc::underflow_exception& e ) {}
         }
      }
      else
      {
         db.modify( *itr, [&]( bucket_object& bo )
         {
            bo.blocks++;
         });

         _current_buckets.insert( itr->id );
      }
   }
}

void blockchain_statistics_plugin_impl::on_transaction( const signed_transaction& t )
{
   auto& db = _self.database();

   for( auto bucket_id : _current_buckets )
   {
      //const auto& bucket = bucket_id( db );

      db.modify( bucket_id( db ), [&]( bucket_object& b )
      {
         b.transactions++;
      });
   }
}

void blockchain_statistics_plugin_impl::on_operation( const operation_object& o )
{
   auto& db = _self.database();

   for( auto bucket_id : _current_buckets )
   {
      const auto& bucket = bucket_id( db );

      db.modify( bucket, [&]( bucket_object& b )
      {
         b.operations++;
      });

      flat_set< string > impacted;
      steemit::app::operation_get_impacted_accounts( o.op, impacted );
      const auto& activity_idx = db.get_index_type< account_activity_index >().indices().get< by_name >();

      for( auto account : impacted )
      {
         if( account == "" )
            continue;

         auto itr = activity_idx.find( account );

         if( itr == activity_idx.end() )
         {
            db.create< account_activity_object >( [&]( account_activity_object& a)
            {
               a.account_name = account;
               a.last_activity = db.head_block_time();
               a.last_forum_activity = fc::time_point_sec();
               a.ops_per_day = 0;
               a.forum_ops_per_day = 0;
               a.num_ops = 1;
               a.num_forum_ops = 0;
            });
         }
         else
         {
            db.modify( *itr, [&]( account_activity_object& a )
            {
               a.num_ops++;
               a.ops_per_day = std::min( double( a.num_ops ), 86400 * ( double( a.num_ops ) / ( std::max( db.head_block_time().sec_since_epoch() - db.get_account( account ).created.sec_since_epoch(), 1u ) ) ) );
               a.last_activity = db.head_block_time();
            });
         }
      }

      o.op.visit( operation_process( _self, bucket ) );
   }
}

} // detail

blockchain_statistics_plugin::blockchain_statistics_plugin()
   :_my( new detail::blockchain_statistics_plugin_impl( *this ) ) {}

blockchain_statistics_plugin::~blockchain_statistics_plugin() {}

void blockchain_statistics_plugin::plugin_set_program_options(
   boost::program_options::options_description& cli,
   boost::program_options::options_description& cfg
)
{
   cli.add_options()
         ("chain-stats-bucket-size", boost::program_options::value<string>()->default_value("[60,3600,21600,86400,604800,2592000]"),
           "Track market history by grouping orders into buckets of equal size measured in seconds specified as a JSON array of numbers")
         ("chain-stats-history-per-size", boost::program_options::value<uint32_t>()->default_value(100),
           "How far back in time to track history for each bucket size, measured in the number of buckets (default: 100)")
         ("chain-stats-activity-api-limit", boost::program_options::value<uint32_t>()->default_value(1000),
           "The limit of how many objects should be returned in the account activity API calls (default: 1000)")
         ;
   cfg.add(cli);
}

void blockchain_statistics_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   try
   {
      database().applied_block.connect( [&]( const signed_block& b ){ _my->on_block( b ); } );
      database().on_applied_transaction.connect( [&]( const signed_transaction& t ){ _my->on_transaction( t ); } );
      database().post_apply_operation.connect( [&]( const operation_object& o ){ _my->on_operation( o ); } );

      database().add_index< primary_index< bucket_index > >();
      database().add_index< primary_index< account_activity_index > >();

      if( options.count( "chain-stats-bucket-size" ) )
      {
         const std::string& buckets = options[ "chain-stats-bucket-size" ].as< string >();
         _my->_tracked_buckets = fc::json::from_string( buckets ).as< flat_set< uint32_t > >();
      }
      if( options.count( "chain-stats-history-per-edge" ) )
         _my->_maximum_history_per_bucket_size = options[ "chain-stats-history-per-size" ].as< uint32_t >();
      if( options.count( "chain-stats-activity-api-limit" ) )
         _my->_account_activity_api_limit = options[ "chain-stats-activity-api-limit" ].as< uint32_t >();

   } FC_CAPTURE_AND_RETHROW()
}

void blockchain_statistics_plugin::plugin_startup()
{
   app().register_api_factory< blockchain_statistics_api >( "chain_stats_api" );
}

const flat_set< uint32_t >& blockchain_statistics_plugin::get_tracked_buckets() const
{
   return _my->_tracked_buckets;
}

uint32_t blockchain_statistics_plugin::get_max_history_per_bucket() const
{
   return _my->_maximum_history_per_bucket_size;
}

uint32_t blockchain_statistics_plugin::get_account_activity_api_limit() const
{
   return _my->_account_activity_api_limit;
}

} } // steemit::blockchain_statistics

STEEMIT_DEFINE_PLUGIN( blockchain_statistics, steemit::blockchain_statistics::blockchain_statistics_plugin );
