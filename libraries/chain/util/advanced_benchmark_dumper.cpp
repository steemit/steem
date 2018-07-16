
#include <steem/chain/util/advanced_benchmark_dumper.hpp>

#include <appbase/application.hpp>

#include <boost/filesystem.hpp>

#include <chrono>
#include <fstream>

std::map< std::string, float > initialize_filter_map()
{
   std::map< std::string, float > filter;
   filter[ "steem::protocol::account_create_operation" ]                   = 1.0;
   filter[ "steem::protocol::account_create_with_delegation_operation" ]   = 1.0;
   filter[ "steem::protocol::account_update_operation" ]                   = 0.5;
   filter[ "steem::protocol::account_witness_proxy_operation" ]            = 1.0;
   filter[ "steem::protocol::account_witness_vote_operation" ]             = 1.0;
   filter[ "steem::protocol::cancel_transfer_from_savings_operation" ]     = 1.0;
   filter[ "steem::protocol::change_recovery_account_operation" ]          = 1.0;
   filter[ "steem::protocol::claim_reward_balance_operation" ]             = 1.0;
   filter[ "steem::protocol::comment_operation" ]                          = 0.01;
   filter[ "steem::protocol::comment_options_operation" ]                  = 0.2;
   filter[ "steem::protocol::convert_operation" ]                          = 1.0;
   filter[ "steem::protocol::custom_json_operation" ]                      = 0.01;
   filter[ "steem::protocol::custom_operation" ]                           = 1.0;
   filter[ "steem::protocol::decline_voting_rights_operation" ]            = 1.0;
   filter[ "steem::protocol::delegate_vesting_shares_operation" ]          = 0.5;
   filter[ "steem::protocol::delete_comment_operation" ]                   = 1.0;
   filter[ "steem::protocol::escrow_approve_operation" ]                   = 1.0;
   filter[ "steem::protocol::escrow_dispute_operation" ]                   = 1.0;
   filter[ "steem::protocol::escrow_release_operation" ]                   = 1.0;
   filter[ "steem::protocol::escrow_transfer_operation" ]                  = 1.0;
   filter[ "steem::protocol::feed_publish_operation" ]                     = 1.0;
   filter[ "steem::protocol::limit_order_cancel_operation" ]               = 1.0;
   filter[ "steem::protocol::limit_order_create_operation" ]               = 0.5;
   filter[ "steem::protocol::limit_order_create2_operation" ]              = 1.0;
   filter[ "steem::protocol::recover_account_operation" ]                  = 1.0;
   filter[ "steem::protocol::request_account_recovery_operation" ]         = 1.0;
   filter[ "steem::protocol::set_withdraw_vesting_route_operation" ]       = 1.0;
   filter[ "steem::protocol::transfer_from_savings_operation" ]            = 1.0;
   filter[ "steem::protocol::transfer_operation" ]                         = 0.08;
   filter[ "steem::protocol::transfer_to_savings_operation" ]              = 1.0;
   filter[ "steem::protocol::transfer_to_vesting_operation" ]              = 1.0;
   filter[ "steem::protocol::vote_operation" ]                             = 0.005;
   filter[ "steem::protocol::withdraw_vesting_operation" ]                 = 1.0;
   filter[ "steem::protocol::witness_update_operation" ]                   = 1.0;

   return filter;
}

namespace steem { namespace chain { namespace util {

   uint32_t advanced_benchmark_dumper::cnt = 0;
   std::string advanced_benchmark_dumper::virtual_operation_name = "virtual_operation";
   std::string advanced_benchmark_dumper::apply_context_name = "apply_context--->";

   advanced_benchmark_dumper::advanced_benchmark_dumper()
   {
      if( cnt == 0 )
         file_name = "advanced_benchmark.json";
      else
         file_name = std::to_string( cnt ) + "_advanced_benchmark.json";

      ++cnt;
   }

