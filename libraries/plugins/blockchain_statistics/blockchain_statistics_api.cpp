#include <steemit/blockchain_statistics/blockchain_statistics_api.hpp>

namespace steemit { namespace blockchain_statistics {

namespace detail
{
   class blockchain_statistics_api_impl
   {
      public:
         blockchain_statistics_api_impl( steemit::app::application& app )
            :_app( app ) {}

         statistics get_stats_for_time( fc::time_point_sec open, uint32_t interval )const;
         statistics get_stats_for_interval( fc::time_point_sec start, fc::time_point_sec end )const;
         statistics get_lifetime_stats()const;
         vector< account_activity > get_account_activity_by_name( string start, string end, uint32_t limit )const;
         vector< account_activity > get_account_activity_by_last_activity( uint32_t limit )const;
         vector< account_activity > get_account_activity_by_longest_inactive( uint32_t limit )const;
         vector< account_activity > get_account_activity_by_most_active( uint32_t limit )const;
         vector< account_activity > get_account_activity_by_least_active( uint32_t limit )const;
         vector< account_activity > get_account_activity_by_most_ops( uint32_t limit )const;
         vector< account_activity > get_account_activity_by_least_ops( uint32_t limit )const;
         vector< account_activity > get_account_forum_activity_by_last_activity( uint32_t limit )const;
         vector< account_activity > get_account_forum_activity_by_longest_inactive( uint32_t limit )const;
         vector< account_activity > get_account_forum_activity_by_most_active( uint32_t limit )const;
         vector< account_activity > get_account_forum_activity_by_least_active( uint32_t limit )const;
         vector< account_activity > get_account_forum_activity_by_most_ops( uint32_t limit )const;
         vector< account_activity > get_account_forum_activity_by_least_ops( uint32_t limit )const;

         uint32_t                   _account_activity_api_limit = 1000;
         steemit::app::application& _app;
   };

   statistics blockchain_statistics_api_impl::get_stats_for_time( fc::time_point_sec open, uint32_t interval )const
   {
      statistics result;
      const auto& bucket_idx = _app.chain_database()->get_index_type< bucket_index >().indices().get< by_bucket >();
      auto itr = bucket_idx.lower_bound( boost::make_tuple( interval, open ) );

      if( open.sec_since_epoch() - itr->open.sec_since_epoch() >= interval )
         itr++;

      if( itr != bucket_idx.end()
            && itr->seconds == interval
            && open.sec_since_epoch() - itr->open.sec_since_epoch() < interval )
         result += *itr;

      return result;
   }

   statistics blockchain_statistics_api_impl::get_stats_for_interval( fc::time_point_sec start, fc::time_point_sec end )const
   {
      statistics result;
      const auto& bucket_itr = _app.chain_database()->get_index_type< bucket_index >().indices().get< by_bucket >();
      const auto& sizes = _app.get_plugin< blockchain_statistics_plugin >( BLOCKCHAIN_STATISTICS_PLUGIN_NAME )->get_tracked_buckets();
      auto size_itr = sizes.rbegin();
      auto time = start;

      // This is a greedy algorithm, same as the ubiquitous "making change" problem.
      // So long as the bucket sizes share a common denominator, the greedy solution
      // has the same efficiency as the dynamic solution.
      while( size_itr != sizes.rend() && time < end )
      {
         auto itr = bucket_itr.find( boost::make_tuple( *size_itr, time ) );

         while( itr != bucket_itr.end() && itr->seconds == *size_itr && time + itr->seconds <= end )
         {
            time += *size_itr;
            result += *itr;
            itr++;
         }

         size_itr++;
      }

      return result;
   }

   statistics blockchain_statistics_api_impl::get_lifetime_stats()const
   {
      statistics result;
      result += bucket_object_id_type()( *( _app.chain_database() ) );

      return result;
   }

   vector< account_activity > blockchain_statistics_api_impl::get_account_activity_by_name( string start, string end, uint32_t limit )const
   {
      FC_ASSERT( limit <= _account_activity_api_limit, "Cannot query more than 1000 accounts at a time" );

      vector< account_activity > result;
      const auto& activity_idx = _app.chain_database()->get_index_type< account_activity_index >().indices().get< by_name >();

      for( auto itr = activity_idx.lower_bound( start );
         itr!= activity_idx.end() && result.size() < limit && itr->account_name < end;
         itr++ )
      {
         result.push_back( *itr );
      }

      return result;
   }

