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

namespace types
{
   using dump_collection = std::list< std::string >;
   using p_dump_collection = std::shared_ptr< dump_collection >;
};

class text_generator
{
   public:

      static std::string basic_link;

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

template< bool IS_STD >
class performance_checker
{
   public:

      template<typename T>
      using bip_allocator = bip::allocator< T, bip::managed_mapped_file::segment_manager >;

      using _basic_string = bip::basic_string< char, std::char_traits< char >, bip_allocator< char > >;

      typedef typename std::conditional< IS_STD, std::string, _basic_string >::type actual_string_type;

      template< typename T >
      using actual_allocator = typename std::conditional< IS_STD, std::allocator< T >, bip_allocator< T > >::type;

      class performance_account
      {
         public:

            static uint64_t cnt;
            uint64_t id;

            account_name_type account_name;

            template< typename Allocator >
            performance_account( account_name_type _account_name, Allocator a )
            : id( cnt++ ), account_name( _account_name )
            {
            }
      };

      class performance_comment
      {
         public:

            static uint64_t cnt;
            uint64_t id;

            account_name_type author;
            account_name_type parent_author;
            actual_string_type permlink;
            actual_string_type parent_permlink;

            template< typename Allocator >
            performance_comment( account_name_type _author, account_name_type _parent_author, actual_string_type _permlink, actual_string_type _parent_permlink, Allocator a )
            : id( cnt++ ), author( _author ), parent_author( _parent_author ), permlink( _permlink, a ), parent_permlink( _parent_permlink, a )
            {
            }
      };

      class performance_vote
      {
         public:

            static uint64_t cnt;
            uint64_t id;

            uint64_t voter;
            uint64_t comment;

            template< typename Allocator >
            performance_vote( uint64_t _voter, uint64_t _comment, Allocator a )
            : id( cnt++ ), voter( _voter ), comment( _comment )
            {

            }
      };

      template< typename STRING_TYPE, template <typename> class ALLOCATOR >
      class performance
      {
         public:

            using _STRING_TYPE = STRING_TYPE;

            using ACCOUNT_ALLOCATOR = ALLOCATOR<performance_account>;
            using COMMENT_ALLOCATOR = ALLOCATOR<performance_comment>;
            using VOTE_ALLOCATOR = ALLOCATOR<performance_vote>;

         private:

            struct by_account;
            struct by_account_id;
            using account_container_template = multi_index_container
            <
               performance_account,
               indexed_by
               <
                  ordered_unique< tag< by_account_id >, member< performance_account, uint64_t, &performance_account::id > >,
                  ordered_unique< tag< by_account >, member< performance_account, account_name_type, &performance_account::account_name > >
               >,
               ACCOUNT_ALLOCATOR
            >;

            struct by_permlink;
            struct by_parent;
            struct by_comment_id;
            using comment_container_template = multi_index_container
            <
               performance_comment,
               indexed_by
               <
                  ordered_unique< tag< by_comment_id >, member< performance_comment, uint64_t, &performance_comment::id > >,
                  ordered_unique< tag< by_permlink >,
                     composite_key< performance_comment,
                        member< performance_comment, account_name_type, &performance_comment::author >,
                        member< performance_comment, _STRING_TYPE, &performance_comment::permlink >
                     >
                  >,

                  ordered_unique< tag< by_parent >,
                     composite_key< performance_comment,
                        member< performance_comment, account_name_type, &performance_comment::parent_author >,
                        member< performance_comment, _STRING_TYPE, &performance_comment::parent_permlink >
                     >
                  >
               >,
               COMMENT_ALLOCATOR
            >;

         struct by_comment_voter;
         struct by_voter_comment;
         struct by_vote_id;
         using vote_container_template = multi_index_container<
            performance_vote,
            indexed_by<
               ordered_unique< tag< by_vote_id >, member< performance_vote, uint64_t, &performance_vote::id > >,
               ordered_unique< tag< by_comment_voter >,
                  composite_key< performance_vote,
                     member< performance_vote, uint64_t, &performance_vote::comment>,
                     member< performance_vote, uint64_t, &performance_vote::voter>
                  >
               >,
               ordered_unique< tag< by_voter_comment >,
                  composite_key< performance_vote,
                     member< performance_vote, uint64_t, &performance_vote::voter>,
                     member< performance_vote, uint64_t, &performance_vote::comment>
                  >
               >
            >,
            VOTE_ALLOCATOR
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

            account_container_template*   acc_idx = nullptr;
            comment_container_template*   comm_idx = nullptr;
            vote_container_template*      vote_idx = nullptr;

            std::string display_comment( const performance_comment& comment );

            void pre_init();
            ACCOUNT_ALLOCATOR getAccountAllocator();
            COMMENT_ALLOCATOR getCommentAllocator();
            VOTE_ALLOCATOR getVoteAllocator();
            _STRING_TYPE getString( const std::string& str );

            template< typename ORDERED_TYPE >
            void get_comments_internal( types::p_dump_collection data = types::p_dump_collection() );

            template< typename ORDERED_TYPE >
            void get_votes_internal( types::p_dump_collection data = types::p_dump_collection() );

            template< typename T >
            void delete_idx( T*& obj );
           
         public:

            performance( uint64_t _file_size );

            ~performance();

            void init( text_generator& _accounts, text_generator& _comments );

            void get_accounts( types::p_dump_collection data = types::p_dump_collection() );

            void get_comments_pl( types::p_dump_collection data = types::p_dump_collection() );
            void get_comments_p( types::p_dump_collection data = types::p_dump_collection() );

            void get_votes_cv( types::p_dump_collection data = types::p_dump_collection() );
            void get_votes_vc( types::p_dump_collection data = types::p_dump_collection() );

            void get_mixed_data( types::p_dump_collection data = types::p_dump_collection() );

            void dump( const types::p_dump_collection& data, uint32_t idx ); 

            void timestamp( std::string description, bool total_time = false, bool with_time = true, bool range_time = false );
      };

   private:

      typedef performance< actual_string_type, actual_allocator > t_performance;

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
