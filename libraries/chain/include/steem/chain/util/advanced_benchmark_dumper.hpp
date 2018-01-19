#pragma once

#include <fc/time.hpp>
#include <fc/variant.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/exception/exception.hpp>
#include <fc/io/json.hpp>

#include <sys/time.h>

namespace steem { namespace chain { namespace util {

class advanced_benchmark_dumper
{                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           
   public:

      class item
      {
         public:

            std::string op_name;
            mutable uint64_t time;

            item( std::string _op_name, uint64_t _time )
            : op_name( _op_name ), time( _time )
            {

            }

            bool operator<( const item& obj ) const
            {
               return op_name < obj.op_name;
            }

            void change_time( uint64_t _time ) const
            {
               time += _time;
            }
      };

   private:

      static uint32_t cnt;

      bool enabled = false;

      uint32_t flush_cnt = 0;
      uint32_t flush_max = 50000;

      uint64_t time_begin = 0;

      std::string file_name;

      std::set< item > items;

   public:

      advanced_benchmark_dumper();
      ~advanced_benchmark_dumper();

      void set_enabled( bool val ) { enabled = val; }
      bool is_enabled() { return enabled; }

      void begin();
      void end( const std::string& str );
      void dump();
};

} } } // steem::chain::util

FC_REFLECT( steem::chain::util::advanced_benchmark_dumper::item, (op_name)(time) )