   vector< account_activity > blockchain_statistics_api_impl::get_account_activity_by_last_activity( uint32_t limit )const
   {
      FC_ASSERT( limit <= _account_activity_api_limit, "Cannot query more than 1000 accounts at a time" );

      vector< account_activity > result;
      const auto& activity_idx = _app.chain_database()->get_index_type< account_activity_index >().indices().get< by_last_activity >();

      for( auto itr = activity_idx.rbegin();
         itr != activity_idx.rend() && result.size() < limit;
         itr++ )
      {
         result.push_back( *itr );
      }

      return result;
   }

   vector< account_activity > blockchain_statistics_api_impl::get_account_activity_by_longest_inactive( uint32_t limit )const
   {
      FC_ASSERT( limit <= _account_activity_api_limit, "Cannot query more than 1000 accounts at a time" );

      vector< account_activity > result;
      const auto& activity_idx = _app.chain_database()->get_index_type< account_activity_index >().indices().get< by_last_activity >();

      for( auto itr = activity_idx.begin();
         itr != activity_idx.end() && result.size() < limit;
         itr++ )
      {
         result.push_back( *itr );
      }

      return result;
   }

   vector< account_activity > blockchain_statistics_api_impl::get_account_activity_by_most_active( uint32_t limit )const
   {
      FC_ASSERT( limit <= _account_activity_api_limit, "Cannot query more than 1000 accounts at a time" );

      vector< account_activity > result;
      const auto& activity_idx = _app.chain_database()->get_index_type< account_activity_index >().indices().get< by_activity >();

      for( auto itr = activity_idx.rbegin();
         itr != activity_idx.rend() && result.size() < limit;
         itr++ )
      {
         result.push_back( *itr );
      }

      return result;
   }

   vector< account_activity > blockchain_statistics_api_impl::get_account_activity_by_least_active( uint32_t limit )const
   {
      FC_ASSERT( limit <= _account_activity_api_limit, "Cannot query more than 1000 accounts at a time" );

      vector< account_activity > result;
      const auto& activity_idx = _app.chain_database()->get_index_type< account_activity_index >().indices().get< by_activity >();

      for( auto itr = activity_idx.begin();
         itr != activity_idx.end() && result.size() < limit;
         itr++ )
      {
         result.push_back( *itr );
      }

      return result;
   }

   vector< account_activity > blockchain_statistics_api_impl::get_account_activity_by_most_ops( uint32_t limit )const
   {
      FC_ASSERT( limit <= _account_activity_api_limit, "Cannot query more than 1000 accounts at a time" );

      vector< account_activity > result;
      const auto& activity_idx = _app.chain_database()->get_index_type< account_activity_index >().indices().get< by_num_ops >();

      for( auto itr = activity_idx.rbegin();
         itr != activity_idx.rend() && result.size() < limit;
         itr++ )
      {
         result.push_back( *itr );
      }

      return result;
   }

   vector< account_activity > blockchain_statistics_api_impl::get_account_activity_by_least_ops( uint32_t limit )const
   {
      FC_ASSERT( limit <= _account_activity_api_limit, "Cannot query more than 1000 accounts at a time" );

      vector< account_activity > result;
      const auto& activity_idx = _app.chain_database()->get_index_type< account_activity_index >().indices().get< by_num_ops >();

      for( auto itr = activity_idx.begin();
         itr != activity_idx.end() && result.size() < limit;
         itr++ )
      {
         result.push_back( *itr );
      }

      return result;
   }

   vector< account_activity > blockchain_statistics_api_impl::get_account_forum_activity_by_last_activity( uint32_t limit )const
   {
      FC_ASSERT( limit <= _account_activity_api_limit, "Cannot query more than 1000 accounts at a time" );

      vector< account_activity > result;
      const auto& activity_idx = _app.chain_database()->get_index_type< account_activity_index >().indices().get< by_last_forum_activity >();

      for( auto itr = activity_idx.rbegin();
         itr != activity_idx.rend() && result.size() < limit;
         itr++ )
      {
         result.push_back( *itr );
      }

      return result;
   }

