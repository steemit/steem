#pragma once

#include <iostream>

#include <boost/operators.hpp>

using namespace boost::multi_index;

namespace chainbase
{

namespace ce
{

   enum class Direction : bool { prev, next };

   template< typename CMP >
   class modifier
   {
      private:

         CMP cmp;

         template< bool FORWARD_ITERATOR, Direction DIRECTION, typename OBJECT >
         bool make_cmp( const OBJECT& item, const OBJECT& current_item )
         {
            if( DIRECTION == Direction::next )
            {
               if( FORWARD_ITERATOR )
                  return cmp( item, current_item );
               else
                  return !cmp( item, current_item );
            }
            else
            {
               if( FORWARD_ITERATOR )
                  return !cmp( item, current_item );
               else
                  return cmp( item, current_item );
            }
         }

         template< bool FORWARD_ITERATOR, Direction DIRECTION, typename ITEM, typename KEY, typename INDEX >
         void correct( const ITEM& item, const ITEM& current_item, const KEY& key, const INDEX& index )
         {
            if( FORWARD_ITERATOR )
            {
               if( DIRECTION == Direction::next )
               {
                  auto found = index.lower_bound( key );
                  if( found == index.end() )
                  {
                     item->set_iterator( false/*begin_pos*/ );
                     return;
                  }

                  bool res = make_cmp< FORWARD_ITERATOR, DIRECTION >( *found, *( *current_item ) );
                  if( res )
                     item->set_iterator( false/*begin_pos*/ );
                  else
                     item->change_iterator( &found );
               }
               else
               {
                  auto found = index.upper_bound( key );

                  if( found == index.begin() )
                  {
                     item->set_iterator( true/*begin_pos*/ );
                     item->change_status( true );
                     return;
                  }

                  --found;
                  bool res = make_cmp< FORWARD_ITERATOR, DIRECTION >( *found, *( *current_item ) );
                  if( res )
                  {
                     item->set_iterator( true/*begin_pos*/ );
                     item->change_status( true );
                  }
                  else
                     item->change_iterator( &found );
               }
            }
            else
            {
               if( DIRECTION == Direction::next )
               {
                  auto found = index.upper_bound( key );

                  bool res = make_cmp< FORWARD_ITERATOR, DIRECTION >( *found, *( *current_item ) );
                  if( res )
                  {
                     item->set_iterator( true/*begin_pos*/ );
                     item->change_status( true );
                  }
                  else
                     item->change_iterator( &found );
               }
               else
               {
                  auto found = index.lower_bound( key );

                  bool res = make_cmp< FORWARD_ITERATOR, DIRECTION >( *found, *( *current_item ) );
                  if( res )
                     item->set_iterator( false/*begin_pos*/ );
                  else
                     item->change_iterator( &found );
               }
            }
         }

      public:

         modifier( const CMP& _cmp ): cmp( _cmp )
         {
         }

         template< typename KEY, typename INDEX >
         bool check_current( const KEY& key, const INDEX& index )
         {
            bool found = index.find( key ) != index.end();
            return found;
         }

         template< bool FORWARD_ITERATOR, Direction DIRECTION, typename ITEM, typename KEY, typename INDEX >
         void run( const ITEM& item, const ITEM& current_item, const KEY& key, const INDEX& index )
         {
            if( item->end() )
               return;

            correct< FORWARD_ITERATOR, DIRECTION >( item, current_item, key, index );
         }

   };

}

}

