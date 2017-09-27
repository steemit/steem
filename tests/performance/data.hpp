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
#include <boost/multi_index/composite_key.hpp>

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

class permlink_generator: public text_generator
{
   private:

      uint32_t number_permlinks;

   public:

      permlink_generator( uint32_t _number_permlinks );
      ~permlink_generator();

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

template< typename STRING_TYPE >
class performance_comment//: public object < performance_comment_type, performance_comment >
{
   public:

      account_name_type author;
      account_name_type parent_author;
      //id_type           id;
      STRING_TYPE       permlink;
      STRING_TYPE       parent_permlink;

      template< typename Allocator >
      performance_comment( account_name_type _author, account_name_type _parent_author, /*id_type _id,*/ STRING_TYPE _permlink, STRING_TYPE _parent_permlink, Allocator a )
      : author( _author ), parent_author( _parent_author ), /*id( _id ),*/ permlink( _permlink ), parent_permlink( _parent_permlink )
      {
      }
};

using account_interprocess_allocator_type = bip::allocator< performance_account, bip::managed_mapped_file::segment_manager >;
using account_std_allocator_type = std::allocator< performance_account >;

using comment_interprocess_allocator_type = bip::allocator< performance_comment< shared_string >, bip::managed_mapped_file::segment_manager >;
using comment_std_allocator_type = std::allocator< performance_comment< std::string > >;

template< typename STRING_TYPE, typename ACCOUNT_ALLOCATOR, typename COMMENT_ALLOCATOR >
class performance
{
   private:

      struct by_account;
      using account_container_template = multi_index_container
      <
         performance_account,
         indexed_by
         <
            ordered_non_unique< tag< by_account >, member< performance_account, account_name_type, &performance_account::account_name > >
         >,
         ACCOUNT_ALLOCATOR
      >;

      struct by_permlink;
      struct by_parent;
      using comment_container_template = multi_index_container
      <
         performance_comment< STRING_TYPE >,
         indexed_by
         <
            ordered_unique< tag< by_permlink >,
               composite_key< performance_comment< STRING_TYPE >,
                  member< performance_comment< STRING_TYPE >, account_name_type, &performance_comment< STRING_TYPE >::author >,
                  member< performance_comment< STRING_TYPE >, STRING_TYPE, &performance_comment< STRING_TYPE >::permlink >
               >
               //,composite_key_compare< std::less< account_name_type >, chainbase::strcmp_less >
            >,

            ordered_unique< tag< by_parent >,
               composite_key< performance_comment< STRING_TYPE >,
                  member< performance_comment< STRING_TYPE >, account_name_type, &performance_comment< STRING_TYPE >::parent_author >,
                  member< performance_comment< STRING_TYPE >, STRING_TYPE, &performance_comment< STRING_TYPE >::parent_permlink >
                  //,member< performance_comment< STRING_TYPE >, comment_id_type, &performance_comment< STRING_TYPE >::id >
               >
               //,composite_key_compare< std::less< account_name_type >, chainbase::strcmp_less, std::less< comment_id_type > >
            >
         >,
         COMMENT_ALLOCATOR
      >;

   private:

      const uint64_t file_size;
      const std::string file_name = "performance_data.db";
      const std::string time_file_name = "timelines.txt";

      uint64_t start_time_in_miliseconds;
      uint64_t last_time_in_miliseconds;
      uint64_t range_time_in_miliseconds;

      std::ofstream stream_time;

      std::shared_ptr< bip::managed_mapped_file > seg;

      account_container_template* acc_idx = nullptr;
      comment_container_template* comm_idx =nullptr;

      void pre_init();
      ACCOUNT_ALLOCATOR getAccountAllocator();
      COMMENT_ALLOCATOR getCommentAllocator();
      STRING_TYPE getString( const std::string& str );

      template< typename ORDERED_TYPE >
      void get_comments_internal( types::p_dump_collection data = types::p_dump_collection() );

   public:

      performance( uint64_t _file_size );

      ~performance();

      void init( text_generator& _accounts, text_generator& _comments );

      void get_accounts( types::p_dump_collection data = types::p_dump_collection() );

      void get_comments_pl( types::p_dump_collection data = types::p_dump_collection() );
      void get_comments_p( types::p_dump_collection data = types::p_dump_collection() );

      void dump( const types::p_dump_collection& data, uint32_t idx ); 

      void timestamp( std::string description, bool total_time = false, bool with_time = true, bool range_time = false );
};

template< bool IS_STD >
class performance_checker
{
   private:

      typedef typename std::conditional< IS_STD,
         performance< std::string, account_std_allocator_type, comment_std_allocator_type >,
         performance< shared_string, account_interprocess_allocator_type, comment_interprocess_allocator_type >
      >::type t_performance;

      const uint32_t number_accounts;
      const uint32_t number_permlinks;
      const uint64_t file_size;

      t_performance p;

      const std::string title = IS_STD ? "***std::allocator***" : "***boost::interprocess::allocator***";

   public:

      performance_checker( uint32_t _number_accounts, uint32_t _number_permlinks, uint64_t _file_size );
      ~performance_checker();

      void run();
      void run_stress();
};

} }