   vector< account_activity > blockchain_statistics_api_impl::get_account_forum_activity_by_longest_inactive( uint32_t limit )const
   {
      FC_ASSERT( limit <= _account_activity_api_limit, "Cannot query more than 1000 accounts at a time" );

      vector< account_activity > result;
      const auto& activity_idx = _app.chain_database()->get_index_type< account_activity_index >().indices().get< by_last_forum_activity >();

      for( auto itr = activity_idx.begin();
         itr != activity_idx.end() && result.size() < limit;
         itr++ )
      {
         result.push_back( *itr );
      }

      return result;
   }

   vector< account_activity > blockchain_statistics_api_impl::get_account_forum_activity_by_most_active( uint32_t limit )const
   {
      FC_ASSERT( limit <= _account_activity_api_limit, "Cannot query more than 1000 accounts at a time" );

      vector< account_activity > result;
      const auto& activity_idx = _app.chain_database()->get_index_type< account_activity_index >().indices().get< by_forum_activity >();

      for( auto itr = activity_idx.rbegin();
         itr != activity_idx.rend() && result.size() < limit;
         itr++ )
      {
         result.push_back( *itr );
      }

      return result;
   }

   vector< account_activity > blockchain_statistics_api_impl::get_account_forum_activity_by_least_active( uint32_t limit )const
   {
      FC_ASSERT( limit <= _account_activity_api_limit, "Cannot query more than 1000 accounts at a time" );

      vector< account_activity > result;
      const auto& activity_idx = _app.chain_database()->get_index_type< account_activity_index >().indices().get< by_forum_activity >();

      for( auto itr = activity_idx.begin();
         itr != activity_idx.end() && result.size() < limit;
         itr++ )
      {
         result.push_back( *itr );
      }

      return result;
   }

   vector< account_activity > blockchain_statistics_api_impl::get_account_forum_activity_by_most_ops( uint32_t limit )const
   {
      FC_ASSERT( limit <= _account_activity_api_limit, "Cannot query more than 1000 accounts at a time" );

      vector< account_activity > result;
      const auto& activity_idx = _app.chain_database()->get_index_type< account_activity_index >().indices().get< by_num_forum_ops >();

      for( auto itr = activity_idx.rbegin();
         itr != activity_idx.rend() && result.size() < limit;
         itr++ )
      {
         result.push_back( *itr );
      }

      return result;
   }

