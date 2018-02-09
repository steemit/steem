#pragma once

#include <fc/time.hpp>
#include <fc/variant.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/exception/exception.hpp>
#include <fc/io/json.hpp>

#include <sys/time.h>

#include <utility>

namespace steem { namespace chain { namespace util {

template <typename T>
struct is_associative_cntr : public std::false_type {};
template <typename... TArgs>
struct is_associative_cntr<std::set<TArgs...>> : public std::true_type {};
template <typename... TArgs>
struct is_associative_cntr<std::multiset<TArgs...>> : public std::true_type {};
template <typename... TArgs>
struct is_associative_cntr<std::map<TArgs...>> : public std::true_type {};
template <typename... TArgs>
struct is_associative_cntr<std::multimap<TArgs...>> : public std::true_type {};

template <typename TCntr,
   typename T = decltype(std::declval<TCntr>().emplace(std::declval<typename TCntr::value_type>()))>
struct enable_if_associative_cntr
   : public std::enable_if<is_associative_cntr<TCntr>::value, T> {};

template <typename TCntr, typename T = typename TCntr::iterator>
struct enable_if_not_associative_cntr
   : public std::enable_if<!is_associative_cntr<TCntr>::value, T> {};

class advanced_benchmark_dumper
{                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           
   public:

      struct item
      {
         std::string op_name;
         mutable uint64_t time;

         item( std::string _op_name, uint64_t _time ): op_name( _op_name ), time( _time ) {}

         bool operator<( const item& obj ) const { return op_name < obj.op_name; }
         void inc( uint64_t _time ) const { time += _time; }
      };

      struct ritem
      {
         std::string op_name;
         uint64_t time;

         ritem( std::string _op_name, uint64_t _time ): op_name( _op_name ), time( _time ){}

         bool operator<( const ritem& obj ) const { return time > obj.time; }
      };

      template< typename COLLECTION >
      struct total_info
      {
         uint64_t total_time = 0;

         COLLECTION items;// dla r musi tutaj byc list nie set
         
         total_info(){}
         total_info( uint64_t _total_time ): total_time( _total_time ) {}

         void inc( uint64_t _time ) { total_time += _time; }

         template <typename... TArgs, typename TType = COLLECTION>
         typename enable_if_associative_cntr<TType>::type add_item(TArgs&&... args)
            { return items.emplace(std::forward<TArgs>(args)...); }

         template <typename... TArgs, typename TType = COLLECTION>
         typename enable_if_not_associative_cntr<TType>::type add_item(TArgs&&... args)
            { return items.emplace_back(std::forward<TArgs>(args)...); }
      };

   private:

      static uint32_t cnt;
      static std::string virtual_operation_name;
      static std::string apply_context_name;

      bool enabled = false;

      uint32_t flush_cnt = 0;
      uint32_t flush_max = 500000;

      uint64_t time_begin = 0;

      std::string file_name;

      total_info< std::set< item > > info;

      template< typename COLLECTION >
      void dump_impl( const total_info< COLLECTION >& src, const std::string& src_file_name );

   public:

      advanced_benchmark_dumper();
      ~advanced_benchmark_dumper();

      static std::string& get_virtual_operation_name(){ return virtual_operation_name; }

      template< bool IS_PRE_OPERATION >
      std::string generate_desc( const std::string& desc1, const std::string& desc2 )
      {
         std::stringstream s;
         s << ( IS_PRE_OPERATION ? "pre--->" : "post--->" ) << desc1 << "--->" << desc2;

         return s.str();
      }

      void set_enabled( bool val ) { enabled = val; }
      bool is_enabled() { return enabled; }

      void begin();
      template< bool APPLY_CONTEXT = false >
      void end( const std::string& str );
      void end( const std::string& plugin_name, const std::string& item_name );

      void dump();
};

} } } // steem::chain::util

FC_REFLECT( steem::chain::util::advanced_benchmark_dumper::item, (op_name)(time) )
FC_REFLECT( steem::chain::util::advanced_benchmark_dumper::ritem, (op_name)(time) )

FC_REFLECT( steem::chain::util::advanced_benchmark_dumper::total_info< std::set< steem::chain::util::advanced_benchmark_dumper::item > >, (total_time)(items) )
FC_REFLECT( steem::chain::util::advanced_benchmark_dumper::total_info< std::multiset< steem::chain::util::advanced_benchmark_dumper::ritem > >, (total_time)(items) )

