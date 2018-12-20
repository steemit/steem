#pragma once

#include <fc/log/logger.hpp>

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

   ptr_type cache( Value&& v )
   {
      auto key = _get_key( v );
      auto value = std::make_shared< Value >( std::move( v ) );

      _cache.insert( std::make_pair( key, value ) );

      return value;
   }

   void invalidate( const Value& v )
   {
      invalidate( _get_key( v ) );
   }

   void invalidate( const Key& k )
   {
      _cache.erase( k );
   }

   template< typename Modifier >
   void replace( const Value& v, Modifier mod )
   {
      replace( _get_key( v ), mod );
   }

   template< typename Modifier >
   void replace( const Key& k, Modifier mod )
   {
      mod( *(_cache[ k ]) );
   }

   ptr_type get( const Key& k )
   {
      auto itr = _cache.find( k );
      return itr != _cache.end() ? itr->second : ptr_type();
   }

   void clear()
   {
      _cache.clear();
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
