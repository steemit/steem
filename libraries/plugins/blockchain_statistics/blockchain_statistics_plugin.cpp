#include <steemit/blockchain_statistics/blockchain_statistics_api.hpp>

#include <steemit/app/impacted.hpp>
#include <steemit/chain/account_object.hpp>
#include <steemit/chain/comment_object.hpp>
#include <steemit/chain/history_object.hpp>

#include <steemit/chain/database.hpp>
#include <steemit/chain/index.hpp>
#include <steemit/chain/operation_notification.hpp>

namespace steemit { namespace blockchain_statistics {

namespace detail
{

using namespace steemit::protocol;

class blockchain_statistics_plugin_impl
{
   public:
      blockchain_statistics_plugin_impl( blockchain_statistics_plugin& plugin )
         :_self( plugin ) {}
      virtual ~blockchain_statistics_plugin_impl() {}

      void on_block( const signed_block& b );
      void pre_operation( const operation_notification& o );
      void post_operation( const operation_notification& o );

      blockchain_statistics_plugin&       _self;
      flat_set< uint32_t >                _tracked_buckets = { 60, 3600, 21600, 86400, 604800, 2592000 };
      flat_set< bucket_id_type >   _current_buckets;
      uint32_t                            _maximum_history_per_bucket_size = 100;
};

struct operation_process
{
   const blockchain_statistics_plugin& _plugin;
   const bucket_object&                _bucket;
   chain::database&                    _db;

   operation_process( blockchain_statistics_plugin& bsp, const bucket_object& b )
      :_plugin( bsp ), _bucket( b ), _db( bsp.database() ) {}

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
         b.paid_accounts_created++;
      });
   }

