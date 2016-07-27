#include <steemit/follow/follow_operations.hpp>
#include <steemit/follow/follow_objects.hpp>

namespace steemit { namespace follow {

void follow_evaluator::do_apply( const follow_operation& o )
{
   static map< string, follow_type > follow_type_map = []()
   {
      map< string, follow_type > follow_map;
      follow_map[ "blog" ] = follow_type::blog;

      return follow_map;
   }();

   const auto& idx = db().get_index_type<follow_index>().indices().get< by_follower_following >();
   auto itr = idx.find( boost::make_tuple( o.follower, o.following ) );

   set< follow_type > what;

   for( auto where : o.what )
   {
      switch( follow_type_map[ where ] )
      {
         case blog:
            what.insert( blog );
            break;
         default:
            return;
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
   return;
}

} } // steemit::follow