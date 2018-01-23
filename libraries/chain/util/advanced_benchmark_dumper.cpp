
#include <steem/chain/util/advanced_benchmark_dumper.hpp>
#include <chrono>

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
      time_begin = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now().time_since_epoch() ).count();
   }

   template< bool APPLY_CONTEXT >
   void advanced_benchmark_dumper::end( const std::string& str )
   {
      uint64_t time = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now().time_since_epoch() ).count() - time_begin;
      std::pair< std::set< item >::iterator, bool > res = items.emplace( item( APPLY_CONTEXT ? ( apply_context_name + str ) : str, time ) );
      if( !res.second )
         res.first->change_time( time );

      ++flush_cnt;
      if( flush_cnt >= flush_max )
      {
         flush_cnt = 0;
         dump();
      }
   }

   template void advanced_benchmark_dumper::end< true >( const std::string& str );
   template void advanced_benchmark_dumper::end< false >( const std::string& str );

   void advanced_benchmark_dumper::dump()
   {
      const fc::path path( file_name.c_str() );
      try
      {
         fc::json::save_to_file( items, path );
      }
      catch ( const fc::exception& except )
      {
         elog( "error writing benchmark data to file ${filename}: ${error}",
               ( "filename", path )("error", except.to_detail_string() ) );
      }
   }

} } } // steem::chain::util
