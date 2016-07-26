#include <steemit/follow/follow_operations.hpp>
#include <steemit/follow/follow_objects.hpp>

namespace steemit { namespace follow {

void follow_evaluator::do_apply( const follow_operation& o )
{
   const auto& idx = db().get_index_type<follow_index>().indices().get< by_follower_following >();
   auto itr = idx.find( boost::make_tuple( o.follower, o.following ) );

   if( itr == idx.end() )
   {
      db().create<follow_object>( [&]( follow_object& obj )
      {
         obj.follower = o.follower;
         obj.following = o.following;
         obj.what = o.what;
      });
   }
   else
   {
      db().modify( *itr, [&]( follow_object& obj )
      {
         obj.what = o.what;
      });
   }
}

void reblog_evaluator::do_apply( const reblog_operation& o )
{
   return;
}

} } // steemit::follow