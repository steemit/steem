#pragma once

#include<iostream>

#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>

#include <steem/chain/database.hpp>

namespace steem { namespace chain {

using boost::multi_index_container;
using namespace boost::multi_index;
namespace bip=boost::interprocess;

//typedef bip::basic_string< char, std::char_traits< char >, allocator< char > > shared_string;

namespace types
{
   using dump_collection = std::list< std::string >;
   using p_dump_collection = std::shared_ptr< dump_collection >;
};

class text_generator
{
   public:

      using Items = std::list< std::string >;

   protected:

      uint32_t number_items;

      Items items;

      std::string create_text( uint32_t idx, uint32_t repeat, const std::string& basic_str, const std::string& data );

   public:

      text_generator( uint32_t _number_items ): number_items( _number_items ){}

      virtual void generate() = 0;
      Items& getItems(){ return items; }
};

class account_names_generator: public text_generator
{
   public:

      account_names_generator( uint32_t _number_accounts );
      ~account_names_generator();

      void generate() override;
};

class comments_generator: public text_generator
{
   private:

      uint32_t number_comments;

   public:

      comments_generator( uint32_t _number_comments );
      ~comments_generator();

      void generate() override;
};

class performance_account
{
   public:

      account_name_type account_name;

      static uint32_t cnt;
      const uint32_t id;

      template< typename Allocator >
      performance_account( account_name_type _account_name, Allocator a )
      : account_name( _account_name ), id( cnt++ )
      {
      }
};

using account_interprocess_allocator_type = bip::allocator< performance_account, bip::managed_mapped_file::segment_manager >;
using account_std_allocator_type = std::allocator< performance_account >;

struct by_account;

template< typename ALLOCATOR >
using account_container_template = multi_index_container
<
   performance_account,
   indexed_by
   <
      ordered_non_unique< tag< by_account >, member< performance_account, account_name_type, &performance_account::account_name > >
   >,
   ALLOCATOR
>;

template< typename STRING_TYPE >
class performance_comment
{
   public:

      account_name_type account_name;
      STRING_TYPE comment;

      static uint32_t cnt;
      const uint32_t id;

      template< typename Allocator >
      performance_comment( account_name_type _account_name, STRING_TYPE _comment, Allocator a )
      : account_name( _account_name ), comment( _comment ), id( cnt++ )
      {
      }
};

using comment_interprocess_allocator_type = bip::allocator< performance_comment< shared_string >, bip::managed_mapped_file::segment_manager >;
using comment_std_allocator_type = std::allocator< performance_comment< std::string > >;

struct by_comment_account;
struct by_comment;

template< typename STRING_TYPE, typename ALLOCATOR >
using comment_container_template = multi_index_container
<
   performance_comment< STRING_TYPE >,
   indexed_by
   <
      ordered_non_unique< tag< by_comment_account >, member< performance_comment< STRING_TYPE >, account_name_type, &performance_comment< STRING_TYPE >::account_name > >,
      ordered_non_unique< tag< by_comment >, member< performance_comment< STRING_TYPE >, STRING_TYPE, &performance_comment< STRING_TYPE >::comment > >
   >,
   ALLOCATOR
>;

template< typename STRING_TYPE, typename ACCOUNT_ALLOCATOR, typename COMMENT_ALLOCATOR >
class performance
{
   private:

      const uint64_t file_size;
      const std::string file_name = "performance_data.db";
      const std::string time_file_name = "timelines.txt";

      std::ofstream stream_time;

      std::shared_ptr< bip::managed_mapped_file > seg;

      account_container_template< ACCOUNT_ALLOCATOR >* acc_idx = nullptr;
      comment_container_template< STRING_TYPE, COMMENT_ALLOCATOR >* comm_idx =nullptr;

      void pre_init();
      ACCOUNT_ALLOCATOR getAccountAllocator();
      COMMENT_ALLOCATOR getCommentAllocator();
      STRING_TYPE getString( const std::string& str );

   public:

      performance( uint64_t _file_size );

      ~performance();

      void init( text_generator& _accounts, text_generator& _comments );

      void get_accounts( types::p_dump_collection data = types::p_dump_collection() );
      template< typename ORDERED_TYPE >
      void get_comments( types::p_dump_collection data = types::p_dump_collection() );

      void dump( const types::p_dump_collection& data, uint32_t idx ); 

      void timestamp( std::string description );
};

using performance_interprocess = performance< shared_string, account_interprocess_allocator_type, comment_interprocess_allocator_type >;
using performance_std = performance< std::string, account_std_allocator_type, comment_std_allocator_type >;

} }