   vector< account_activity > blockchain_statistics_api_impl::get_account_forum_activity_by_least_ops( uint32_t limit )const
   {
      FC_ASSERT( limit <= _account_activity_api_limit, "Cannot query more than 1000 accounts at a time" );

      vector< account_activity > result;
      const auto& activity_idx = _app.chain_database()->get_index_type< account_activity_index >().indices().get< by_num_forum_ops >();

      for( auto itr = activity_idx.begin();
         itr != activity_idx.end() && result.size() < limit;
         itr++ )
      {
         result.push_back( *itr );
      }

      return result;
   }

} // detail

blockchain_statistics_api::blockchain_statistics_api( const steemit::app::api_context& ctx )
{
   my = std::make_shared< detail::blockchain_statistics_api_impl >( ctx.app );
}

void blockchain_statistics_api::on_api_startup()
{
   my->_account_activity_api_limit = my->_app.get_plugin< blockchain_statistics_plugin >( BLOCKCHAIN_STATISTICS_PLUGIN_NAME )->get_account_activity_api_limit();
}

statistics blockchain_statistics_api::get_stats_for_time( fc::time_point_sec open, uint32_t interval )const
{
   return my->get_stats_for_time( open, interval );
}

statistics blockchain_statistics_api::get_stats_for_interval( fc::time_point_sec start, fc::time_point_sec end )const
{
   return my->get_stats_for_interval( start, end );
}

statistics blockchain_statistics_api::get_lifetime_stats()const
{
   return my->get_lifetime_stats();
}

vector< account_activity > blockchain_statistics_api::get_account_activity_by_name( string start, string end, uint32_t limit )const
{
   return my->get_account_activity_by_name( start, end, limit );
}

vector< account_activity > blockchain_statistics_api::get_account_activity_by_last_activity( uint32_t limit )const
{
   return my->get_account_activity_by_last_activity( limit );
}

vector< account_activity > blockchain_statistics_api::get_account_activity_by_longest_inactive( uint32_t limit )const
{
   return my->get_account_activity_by_longest_inactive( limit );
}

vector< account_activity > blockchain_statistics_api::get_account_activity_by_most_active( uint32_t limit )const
{
   return my->get_account_activity_by_most_active( limit );
}

vector< account_activity > blockchain_statistics_api::get_account_activity_by_least_active( uint32_t limit )const
{
   return my->get_account_activity_by_least_active( limit );
}

vector< account_activity > blockchain_statistics_api::get_account_activity_by_most_ops( uint32_t limit )const
{
   return my->get_account_activity_by_most_ops( limit );
}

vector< account_activity > blockchain_statistics_api::get_account_activity_by_least_ops( uint32_t limit )const
{
   return my->get_account_activity_by_least_ops( limit );
}

vector< account_activity > blockchain_statistics_api::get_account_forum_activity_by_last_activity( uint32_t limit )const
{
   return my->get_account_forum_activity_by_last_activity( limit );
}

vector< account_activity > blockchain_statistics_api::get_account_forum_activity_by_longest_inactive( uint32_t limit )const
{
   return my->get_account_forum_activity_by_longest_inactive( limit );
}

vector< account_activity > blockchain_statistics_api::get_account_forum_activity_by_most_active( uint32_t limit )const
{
   return my->get_account_forum_activity_by_most_active( limit );
}

vector< account_activity > blockchain_statistics_api::get_account_forum_activity_by_least_active( uint32_t limit )const
{
   return my->get_account_forum_activity_by_least_active( limit );
}

vector< account_activity > blockchain_statistics_api::get_account_forum_activity_by_most_ops( uint32_t limit )const
{
   return my->get_account_forum_activity_by_most_ops( limit );
}

vector< account_activity > blockchain_statistics_api::get_account_forum_activity_by_least_ops( uint32_t limit )const
{
   return my->get_account_forum_activity_by_least_ops( limit );
}

statistics& statistics::operator +=( const bucket_object& b )
{
   this->blocks                           += b.blocks;
   this->operations                       += b.operations;
   this->transactions                     += b.transactions;
   this->transfers                        += b.transfers;
   this->steem_transferred                += b.steem_transferred;
   this->sbd_transferred                  += b.sbd_transferred;
   this->sbd_paid_as_interest             += b.sbd_paid_as_interest;
   this->accounts_created                 += b.accounts_created;
   this->total_posts                      += b.total_posts;
   this->top_level_posts                  += b.top_level_posts;
   this->replies                          += b.replies;
   this->posts_modified                   += b.posts_modified;
   this->posts_deleted                    += b.posts_deleted;
   this->total_votes                      += b.total_votes;
   this->new_votes                        += b.new_votes;
   this->changed_votes                    += b.changed_votes;
   this->payouts                          += b.payouts;
   this->sbd_paid_to_comments             += b.sbd_paid_to_comments;
   this->vests_paid_to_comments           += b.vests_paid_to_comments;
   this->vests_paid_to_curators           += b.vests_paid_to_curators;
   this->liquidity_rewards_paid           += b.liquidity_rewards_paid;
   this->transfers_to_vesting             += b.transfers_to_vesting;
   this->steem_vested                     += b.steem_vested;
   this->vesting_withdrawals_processed    += b.vesting_withdrawals_processed;
   this->vests_withdrawn                  += b.vests_withdrawn;
   this->limit_orders_created             += b.limit_orders_created;
   this->limit_orders_filled              += b.limit_orders_filled;
   this->limit_orders_cancelled           += b.limit_orders_cancelled;
   this->total_pow                        += b.total_pow;
   this->estimated_hashpower              += b.estimated_hashpower;

   return ( *this );
}

} } // steemit::blockchain_statistics
