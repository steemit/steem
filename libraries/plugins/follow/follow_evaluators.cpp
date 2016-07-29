#include <steemit/follow/follow_operations.hpp>
#include <steemit/follow/follow_objects.hpp>

#include <steemit/chain/account_object.hpp>
#include <steemit/chain/comment_object.hpp>

namespace steemit { namespace follow {

void follow_evaluator::do_apply( const follow_operation& o )
{
   static map< string, follow_type > follow_type_map = []()
   {
      map< string, follow_type > follow_map;
      follow_map[ "blog" ] = follow_type::blog;
      follow_map[ "mute" ] = follow_type::mute;

      return follow_map;
   }();

   const auto& idx = db().get_index_type<follow_index>().indices().get< by_follower_following >();
   auto itr = idx.find( boost::make_tuple( o.follower, o.following ) );

   set< follow_type > what;

   for( auto target : o.what )
   {
      switch( follow_type_map[ target ] )
      {
         case blog:
            what.insert( blog );
            break;
         case mute:
            what.insert( mute );
            break;
         default:
            ilog( "Encountered unknown option ${o}", ("o", target) );
            break;
      }
   }

   if( itr == idx.end() )
   {
      db().create<follow_object>( [&]( follow_object& obj )
      {
         obj.follower = o.follower;
         obj.following = o.following;
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

void reblog_evaluator::do_apply( const reblog_operation& o )
{
   try
      {
         auto& db = _plugin->database();
         const auto& c = db.get_comment( o.author, o.permlink );
         if( c.parent_author.size() > 0 ) return;

         const auto& idx = db.get_index_type< follow_index >().indices().get< by_following_follower >();
         auto itr = idx.find( c.author );

         const auto& feed_idx = db.get_index_type< feed_index >().indices().get< by_feed >();
         const auto& comment_idx = db.get_index_type< feed_index >().indices().get< by_comment >();

         while( itr != idx.end() && itr->following == c.author )
         {
            auto account_id = db.get_account( itr->follower ).id;

            if( itr->what.find( follow_type::blog ) != itr->what.end()
               && comment_idx.find( boost::make_tuple( account_id, c.id ) ) == comment_idx.end() )
            {
               uint32_t next_id = 0;
               auto last_feed = feed_idx.lower_bound( account_id );

               if( last_feed != feed_idx.end() && last_feed->account == account_id )
               {
                  next_id = last_feed->account_feed_id + 1;
               }

               db.create< feed_object >( [&]( feed_object& f )
               {
                  f.account = account_id;
                  f.comment = c.id;
                  f.account_feed_id = next_id;
               });

               const auto& old_feed_idx = db.get_index_type< feed_index >().indices().get< by_old_feed >();
               auto old_feed = old_feed_idx.lower_bound( account_id );

               while( old_feed->account == account_id && next_id - old_feed->account_feed_id > _plugin->max_feed_size )
               {
                  db.remove( *old_feed );
                  old_feed = old_feed_idx.lower_bound( account_id );
               };
            }

            ++itr;
         }
      }
      FC_LOG_AND_RETHROW()
}

} } // steemit::follow