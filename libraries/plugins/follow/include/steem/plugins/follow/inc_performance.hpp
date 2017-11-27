#pragma once

#include <steem/protocol/types.hpp>
#include <steem/chain/steem_object_types.hpp>
#include <steem/chain/database.hpp>

namespace steem { namespace plugins{ namespace follow {

using namespace steem::chain;
using steem::chain::database;
 
using steem::protocol::account_name_type;

class performance_impl;

class dumper
{
   private:
      
      std::ofstream f;

      static std::unique_ptr< dumper > self;

      dumper() :
#if ENABLE_STD_ALLOCATOR == 1
      f( "std_dumped_objects.txt" )
#else
      f( "bip_dumped_objects.txt" )
#endif
      {
      }

   public:

      ~dumper()
      {
         f.flush();
         f.close();
      }

      static std::unique_ptr< dumper >& instance()
      {
         if( !self )
            self = std::unique_ptr< dumper >( new dumper() );

         return self;
      }

      template< typename T, typename T2 >
      void dump( const char* message, const T& data, const T2& data2 )
      {
         static uint64_t counter = 0;
         f<<counter++<<" "<<message<<" "<<data<<" "<<data2<<"\n";
         f.flush();
      }
};

struct performance_data
{
   enum class t_creation_type{ full_feed, part_feed, full_blog };

   const account_name_type* account;
   const time_point_sec* time;
   const comment_id_type& comment;

   t_creation_type creation_type;
   bool creation;
   bool is_empty;

   performance_data( const account_name_type& _account, const time_point_sec& _time, const comment_id_type& _comment, t_creation_type _creation_type, bool _is_empty )
   : account( &_account ), time( &_time ), comment( _comment ), creation_type( _creation_type ), creation( true ), is_empty( _is_empty )
   {

   }

   performance_data( const comment_id_type& _comment, t_creation_type _creation_type )
   : account( nullptr ), time( nullptr ), comment( _comment ), creation_type( _creation_type ), creation( true ), is_empty( false )
   {

   }
};

class performance
{

   private:

      std::unique_ptr< performance_impl > my;

   public:
   
      performance( database& _db );
      ~performance();

      template< typename MultiContainer, typename Index >
      uint32_t delete_old_objects( const account_name_type& start_account, uint32_t max_size, performance_data& _pd ) const;

      template< typename T, typename T2 >
      static void dump( const char* message, const T& data, const T2& data2 )
      {
         dumper::instance()->dump( message, data, data2 );
      }
};

} } } //steem::follow
