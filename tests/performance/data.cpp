#include "data.hpp"
#include <boost/interprocess/shared_memory_object.hpp>

#include <iostream>
#include <fstream>
#include <chrono>

namespace steem { namespace chain {

uint32_t performance_account::cnt = 0;

template < typename STRING_TYPE >
uint32_t performance_comment< STRING_TYPE >::cnt = 0;

std::string text_generator::create_text( uint32_t idx, uint32_t repeat, const std::string& basic_str, const std::string& data )
{
   std::string ret = basic_str;

   for( uint32_t i = 0; i<= repeat; ++i )
   {
      ret += "-";
      ret += std::to_string( idx );
      ret += "-";
      ret += data;
   }

   return ret;
}

account_names_generator::account_names_generator( uint32_t _number_accounts )
                        : text_generator( _number_accounts )
{

}

account_names_generator::~account_names_generator()
{

}

void account_names_generator::generate()
{
   for( uint32_t i = 0; i<number_items; i++ )
      items.push_back( create_text( i, 0/*repeat*/, "a:", "xyz" ) );
}

comments_generator::comments_generator( uint32_t _number_comments )
                  : text_generator( _number_comments )
{

}

comments_generator::~comments_generator()
{

}

void comments_generator::generate()
{
   for( uint32_t i = 0; i<number_items; i++ )
      items.push_back( create_text( i, i % 20/*repeat*/, "comment:", "This is comment" ) );
}

template< typename STRING_TYPE, typename ACCOUNT_ALLOCATOR, typename COMMENT_ALLOCATOR >
performance< STRING_TYPE, ACCOUNT_ALLOCATOR, COMMENT_ALLOCATOR >::performance( uint64_t _file_size )
            : file_size( _file_size )
{
   last_time_in_miliseconds = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now().time_since_epoch() ).count();
   start_time_in_miliseconds = last_time_in_miliseconds;
   range_time_in_miliseconds = last_time_in_miliseconds;
   stream_time.open( time_file_name );
}

template< typename STRING_TYPE, typename ACCOUNT_ALLOCATOR, typename COMMENT_ALLOCATOR >
performance< STRING_TYPE, ACCOUNT_ALLOCATOR, COMMENT_ALLOCATOR >::~performance()
{
   timestamp( "total time", true/*total_time*/ );
   stream_time.close();
}

template<>
void performance< shared_string, account_interprocess_allocator_type, comment_interprocess_allocator_type >::pre_init()
{
   //bool removed = bip::shared_memory_object::remove( file_name.c_str() );

   seg.reset( new bip::managed_mapped_file( bip::open_or_create, file_name.c_str(), file_size ) );
   timestamp("creating file");

   acc_idx = seg->find_or_construct< account_container_template< account_interprocess_allocator_type > >("performance_accounts")( account_interprocess_allocator_type( seg->get_segment_manager() ) );
   FC_ASSERT( acc_idx );
   timestamp("creating multi-index for accounts");

   comm_idx = seg->find_or_construct< comment_container_template< shared_string, comment_interprocess_allocator_type > >("performance_comments")( comment_interprocess_allocator_type( seg->get_segment_manager() ) );
   FC_ASSERT( comm_idx );
   timestamp("creating multi-index for comments");
}

template<>
void performance< std::string, account_std_allocator_type, comment_std_allocator_type >::pre_init()
{
   acc_idx = new account_container_template< account_std_allocator_type >( account_std_allocator_type() );
   FC_ASSERT( acc_idx );
   timestamp("creating multi-index for accounts");

   comm_idx = new comment_container_template< std::string, comment_std_allocator_type >( comment_std_allocator_type() );
   FC_ASSERT( comm_idx );
   timestamp("creating multi-index for comments");
}

template<>
account_interprocess_allocator_type performance< shared_string, account_interprocess_allocator_type, comment_interprocess_allocator_type >::getAccountAllocator()
{
   return account_interprocess_allocator_type( seg->get_segment_manager() );
}

template<>
account_std_allocator_type performance< std::string, account_std_allocator_type, comment_std_allocator_type >::getAccountAllocator()
{
   return account_std_allocator_type();
}

template<>
comment_interprocess_allocator_type performance< shared_string, account_interprocess_allocator_type, comment_interprocess_allocator_type >::getCommentAllocator()
{
   return comment_interprocess_allocator_type( seg->get_segment_manager() );
}

template<>
comment_std_allocator_type performance< std::string, account_std_allocator_type, comment_std_allocator_type >::getCommentAllocator()
{
   return comment_std_allocator_type();
}

template<>
shared_string performance< shared_string, account_interprocess_allocator_type, comment_interprocess_allocator_type >::getString( const std::string& str )
{
   return shared_string( str.c_str(), seg->get_segment_manager() );
}

template<>
std::string performance< std::string, account_std_allocator_type, comment_std_allocator_type >::getString( const std::string& str )
{
   return str;
}

template< typename STRING_TYPE, typename ACCOUNT_ALLOCATOR, typename COMMENT_ALLOCATOR >
void performance< STRING_TYPE, ACCOUNT_ALLOCATOR, COMMENT_ALLOCATOR >::init( text_generator& _accounts, text_generator& _comments )
{

   pre_init();

   text_generator::Items& accounts = _accounts.getItems();
   text_generator::Items& comments = _comments.getItems();

   timestamp( "****init data in memory ( start )", false/*total_time*/, false/*with_time*/, true/*range_time*/ );

   int32_t idx = 0;
   std::for_each( accounts.begin(), accounts.end(), [&]( const std::string& account )
      {
         acc_idx->insert( performance_account( account, getAccountAllocator() ) );

         ++idx;
         if( ( idx % 1000 ) == 0 )
            timestamp( std::to_string( idx / 1000 ) + " thousands" );

         std::for_each( comments.begin(), comments.end(), [&]( const std::string& comment )
            {
               comm_idx->insert( performance_comment< STRING_TYPE >( account, getString( comment ), getCommentAllocator() ) );
            }
         );
      }
   );
   timestamp( "****init data in memory completed in time", false/*total_time*/, true/*with_time*/, true/*range_time*/ );
}

template< typename STRING_TYPE, typename ACCOUNT_ALLOCATOR, typename COMMENT_ALLOCATOR >
void performance< STRING_TYPE, ACCOUNT_ALLOCATOR, COMMENT_ALLOCATOR >::get_accounts( types::p_dump_collection data )
{
   FC_ASSERT( acc_idx );

   const auto& accounts = acc_idx->get<by_account>();
   std::for_each( accounts.begin(), accounts.end(), [&]( const performance_account& obj )
      {
         if( data )
            data->push_back( obj.account_name );
      }
   );
}

template< typename STRING_TYPE, typename ACCOUNT_ALLOCATOR, typename COMMENT_ALLOCATOR >
template< typename ORDERED_TYPE >
void performance< STRING_TYPE, ACCOUNT_ALLOCATOR, COMMENT_ALLOCATOR >::get_comments( types::p_dump_collection data )
{
   FC_ASSERT( comm_idx );

   const auto& comments = comm_idx->get< ORDERED_TYPE >();
   std::for_each( comments.begin(), comments.end(), [&]( const performance_comment< STRING_TYPE >& obj )
      {
         std::string tmp = obj.account_name + ":";
         tmp += obj.comment.c_str();

         if( data )
            data->push_back( tmp );
      }
   );
}

template< typename STRING_TYPE, typename ACCOUNT_ALLOCATOR, typename COMMENT_ALLOCATOR >
void performance< STRING_TYPE, ACCOUNT_ALLOCATOR, COMMENT_ALLOCATOR >::dump( const types::p_dump_collection& data, uint32_t idx )
{
   std::ofstream stream;

   std::string _name = "out_" + std::to_string( idx );
   _name += ".txt";

   stream.open( _name );

   std::for_each( data->begin(), data->end(), [&]( const std::string& str )
      {
         stream << str << "\n";
      }
   );

   stream.close();
}

template< typename STRING_TYPE, typename ACCOUNT_ALLOCATOR, typename COMMENT_ALLOCATOR >
void performance< STRING_TYPE, ACCOUNT_ALLOCATOR, COMMENT_ALLOCATOR >::timestamp( std::string description, bool total_time, bool with_time, bool range_time )
{
   uint64_t actual = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now().time_since_epoch() ).count();
   uint64_t tmp = total_time?start_time_in_miliseconds:( ( range_time && with_time )?range_time_in_miliseconds:last_time_in_miliseconds);

