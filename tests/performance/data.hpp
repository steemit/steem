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
//#include <steem/chain/account_object.hpp>

namespace steem { namespace chain {

using boost::multi_index_container;
using namespace boost::multi_index;
namespace bip=boost::interprocess;

namespace types
{
   using dump_collection = std::list< std::string >;
   using p_dump_collection = std::shared_ptr< dump_collection >;
};

class redirecting_ostream
{
public:
   explicit redirecting_ostream(const char* outFileName)
   {
      logFile.open(outFileName);
      if(!logFile)
         std::cerr << "Cannot open `" << outFileName << "' for write!" << std::endl;
   }

   void close()
   {
      logFile.close();
   }

   void flush()
   {
      std::cout.flush();
      logFile.flush();
   }

   // for regular output of variables and stuff
   template<typename T>
   redirecting_ostream& operator<<(const T& something)
   {
      std::cout << something;
      if(logFile)
         logFile << something;

      return *this;
   }
   /// for manipulators
   typedef std::ostream& (*stream_function)(std::ostream&);

   redirecting_ostream& operator<<(stream_function func)
   {
      func(std::cout);
      func(logFile);
      return *this;
   }

private:
   std::ofstream logFile;
};

class text_generator
{
   public:

      static std::string basic_link;

      typedef std::list< std::string > Items;

   protected:

      uint32_t number_items;

      Items items;

      std::string create_text( uint32_t idx, uint32_t repeat, const std::string& basic_str, const std::string& data );

   public:

      text_generator( uint32_t _number_items ): number_items( _number_items ){}

      virtual void generate() = 0;
      const Items& getItems() const { return items; }
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

      typedef uint64_t id_type;

      template<typename T>
      using bip_allocator = bip::allocator< T, bip::managed_mapped_file::segment_manager >;

      using bip_string = bip::basic_string< char, std::char_traits< char >, bip_allocator< char > >;
      template <typename T>
      using bip_vector = typename bip::vector< T, bip_allocator< T > >;
      
      typedef typename std::conditional< IS_STD, std::string, bip_string >::type actual_string;

      template <typename T>
      using actual_vector = typename std::conditional< IS_STD, std::vector<T>, bip_vector<T> >::type;
      
      template< typename T >
      using actual_allocator = typename std::conditional< IS_STD, std::allocator< T >, bip_allocator< T > >::type;

      class test_account
      {
      public:
         static id_type cnt;

         template< typename Allocator >
         test_account( account_name_type _account_name, Allocator a )
         : id( cnt++ ), name( _account_name ), json_metadata(a)
         {
         }

         id_type           id;
            
         account_name_type name;
         public_key_type   memo_key;
         actual_string     json_metadata;
         account_name_type proxy;

         time_point_sec    last_account_update;

         time_point_sec    created;
         bool              mined = true;
         bool              owner_challenged = false;
         bool              active_challenged = false;
         time_point_sec    last_owner_proved = time_point_sec::min();
         time_point_sec    last_active_proved = time_point_sec::min();
         account_name_type recovery_account;
         account_name_type reset_account = STEEM_NULL_ACCOUNT;
         time_point_sec    last_account_recovery;
         uint32_t          comment_count = 0;
         uint32_t          lifetime_vote_count = 0;
         uint32_t          post_count = 0;

         bool              can_vote = true;
         uint16_t          voting_power = STEEM_100_PERCENT;   ///< current voting power of this account, it falls after every vote
         time_point_sec    last_vote_time; ///< used to increase the voting power of this account the longer it goes without voting.

         asset             balance = asset( 0, STEEM_SYMBOL );  ///< total liquid shares held by this account
         asset             savings_balance = asset( 0, STEEM_SYMBOL );  ///< total liquid shares held by this account

         asset             sbd_balance = asset( 0, SBD_SYMBOL ); /// total sbd balance
         uint128_t         sbd_seconds; ///< total sbd * how long it has been hel
         time_point_sec    sbd_seconds_last_update; ///< the last time the sbd_seconds was updated
         time_point_sec    sbd_last_interest_payment; ///< used to pay interest at most once per month