   advanced_benchmark_dumper::~advanced_benchmark_dumper()
   {
      dump();
   }

   void advanced_benchmark_dumper::begin()
   {
      time_begin = fc::time_point::now();
   }

   template< bool APPLY_CONTEXT >
   void advanced_benchmark_dumper::end( const std::string& str )
   {
      end< APPLY_CONTEXT >( str, 0 );
   }

   template< bool APPLY_CONTEXT >
   void advanced_benchmark_dumper::end( const std::string& str, const uint64_t size )
   {
      static const std::map< std::string, float > filter = initialize_filter_map();
      uint64_t time = (uint64_t) ( fc::time_point::now() - time_begin ).count();

      auto itr = filter.find( str );
      if( itr == filter.end() ) return;
      if( ( static_cast<float>( std::rand() ) / RAND_MAX ) > itr->second ) return;

      auto res = info.emplace( APPLY_CONTEXT ? (apply_context_name + str) : str, size, 1, time );

      if( !res.second )
      {
         res.first->inc( time );
      }

      info.inc( time );

      timings[ APPLY_CONTEXT ? (apply_context_name + str) : str ].push_back( std::make_pair( size, time ) );

      ++flush_cnt;
      if( flush_cnt >= flush_max )
      {
         flush_cnt = 0;
         dump();
      }
   }

   template void advanced_benchmark_dumper::end< true >( const std::string& str, const uint64_t size );
   template void advanced_benchmark_dumper::end< false >( const std::string& str, const uint64_t size );

   template void advanced_benchmark_dumper::end< true >( const std::string& str );
   template void advanced_benchmark_dumper::end< false >( const std::string& str );

   template< typename COLLECTION >
   void advanced_benchmark_dumper::dump_impl( const total_info< COLLECTION >& src, const std::string& src_file_name )
   {
      const fc::path path( src_file_name.c_str() );
      try
      {
         fc::json::save_to_file( src, path );
      }
      catch ( const fc::exception& except )
      {
         elog( "error writing benchmark data to file ${filename}: ${error}",
               ( "filename", path )("error", except.to_detail_string() ) );
      }
   }

#ifdef DEFAULT_LOGGER
# undef DEFAULT_LOGGER
#endif
#define DEFAULT_LOGGER "benchmark"

   void advanced_benchmark_dumper::dump( bool log )
   {
      total_info< std::multiset< ritem > > rinfo( info.total_time );

      std::for_each(info.items.begin(), info.items.end(), [&rinfo, log]( const item& obj )
      {
         //rinfo.items.emplace( obj.op_name, obj.time );
         rinfo.emplace( obj.op_name, obj.size, obj.count, obj.time );
         if( log )
         {
            ilog( "${o}, ${s}, ${c}, ${t}", ("o", obj.op_name)("s", obj.size)("c", obj.count)("t", obj.time) );
         }
      });

      if( log )
      {
         boost::filesystem::path bench_dir = appbase::app().data_dir() / "benchmarks";
         if( boost::filesystem::exists( bench_dir ) )
         {
            if( !boost::filesystem::is_directory( bench_dir ) ) return;
         }
         else
         {
            boost::filesystem::create_directory( bench_dir );
         }

         for( auto op_itr = timings.begin(); op_itr != timings.end(); ++op_itr )
         {
            ilog( "${op}", ("op", op_itr->first) );
            std::ofstream bench_file;
            bench_file.open( ( bench_dir / ( "bench_" + op_itr->first ) ).string() );

            for( auto time_itr = op_itr->second.begin(); time_itr != op_itr->second.end(); ++time_itr )
            {
               ilog( "${size}, ${time}", ("size", time_itr->first)("time", time_itr->second) );
               bench_file << time_itr->first << ", " << time_itr->second << std::endl;
            }

            bench_file.flush();
            bench_file.close();
         }
      }

      dump_impl( info, file_name );
      dump_impl( rinfo, "r_" + file_name );
   }

} } } // steem::chain::util
