#include <steem/plugins/follow/inc_performance.hpp>

#include <steem/chain/database.hpp>
#include <steem/plugins/follow/follow_objects.hpp>
 
namespace steem { namespace plugins{ namespace follow {

class performance_impl
{
   database& db;

   public:
   
      performance_impl( database& _db );
      ~performance_impl();

      void mark_deleted_feed_objects( const account_name_type& follower, uint32_t next_id, uint32_t max_feed_size ) const;
};

performance_impl::performance_impl( database& _db )
               : db( _db )
{

}

performance_impl::~performance_impl()
{

}

void performance_impl::mark_deleted_feed_objects( const account_name_type& follower, uint32_t next_id, uint32_t max_feed_size ) const
{
   // const auto& old_feed_idx = db.get_index< feed_index >().indices().get< by_old_feed >();
   // auto old_feed = old_feed_idx.lower_bound( follower );

   // while( old_feed->account == follower && next_id - old_feed->account_feed_id > max_feed_size )
   // {
   //    db.remove( *old_feed );
   //    old_feed = old_feed_idx.lower_bound( follower );
   // }

   const auto& feed_it = db.get_index< feed_index >().indices().get< by_feed >();

   if( feed_it.size() == 0 )
      return;

   const uint32_t val = next_id - max_feed_size;

   auto r_it = feed_it.upper_bound( follower );

   auto begin_it = feed_it.end();
   auto end_it = feed_it.end();

   if( r_it == end_it )
      --r_it;

   while( ( r_it->account == follower ) && ( val > r_it->account_feed_id ) )
   {
      db.modify( *r_it, [&]( feed_object& f )
      {
         f.activated = 0;
      });

      if( r_it == begin_it )
         break;

      --r_it;
   }
}

performance::performance( database& _db )
         : my( new performance_impl( _db ) )
{

}

performance::~performance()
{

}

void performance::mark_deleted_feed_objects( const account_name_type& follower, uint32_t next_id, uint32_t max_feed_size ) const
{
   FC_ASSERT( my );
   my->mark_deleted_feed_objects( follower, next_id, max_feed_size );
}

} } } //steem::follow
