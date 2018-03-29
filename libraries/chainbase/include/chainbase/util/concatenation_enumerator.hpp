#pragma once

#include <iostream>

using namespace boost::multi_index;

namespace chainbase
{

namespace ce
{
   class abstract_sub_checker
   {
      public:

         using pself = std::unique_ptr< abstract_sub_checker >;

      public:

         virtual bool exists( size_t key ) = 0;
   };

   template< typename COLLECTION_POINTER >
   class sub_checker : public abstract_sub_checker
   {
      private:

         const COLLECTION_POINTER collection;

      public:

         sub_checker( const COLLECTION_POINTER _collection )
                  : collection( _collection )
         {
         }

         bool exists( size_t key ) override
         {
            return collection->lower_bound( key ) != collection->upper_bound( key );
         }
   };

   template< typename OBJECT >
   class abstract_sub_enumerator
   {
      public:

         using pself = std::shared_ptr< abstract_sub_enumerator >;

         using reference = const OBJECT&;
         using pointer = const OBJECT*;

      protected:

         virtual void dec() = 0;
         virtual void inc() = 0;

      public:

         abstract_sub_enumerator& operator--()
         {
            dec();
            return *this;
         }

         abstract_sub_enumerator& operator++()
         {
            inc();
            return *this;
         }

         virtual size_t get_id() const = 0;
         virtual reference operator*() const = 0;
         virtual pointer operator->() const = 0;
         virtual bool operator==( const abstract_sub_enumerator& obj ) const = 0;
         virtual bool operator<( const abstract_sub_enumerator& obj ) const = 0;
         virtual bool begin() const = 0;
         virtual bool end() const = 0;

         //Set inactive status
         virtual void change_status( bool val ) = 0;
         //This method informs, that given iterator is inactive. Something like 'end()', but from the second way.
         virtual bool is_inactive() const = 0;
   };

   template< typename ITERATOR, typename OBJECT, typename CMP >
   class sub_enumerator: public abstract_sub_enumerator< OBJECT >
   {
      public:

         using reference = typename abstract_sub_enumerator< OBJECT >::reference;
         using pointer = typename abstract_sub_enumerator< OBJECT >::pointer;
 
      private:

         bool inactive = false;

      protected:

         void dec() override
         {
            assert( it != it_begin );
            --it;
         }

         void inc() override
         {
            assert( it != it_end );
            ++it;

            if( inactive )
               inactive = false;
         }

      public:

         ITERATOR it;

         ITERATOR it_begin;
         ITERATOR it_end;

         const CMP cmp;

         sub_enumerator( const ITERATOR& _it, const ITERATOR& _end, const CMP& _cmp )
         : it_end( _end ), cmp( _cmp )
         {
            it = _it;
            it_begin = _it;//TEMPORARY!!!!!!!!!!!
         }

         size_t get_id() const override
         {
            assert( it != it_end );
            return size_t( it->id );
         }

         sub_enumerator operator--()
         {
            dec();
            return *this;
         }

         sub_enumerator operator++()
         {
            inc();
            return *this;
         }

         reference operator*() const override
         {
            assert( it != it_end );
            return *it;
         }

         pointer operator->() const override
         {
            assert( it != it_end );
            return &( *it );
         }

         bool operator==( const abstract_sub_enumerator< OBJECT >& obj ) const override
         {
            return get_id() == obj.get_id();
         }

         bool operator<( const abstract_sub_enumerator< OBJECT >& obj ) const override
         {
            return cmp( *( *this ), *obj );
         }

         bool begin() const override
         {
            return it == it_begin;
         }

         bool end() const override
         {
            return it == it_end;
         }

         void change_status( bool val ) override
         {
            inactive = val;
         }

         bool is_inactive() const
         {
            return inactive;
         }
   };

   template< typename OBJECT, typename CMP >
   class concatenation_enumerator
   {
      private:

         using self = concatenation_enumerator< OBJECT, CMP >;
         using pitem = typename abstract_sub_enumerator< OBJECT >::pself;

         using reference = typename abstract_sub_enumerator< OBJECT >::reference;
         using pointer = typename abstract_sub_enumerator< OBJECT >::pointer;

         using pitem_idx = std::pair< pitem, int32_t >;

      protected:

         enum class Direction : bool      { prev, next };
         enum Position        : int32_t   { pos_begin = -2, pos_end = -1 };

         Direction direction = Direction::next;

      private:

         void add() {}

         template< typename TUPLE, typename... ELEMENTS >
         void add( const TUPLE& tuple, ELEMENTS... elements )
         {
            using t_without_ref = typename std::remove_reference< decltype( std::get<0>( tuple ) ) >::type;
            using t_without_ref_const = typename std::remove_const< t_without_ref >::type;

            iterators.push_back( pitem( new sub_enumerator< t_without_ref_const , OBJECT, CMP >( std::get<0>( tuple ), std::get<1>( tuple ), cmp ) ) );
            add( elements... );
         }

