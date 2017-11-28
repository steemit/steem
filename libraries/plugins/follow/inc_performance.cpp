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

   public:
   
      performance_impl( database& _db );
      ~performance_impl();

      template< typename MultiContainer, typename Index >
      uint32_t delete_old_objects( const account_name_type& start_account, uint32_t max_size ) const;
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

template< typename MultiContainer, typename Index >
uint32_t performance_impl::delete_old_objects( const account_name_type& start_account, uint32_t max_size ) const
{
   const auto& old_idx = db.get_index< MultiContainer >().indices().get< Index >();

   auto it_l = old_idx.lower_bound( start_account );
   auto it_u = old_idx.upper_bound( start_account );
 
   if( it_l == it_u )
      return 0;

   uint32_t next_id = get_actual_id( *it_l ) + 1;

   auto it = it_u;

   --it;

   std::string desc;

   while( it->account == start_account && next_id - get_actual_id( *it ) > max_size )
   {
      if( it == it_l )
      {
         desc = "remove-";
         desc += get_actual_name( *it );

         //performance::dump( desc.c_str(), std::string( it->account ), get_actual_id( *it ) );
         db.remove( *it );
         break;
      }

      auto old_itr = it;
      --it;

      desc = "remove-";
      desc += get_actual_name( *it );

      //performance::dump( desc.c_str(), std::string( old_itr->account ), get_actual_id( *old_itr ) );
      db.remove( *old_itr );
   }

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
uint32_t performance::delete_old_objects( const account_name_type& start_account, uint32_t max_size ) const
{
   FC_ASSERT( my );
   return my->delete_old_objects< MultiContainer, Index >( start_account, max_size );
}

template uint32_t performance::delete_old_objects< feed_index, by_feed >( const account_name_type& start_account, uint32_t max_size ) const;
template uint32_t performance::delete_old_objects< blog_index, by_blog >( const account_name_type& start_account, uint32_t max_size ) const;

} } } //steem::follow