   void operator()( const pow_operation& op )const
   {
      _db.modify( _bucket, [&]( bucket_object& b )
      {
         auto& worker = _db.get_account( op.worker_account );

         if( worker.created == _db.head_block_time() )
            b.mined_accounts_created++;

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
      _db.modify( _bucket, [&]( bucket_object& b )
      {
         auto& comment = _db.get_comment( op.author, op.permlink );

         if( comment.created == _db.head_block_time() )
         {
            if( comment.parent_author.length() )
               b.replies++;
            else
               b.root_comments++;
         }
         else
         {
            if( comment.parent_author.length() )
               b.reply_edits++;
            else
               b.root_comment_edits++;
         }
      });
   }

   void operator()( const vote_operation& op )const
   {
      _db.modify( _bucket, [&]( bucket_object& b )
      {
         const auto& cv_idx = _db.get_index< comment_vote_index >().indices().get< by_comment_voter >();
         auto& comment = _db.get_comment( op.author, op.permlink );
         auto& voter = _db.get_account( op.voter );
         auto itr = cv_idx.find( boost::make_tuple( comment.id, voter.id ) );

         if( itr->num_changes )
         {
            if( comment.parent_author.size() )
               b.new_reply_votes++;
            else
               b.new_root_votes++;
         }
         else
         {
            if( comment.parent_author.size() )
               b.changed_reply_votes++;
            else
               b.changed_root_votes++;
         }
      });
   }

   void operator()( const author_reward_operation& op )const
   {
      _db.modify( _bucket, [&]( bucket_object& b )
      {
         b.payouts++;
         b.sbd_paid_to_authors += op.sbd_payout.amount;
         b.vests_paid_to_authors += op.vesting_payout.amount;
      });
   }

   void operator()( const curation_reward_operation& op )const
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
      auto& account = _db.get_account( op.from_account );

      _db.modify( _bucket, [&]( bucket_object& b )
      {
         b.vesting_withdrawals_processed++;
         if( op.deposited.symbol == STEEM_SYMBOL )
            b.vests_withdrawn += op.withdrawn.amount;
         else
            b.vests_transferred += op.withdrawn.amount;

         if( account.vesting_withdraw_rate.amount == 0 )
            b.finished_vesting_withdrawals++;
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
         b.limit_orders_filled += 2;
      });
   }

   void operator()( const limit_order_cancel_operation& op )const
   {
      _db.modify( _bucket, [&]( bucket_object& b )
      {
         b.limit_orders_cancelled++;
      });
   }

   void operator()( const convert_operation& op )const
   {
      _db.modify( _bucket, [&]( bucket_object& b )
      {
         b.sbd_conversion_requests_created++;
         b.sbd_to_be_converted += op.amount.amount;
      });
   }

   void operator()( const fill_convert_request_operation& op )const
   {
      _db.modify( _bucket, [&]( bucket_object& b )
      {
         b.sbd_conversion_requests_filled++;
         b.steem_converted += op.amount_out.amount;
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
      db.modify( db.get( bucket_id_type() ), [&]( bucket_object& bo )
      {
         bo.blocks++;
      });
   }

   _current_buckets.clear();
   _current_buckets.insert( bucket_id_type() );

   const auto& bucket_idx = db.get_index< bucket_index >().indices().get< by_bucket >();

   uint32_t trx_size = 0;
   uint32_t num_trx =b.transactions.size();

   for( auto trx : b.transactions )
   {
      trx_size += fc::raw::pack_size( trx );
   }


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

      db.modify( *itr, [&]( bucket_object& bo )
      {
         bo.transactions += num_trx;
         bo.bandwidth += trx_size;
      });
   }
}

void blockchain_statistics_plugin_impl::pre_operation( const operation_notification& o )
{
   auto& db = _self.database();

   for( auto bucket_id : _current_buckets )
   {
      if( o.op.which() == operation::tag< delete_comment_operation >::value )
      {
         delete_comment_operation op = o.op.get< delete_comment_operation >();
         auto comment = db.get_comment( op.author, op.permlink );
         const auto& bucket = db.get(bucket_id);

         db.modify( bucket, [&]( bucket_object& b )
         {
            if( comment.parent_author.length() )
               b.replies_deleted++;
            else
               b.root_comments_deleted++;
         });
      }
      else if( o.op.which() == operation::tag< withdraw_vesting_operation >::value )
      {
         withdraw_vesting_operation op = o.op.get< withdraw_vesting_operation >();
         auto& account = db.get_account( op.account );
         const auto& bucket = db.get(bucket_id);

         auto new_vesting_withdrawal_rate = op.vesting_shares.amount / STEEMIT_VESTING_WITHDRAW_INTERVALS;
         if( op.vesting_shares.amount > 0 && new_vesting_withdrawal_rate == 0 )
            new_vesting_withdrawal_rate = 1;

         if( !db.has_hardfork( STEEMIT_HARDFORK_0_1 ) )
            new_vesting_withdrawal_rate *= 1000000;

         db.modify( bucket, [&]( bucket_object& b )
         {
            if( account.vesting_withdraw_rate.amount > 0 )
               b.modified_vesting_withdrawal_requests++;
            else
               b.new_vesting_withdrawal_requests++;

            // TODO: Figure out how to change delta when a vesting withdraw finishes. Have until March 24th 2018 to figure that out...
            b.vesting_withdraw_rate_delta += new_vesting_withdrawal_rate - account.vesting_withdraw_rate.amount;
         });
      }
   }
}

void blockchain_statistics_plugin_impl::post_operation( const operation_notification& o )
{
   try
   {
   auto& db = _self.database();

   for( auto bucket_id : _current_buckets )
   {
      const auto& bucket = db.get(bucket_id);

      if( !is_virtual_operation( o.op ) )
      {
         db.modify( bucket, [&]( bucket_object& b )
         {
            b.operations++;
         });
      }
      o.op.visit( operation_process( _self, bucket ) );
   }
   } FC_CAPTURE_AND_RETHROW()
}

} // detail

blockchain_statistics_plugin::blockchain_statistics_plugin( application* app )
   :plugin( app ), _my( new detail::blockchain_statistics_plugin_impl( *this ) ) {}

blockchain_statistics_plugin::~blockchain_statistics_plugin() {}

void blockchain_statistics_plugin::plugin_set_program_options(
   boost::program_options::options_description& cli,
   boost::program_options::options_description& cfg
)
{
   cli.add_options()
         ("chain-stats-bucket-size", boost::program_options::value<string>()->default_value("[60,3600,21600,86400,604800,2592000]"),
           "Track blockchain statistics by grouping orders into buckets of equal size measured in seconds specified as a JSON array of numbers")
         ("chain-stats-history-per-bucket", boost::program_options::value<uint32_t>()->default_value(100),
           "How far back in time to track history for each bucket size, measured in the number of buckets (default: 100)")
         ;
   cfg.add(cli);
}

void blockchain_statistics_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   try
   {
      ilog( "chain_stats_plugin: plugin_initialize() begin" );
      chain::database& db = database();

      db.applied_block.connect( [&]( const signed_block& b ){ _my->on_block( b ); } );
      db.pre_apply_operation.connect( [&]( const operation_notification& o ){ _my->pre_operation( o ); } );
      db.post_apply_operation.connect( [&]( const operation_notification& o ){ _my->post_operation( o ); } );

      add_plugin_index< bucket_index >(db);

      if( options.count( "chain-stats-bucket-size" ) )
      {
         const std::string& buckets = options[ "chain-stats-bucket-size" ].as< string >();
         _my->_tracked_buckets = fc::json::from_string( buckets ).as< flat_set< uint32_t > >();
      }
      if( options.count( "chain-stats-history-per-bucket" ) )
         _my->_maximum_history_per_bucket_size = options[ "chain-stats-history-per-bucket" ].as< uint32_t >();

      wlog( "chain-stats-bucket-size: ${b}", ("b", _my->_tracked_buckets) );
      wlog( "chain-stats-history-per-bucket: ${h}", ("h", _my->_maximum_history_per_bucket_size) );

      ilog( "chain_stats_plugin: plugin_initialize() end" );
   } FC_CAPTURE_AND_RETHROW()
}

void blockchain_statistics_plugin::plugin_startup()
{
   ilog( "chain_stats plugin: plugin_startup() begin" );

   app().register_api_factory< blockchain_statistics_api >( "chain_stats_api" );

   ilog( "chain_stats plugin: plugin_startup() end" );
}

const flat_set< uint32_t >& blockchain_statistics_plugin::get_tracked_buckets() const
{
   return _my->_tracked_buckets;
}

uint32_t blockchain_statistics_plugin::get_max_history_per_bucket() const
{
   return _my->_maximum_history_per_bucket_size;
}

} } // steemit::blockchain_statistics

STEEMIT_DEFINE_PLUGIN( blockchain_statistics, steemit::blockchain_statistics::blockchain_statistics_plugin );