         template< typename CONTAINER, typename CONDITION >
         void find_active_iterators( CONTAINER& dst, CONDITION&& condition )
         {
            int32_t cnt = iterators.size() - 1;

            auto it = iterators.rbegin();
            auto it_end = iterators.rend();

            while( it != it_end )
            {
               if( condition( *it ) )
                  dst.emplace( std::make_pair( *it, cnt ) );

               ++it;
               --cnt;
            }
         }

         template< typename CONDITION, typename ACTION, typename COMPARISION, typename CHECKER >
         void change_direction_impl( CONDITION&& condition, ACTION&& action, COMPARISION&& comparision, CHECKER&& checker )
         {
            struct _sorter
            {
               bool operator()( const pitem_idx& a, const pitem_idx& b ){ return a.second < b.second; };
            };
            std::set< pitem_idx, _sorter > row;

            find_active_iterators( row, condition );

            if( idx.current < 0 )
            {
               for( auto& item : row )
                  action( item.first );

               return;
            }

            auto current_it = iterators[ idx.current ];
            bool current_it_active = false;

            for( auto& item : row )
            {
               if( item.second == idx.current )
                  current_it_active = true;
               else
               {
                  if( checker( item.first, current_it ) )
                     action( item.first );
                  else
                  {
                     auto res = comparision( item.first, current_it );
                     if( res )
                        action( item.first );
                  }
               }
            }

            if( current_it_active )
               action( iterators[ idx.current ] );
         }

         template< Direction DIRECTION >
         bool change_direction()
         {
            //Direction is still the same.
            if( direction == DIRECTION )
               return false;

            direction = DIRECTION;

            assert( DIRECTION == Direction::prev || DIRECTION == Direction::next );

            if( DIRECTION == Direction::prev )
            {
               assert( idx.current != Position::pos_begin );
               change_direction_impl(  []( const pitem& item ){ return !item->is_inactive(); },
                                       []( const pitem& item )
                                       {
                                          if( item->begin() )
                                             item->change_status( true );
                                          else
                                             --( *item );
                                       },
                                       [ this ]( const pitem& item, const pitem& item2 ){ return !cmp( *( *item ), *( *item2 ) ); },
                                       []( const pitem& item, const pitem& item2 )
                                       {
                                          if( item->end() )
                                             return true;
                                          else
                                             return *item == *item2;
                                       }
                                    );
               return true;
            }
            else if( DIRECTION == Direction::next )
            {
               assert( idx.current != Position::pos_end );
               change_direction_impl(  []( const pitem& item ){ return !item->end(); },
                                       []( const pitem& item ){ ++( *item ); },
                                       [ this ]( const pitem& item, const pitem& item2 ){ return cmp( *( *item ), *( *item2 ) ); },
                                       []( const pitem& item, const pitem& item2 )
                                       {
                                          if( item->is_inactive() )
                                          {
                                             item->change_status( false );
                                             return false;
                                          }
                                          else
                                             return *item == *item2;
                                       }
                                    );
               return true;
            }

            return false;
         }

         template< Direction DIRECTION, Position position >
         void find_impl()
         {
            assert( DIRECTION == Direction::prev || DIRECTION == Direction::next );

            struct _sorter
            {
               const self& obj;
               Direction direction;

               _sorter( const self& _obj, Direction _direction ): obj( _obj ), direction( _direction ) {}

               bool operator()( const pitem_idx& a, const pitem_idx& b )
               {
                  if( *( a.first ) == *( b.first ) )
                     return false;
                  else
                  {
                     if( direction == Direction::next )
                        return obj.cmp( *( *a.first ), *( *b.first ) );
                     else
                        return !obj.cmp( *( *a.first ), *( *b.first ) );
                  }
               };
            };

            _sorter __sorter( *this, DIRECTION );
            std::set< pitem_idx, _sorter > row( __sorter );

            find_active_iterators( row, []( const pitem& item ){ return !item->end() && !item->is_inactive(); } );

            if( row.size() < 2 )
            {
               idx.current = ( row.size() == 0 ) ? position : ( row.begin()->second );
               return;
            }

            idx.current = row.begin()->second;
            assert( !iterators[ idx.current ]->end() && !iterators[ idx.current ]->is_inactive() );
         }

         template< Direction DIRECTION >
         void find()
         {
            assert( DIRECTION == Direction::prev || DIRECTION == Direction::next );

            if( DIRECTION == Direction::next )
               find_impl< Direction::next, Position::pos_end >();
            else
               find_impl< Direction::prev, Position::pos_begin >();
         }

