#pragma once

#include <fc/time.hpp>
#include <fc/variant.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/exception/exception.hpp>
#include <fc/io/json.hpp>

#include <sys/time.h>

namespace steem { namespace utilities {

/**
 * Time and memory usage measuring tool.
 * Call \see initialize when you're ready to start the measurments.
 * Call \see measure as many times you need.
 * Call \see dump to print the measurements into the file json style.
 * The values of \see measure and \see dump are returned if you need further processing (e.g. pretty print to console).
 */
class benchmark_dumper
{
public:
   class measurement
   {
   public:
      void set(uint32_t bn, int64_t rm, int32_t cs, uint64_t cm, uint64_t pm)
      {
         block_number = bn;
         real_ms = rm;
         cpu_ms = cs;
         current_mem = cm;
         peak_mem = pm;
      }

   public:
      uint32_t block_number = 0;
      int64_t  real_ms = 0;
      int32_t  cpu_ms = 0;
      uint64_t current_mem = 0;
      uint64_t peak_mem = 0;
   };

   void initialize()
   {
      _init_sys_time = _last_sys_time = fc::time_point::now();
      _init_cpu_time = _last_cpu_time = clock();
      _pid = getpid();
   }

   const measurement& measure(uint32_t block_number)
   {
      uint64_t current_virtual = 0;
      uint64_t peak_virtual = 0;
      read_mem(_pid, &current_virtual, &peak_virtual);
   
      fc::time_point current_sys_time = fc::time_point::now();
      clock_t current_cpu_time = clock();
   
      measurement data;
      data.set( block_number,
                (current_sys_time - _last_sys_time).count()/1000, // real_ms
                int((current_cpu_time - _last_cpu_time) * 1000 / CLOCKS_PER_SEC), // cpu_ms
                current_virtual,
                peak_virtual );
      _measurements.push_back( data );
   
      _last_sys_time = current_sys_time;
      _last_cpu_time = current_cpu_time;
      _total_blocks = block_number;
   
      return _measurements.back();
   }

   void dump(const char* file_name, measurement* total_measurement = nullptr)
   {
      const fc::path path(file_name);
      try
      {
         fc::json::save_to_file(_measurements, path);
      }
      catch ( const fc::exception& except )
      {
         elog( "error writing benchmark data to file ${filename}: ${error}",
               ( "filename", path )("error", except.to_detail_string() ) );
      }

      if( total_measurement == nullptr )
         return;

      uint64_t current_virtual = 0;
      uint64_t peak_virtual = 0;
      read_mem(_pid, &current_virtual, &peak_virtual);
   
      total_measurement->set(_total_blocks,
         (_last_sys_time - _init_sys_time).count()/1000,
         int((_last_cpu_time - _init_cpu_time) * 1000 / CLOCKS_PER_SEC),
         current_virtual,
         peak_virtual );
   }

private:
   bool read_mem(pid_t pid, uint64_t* current_virtual, uint64_t* peak_virtual);

private:
   fc::time_point _init_sys_time;
   fc::time_point _last_sys_time;
   clock_t        _init_cpu_time = 0;
   clock_t        _last_cpu_time = 0;
   uint64_t       _total_blocks = 0;
   pid_t          _pid = 0;
   typedef std::vector<measurement> TMeasurements;   
   TMeasurements  _measurements;
};

} } // steem::utilities

FC_REFLECT( steem::utilities::benchmark_dumper::measurement, (block_number)(real_ms)(cpu_ms)(current_mem)(peak_mem) )
