#include <steemit/follow/follow_operations.hpp>
#include <steemit/follow/follow_objects.hpp>

#include <steemit/chain/account_object.hpp>
#include <steemit/chain/comment_object.hpp>

namespace steemit { namespace follow {

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

      const auto& following = db().get_account( o.following );
      const auto& follower = db().get_account( o.follower );
      const auto& idx = db().get_index<follow_index>().indices().get< by_follower_following >();
      auto itr = idx.find( boost::make_tuple( follower.id, following.id ) );

      uint16_t what = 0;

      for( auto target : o.what )
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

      if( itr == idx.end() )
      {
         db().create< follow_object >( [&]( follow_object& obj )
         {
            obj.follower = follower.id;
            obj.following = following.id;
            obj.what = what;
         });
      }
      else
      {
         db().modify( *itr, [&]( follow_object& obj )
         {
            obj.what = what;
         });
      }
   }
   FC_CAPTURE_AND_RETHROW( (o) )
}

void reblog_evaluator::do_apply( const reblog_operation& o )
{
   try
   {
      auto& db = _plugin->database();
      const auto& c = db.get_comment( o.author, o.permlink );

      FC_ASSERT( c.parent_author.size() == 0, "Only top level posts can be reblogged" );

      _plugin->takedown( c );

      const auto& reblog_account = db.get_account( o.account );
      const auto& blog_idx = db.get_index< blog_index >().indices().get< by_blog >();
      const auto& blog_comment_idx = db.get_index< blog_index >().indices().get< by_comment >();

      auto next_blog_id = 0;
      auto last_blog = blog_idx.lower_bound( reblog_account.id );

      if( last_blog != blog_idx.end() && last_blog->account == reblog_account.id )
      {
         next_blog_id = last_blog->blog_feed_id + 1;
      }

      auto blog_itr = blog_comment_idx.find( boost::make_tuple( c.id, reblog_account.id ) );

      FC_ASSERT( blog_itr == blog_comment_idx.end(), "Account has already reblogged this post" );
      db.create< blog_object >( [&]( blog_object& b )
      {
         b.account = reblog_account.id;
         b.comment = c.id;
         b.reblogged_on = db.head_block_time();
         b.blog_feed_id = next_blog_id;
      });

      const auto& feed_idx = db.get_index< feed_index >().indices().get< by_feed >();
      const auto& comment_idx = db.get_index< feed_index >().indices().get< by_comment >();
      const auto& idx = db.get_index< follow_index >().indices().get< by_following_follower >();
      auto itr = idx.find( reblog_account.id );

      while( itr != idx.end() && itr->following == reblog_account.id )
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
               auto& fd = db.create< feed_object >( [&]( feed_object& f )
               {
                  f.account = itr->follower;
                  f.first_reblogged_by = reblog_account.id;
                  f.first_reblogged_on = db.head_block_time();
                  f.comment = c.id;
                  f.reblogs = 1;
                  f.account_feed_id = next_id;
               });

            }
            else
            {
               db.modify( *feed_itr, [&]( feed_object& f )
               {
                  f.reblogs++;
               });
            }

            const auto& old_feed_idx = db.get_index< feed_index >().indices().get< by_old_feed >();
            auto old_feed = old_feed_idx.lower_bound( itr->follower );

            while( old_feed->account == itr->follower && next_id - old_feed->account_feed_id > _plugin->max_feed_size )
            {
               db.remove( *old_feed );
               old_feed = old_feed_idx.lower_bound( itr->follower );
            };
         }

         ++itr;
      }
   }
   FC_CAPTURE_AND_RETHROW( (o) )
}

} } // steemit::follow