         template< Position position, typename ACTION >
         void move_impl( ACTION&& action )
         {
            if( idx.current == position )
               return;

            assert( static_cast< size_t >( idx.current ) < iterators.size() && !iterators[ idx.current ]->end() && !iterators[ idx.current ]->is_inactive() );

            for( size_t i = 0 ; i < iterators.size(); ++i )
            {
               if( static_cast< int32_t >( i ) != idx.current && !iterators[ i ]->end() && !iterators[ i ]->is_inactive() && ( *iterators[ i ] ) == ( *iterators[ idx.current ] ) )
                  action( iterators[ i ] );
            }
            //Move given iterator at the end, because it is used to comparision.
            action( iterators[ idx.current ] );
         }

         template< Direction DIRECTION >
         void move()
         {
            assert( DIRECTION == Direction::prev || DIRECTION == Direction::next );

            if( DIRECTION == Direction::next )
            {
               if( !change_direction< Direction::next >() )
                  move_impl< Position::pos_end >( []( const pitem& item ){ ++( *item ); } );
            }
            else
            {
               if( !change_direction< Direction::prev >() )
                  move_impl< Position::pos_begin >( []( const pitem& item )
                                                   {
                                                      if( item->begin() )
                                                         item->change_status( true );
                                                      else
                                                         --( *item );
                                                   }
                                                );
            }
         }

      protected:

         std::vector< pitem > iterators;

         struct
         {
            int32_t current;
         } idx;

         template< Direction DIRECTION >
         void action()
         {
            move< DIRECTION >();
            find< DIRECTION >();
         }


      public:

         CMP cmp;

         template< typename... ELEMENTS >
         concatenation_enumerator( const CMP& _cmp, ELEMENTS... elements )
         : cmp( _cmp )
         {
            add( elements... );

            find< Direction::next >();
         }

         reference operator*() const
         {
            assert( idx.current != Position::pos_end );
            return *( *( iterators[ idx.current ] ) );
         }

         pointer operator->() const
         {
            assert( idx.current != Position::pos_end );
            return &( *( iterators[ idx.current ] ) );
         }

         concatenation_enumerator& operator++()
         {
            action< Direction::next >();

            return *this;
         }

         concatenation_enumerator& operator--()
         {
            action< Direction::prev >();

            return *this;
         }

         bool end() const
         {
           return idx.current == Position::pos_end;
         }
   };

   template< typename OBJECT, typename CMP >
   class concatenation_enumerator_ex: public concatenation_enumerator< OBJECT, CMP >
   {
      protected:
         
         using base_class = concatenation_enumerator< OBJECT, CMP >;
         using Direction = typename base_class::Direction;
         using Position = typename base_class::Position;

      private:

         using pitem = typename abstract_sub_checker::pself;

         std::vector< pitem > containers;

         void add(){}

         template< typename TUPLE, typename... ELEMENTS >
         void add( const TUPLE& tuple, ELEMENTS... elements )
         {
            using t_without_ref = typename std::remove_reference< decltype( std::get<2>( tuple ) ) >::type;
            using t_without_ref_const = typename std::remove_const< t_without_ref >::type;

            containers.push_back( pitem( new sub_checker< t_without_ref_const >( std::get<2>( tuple ) ) ) );

            add( elements... );
         }

      protected:

         bool find_with_key_search_impl( size_t key )
         {
            assert( this->idx.current != Position::pos_end );

            for( size_t i = this->idx.current + 1; i < containers.size(); ++i )
            {
               if( containers[ i ]->exists( key ) )
                  return true;
            }

            return false;
         }

         template< Direction DIRECTION >
         void find_with_key_search()
         {
            if( this->idx.current == Position::pos_end )
               return;

            size_t key;

            while( this->idx.current != Position::pos_end )
            {
               key = this->iterators[ this->idx.current ]->get_id();

               if( !find_with_key_search_impl( key ) )
                  break;
            
               this-> template action< DIRECTION >();
            }
         }

      public:

      template< typename... ELEMENTS >
      concatenation_enumerator_ex( const CMP& _cmp, ELEMENTS... elements )
                              : concatenation_enumerator< OBJECT, CMP >( _cmp, elements... )
      {
         add( elements... );
         find_with_key_search< Direction::next >();
      }

      concatenation_enumerator_ex& operator++()
      {
         this-> template action< Direction::next >();
         find_with_key_search< Direction::next >();

         return *this;
      }

      concatenation_enumerator_ex& operator--()
      {
         this-> template action< Direction::prev >();
         find_with_key_search< Direction::prev >();

         return *this;
      }

   };

}

}

