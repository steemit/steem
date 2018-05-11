#pragma once

#include <iostream>

#include <boost/operators.hpp>

using namespace boost::multi_index;

namespace chainbase
{

namespace ce
{

   enum class Direction : bool { prev, next };

   class modifier
   {
      private:

         using levels = std::set< uint32_t >;
         using t_items = std::map< size_t/*id*/, levels >;

      private:

         bool current_is_invalidated = false;
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

         template< Direction DIRECTION, typename CMP, typename ITEM >
         void dec( CMP& cmp, const ITEM& item, const ITEM& current_item )
         {
            bool _cmp = true;
            bool done = false;

            while( _cmp && !done )
            {
               _cmp = !cmp( *( *item ), *( *current_item ) );
               if( _cmp )
               {
                  if( item->begin() )
                  {
                     if( DIRECTION == Direction::prev )
                        item->change_status( true );
                     done = true;
                  }
                  else
                     --( *item );
               }
            }
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

         bool empty() const
         {
            return items.empty();
         }

         template< typename ITEM >
         bool start( const ITEM& current_item, uint32_t current_level )
         {
            if( empty() )
               return false;

            current_is_invalidated = exists( current_item, current_level );

            return true;
         }

         template< Direction DIRECTION, typename CMP, typename ITEM >
         void run( CMP& cmp, const ITEM& item, const ITEM& current_item, uint32_t level, uint32_t current_level )
         {
            if( current_is_invalidated )
            {
               dec< DIRECTION >( cmp, item, current_item );
               inc( cmp, item, current_item );
            }
            else
               if( DIRECTION == Direction::next )
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
         
         modifier& operator=( const modifier& obj )
         {
            current_is_invalidated = obj.current_is_invalidated;
            items = obj.items;

            return *this;
         }
   };

}

}

