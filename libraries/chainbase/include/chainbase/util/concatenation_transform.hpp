#pragma once

#include <iostream>

#include <boost/operators.hpp>

using namespace boost::multi_index;

namespace chainbase
{

namespace ce
{
   class abstract_transformation
   {
      public:
   };

   class modifier
   {
      private:

         using levels = std::set< uint32_t >;
         using t_items = std::map< size_t/*id*/, levels >;

      private:

         bool current_level_is_active = false;
         t_items items;

         template< typename ITEM >
         bool exists( const ITEM& item, uint32_t level )
         {
            size_t id = item->get_id();

            auto _found = items.find( id );
            if( _found == items.end() )
               return false;

            auto found = _found->second.find( level );
            return found != _found->second.end();
         }

         template< typename ITEM >
         bool allowed( const ITEM& item, uint32_t level )
         {
            if( items.empty() )
               return false;

            return exists( item, level );
         }

         template< typename CMP, typename ITEM >
         void dec( CMP& cmp, const ITEM& item, const ITEM& current_item )
         {
            bool _cmp = true;

            while( _cmp && !item->begin() )
            {
               --( *item );
               _cmp = !cmp( *( *item ), *( *current_item ) );
            }
            if( !_cmp )
               ++( *item );
         }

         template< typename CMP, typename ITEM >
         void inc( CMP& cmp, const ITEM& item, const ITEM& current_item )
         {
            bool _cmp = true;

            while( _cmp && !item->end() )
            {
               _cmp = cmp( *( *item ), *( *current_item ) );
               if( _cmp )
                  ++( *item );
            }
         }

      public:

         modifier()
         {
         }

         template< typename ITEM >
         void start( const ITEM& current_item, uint32_t current_level )
         {
            current_level_is_active = allowed( current_item, current_level );
         }

         template< typename CMP, typename ITEM >
         void run( CMP& cmp, const ITEM& item, const ITEM& current_item, uint32_t level, uint32_t current_level )
         {
            bool res = allowed( item, level );

            if( current_level_is_active )
            {
               dec( cmp, item, current_item );
               inc( cmp, item, current_item );
            }
 
            if( res )
               inc( cmp, item, current_item );
         }

         void end()
         {
            items.clear();
         }

         void add_modify( uint32_t id, uint32_t current_level )
         {
            auto found = items.find( id );

            if( found == items.end() )
               items.insert( std::make_pair( id, levels( { current_level } ) ) );
            else
               found->second.insert( current_level );
         }
   };

}

}

