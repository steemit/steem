#include <steem/plugins/follow/follow_plugin.hpp>
#include <steem/plugins/follow/follow_operations.hpp>
#include <steem/plugins/follow/follow_objects.hpp>
#include <steem/plugins/follow/inc_performance.hpp>

#include <steem/chain/account_object.hpp>
#include <steem/chain/comment_object.hpp>
#include <steem/chain/util/reward.hpp>

namespace steem { namespace plugins { namespace follow {

void follow_evaluator::do_apply( const follow_operation& o )
{
   try
   {
      static map< string, follow_type > follow_type_map = []()
      {
         map< string, follow_type > follow_map;
         follow_map[ "undefined" ] = follow_type::undefined;
         follow_map[ "blog" ] = follow_type::blog;
         follow_map[ "ignore" ] = follow_type::ignore;

         return follow_map;
      }();

      const auto& idx = _db.get_index<follow_index>().indices().get< by_account >();
      auto following_itr = idx.find( o.following );
      auto follower_itr = idx.find( o.follower );

      uint16_t what = 0;

      for( const auto& target : o.what )
      {
         switch( follow_type_map[ target ] )
         {
            case blog:
               what |= 1 << blog;
               break;
            case ignore:
               what |= 1 << ignore;
               break;
            default:
               //ilog( "Encountered unknown option ${o}", ("o", target) );
               break;
         }
      }

      if( what & ( 1 << ignore ) )
         FC_ASSERT( !( what & ( 1 << blog ) ), "Cannot follow blog and ignore author at the same time" );
 
      if( following_itr == idx.end() )
      {
         _db.create< follow_object >( [&]( follow_object& obj )
         {
            obj.account = o.following;
            obj.followers.push_back( std::make_pair( o.follower, what ) );
         });
      }
      else
      {
         using t_internal = std::pair< account_name_type, uint32_t >;

         _db.modify( *following_itr, [&]( follow_object& obj )
         {
            auto found = std::find_if( obj.followers.begin(), obj.followers.end(),
               [&]( const std::pair< account_name_type, uint32_t >& item )
               {
                  return item.first == o.follower;
               }
            );

            // auto found = std::lower_bound( obj.followers.begin(), obj.followers.end(), t_internal( o.follower, what ),
            //     [&]( const t_internal& item, const t_internal& src_item )
            //     {
            //        return item.first < src_item.first;
            //     }
            //  );

            if( found == obj.followers.end() /*|| found->first != o.follower*/ )
            {
               obj.followers.push_back( t_internal( o.follower, what ) );
               //std::sort( obj.followers.begin(), obj.followers.end() );
            }
            else
               found->second = what;

         });
      }

      if( follower_itr == idx.end() )
      {
         _db.create< follow_object >( [&]( follow_object& obj )
         {
            obj.account = o.follower;
            obj.followings.push_back( std::make_pair( o.following, what ) );
         });
      }
      else
      {
         using t_internal = std::pair< account_name_type, uint32_t >;

         _db.modify( *follower_itr, [&]( follow_object& obj )
         {
            auto found = std::find_if( obj.followings.begin(), obj.followings.end(),
               [&]( const std::pair< account_name_type, uint32_t >& item )
               {
                  return item.first == o.following;
               }
            );

            // auto found = std::lower_bound( obj.followings.begin(), obj.followings.end(), t_internal( o.following, what ),
            //     [&]( const t_internal& item, const t_internal& src_item )
            //     {
            //        return item.first < src_item.first;
            //     }
            //  );

            if( found == obj.followings.end() /*|| found->first != o.following*/ )
            {
               obj.followings.push_back(  t_internal( o.following, what ) );
               //std::sort( obj.followings.begin(), obj.followings.end() );
            }
            else
               found->second = what;

         });
      }

   }
   FC_CAPTURE_AND_RETHROW( (o) )
}

void reblog_evaluator::do_apply( const reblog_operation& o )
{
   try
   {
      performance perf( _db );

      const auto& c = _db.get_comment( o.author, o.permlink );
      FC_ASSERT( c.parent_author.size() == 0, "Only top level posts can be reblogged" );

      const auto& blog_idx = _db.get_index< blog_index >().indices().get< by_blog >();
      const auto& blog_comment_idx = _db.get_index< blog_index >().indices().get< by_comment >();

      auto next_blog_id = 0;
      auto last_blog = blog_idx.lower_bound( o.account );

      if( last_blog != blog_idx.end() && last_blog->account == o.account )
      {
         next_blog_id = last_blog->blog_feed_id + 1;
      }

      auto blog_itr = blog_comment_idx.find( boost::make_tuple( c.id, o.account ) );

      FC_ASSERT( blog_itr == blog_comment_idx.end(), "Account has already reblogged this post" );
      _db.create< blog_object >( [&]( blog_object& b )
      {
         b.account = o.account;
         b.comment = c.id;
         b.reblogged_on = _db.head_block_time();
         b.blog_feed_id = next_blog_id;
      });

      const auto& stats_idx = _db.get_index< blog_author_stats_index,by_blogger_guest_count>();
      auto stats_itr = stats_idx.lower_bound( boost::make_tuple( o.account, c.author ) );
      if( stats_itr != stats_idx.end() && stats_itr->blogger == o.account && stats_itr->guest == c.author ) {
         _db.modify( *stats_itr, [&]( blog_author_stats_object& s ) {
            ++s.count;
         });
      } else {
         _db.create<blog_author_stats_object>( [&]( blog_author_stats_object& s ) {
            s.count = 1;
            s.blogger = o.account;
            s.guest   = c.author;
         });
      }

      const auto& comment_idx = _db.get_index< feed_index >().indices().get< by_comment >();
      const auto& idx = _db.get_index< follow_index >().indices().get< by_account >();
      const auto& old_feed_idx = _db.get_index< feed_index >().indices().get< by_feed >();
      auto itr = idx.find( o.account );

      performance_data pd;

      if( _db.head_block_time() >= _plugin->start_feeds )
      {
         const t_vector< std::pair< account_name_type, uint16_t > >& _v = itr->followers;
         for( auto& item : _v )
         {
            if( item.second & ( 1 << blog ) )
            {
               auto feed_itr = comment_idx.find( boost::make_tuple( c.id, item.first ) );
               bool is_empty = feed_itr == comment_idx.end();

               pd.init( o.account, _db.head_block_time(), c.id, is_empty, is_empty ? 0 : feed_itr->account_feed_id );
               uint32_t next_id = perf.delete_old_objects< performance_data::t_creation_type::full_feed >( old_feed_idx, item.first, _plugin->max_feed_size, pd );

               if( pd.s.creation )
               {
                  if( is_empty )
                  {
                     //performance::dump( "create-feed1", std::string( item.first ), next_id );
                     _db.create< feed_object >( [&]( feed_object& f )
                     {
                        f.account = item.first;
                        f.reblogged_by.push_back( o.account );
                        f.first_reblogged_by = o.account;
                        f.first_reblogged_on = _db.head_block_time();
                        f.comment = c.id;
                        f.account_feed_id = next_id;
                     });
                  }
                  else
                  {
                     if( pd.s.allow_modify )
                     {
                        //performance::dump( "modify-feed1", std::string( feed_itr->account ), feed_itr->account_feed_id );
                        _db.modify( *feed_itr, [&]( feed_object& f )
                        {
                           f.reblogged_by.push_back( o.account );
                        });
                     }
                  }
               }

            }
         }
      }
   }
   FC_CAPTURE_AND_RETHROW( (o) )
}

} } } // steem::follow
