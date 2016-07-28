#include <steemit/chain/account_object.hpp>

#include <steemit/follow/follow_api.hpp>

namespace steemit { namespace follow {

namespace detail
{

class follow_api_impl
{
   public:
      follow_api_impl( steemit::app::application& _app )
         :app(_app) {}

      vector< follow_object > get_followers( string following, string start_follower, uint16_t limit )const;
      vector< follow_object > get_following( string follower, string start_following, uint16_t limit )const;

      vector< feed_entry > get_feed_entries( string account, uint32_t entry_id = 0, uint16_t limit = 500 )const;
      vector< comment_feed_entry > get_feed( string account, uint32_t entry_id = 0, uint16_t limit = 500 )const;

      steemit::app::application& app;
};

vector< follow_object > follow_api_impl::get_followers( string following, string start_follower, uint16_t limit )const
{
   FC_ASSERT( limit <= 100 );
   vector<follow_object> result;
   const auto& idx = app.chain_database()->get_index_type<follow_index>().indices().get<by_following_follower>();
   auto itr = idx.lower_bound( std::make_tuple( following, start_follower ) );
   while( itr != idx.end() && limit && itr->following == following ) {
      result.push_back(*itr);
      ++itr;
      --limit;
   }

   return result;
}

vector< follow_object > follow_api_impl::get_following( string follower, string start_following, uint16_t limit )const
{
   FC_ASSERT( limit <= 100 );
   vector<follow_object> result;
   const auto& idx = app.chain_database()->get_index_type<follow_index>().indices().get<by_follower_following>();
   auto itr = idx.lower_bound( std::make_tuple( follower, start_following ) );
   while( itr != idx.end() && limit && itr->follower == follower ) {
      result.push_back(*itr);
      ++itr;
      --limit;
   }

   return result;
}

vector< feed_entry > follow_api_impl::get_feed_entries( string account, uint32_t entry_id, uint16_t limit )const
{
   FC_ASSERT( limit <= 500, "Cannot retrieve more than 500 feed entries at a time." );

   if( entry_id == 0 )
      entry_id = ~0;

   vector< feed_entry > results;
   results.reserve( limit );

   const auto& acc_id = app.chain_database()->get_account( account ).id;
   const auto& feed_idx = app.chain_database()->get_index_type< feed_index >().indices().get< by_feed >();
   auto itr = feed_idx.lower_bound( boost::make_tuple( acc_id, entry_id ) );

   while( itr != feed_idx.end() && itr->account == acc_id && results.size() < limit )
   {
      const auto& comment = itr->comment( *app.chain_database() );
      feed_entry entry;
      entry.author = comment.author;
      entry.permlink = comment.permlink;
      entry.entry_id = itr->account_feed_id;
      results.push_back( entry );

      ++itr;
   }

   return results;
}

vector< comment_feed_entry > follow_api_impl::get_feed( string account, uint32_t entry_id, uint16_t limit )const
{
   FC_ASSERT( limit <= 500, "Cannot retrieve more than 500 feed entries at a time." );

   if( entry_id == 0 )
      entry_id = ~0;

   vector< comment_feed_entry > results;
   results.reserve( limit );

   const auto& acc_id = app.chain_database()->get_account( account ).id;
   const auto& feed_idx = app.chain_database()->get_index_type< feed_index >().indices().get< by_feed >();
   auto itr = feed_idx.lower_bound( boost::make_tuple( acc_id, entry_id ) );

   while( itr != feed_idx.end() && itr->account == acc_id && results.size() < limit )
   {
      const auto& comment = itr->comment( *app.chain_database() );
      comment_feed_entry entry;
      entry.comment = comment;
      entry.entry_id = itr->account_feed_id;
      results.push_back( entry );

      ++itr;
   }

   return results;
}

} // detail

follow_api::follow_api( const steemit::app::api_context& ctx )
{
   my = std::make_shared< detail::follow_api_impl >( ctx.app );
}

void follow_api::on_api_startup() {}

vector<follow_object> follow_api::get_followers( string following, string start_follower, uint16_t limit )const
{
   return my->get_followers( following, start_follower, limit );
}

vector<follow_object> follow_api::get_following( string follower, string start_following, uint16_t limit )const
{
   return my->get_following( follower, start_following, limit );
}

vector< feed_entry > follow_api::get_feed_entries( string account, uint32_t entry_id, uint16_t limit )const
{
   return my->get_feed_entries( account, entry_id, limit );
}

vector< comment_feed_entry > follow_api::get_feed( string account, uint32_t entry_id, uint16_t limit )const
{
   return my->get_feed( account, entry_id, limit );
}

} } // steemit::follow