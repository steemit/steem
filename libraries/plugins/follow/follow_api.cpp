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

      steemit::app::application& app;
};

vector<follow_object> follow_api_impl::get_followers( string following, string start_follower, uint16_t limit )const
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

vector<follow_object> follow_api_impl::get_following( string follower, string start_following, uint16_t limit )const
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

} // detail

follow_api::follow_api( const steemit::app::api_context& ctx )
{
   my = std::make_shared< detail::follow_api_impl >( ctx.app );
}

vector<follow_object> follow_api::get_followers( string following, string start_follower, uint16_t limit )const
{
   return my->get_followers( following, start_follower, limit );
}

vector<follow_object> follow_api::get_following( string follower, string start_following, uint16_t limit )const
{
   return my->get_following( follower, start_following, limit );
}

} } // steemit::follow