   if( with_time ) 
   {
      FILE* file = fopen("/proc/self/status", "r");
      int result = -1;
      char line[128];

      while (fgets(line, 128, file) != NULL)
      {
         if (strncmp(line, "VmRSS:", 6) == 0)
         {
               int i = strlen(line);
               const char* p = line;
               while (*p <'0' || *p > '9') p++;
               line[i-3] = '\0';
               result = atoi(p);
               break;
         }
      }
      fclose(file);

      if( range_time )
         stream_time << description << ": " << actual - tmp << " ms\n";
      else
         stream_time << description << ": " << actual - tmp << " ms " << result << " kB\n";
   }
   else
      stream_time << description << "\n";

   last_time_in_miliseconds = actual;
   if( range_time )
      range_time_in_miliseconds = actual;

   stream_time.flush();
}

template performance< shared_string, account_interprocess_allocator_type, comment_interprocess_allocator_type >::performance( uint64_t _file_size );
template performance< shared_string, account_interprocess_allocator_type, comment_interprocess_allocator_type >::~performance();
template void performance< shared_string, account_interprocess_allocator_type, comment_interprocess_allocator_type >::init( text_generator& _accounts, text_generator& _comments );
template void performance< shared_string, account_interprocess_allocator_type, comment_interprocess_allocator_type >::get_accounts( types::p_dump_collection data );
template void performance< shared_string, account_interprocess_allocator_type, comment_interprocess_allocator_type >::dump( const types::p_dump_collection& data, uint32_t idx );
template void performance< shared_string, account_interprocess_allocator_type, comment_interprocess_allocator_type >::timestamp( std::string description, bool total_time, bool with_time, bool range_time );