         asset             savings_sbd_balance = asset( 0, SBD_SYMBOL ); /// total sbd balance
         uint128_t         savings_sbd_seconds; ///< total sbd * how long it has been hel
         time_point_sec    savings_sbd_seconds_last_update; ///< the last time the sbd_seconds was updated
         time_point_sec    savings_sbd_last_interest_payment; ///< used to pay interest at most once per month

         uint8_t           savings_withdraw_requests = 0;

         asset             reward_sbd_balance = asset( 0, SBD_SYMBOL );
         asset             reward_steem_balance = asset( 0, STEEM_SYMBOL );
         asset             reward_vesting_balance = asset( 0, VESTS_SYMBOL );
         asset             reward_vesting_steem = asset( 0, STEEM_SYMBOL );

         share_type        curation_rewards = 0;
         share_type        posting_rewards = 0;

         asset             vesting_shares = asset( 0, VESTS_SYMBOL ); ///< total vesting shares held by this account, controls its voting power
         asset             delegated_vesting_shares = asset( 0, VESTS_SYMBOL );
         asset             received_vesting_shares = asset( 0, VESTS_SYMBOL );

         asset             vesting_withdraw_rate = asset( 0, VESTS_SYMBOL ); ///< at the time this is updated it can be at most vesting_shares/104
         time_point_sec    next_vesting_withdrawal = fc::time_point_sec::maximum(); ///< after every withdrawal this is incremented by 1 week
         share_type        withdrawn = 0; /// Track how many shares have been withdrawn
         share_type        to_withdraw = 0; /// Might be able to look this up with operation history.
         uint16_t          withdraw_routes = 0;

         fc::array<share_type, STEEM_MAX_PROXY_RECURSION_DEPTH> proxied_vsf_votes;// = std::vector<share_type>( STEEM_MAX_PROXY_RECURSION_DEPTH, 0 ); ///< the total VFS votes proxied to this account

         uint16_t          witnesses_voted_for = 0;

         time_point_sec    last_post;
         time_point_sec    last_root_post = fc::time_point_sec::min();
         uint32_t          post_bandwidth = 0;            
      };

      class test_comment
      {
      public:

         static id_type cnt;

         template< typename Allocator >
         test_comment( const account_name_type& _author, const account_name_type& _parent_author,
           const actual_string& _permlink, const actual_string& _parent_permlink,
           const actual_string& _body, Allocator a )
         : id( cnt++ ), category(a), parent_author( _parent_author ), 
          parent_permlink( _parent_permlink, a ), author( _author ), permlink( _permlink, a ),
          body(_body, a), beneficiaries( a )
         {
         }

         id_type           id;
            
         actual_string     category;
         account_name_type parent_author;
         actual_string     parent_permlink;
         account_name_type author;
         actual_string     permlink;
         actual_string     body;

         time_point_sec    last_update;
         time_point_sec    created;
         time_point_sec    active; ///< the last time this post was "touched" by voting or reply
         time_point_sec    last_payout;

         uint16_t          depth = 0; ///< used to track max nested depth
         uint32_t          children = 0; ///< used to track the total number of children, grandchildren, etc...

         /// index on pending_payout for "things happning now... needs moderation"
         /// TRENDING = UNCLAIMED + PENDING
         share_type        net_rshares; // reward is proportional to rshares^2, this is the sum of all votes (positive and negative)
         share_type        abs_rshares; /// this is used to track the total abs(weight) of votes for the purpose of calculating cashout_time
         share_type        vote_rshares; /// Total positive rshares from all votes. Used to calculate delta weights. Needed to handle vote changing and removal.

         share_type        children_abs_rshares; /// this is used to calculate cashout time of a discussion.
         time_point_sec    cashout_time; /// 24 hours from the weighted average of vote time
         time_point_sec    max_cashout_time;
         uint64_t          total_vote_weight = 0; /// the total weight of voting rewards, used to calculate pro-rata share of curation payouts

