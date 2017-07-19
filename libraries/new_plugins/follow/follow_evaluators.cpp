#include <steemit/plugins/follow/follow_plugin.hpp>
#include <steemit/plugins/follow/follow_operations.hpp>
#include <steemit/plugins/follow/follow_objects.hpp>

#include <steemit/chain/account_object.hpp>
#include <steemit/chain/comment_object.hpp>

namespace steemit { namespace plugins { namespace follow {

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

      const auto& idx = _db.get_index<follow_index>().indices().get< by_follower_following >();
      auto itr = idx.find( boost::make_tuple( o.follower, o.following ) );

      uint16_t what = 0;
      bool is_following = false;

      for( auto target : o.what )
      {
         switch( follow_type_map[ target ] )
         {
            case blog:
               what |= 1 << blog;
               is_following = true;
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

      bool was_followed = false;

      if( itr == idx.end() )
      {
         _db.create< follow_object >( [&]( follow_object& obj )
         {
            obj.follower = o.follower;
            obj.following = o.following;
            obj.what = what;
         });
      }
      else
      {
         was_followed = itr->what & 1 << blog;

         _db.modify( *itr, [&]( follow_object& obj )
         {
            obj.what = what;
         });
      }

      const auto& follower = _db.find< follow_count_object, by_account >( o.follower );

      if( follower == nullptr )
      {
         _db.create< follow_count_object >( [&]( follow_count_object& obj )
         {
            obj.account = o.follower;

            if( is_following )
               obj.following_count = 1;
         });
      }
      else
      {
         _db.modify( *follower, [&]( follow_count_object& obj )
         {
            if( was_followed )
               obj.following_count--;
            if( is_following )
               obj.following_count++;
         });
      }

      const auto& following = _db.find< follow_count_object, by_account >( o.following );

      if( following == nullptr )
      {
         _db.create< follow_count_object >( [&]( follow_count_object& obj )
         {
            obj.account = o.following;

            if( is_following )
               obj.follower_count = 1;
         });
      }
      else
      {
         _db.modify( *following, [&]( follow_count_object& obj )
         {
            if( was_followed )
               obj.follower_count--;
            if( is_following )
               obj.follower_count++;
         });
      }
   }
   FC_CAPTURE_AND_RETHROW( (o) )
}

void reblog_evaluator::do_apply( const reblog_operation& o )
{
   try
   {
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

      const auto& feed_idx = _db.get_index< feed_index >().indices().get< by_feed >();
      const auto& comment_idx = _db.get_index< feed_index >().indices().get< by_comment >();
      const auto& idx = _db.get_index< follow_index >().indices().get< by_following_follower >();
      auto itr = idx.find( o.account );

      if( _db.head_block_time() >= _plugin->start_feeds )
      {
         while( itr != idx.end() && itr->following == o.account )
         {

            if( itr->what & ( 1 << blog ) )
            {
               uint32_t next_id = 0;
               auto last_feed = feed_idx.lower_bound( itr->follower );

               if( last_feed != feed_idx.end() && last_feed->account == itr->follower )
               {
                  next_id = last_feed->account_feed_id + 1;
               }

               auto feed_itr = comment_idx.find( boost::make_tuple( c.id, itr->follower ) );

               if( feed_itr == comment_idx.end() )
               {
                  _db.create< feed_object >( [&]( feed_object& f )
                  {
                     f.account = itr->follower;
                     f.reblogged_by.push_back( o.account );
                     f.first_reblogged_by = o.account;
                     f.first_reblogged_on = _db.head_block_time();
                     f.comment = c.id;
                     f.reblogs = 1;
                     f.account_feed_id = next_id;
                  });
               }
               else
               {
                  _db.modify( *feed_itr, [&]( feed_object& f )
                  {
                     f.reblogged_by.push_back( o.account );
                     f.reblogs++;
                  });
               }

               const auto& old_feed_idx = _db.get_index< feed_index >().indices().get< by_old_feed >();
               auto old_feed = old_feed_idx.lower_bound( itr->follower );

               while( old_feed->account == itr->follower && next_id - old_feed->account_feed_id > _plugin->max_feed_size )
               {
                  _db.remove( *old_feed );
                  old_feed = old_feed_idx.lower_bound( itr->follower );
               };
            }

            ++itr;
         }
      }
   }
   FC_CAPTURE_AND_RETHROW( (o) )
}

} } } // steemit::follow