template void performance< shared_string, account_interprocess_allocator_type, comment_interprocess_allocator_type >::get_comments< by_comment_account >( types::p_dump_collection data );
template void performance< shared_string, account_interprocess_allocator_type, comment_interprocess_allocator_type >::get_comments< by_comment >( types::p_dump_collection data );


template performance< std::string, account_std_allocator_type, comment_std_allocator_type >::performance( uint64_t _file_size );
template performance< std::string, account_std_allocator_type, comment_std_allocator_type >::~performance();
template void performance< std::string, account_std_allocator_type, comment_std_allocator_type >::init( text_generator& _accounts, text_generator& _comments );
template void performance< std::string, account_std_allocator_type, comment_std_allocator_type >::get_accounts( types::p_dump_collection data );
template void performance< std::string, account_std_allocator_type, comment_std_allocator_type >::dump( const types::p_dump_collection& data, uint32_t idx );
template void performance< std::string,account_std_allocator_type, comment_std_allocator_type >::timestamp( std::string description, bool total_time, bool with_time, bool range_time );

template void performance< std::string, account_std_allocator_type, comment_std_allocator_type >::get_comments< by_comment_account >( types::p_dump_collection data );
template void performance< std::string, account_std_allocator_type, comment_std_allocator_type >::get_comments< by_comment >( types::p_dump_collection data );

} }

