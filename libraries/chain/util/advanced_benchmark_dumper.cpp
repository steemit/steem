
#include <steem/chain/util/advanced_benchmark_dumper.hpp>

#include <appbase/application.hpp>

#include <chrono>
#include <fstream>

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
      uint64_t time = (uint64_t) ( fc::time_point::now() - time_begin ).count();

      if( str == "condenser_api<-block"
       || str == "debug_node<-block" )
      {
         return;
      }

      if( str == "steem::protocol::comment_operation"
       || str == "steem::protocol::vote_operation"
       || str == "steem::protocol::custom_json_operation"
       || str == "steem::protocol::claim_reward_balance_operation" )
      {
         if( ( static_cast<float>(std::rand()) / RAND_MAX ) < 0.75 ) return;
      }

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


         for( auto op_itr = timings.begin(); op_itr != timings.end(); ++op_itr )
         {
            ilog( "${op}", ("op", op_itr->first) );
            std::ofstream bench_file;
            bench_file.open( ( appbase::app().data_dir() / ( "bench_" + op_itr->first ) ).string() );

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
