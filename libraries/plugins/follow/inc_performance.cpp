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

   template< typename Object >
   void modify( const Object& obj, const account_name_type& start_account, uint32_t next_id, performance_data& pd ) const;

   template< typename Iterator >
   void skip_modify( Iterator& actual, performance_data& pd ) const;

   template< typename Iterator >
   void smart( bool is_delayed, bool& init, Iterator& delayed, Iterator& actual, performance_data& pd ) const;

   public:
   
      performance_impl( database& _db );
      ~performance_impl();

      template< typename MultiContainer, typename Index >
      uint32_t delete_old_objects( const account_name_type& start_account, uint32_t max_size, performance_data& pd ) const;
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
void performance_impl::modify( const feed_object& obj, const account_name_type& start_account, uint32_t next_id, performance_data& pd ) const
{
   //std::string desc = "MODIFY!-";
   //desc += get_actual_name( obj );

   if( pd.s.creation_type == performance_data::t_creation_type::full_feed )
   {
      //performance::dump( desc.c_str(), std::string( start_account ), obj.account_feed_id );
      //performance::dump( "NEW!-", std::string( start_account ), next_id );
      pd.s.creation = false;

      db.modify( obj, [&]( feed_object& f )
      {
         f.account = start_account;
         f.reblogged_by.clear();
         f.reblogged_by.push_back( *pd.account );
         f.first_reblogged_by = *pd.account;
         f.first_reblogged_on = *pd.time;
         f.comment = *pd.comment;
         f.account_feed_id = next_id;
      });
   }
   else if( pd.s.creation_type == performance_data::t_creation_type::part_feed )
   {
      //performance::dump( desc.c_str(), std::string( start_account ), obj.account_feed_id );
      //performance::dump( "NEW!-", std::string( start_account ), next_id );
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
}

template<>
void performance_impl::modify( const blog_object& obj, const account_name_type& start_account, uint32_t next_id, performance_data& pd ) const
{
   //std::string desc = "MODIFY!-";
   //desc += get_actual_name( obj );

   if( pd.s.creation_type == performance_data::t_creation_type::full_blog )
   {
      //performance::dump( desc.c_str(), std::string( start_account ), obj.blog_feed_id );
      //performance::dump( "NEW!-", std::string( start_account ), next_id );

      pd.s.creation = false;

      db.modify( obj, [&]( blog_object& b )
      {
         b.account = start_account;
         b.comment = *pd.comment;
         b.blog_feed_id = next_id;
         b.reblogged_on = time_point_sec();
      });
   };
}

template< typename Iterator >
void performance_impl::skip_modify( Iterator& actual, performance_data& pd ) const
{
   if( pd.s.creation_type == performance_data::t_creation_type::full_feed )
   {
      uint32_t _id = get_actual_id( *actual );
      if( _id == pd.old_id )
      {
         std::string desc = "SKIP-MODIFY!-";
         desc += get_actual_name( *actual );
         //performance::dump( desc.c_str(), std::string( actual->account ), _id );
         pd.s.allow_modify = false;
      }
   }
}

template< typename Iterator >
void performance_impl::smart( bool is_delayed, bool& init, Iterator& delayed, Iterator& actual, performance_data& pd ) const
{
   //std::string desc = "remove-";

   if( is_delayed )
   {
      if( init )
         init = false;
      else
      {
         //desc += get_actual_name( *delayed );

         //performance::dump( desc.c_str(), std::string( delayed->account ), get_actual_id( *delayed ) );
         skip_modify( delayed, pd );
         db.remove( *delayed );
      }
      delayed = actual;
   }
   else
   {
      //desc += get_actual_name( *actual );

      //performance::dump( desc.c_str(), std::string( actual->account ), get_actual_id( *actual ) );
      skip_modify( actual, pd );
      db.remove( *actual );
   }
}

template< typename MultiContainer, typename Index >
uint32_t performance_impl::delete_old_objects( const account_name_type& start_account, uint32_t max_size, performance_data& pd ) const
{
   const auto& old_idx = db.get_index< MultiContainer >().indices().get< Index >();

   auto it_l = old_idx.lower_bound( start_account );
   auto it_u = old_idx.upper_bound( start_account );
 
   if( it_l == it_u )
      return 0;

   uint32_t next_id = get_actual_id( *it_l ) + 1;

   auto it = it_u;

   --it;
   auto delayed_it = it;
   bool is_init = true;

   while( it->account == start_account && next_id - get_actual_id( *it ) > max_size )
   {
      if( it == it_l )
      {
         smart( pd.s.is_empty, is_init, delayed_it, it, pd );
         break;
      }

      auto old_itr = it;
      --it;

      smart( pd.s.is_empty, is_init, delayed_it, old_itr, pd );
   }

   if( !is_init )
     modify( *delayed_it, start_account, next_id, pd );

   return next_id;
}

performance::performance( database& _db )
         : my( new performance_impl( _db ) )
{

}

performance::~performance()
{

}

template< typename MultiContainer, typename Index >
uint32_t performance::delete_old_objects( const account_name_type& start_account, uint32_t max_size, performance_data& pd ) const
{
   FC_ASSERT( my );
   return my->delete_old_objects< MultiContainer, Index >( start_account, max_size, pd );
}

template uint32_t performance::delete_old_objects< feed_index, by_feed >( const account_name_type& start_account, uint32_t max_size, performance_data& pd ) const;
template uint32_t performance::delete_old_objects< blog_index, by_blog >( const account_name_type& start_account, uint32_t max_size, performance_data& pd ) const;

} } } //steem::follow
