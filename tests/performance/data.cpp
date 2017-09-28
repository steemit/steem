#include "data.hpp"
#include <boost/interprocess/shared_memory_object.hpp>

#include <iostream>
#include <fstream>
#include <chrono>

namespace steem { namespace chain {

std::string text_generator::basic_link = "01234567890ABCDEF";

template< bool IS_STD >
uint64_t performance_checker< IS_STD >::test_account::cnt = 0;
template< bool IS_STD >
uint64_t performance_checker< IS_STD >::test_comment::cnt = 0;
template< bool IS_STD >
uint64_t performance_checker< IS_STD >::test_vote::cnt = 0;

std::string text_generator::create_text( uint32_t idx, uint32_t repeat, const std::string& basic_str, const std::string& data )
{
   std::string ret = basic_str + std::to_string( idx );

   for( uint32_t i = 0; i<= repeat; ++i )
   {
      ret += data;
   }

   return std::move(ret);
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

permlink_generator::permlink_generator( uint32_t _number_permlinks )
                  : text_generator( _number_permlinks )
{

}

permlink_generator::~permlink_generator()
{

}

void permlink_generator::generate()
{
   for( uint32_t i = 0; i<number_items; i++ )
      items.push_back( create_text( i, i % 10/*repeat*/, "", basic_link ) );
}

template< bool IS_STD >
performance_checker< IS_STD >::performance::performance( uint64_t _file_size )
            : file_size( _file_size )
{
   last_time_in_miliseconds = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now().time_since_epoch() ).count();
   start_time_in_miliseconds = last_time_in_miliseconds;
   range_time_in_miliseconds = last_time_in_miliseconds;
   stream_time.open( time_file_name );
}

template< bool IS_STD >
performance_checker< IS_STD >::performance::~performance()
{
   timestamp( "total time", true/*total_time*/ );
   stream_time.close();

   delete_idx( acc_idx );
   delete_idx( comm_idx );
   delete_idx( vote_idx );
}

template<>
void performance_checker< false >::performance::pre_init()
{
   std::remove( file_name.c_str() );

   seg.reset( new bip::managed_mapped_file( bip::open_or_create, file_name.c_str(), file_size ) );
   timestamp("creating file");

   acc_idx = seg->find_or_construct< account_container_template >("performance_accounts")( ACCOUNT_ALLOCATOR( seg->get_segment_manager() ) );
   FC_ASSERT( acc_idx );
   timestamp("creating multi-index for accounts");

   comm_idx = seg->find_or_construct< comment_container_template >("performance_comments")( COMMENT_ALLOCATOR( seg->get_segment_manager() ) );
   FC_ASSERT( comm_idx );
   timestamp("creating multi-index for comments");

   vote_idx = seg->find_or_construct< vote_container_template >("performance_votes")( VOTE_ALLOCATOR( seg->get_segment_manager() ) );
   FC_ASSERT( vote_idx );
   timestamp("creating multi-index for votes");
}

template<>
void performance_checker< true >::performance::pre_init()
{
   acc_idx = new account_container_template( ACCOUNT_ALLOCATOR() );
   FC_ASSERT( acc_idx );
   timestamp("creating multi-index for accounts");

   comm_idx = new comment_container_template( COMMENT_ALLOCATOR() );
   FC_ASSERT( comm_idx );
   timestamp("creating multi-index for comments");

   vote_idx = new vote_container_template( VOTE_ALLOCATOR() );
   FC_ASSERT( vote_idx );
   timestamp("creating multi-index for votes");
}

template< bool IS_STD >
void performance_checker< IS_STD >::performance::init( const text_generator& _accounts, const text_generator& _comments )
{
   pre_init();

   const text_generator::Items& accounts = _accounts.getItems();
   const text_generator::Items& comments = _comments.getItems();

   timestamp( "****init data in memory ( start )", false/*total_time*/, false/*with_time*/, true/*range_time*/ );

   const actual_string commentBody(getString(std::string(100, 'a')));

   int32_t idx = 0;
   for(const auto& account : accounts)
   {
      test_account _p_account( account, getAccountAllocator() );
      acc_idx->emplace( std::move( _p_account ) );

      ++idx;
      if( ( idx % 1000 ) == 0 )
         timestamp( std::to_string( idx / 1000 ) + " thousands" );

      for(const auto& comment : comments)
      {
         test_comment _p_comment( account, account, getString( comment ), getString( comment ),
            commentBody, getCommentAllocator() );
         comm_idx->emplace( std::move( _p_comment ) );

         vote_idx->emplace( _p_account.id, _p_comment.id, getVoteAllocator() );
      }
   }
   timestamp( "****init data in memory completed in time", false/*total_time*/, true/*with_time*/, true/*range_time*/ );
}

template< bool IS_STD >
void performance_checker< IS_STD >::performance::get_accounts( types::p_dump_collection data )
{
   FC_ASSERT( acc_idx );

   const auto& accounts = acc_idx->get<by_account>();
   for( const test_account& obj : accounts)
   {
      if( data )
         data->push_back( obj.name );
   }
}

template< bool IS_STD >
std::string performance_checker< IS_STD >::performance::display_comment( const test_comment& comment )
{
   std::string ret = "author: " + comment.author + " parent_author: " + comment.parent_author + " permlink: ";
   ret += comment.permlink.c_str();
   ret += " parent_permlink: ";
   ret += comment.parent_permlink.c_str();

   return std::move(ret);
}

template< bool IS_STD >
template< typename ORDERED_TYPE >
void performance_checker< IS_STD >::performance::get_comments_internal( types::p_dump_collection data )
{
   FC_ASSERT( comm_idx );

   const auto& comments = comm_idx->get< ORDERED_TYPE >();
   for(const test_comment& obj : comments)
   {
      if( data )
         data->push_back( display_comment( obj ) );
   }
}

template< bool IS_STD >
template< typename ORDERED_TYPE >
void performance_checker< IS_STD >::performance::get_votes_internal( types::p_dump_collection data )
{
   FC_ASSERT( vote_idx );

   const auto& votes = vote_idx->get< ORDERED_TYPE >();
   for(const test_vote& obj : votes)
   {
      if( data )
      {
         std::string tmp = "voter id: " + std::to_string( obj.voter ) + " comment id: " + std::to_string( obj.comment );
         data->push_back( tmp );
      }
   }
}

template< bool IS_STD >
void performance_checker< IS_STD >::performance::get_comments_pl( types::p_dump_collection data )
{
   get_comments_internal< by_permlink >( data );
}

template< bool IS_STD >
void performance_checker< IS_STD >::performance::get_comments_p( types::p_dump_collection data )
{
   get_comments_internal< by_parent >( data );
}

template< bool IS_STD >
void performance_checker< IS_STD >::performance::get_votes_cv( types::p_dump_collection data )
{
   get_votes_internal< by_comment_voter >( data );
}

template< bool IS_STD >
void performance_checker< IS_STD >::performance::get_votes_vc( types::p_dump_collection data )
{
   get_votes_internal< by_voter_comment >( data );
}

template< bool IS_STD >
void performance_checker< IS_STD >::performance::get_mixed_data(const text_generator::Items& accountNames,
   const text_generator::Items& accountCommentIds, types::p_dump_collection data )
{
   FC_ASSERT( acc_idx && comm_idx && vote_idx );

   const auto& accountIdx = acc_idx->get<by_account>();
   const auto& commentByIdIdx = comm_idx->get<by_permlink>();
   const auto& voteByVoterIdex = vote_idx->get<by_comment_voter>();

   for(const auto& accountName : accountNames)
   {
      account_name_type _name(accountName);

      auto accountI = accountIdx.find(_name);
      FC_ASSERT(accountI != accountIdx.end());

      for(const auto& commentId : accountCommentIds)
      {
         actual_string _permlink = getString(commentId);
         auto commentI = commentByIdIdx.find(boost::make_tuple( accountI->name, _permlink ));
         FC_ASSERT(commentI != commentByIdIdx.end());
         auto voteI = voteByVoterIdex.find(boost::make_tuple( commentI->id, accountI->id ) );
         FC_ASSERT(voteI != voteByVoterIdex.end());
         if( data )
            data->push_back( display_comment( *commentI ) );
   
      }
   }
}

template< bool IS_STD >
void performance_checker< IS_STD >::performance::dump( const types::p_dump_collection& data, uint32_t idx )
{
   std::ofstream stream;

   std::string _name = "out_" + std::to_string( idx );
   _name += ".txt";

   stream.open( _name );

   for(const auto& str : *data)
   {
      stream << str << "\n";
   }

   stream.close();
}

template< bool IS_STD >
void performance_checker< IS_STD >::performance::timestamp( std::string description, bool total_time, bool with_time, bool range_time )
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

template< bool IS_STD >
performance_checker< IS_STD >::performance_checker( uint32_t _number_accounts, uint32_t _number_permlinks, uint64_t _file_size )
                              :  number_accounts( _number_accounts ), number_permlinks( _number_permlinks ), file_size( _file_size ),
                                 p( _file_size )
{

}

template< bool IS_STD >
performance_checker< IS_STD >::~performance_checker()
{

}

template< bool IS_STD >
void performance_checker< IS_STD >::run()
{
   account_names_generator names( number_accounts );
   names.generate();

   permlink_generator permlinks( number_permlinks );
   permlinks.generate();

   p.timestamp( "generating test data" );

   p.init( names, permlinks );

   int idx = 0;
   //Get accounts
   types::p_dump_collection accounts = types::p_dump_collection( new std::list< std::string >() );
   p.get_accounts( accounts );
   p.dump( accounts, idx++ );
   FC_ASSERT( accounts->size() == number_accounts );

   //Get all comments
   types::p_dump_collection comments_1 = types::p_dump_collection( new std::list< std::string >() );
   p.get_comments_pl( comments_1 );
   FC_ASSERT( comments_1->size() == number_accounts * number_permlinks );
   p.dump( comments_1, idx++ );

   types::p_dump_collection comments_2 = types::p_dump_collection( new std::list< std::string >() );
   p.get_comments_p( comments_2 );
   FC_ASSERT( comments_2->size() == number_accounts * number_permlinks );
   p.dump( comments_2, idx++ );

   //Get all votes
   types::p_dump_collection votes_1 = types::p_dump_collection( new std::list< std::string >() );
   p.get_votes_cv( votes_1 );
   FC_ASSERT( votes_1->size() == number_accounts * number_permlinks );
   p.dump( votes_1, idx++ );

   types::p_dump_collection votes_2 = types::p_dump_collection( new std::list< std::string >() );
   p.get_votes_vc( votes_2 );
   FC_ASSERT( votes_2->size() == number_accounts * number_permlinks );
   p.dump( votes_2, idx++ );

   //Get mixed data
   types::p_dump_collection mixed = types::p_dump_collection( new std::list< std::string >() );
   p.get_mixed_data(names.getItems(), permlinks.getItems(), mixed );
   size_t _size = mixed->size();
   FC_ASSERT( _size == number_accounts );
   p.dump( mixed, idx++ );

   p.timestamp( "dumping data" );
}

template< bool IS_STD >
void performance_checker< IS_STD >::run_stress()
{
   p.timestamp( title, false/*total_time*/, false/*with_time*/ );

   account_names_generator names( number_accounts );
   names.generate();

   permlink_generator permlinks( number_permlinks );
   permlinks.generate();

   p.timestamp( "generating test data" );

   p.init( names, permlinks );

   p.timestamp( "****review data ( start )", false/*total_time*/, false/*with_time*/, true/*range_time*/ );

   for( uint32_t i = 1; i <= 20; ++i )
   {
      p.get_mixed_data(names.getItems(), permlinks.getItems());

      if( ( i % 2 ) == 0 )
         p.timestamp( std::to_string( i ) );
   }
   p.timestamp( "****review data completed in time", false/*total_time*/, true/*with_time*/, true/*range_time*/ );
}

template class performance_checker< true >;
template class performance_checker< false >;

} }

