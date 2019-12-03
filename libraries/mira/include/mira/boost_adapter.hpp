#pragma once
#include <boost/multi_index_container.hpp>

namespace mira {

template< typename Value, typename IndexSpecifierList, typename Allocator >
class boost_multi_index_adapter : public boost::multi_index_container< Value, IndexSpecifierList, Allocator >
{
   private:
      typedef typename Value::id_type id_type;
      int64_t _revision = -1;
      id_type _next_id = 0;

   public:
      using boost::multi_index_container< Value, IndexSpecifierList, Allocator >::multi_index_container;
      static const size_t node_size = sizeof( Value );

      int64_t revision()
      {
         return _revision;
      }

      int64_t set_revision( uint64_t revision )
      {
         _revision = revision;
         return _revision;
      }

      id_type next_id()
      {
         return _next_id;
      }

      void set_next_id( id_type id )
      {
         _next_id = id;
      }

      void close() {}
      void wipe( const boost::filesystem::path& p ) {}
      void flush() {}
      bool open( const boost::filesystem::path& p, const boost::any& opts ) { return true; }
      void trim_cache() {}

      void print_stats() const {}

      size_t get_cache_usage() const { return 0; }
      size_t get_cache_size() const { return 0; }
      void dump_lb_call_counts() {}

      template< typename MetaKey, typename MetaValue >
      bool get_metadata( const MetaKey& k, MetaValue& v ) { return true; }

      template< typename MetaKey, typename MetaValue >
      bool put_metadata( const MetaKey& k, const MetaValue& v ) { return true; }

      void begin_bulk_load() {}
      void end_bulk_load() {}
      void flush_bulk_load() {}

      template< typename Lambda >
      void bulk_load( Lambda&& l ) { l(); }
};

} // mira