         uint16_t          reward_weight = 0;

         /** tracks the total payout this comment has received over time, measured in SBD */
         asset             total_payout_value = asset(0, SBD_SYMBOL);
         asset             curator_payout_value = asset(0, SBD_SYMBOL);
         asset             beneficiary_payout_value = asset( 0, SBD_SYMBOL );

         share_type        author_rewards = 0;

         int32_t           net_votes = 0;

         id_type           root_comment;

         asset             max_accepted_payout = asset( 1000000000, SBD_SYMBOL );       /// SBD value of the maximum payout this post will receive
         uint16_t          percent_steem_dollars = STEEM_100_PERCENT; /// the percent of Steem Dollars to key, unkept amounts will be received as Steem Power
         bool              allow_replies = true;      /// allows a post to disable replies.
         bool              allow_votes   = true;      /// allows a post to receive votes;
         bool              allow_curation_rewards = true;

         actual_vector< beneficiary_route_type > beneficiaries;
      };

      class test_vote
      {
      public:

         static id_type cnt;

         template< typename Allocator >
         test_vote( id_type _voter, id_type _comment, Allocator a )
         : id( cnt++ ), voter( _voter), comment( _comment )
         {

         }

         id_type           id;
         
         id_type           voter;
         id_type           comment;
         uint64_t          weight = 0; ///< defines the score this vote receives, used by vote payout calc. 0 if a negative vote or changed votes.
         int64_t           rshares = 0; ///< The number of rshares this vote is responsible for
         int16_t           vote_percent = 0; ///< The percent weight of the vote
         time_point_sec    last_update; ///< The time of the last update of the vote
         int8_t            num_changes = 0;
      };

      using ACCOUNT_ALLOCATOR = actual_allocator<test_account>;
      using COMMENT_ALLOCATOR = actual_allocator<test_comment>;
      using VOTE_ALLOCATOR = actual_allocator<test_vote>;

      class performance
      {
         public:

         private:

            struct by_account;
            struct by_account_id;
            using account_container_template = multi_index_container
            <
               test_account,
               indexed_by
               <
                  ordered_unique< tag< by_account_id >, member< test_account, uint64_t, &test_account::id > >,
                  ordered_unique< tag< by_account >, member< test_account, account_name_type, &test_account::name > >
               >,
               actual_allocator<test_account>
            >;

            struct by_permlink;
            struct by_parent;
            struct by_comment_id;
            using comment_container_template = multi_index_container
            <
               test_comment,
               indexed_by
               <
                  ordered_unique< tag< by_comment_id >, member< test_comment, uint64_t, &test_comment::id > >,
                  ordered_unique< tag< by_permlink >,
                     composite_key< test_comment,
                        member< test_comment, account_name_type, &test_comment::author >,
                        member< test_comment, actual_string, &test_comment::permlink >
                     >
                  >,

                  ordered_unique< tag< by_parent >,
                     composite_key< test_comment,
                        member< test_comment, account_name_type, &test_comment::parent_author >,
                        member< test_comment, actual_string, &test_comment::parent_permlink >
                     >
                  >
               >,
               actual_allocator<test_comment>
            >;

         struct by_comment_voter;
         struct by_voter_comment;
         struct by_vote_id;
         using vote_container_template = multi_index_container<
            test_vote,
            indexed_by<
               ordered_unique< tag< by_vote_id >, member< test_vote, uint64_t, &test_vote::id > >,
               ordered_unique< tag< by_comment_voter >,
                  composite_key< test_vote,
                     member< test_vote, uint64_t, &test_vote::comment>,
                     member< test_vote, uint64_t, &test_vote::voter>
                  >
               >,
               ordered_unique< tag< by_voter_comment >,
                  composite_key< test_vote,
                     member< test_vote, uint64_t, &test_vote::voter>,
                     member< test_vote, uint64_t, &test_vote::comment>
                  >
               >
            >,
            actual_allocator<test_vote>
         >;

