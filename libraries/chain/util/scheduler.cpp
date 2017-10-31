
#include <steem/chain/util/scheduler.hpp>

#include <steem/chain/database.hpp>

namespace steem { namespace chain { namespace util {

class timed_event_scheduler_impl
{
   public:

      using timed_event_objects = std::list< timed_event_object >;
      using p_timed_event_objects = std::shared_ptr< timed_event_objects >;
      using pair = std::pair< time_point_sec, p_timed_event_objects >;
      using scheduler_allocator = chainbase::allocator< pair >;
      using items = fc::flat_map< time_point_sec, p_timed_event_objects, std::less< time_point_sec >, scheduler_allocator >;
      using pitems = std::unique_ptr< items >;

   private:

      void add( pitems& _events, const time_point_sec& key, const timed_event_object& value );

      void run( scheduler_event_visitor& visitor, p_timed_event_objects& values );

   public:

      pitems buffered_events;
      pitems events;

      database& db;

      timed_event_scheduler_impl( database& _db );
      ~timed_event_scheduler_impl();

      void close();
      void add( const time_point_sec& key, const timed_event_object& value, bool buffered );
      void run( const time_point_sec& head_block_time );

      size_t size();
      size_t size( const time_point_sec& head_block_time );
};

timed_event_scheduler_impl::timed_event_scheduler_impl( database& _db )
                        : db( _db )
{
}

timed_event_scheduler_impl::~timed_event_scheduler_impl()
{
   close();
}

void timed_event_scheduler_impl::close()
{
   if( events )
      events->clear();

   if( buffered_events )
      buffered_events->clear();
}

void timed_event_scheduler_impl::add( pitems& _events, const time_point_sec& key, const timed_event_object& value )
{
   if( !_events )
      _events = pitems( new items( scheduler_allocator( db.get_segment_manager() ) ) );

   const auto& found = _events->find( key );
   if( found!=_events->end() )
   {
      found->second->push_back( value );
   }
   else
   {
      p_timed_event_objects values( new timed_event_objects() );
      values->push_back( value );

      _events->emplace( std::make_pair( key, values ) );
   }
}

void timed_event_scheduler_impl::add( const time_point_sec& key, const timed_event_object& value, bool buffered )
{
   add( buffered ? buffered_events : events, key, value );
}

void timed_event_scheduler_impl::run( scheduler_event_visitor& visitor, p_timed_event_objects& values )
{
   auto it = values->begin();
   const auto& end = values->end();

   while( it != end )
   {
      (*it).visit( visitor );
      if( visitor.removing_allowed_status )
         it = values->erase( it );
      else
         ++it;
   }
}

void timed_event_scheduler_impl::run( const time_point_sec& head_block_time )
{
   if( !events || events->empty() )
      return;

   auto it = events->begin();

#ifdef IS_TEST_NET
   size_t dbg_size = events->size();
#endif

   scheduler_event_visitor visitor( db );

   while( ( it != events->upper_bound( head_block_time ) ) && ( it != events->end() ) )
   {
#ifdef IS_TEST_NET
      const time_point_sec& dbg_time = it->first;
#endif

      run( visitor, it->second );

      if( it->second->empty() )
         it = events->erase( it );
      else
         ++it;
   }

   //Copy newly created events.
   if( buffered_events )
      for( auto& item : *buffered_events )
      {
         std::pair< items::iterator, bool > ret = events->emplace( item );
         if( !ret.second )
         {
            auto& buffered_events = item.second;
            auto& actual_events = ret.first->second;

            FC_ASSERT( actual_events );

#ifdef IS_TEST_NET
            dbg_size = actual_events->size();
#endif

            FC_ASSERT( buffered_events );
            std::copy( buffered_events->begin(), buffered_events->end(), std::back_inserter( *actual_events ) );
         }
      }

#ifdef IS_TEST_NET
   if( events )
      dbg_size = events->size();
#endif

   if( buffered_events )
      buffered_events->clear();
}

#ifdef IS_TEST_NET
size_t timed_event_scheduler_impl::size()
{
   if( !events || events->empty() )
      return 0;

   size_t ret = 0;

   for( auto actual : *events )
      ret += actual.second->size();

   return ret;
}

size_t timed_event_scheduler_impl::size( const time_point_sec& head_block_time )
{
   if( !events || events->empty() )
      return 0;

   size_t ret;

   const auto& found = events->find( head_block_time );
   if( found != events->end() )
      ret = found->second->size();
   else
      ret = 0;

   return ret;
}
#endif

timed_event_scheduler::timed_event_scheduler( database& _db )
                  : impl( new timed_event_scheduler_impl( _db ) )
{

}

timed_event_scheduler::~timed_event_scheduler()
{

}

void timed_event_scheduler::close()
{
   FC_ASSERT( impl );
   impl->close();
}

void timed_event_scheduler::add( const time_point_sec& key, const timed_event_object& value, bool buffered )
{
   FC_ASSERT( impl );
   impl->add( key, value, buffered );
}

void timed_event_scheduler::run( const time_point_sec& head_block_time )
{
   FC_ASSERT( impl );
   impl->run( head_block_time );
}

#ifdef IS_TEST_NET
size_t timed_event_scheduler::size()
{
   FC_ASSERT( impl );
   return impl->size();
}

size_t timed_event_scheduler::size( const time_point_sec& head_block_time )
{
   FC_ASSERT( impl );
   return impl->size( head_block_time );
}
#endif

} } } // steem::chain::util
