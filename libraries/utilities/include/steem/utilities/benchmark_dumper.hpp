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
   struct index_memory_details_t
   {
      index_memory_details_t(std::string&& name, size_t size, size_t i_sizeof,
         size_t item_add_allocation, size_t add_container_allocation)
         : index_name(name), index_size(size), item_sizeof(i_sizeof),
           item_additional_allocation(item_add_allocation),
           additional_container_allocation(add_container_allocation)
      {
         total_index_mem_usage = additional_container_allocation;
         total_index_mem_usage += item_additional_allocation;
         total_index_mem_usage += index_size*item_sizeof;
      }

      std::string    index_name;
      size_t         index_size = 0;
      size_t         item_sizeof = 0;
      /// Additional (ie dynamic container) allocations held in all stored items 
      size_t         item_additional_allocation = 0;
      /// Additional memory used for container internal structures (like tree nodes).
      size_t         additional_container_allocation = 0;
      size_t         total_index_mem_usage = 0;
   };

   typedef std::vector<index_memory_details_t> index_memory_details_cntr_t;

   struct database_object_sizeof_t
   {
      database_object_sizeof_t(std::string&& name, size_t size)
         : object_name(name), object_size(size) {}

      std::string    object_name;
      size_t         object_size = 0;
   };

   typedef std::vector<database_object_sizeof_t> database_object_sizeof_cntr_t;

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
      index_memory_details_cntr_t index_memory_details_cntr;
   };

   typedef std::vector<measurement> TMeasurements;

   struct TAllData
   {
      database_object_sizeof_cntr_t database_object_sizeofs;
      TMeasurements                 measurements;
      measurement                   total_measurement;
   };

   typedef std::function<void(index_memory_details_cntr_t&, bool)> get_indexes_memory_details_t;
   typedef std::function<void(database_object_sizeof_cntr_t&)> get_database_objects_sizeofs_t;

   void initialize(get_database_objects_sizeofs_t get_database_objects_sizeofs,
                   const char* file_name)
   {
      _file_name = file_name;
      _init_sys_time = _last_sys_time = fc::time_point::now();
      _init_cpu_time = _last_cpu_time = clock();
      _pid = getpid();
      get_database_objects_sizeofs(_all_data.database_object_sizeofs);
   }

   const measurement& measure(uint32_t block_number, get_indexes_memory_details_t get_indexes_memory_details)
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
      get_indexes_memory_details(data.index_memory_details_cntr, true);
      _all_data.measurements.push_back( data );
   
      _last_sys_time = current_sys_time;
      _last_cpu_time = current_cpu_time;
      _total_blocks = block_number;

      _all_data.total_measurement.set(_total_blocks,
         (_last_sys_time - _init_sys_time).count()/1000,
         int((_last_cpu_time - _init_cpu_time) * 1000 / CLOCKS_PER_SEC),
         current_virtual,
         peak_virtual );

      dump(false, get_indexes_memory_details);
   
      return _all_data.measurements.back();
   }

   const measurement& dump(bool finalMeasure, get_indexes_memory_details_t get_indexes_memory_details)
   {
      if(finalMeasure)
      {
         /// Collect index data including dynamic container sizes, what can be time consuming
         auto& idxData = _all_data.total_measurement.index_memory_details_cntr;

         idxData.clear();

         get_indexes_memory_details(idxData, false);

         std::sort(idxData.begin(), idxData.end(),
            [](const index_memory_details_t& info1, const index_memory_details_t& info2) -> bool
            {
               return info1.total_index_mem_usage > info2.total_index_mem_usage;
            }
         );
      }
      const fc::path path(_file_name);
      try
      {
         fc::json::save_to_file(_all_data, path);
       }
      catch ( const fc::exception& except )
      {
         elog( "error writing benchmark data to file ${filename}: ${error}",
               ( "filename", path )("error", except.to_detail_string() ) );
      }

      return _all_data.total_measurement;
   }

private:
   bool read_mem(pid_t pid, uint64_t* current_virtual, uint64_t* peak_virtual);

private:
   const char*    _file_name = nullptr;
   fc::time_point _init_sys_time;
   fc::time_point _last_sys_time;
   clock_t        _init_cpu_time = 0;
   clock_t        _last_cpu_time = 0;
   uint64_t       _total_blocks = 0;
   pid_t          _pid = 0;
   TAllData       _all_data;
};

} } // steem::utilities

FC_REFLECT( steem::utilities::benchmark_dumper::index_memory_details_t,
            (index_name)(index_size)(item_sizeof)(item_additional_allocation)
            (additional_container_allocation)(total_index_mem_usage)
          )

FC_REFLECT( steem::utilities::benchmark_dumper::database_object_sizeof_t,
            (object_name)(object_size) )

FC_REFLECT( steem::utilities::benchmark_dumper::measurement,
            (block_number)(real_ms)(cpu_ms)(current_mem)(peak_mem)(index_memory_details_cntr) )

FC_REFLECT( steem::utilities::benchmark_dumper::TAllData,
            (database_object_sizeofs)(measurements)(total_measurement) )
