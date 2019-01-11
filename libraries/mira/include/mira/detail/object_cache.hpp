#pragma once

#include <fc/log/logger.hpp>

#include <map>
#include <memory>

#include <iostream>

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

   void update( const Key& k, Value&& v )
   {
      *(_cache[ k ]) = std::move( v );
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
   static std::shared_ptr< object_cache< Value, Key, KeyFromValue > > get_shared_cache( bool reset = false )
   {
      static std::shared_ptr< object_cache< Value, Key, KeyFromValue > > cache_ptr;

      if( !cache_ptr || reset )
         cache_ptr = std::make_shared< object_cache< Value, Key, KeyFromValue > >();

      return cache_ptr;
   }

   static void reset()
   {
      get_shared_cache( true );
   }
};

} } } // mira::multi_index::detail