         private:

            const uint64_t file_size;
            const std::string file_name = "performance_data.db";
            const std::string time_file_name = IS_STD ? "std_timelines.txt" : "bip_timelines.txt";

            uint64_t start_time_in_miliseconds;
            uint64_t last_time_in_miliseconds;
            uint64_t range_time_in_miliseconds;

            redirecting_ostream stream_time;

            std::shared_ptr< bip::managed_mapped_file > seg;

            account_container_template*   acc_idx = nullptr;
            comment_container_template*   comm_idx = nullptr;
            vote_container_template*      vote_idx = nullptr;

            std::string display_comment( const test_comment& comment );

            void pre_init();
            template <class T>
            actual_allocator<T> buildAllocator(std::true_type /*isStdAlloc*/) const
            {
               return actual_allocator<T>();
            }

            template <class T>
            actual_allocator<T> buildAllocator(std::false_type /*isStdAlloc*/) const
            {
               return actual_allocator<T>(seg->get_segment_manager());
            }

            template <class T>
            actual_allocator<T> buildAllocator() const
            {
               std::integral_constant<bool, IS_STD> selector;
               return buildAllocator<T>(selector);
            }

            ACCOUNT_ALLOCATOR getAccountAllocator() const
            {
               return buildAllocator<test_account>();
            }
            COMMENT_ALLOCATOR getCommentAllocator() const
            {
               return buildAllocator<test_comment>();
            }
            VOTE_ALLOCATOR getVoteAllocator() const
            {
               return buildAllocator<test_vote>();
            }

            actual_string getString( const std::string& str ) const
            {
               auto alloc = buildAllocator<typename actual_string::value_type>();
               return actual_string(str.cbegin(), str.cend(), alloc);
            }

            template< typename ORDERED_TYPE >
            void get_comments_internal( types::p_dump_collection data = types::p_dump_collection() );

            template< typename ORDERED_TYPE >
            void get_votes_internal( types::p_dump_collection data = types::p_dump_collection() );

            template< typename T >
            void delete_idx( T* obj, std::true_type /*isStdAlloc*/) const
            {
               delete obj;
            }

            template< typename T >
            void delete_idx( T* obj, std::false_type /*isStdAlloc*/) const
            {
               if( obj )
               seg->destroy_ptr( obj );
            }
            
            template< typename T >
            void delete_idx( T* obj ) const
            {
               std::integral_constant<bool, IS_STD> selector;
               delete_idx(obj, selector);
            }
           
         public:

            performance( uint64_t _file_size );

            ~performance();

            void init( const text_generator& _accounts, const text_generator& _comments );

            void get_accounts( types::p_dump_collection data = types::p_dump_collection() );

            void get_comments_pl( types::p_dump_collection data = types::p_dump_collection() );
            void get_comments_p( types::p_dump_collection data = types::p_dump_collection() );

            void get_votes_cv( types::p_dump_collection data = types::p_dump_collection() );
            void get_votes_vc( types::p_dump_collection data = types::p_dump_collection() );

            void get_mixed_data(const text_generator::Items& accountNames,
               const text_generator::Items& accountCommentIds,
               types::p_dump_collection data = types::p_dump_collection() );

            void dump( const types::p_dump_collection& data, uint32_t idx ); 

            void timestamp( std::string description, bool total_time = false, bool with_time = true, bool range_time = false );
      };

   private:

      const uint32_t number_accounts;
      const uint32_t number_permlinks;
      const uint64_t file_size;

      performance p;

      const std::string title = IS_STD ? "***std::allocator***" : "***boost::interprocess::allocator***";

   public:

      performance_checker( uint32_t _number_accounts, uint32_t _number_permlinks, uint64_t _file_size );
      ~performance_checker();

      void run();
      void run_stress();
};

} }
