#include <steemit/plugins/follow_api/follow_api_plugin.hpp>
#include <steemit/plugins/follow_api/follow_api.hpp>

#include <steemit/plugins/follow/follow_objects.hpp>

namespace steemit { namespace plugins { namespace follow {

namespace detail {

inline void set_what( vector< follow::follow_type >& what, uint16_t bitmask )
{
   if( bitmask & 1 << follow::blog )
      what.push_back( follow::blog );
   if( bitmask & 1 << follow::ignore )
      what.push_back( follow::ignore );
}

class follow_api_impl
{
   public:
      follow_api_impl() : _db( appbase::app().get_plugin< steemit::plugins::chain::chain_plugin >().db() ) {}

      DECLARE_API(
         (get_followers)
         (get_following)
         (get_follow_count)
         (get_feed_entries)
         (get_feed)
         (get_blog_entries)
         (get_blog)
         (get_account_reputations)
         (get_reblogged_by)
         (get_blog_authors)
      )

      steemit::chain::database& _db;
};

DEFINE_API( follow_api_impl, get_followers )
{
   FC_ASSERT( args.limit <= 1000 );

   get_followers_return result;
   result.followers.reserve( args.limit );

   const auto& idx = _db.get_index< follow::follow_index >().indices().get< follow::by_following_follower >();
   auto itr = idx.lower_bound( std::make_tuple( args.account, args.start ) );
   while( itr != idx.end() && result.followers.size() < args.limit && itr->following == args.account )
   {
      if( args.type == follow::undefined || itr->what & ( 1 << args.type ) )
      {
         api_follow_object entry;
         entry.follower = itr->follower;
         entry.following = itr->following;
         set_what( entry.what, itr->what );
         result.followers.push_back( entry );
      }

      ++itr;
   }

   return result;
}

DEFINE_API( follow_api_impl, get_following )
{
   FC_ASSERT( args.limit <= 1000 );

   get_following_return result;
   result.following.reserve( args.limit );

   const auto& idx = _db.get_index< follow::follow_index >().indices().get< follow::by_follower_following >();
   auto itr = idx.lower_bound( std::make_tuple( args.account, args.start ) );
   while( itr != idx.end() && result.following.size() < args.limit && itr->follower == args.account )
   {
      if( args.type == follow::undefined || itr->what & ( 1 << args.type ) )
      {
         api_follow_object entry;
         entry.follower = itr->follower;
         entry.following = itr->following;
         set_what( entry.what, itr->what );
         result.following.push_back( entry );
      }

      ++itr;
   }

   return result;
}

DEFINE_API( follow_api_impl, get_follow_count )
{
   get_follow_count_return result;
   auto itr = _db.find< follow::follow_count_object, follow::by_account >( args.account );

   if( itr != nullptr )
      result = get_follow_count_return{ itr->account, itr->follower_count, itr->following_count };
   else
      result.account = args.account;

   return result;
}

DEFINE_API( follow_api_impl, get_feed_entries )
{
   FC_ASSERT( args.limit <= 500, "Cannot retrieve more than 500 feed entries at a time." );

   auto entry_id = args.start_entry_id == 0 ? args.start_entry_id : ~0;

   get_feed_entries_return result;
   result.feed.reserve( args.limit );

   const auto& feed_idx = _db.get_index< follow::feed_index >().indices().get< follow::by_feed >();
   auto itr = feed_idx.lower_bound( boost::make_tuple( args.account, entry_id ) );

   while( itr != feed_idx.end() && itr->account == args.account && result.feed.size() < args.limit )
   {
      const auto& comment = _db.get( itr->comment );
      feed_entry entry;
      entry.author = comment.author;
      entry.permlink = steemit::chain::to_string( comment.permlink );
      entry.entry_id = itr->account_feed_id;

      if( itr->first_reblogged_by != account_name_type() )
      {
         entry.reblog_by.reserve( itr->reblogged_by.size() );

         for( const auto& a : itr->reblogged_by )
            entry.reblog_by.push_back(a);

         entry.reblog_on = itr->first_reblogged_on;
      }

      result.feed.push_back( entry );
      ++itr;
   }

   return result;
}

DEFINE_API( follow_api_impl, get_feed )
{
   FC_ASSERT( args.limit <= 500, "Cannot retrieve more than 500 feed entries at a time." );

   auto entry_id = args.start_entry_id == 0 ? args.start_entry_id : ~0;

   get_feed_return result;
   result.feed.reserve( args.limit );

   const auto& feed_idx = _db.get_index< follow::feed_index >().indices().get< follow::by_feed >();
   auto itr = feed_idx.lower_bound( boost::make_tuple( args.account, entry_id ) );

   while( itr != feed_idx.end() && itr->account == args.account && result.feed.size() < args.limit )
   {
      const auto& comment = _db.get( itr->comment );
      comment_feed_entry entry;
      entry.comment = database_api::api_comment_object( comment, _db );
      entry.entry_id = itr->account_feed_id;

      if( itr->first_reblogged_by != account_name_type() )
      {
         entry.reblog_by.reserve( itr->reblogged_by.size() );

         for( const auto& a : itr->reblogged_by )
            entry.reblog_by.push_back( a );

         entry.reblog_on = itr->first_reblogged_on;
      }

      result.feed.push_back( entry );
      ++itr;
   }

   return result;
}

DEFINE_API( follow_api_impl, get_blog_entries )
{
   FC_ASSERT( args.limit <= 500, "Cannot retrieve more than 500 blog entries at a time." );

   auto entry_id = args.start_entry_id == 0 ? args.start_entry_id : ~0;

   get_blog_entries_return result;
   result.blog.reserve( args.limit );

   const auto& blog_idx = _db.get_index< follow::blog_index >().indices().get< follow::by_blog >();
   auto itr = blog_idx.lower_bound( boost::make_tuple( args.account, entry_id ) );

   while( itr != blog_idx.end() && itr->account == args.account && result.blog.size() < args.limit )
   {
      const auto& comment = _db.get( itr->comment );
      blog_entry entry;
      entry.author = comment.author;
      entry.permlink = steemit::chain::to_string( comment.permlink );
      entry.blog = args.account;
      entry.reblog_on = itr->reblogged_on;
      entry.entry_id = itr->blog_feed_id;

      result.blog.push_back( entry );
      ++itr;
   }

   return result;
}

DEFINE_API( follow_api_impl, get_blog )
{
   FC_ASSERT( args.limit <= 500, "Cannot retrieve more than 500 blog entries at a time." );

   auto entry_id = args.start_entry_id == 0 ? args.start_entry_id : ~0;

   get_blog_return result;
   result.blog.reserve( args.limit );

   const auto& blog_idx = _db.get_index< follow::blog_index >().indices().get< follow::by_blog >();
   auto itr = blog_idx.lower_bound( boost::make_tuple( args.account, entry_id ) );

   while( itr != blog_idx.end() && itr->account == args.account && result.blog.size() < args.limit )
   {
      const auto& comment = _db.get( itr->comment );
      comment_blog_entry entry;
      entry.comment = database_api::api_comment_object( comment, _db );
      entry.blog = args.account;
      entry.reblog_on = itr->reblogged_on;
      entry.entry_id = itr->blog_feed_id;

      result.blog.push_back( entry );
      ++itr;
   }

   return result;
}

DEFINE_API( follow_api_impl, get_account_reputations )
{
   FC_ASSERT( args.limit <= 1000, "Cannot retrieve more than 1000 account reputations at a time." );

   const auto& acc_idx = _db.get_index< steemit::chain::account_index >().indices().get< steemit::chain::by_name >();
   const auto& rep_idx = _db.get_index< follow::reputation_index >().indices().get< follow::by_account >();

   auto acc_itr = acc_idx.lower_bound( args.account_lower_bound );

   get_account_reputations_return result;
   result.reputations.reserve( args.limit );

   while( acc_itr != acc_idx.end() && result.reputations.size() < args.limit )
   {
      auto itr = rep_idx.find( acc_itr->name );
      account_reputation rep;

      rep.account = acc_itr->name;
      rep.reputation = itr != rep_idx.end() ? itr->reputation : 0;

      result.reputations.push_back( rep );
      ++acc_itr;
   }

   return result;
}

DEFINE_API( follow_api_impl, get_reblogged_by )
{
   get_reblogged_by_return result;

   const auto& post = _db.get_comment( args.author, args.permlink );
   const auto& blog_idx = _db.get_index< follow::blog_index, follow::by_comment >();

   auto itr = blog_idx.lower_bound( post.id );

   while( itr != blog_idx.end() && itr->comment == post.id && result.accounts.size() < 2000 )
   {
      result.accounts.push_back( itr->account );
      ++itr;
   }

   return result;
}

DEFINE_API( follow_api_impl, get_blog_authors )
{
   get_blog_authors_return result;

   const auto& stats_idx = _db.get_index< follow::blog_author_stats_index, follow::by_blogger_guest_count >();
   auto itr = stats_idx.lower_bound( args.blog_account );

   while( itr != stats_idx.end() && itr->blogger == args.blog_account && result.blog_authors.size() < 2000 )
   {
      result.blog_authors.push_back( reblog_count{ itr->guest, itr->count } );
      ++itr;
   }

   return result;
}

} // detail

follow_api::follow_api(): my( new detail::follow_api_impl() )
{
   JSON_RPC_REGISTER_API(
      STEEM_FOLLOW_API_PLUGIN_NAME,
      (get_followers)
      (get_following)
      (get_follow_count)
      (get_feed_entries)
      (get_feed)
      (get_blog_entries)
      (get_blog)
      (get_account_reputations)
      (get_reblogged_by)
      (get_blog_authors)
   );
}

follow_api::~follow_api() {}

DEFINE_API( follow_api, get_followers )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_followers( args );
   });
}

DEFINE_API( follow_api, get_following )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_following( args );
   });
}

DEFINE_API( follow_api, get_follow_count )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_follow_count( args );
   });
}

DEFINE_API( follow_api, get_feed_entries )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_feed_entries( args );
   });
}

DEFINE_API( follow_api, get_feed )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_feed( args );
   });
}

DEFINE_API( follow_api, get_blog_entries )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_blog_entries( args );
   });
}

DEFINE_API( follow_api, get_blog )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_blog( args );
   });
}

DEFINE_API( follow_api, get_account_reputations )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_account_reputations( args );
   });
}

DEFINE_API( follow_api, get_reblogged_by )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_reblogged_by( args );
   });
}

DEFINE_API( follow_api, get_blog_authors )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_blog_authors( args );
   });
}

} } } // steemit::plugins::follow
