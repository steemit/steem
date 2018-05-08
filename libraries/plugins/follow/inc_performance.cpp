#include <steem/plugins/follow/inc_performance.hpp>

#include <steem/chain/database.hpp>
#include <steem/plugins/follow/follow_objects.hpp>

namespace steem { namespace plugins{ namespace follow {

std::unique_ptr< dumper > dumper::self;

class performance_impl
{
   database& db;

   template< typename Object >
   uint32_t get_actual_id( const Object& it ) const;

   template< typename Object >
   const char* get_actual_name( const Object& it ) const;

   template< performance_data::t_creation_type CreationType, typename Object >
   void modify( const Object& obj, const account_name_type& start_account, uint32_t next_id, performance_data& pd ) const;

   template< typename Iterator >
   void skip_modify( const Iterator& actual, performance_data& pd ) const;

   template< performance_data::t_creation_type CreationType, typename Iterator >
   void remember_last( bool is_delayed, bool& init, Iterator& actual, performance_data& pd ) const;

   public:

      performance_impl( database& _db );
      ~performance_impl();

      template< performance_data::t_creation_type CreationType, typename Index >
      uint32_t delete_old_objects( const Index& old_idx, const account_name_type& start_account, uint32_t max_size, performance_data& pd ) const;
};


performance_impl::performance_impl( database& _db )
               : db( _db )
{
}

performance_impl::~performance_impl()
{

}

template<>
uint32_t performance_impl::get_actual_id( const feed_object& obj ) const
{
   return obj.account_feed_id;
}

template<>
uint32_t performance_impl::get_actual_id( const blog_object& obj ) const
{
   return obj.blog_feed_id;
}

template<>
const char* performance_impl::get_actual_name( const feed_object& obj ) const
{
   return "feed";
}

template<>
const char* performance_impl::get_actual_name( const blog_object& obj ) const
{
   return "blog";
}

template<>
void performance_impl::modify< performance_data::t_creation_type::full_feed >( const feed_object& obj, const account_name_type& start_account, uint32_t next_id, performance_data& pd ) const
{
   pd.s.creation = false;

   db.modify( obj, [&]( feed_object& f )
   {
      f.account = start_account;

      if( f.reblogged_by.size() == 1 )
         f.reblogged_by[0] = *pd.account;
      else
      {
         f.reblogged_by.clear();
         f.reblogged_by.push_back( *pd.account );
      }

      f.first_reblogged_by = *pd.account;
      f.first_reblogged_on = *pd.time;
      f.comment = *pd.comment;
      f.account_feed_id = next_id;
   });
}

template<>
void performance_impl::modify< performance_data::t_creation_type::part_feed >( const feed_object& obj, const account_name_type& start_account, uint32_t next_id, performance_data& pd ) const
{
   pd.s.creation = false;

   db.modify( obj, [&]( feed_object& f )
   {
      f.account = start_account;
      f.reblogged_by.clear();
      f.comment = *pd.comment;
      f.account_feed_id = next_id;
      f.first_reblogged_by = account_name_type();
      f.first_reblogged_on = time_point_sec();
   });

}

template<>
void performance_impl::modify< performance_data::t_creation_type::full_blog >( const blog_object& obj, const account_name_type& start_account, uint32_t next_id, performance_data& pd ) const
{
   pd.s.creation = false;

   db.modify( obj, [&]( blog_object& b )
   {
      b.account = start_account;
      b.comment = *pd.comment;
      b.blog_feed_id = next_id;
      b.reblogged_on = time_point_sec();
   });
}

template< typename Iterator >
void performance_impl::skip_modify( const Iterator& actual, performance_data& pd ) const
{
   uint32_t _id = get_actual_id( *actual );
   if( _id == pd.old_id )
   {
      pd.s.allow_modify = false;
   }
}

template< performance_data::t_creation_type CreationType, typename Iterator >
void performance_impl::remember_last( bool is_delayed, bool& init, Iterator& actual, performance_data& pd ) const
{
   if( is_delayed )
   {
      if( init )
         init = false;
      else
      {
         auto removed = std::prev( actual );
         if( CreationType == performance_data::t_creation_type::full_feed )
            skip_modify( removed, pd );
         db.remove( *removed );
      }
   }
   else
   {
      if( CreationType == performance_data::t_creation_type::full_feed )
         skip_modify( actual, pd );
      db.remove( *actual );
   }
}

template< performance_data::t_creation_type CreationType, typename Index >
uint32_t performance_impl::delete_old_objects( const Index& old_idx, const account_name_type& start_account, uint32_t max_size, performance_data& pd ) const
{
   auto it_l = old_idx.lower_bound( start_account );
   auto it_u = old_idx.upper_bound( start_account );

   if( it_l == it_u )
      return 0;

   uint32_t next_id = get_actual_id( *it_l ) + 1;

   auto r_end = old_idx.rend();
   decltype( r_end ) it( it_u );

   bool is_init = true;

   if( pd.s.allow_delete )
      while( it != r_end && it->account == start_account && next_id - get_actual_id( *it ) > max_size )
      {
         auto old_itr = it;
         ++it;

         remember_last< CreationType >( pd.s.is_empty, is_init, old_itr, pd );
      }

   if( !is_init )
     modify< CreationType >( *std::prev( it ), start_account, next_id, pd );

   return next_id;
}

performance::performance( database& _db )
         : my( new performance_impl( _db ) )
{

}

performance::~performance()
{

}

template< performance_data::t_creation_type CreationType, typename Index >
uint32_t performance::delete_old_objects( const Index& old_idx, const account_name_type& start_account, uint32_t max_size, performance_data& pd ) const
{
   FC_ASSERT( my );
   return my->delete_old_objects< CreationType >( old_idx, start_account, max_size, pd );
}

using t_feed = decltype( ((database*)nullptr)->get_index< feed_index >().indices().get< by_feed >() );
using t_blog = decltype( ((database*)nullptr)->get_index< blog_index >().indices().get< by_blog >() );

template uint32_t performance::delete_old_objects< performance_data::t_creation_type::full_feed >( t_feed& old_idx, const account_name_type& start_account, uint32_t max_size, performance_data& pd ) const;
template uint32_t performance::delete_old_objects< performance_data::t_creation_type::part_feed >( t_feed& old_idx, const account_name_type& start_account, uint32_t max_size, performance_data& pd ) const;
template uint32_t performance::delete_old_objects< performance_data::t_creation_type::full_blog >( t_blog& old_idx, const account_name_type& start_account, uint32_t max_size, performance_data& pd ) const;

} } } //steem::follow
