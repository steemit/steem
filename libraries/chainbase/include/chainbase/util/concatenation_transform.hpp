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

         template< bool FORWARD_ITERATOR, Direction DIRECTION, typename CMP, typename ITEM >
         bool make_cmp( CMP& cmp, const ITEM& item, const ITEM& current_item )
         {
            if( DIRECTION == Direction::next )
            {
               if( FORWARD_ITERATOR )
                  return cmp( *( *item ), *( *current_item ) );
               else
                  return !cmp( *( *item ), *( *current_item ) );
            }
            else
            {
               if( FORWARD_ITERATOR )
                  return !cmp( *( *item ), *( *current_item ) );
               else
                  return cmp( *( *item ), *( *current_item ) );
            }
         }

         template< bool FORWARD_ITERATOR, Direction DIRECTION, typename CMP, typename ITEM >
         void dec( CMP& cmp, const ITEM& item, const ITEM& current_item )
         {
            bool _cmp = true;
            bool done = false;

            while( _cmp && !done && !item->end() )
            {
               _cmp = make_cmp< FORWARD_ITERATOR, Direction::prev >( cmp, item, current_item );

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

         template< bool FORWARD_ITERATOR, Direction DIRECTION, typename CMP, typename ITEM >
         void inc( CMP& cmp, const ITEM& item, const ITEM& current_item )
         {
            bool _cmp = true;

            while( _cmp && !item->end() )
            {
               _cmp = make_cmp< FORWARD_ITERATOR, Direction::next >( cmp, item, current_item );

               if( _cmp )
               {
                  ++( *item );
                  if( DIRECTION != Direction::next && item->end() )
                  {
                     --( *item );
                     break;
                  }
               }
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

         template< bool FORWARD_ITERATOR, Direction DIRECTION, typename CMP, typename ITEM >
         void run( CMP& cmp, const ITEM& item, const ITEM& current_item, uint32_t level, uint32_t current_level )
         {
            dec< FORWARD_ITERATOR, DIRECTION >( cmp, item, current_item );
            inc< FORWARD_ITERATOR, DIRECTION >( cmp, item, current_item );
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

