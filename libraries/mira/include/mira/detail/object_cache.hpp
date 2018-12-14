#pragma once

#include <map>
#include <memory>

namespace mira { namespace multi_index { namespace detail {

template< typename Value, typename Key, typename KeyFromValue >
struct object_cache_factory; // forward declaration

template< typename Value, typename Key, typename KeyFromValue >
struct object_cache
{
public:
   typedef typename std::shared_ptr< Value > ptr_type;
   typedef object_cache_factory<
      Value,
      Key,
      KeyFromValue >                         factory_type;

private:
   KeyFromValue                              _get_key;
   std::map< Key, ptr_type >                 _cache;

public:

   void cache( ptr_type& v )
   {
      _cache.insert( std::make_pair( _get_key( *v ), v ) );
   }

   void invalidate( const Value& v )
   {
      invalidate( _get_key( v ) );
   }

   void invalidate( const Key& k )
   {
      _cache.erase( k );
   }

   ptr_type get( const Key& k )
   {
      auto itr = _cache.find( k );
      return itr != _cache.end() ? itr->second : ptr_type();
   }
};

template< typename Value, typename Key, typename KeyFromValue >
struct object_cache_factory
{
   object_cache< Value, Key, KeyFromValue >& static_create()
   {
      static object_cache< Value, Key, KeyFromValue >* cache_ptr = nullptr;
      if( cache_ptr == nullptr )
         cache_ptr = new object_cache< Value, Key, KeyFromValue >;

      return *cache_ptr;
   }
};

} } } // mira::multi_index::detail
