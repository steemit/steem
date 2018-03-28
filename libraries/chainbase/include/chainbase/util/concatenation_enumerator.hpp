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

      public:

         CMP cmp;

      private:

         template< typename CONTAINER, typename CONDITION >
         void find_active_iterators( CONTAINER& dst, CONDITION&& condition )
         {
            int32_t cnt = 0;
            for( auto& item : iterators )
            {
               if( condition( item ) )
                  dst.emplace( std::make_pair( item, cnt ) );
               ++cnt;
            }
         }

         template< typename CONDITION, typename ACTION, typename COMPARISION, typename CHECKER >
         void change_direction_impl( int32_t idx, CONDITION&& condition, ACTION&& action, COMPARISION&& comparision, CHECKER&& checker )
         {
            struct _sorter
            {
               bool operator()( const pitem_idx& a, const pitem_idx& b ){ return a.second < b.second; };
            };
            std::set< pitem_idx, _sorter > row;

            find_active_iterators( row, condition );

            if( idx < 0 )
            {
               for( auto& item : row )
                  action( item.first );

               return;
            }

            auto current_it = iterators[ idx ];
            bool current_it_active = false;

            for( auto& item : row )
            {
               if( item.second == idx )
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
               action( iterators[ idx ] );
         }

         template< Direction DIRECTION >
         bool change_direction( int32_t idx )
         {
            //Direction is still the same.
            if( direction == DIRECTION )
               return false;

            direction = DIRECTION;

            assert( DIRECTION == Direction::prev || DIRECTION == Direction::next );

            if( DIRECTION == Direction::prev )
            {
               assert( idx != Position::pos_begin );
               change_direction_impl( idx,
                                    []( const pitem& item ){ return !item->is_inactive(); },
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
               assert( idx != Position::pos_end );
               change_direction_impl( idx,
                                    []( const pitem& item ){ return !item->end(); },
                                    []( const pitem& item ){ ++( *item ); },
                                    [ this ]( const pitem& item, const pitem& item2 ){ return cmp( *( *item ), *( *item2 ) ); },
                                    []( const pitem& item, const pitem& item2 )
                                    {
                                       if( *item == *item2 )
                                          return true;
                                       else
                                       {
                                          if( item->is_inactive() )
                                             item->change_status( false );
                                          return false;
                                       }
                                    }
                                    );
               return true;
            }

            return false;
         }

      protected:

         Direction direction = Direction::next;

         std::vector< pitem > iterators;

         struct
         {
            int32_t current;
         } idx;

         void add() {}

         template< typename TUPLE, typename... ELEMENTS >
         void add( const TUPLE& tuple, ELEMENTS... elements )
         {
            using t_without_ref = typename std::remove_reference< decltype( std::get<0>( tuple ) ) >::type;
            using t_without_ref_const = typename std::remove_const< t_without_ref >::type;

            iterators.push_back( pitem( new sub_enumerator< t_without_ref_const , OBJECT, CMP >( std::get<0>( tuple ), std::get<1>( tuple ), cmp ) ) );
            add( elements... );
         }

      private:

         template< Direction DIRECTION, Position position >
         int32_t find_impl()
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
                     return a.second > b.second;
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

            switch( row.size() )
            {
               case 0: return position;
               case 1: return row.begin()->second;
               default: break;
            }

            int32_t ret;
            ret = row.begin()->second;
            assert( !iterators[ ret ]->end() && !iterators[ ret ]->is_inactive() );

            return ret;
         }

         template< Position position, typename ACTION >
         void move_impl( int32_t idx, ACTION&& action )
         {
            if( idx == position )
               return;

            assert( static_cast< size_t >( idx ) < iterators.size() && !iterators[ idx ]->end() && !iterators[ idx ]->is_inactive() );

            for( size_t i = 0 ; i < iterators.size(); ++i )
            {
               if( static_cast< int32_t >( i ) != idx && !iterators[ i ]->end() && !iterators[ i ]->is_inactive() && ( *iterators[ i ] ) == ( *iterators[ idx ] ) )
                  action( iterators[ i ] );
            }
            //Move given iterator at the end, because it is used to comparision.
            action( iterators[ idx ] );
         }

      protected:

         int32_t find( Direction _direction )
         {
            assert( _direction == Direction::prev || _direction == Direction::next );

            if( _direction == Direction::next )
               return find_impl< Direction::next, Position::pos_end >();
            else
               return find_impl< Direction::prev, Position::pos_begin >();
         }

         void move( int32_t idx, Direction _direction )
         {
            assert( _direction == Direction::prev || _direction == Direction::next );

            if( _direction == Direction::next )
            {
               if( !change_direction< Direction::next >( idx ) )
                  move_impl< Position::pos_end >( idx, []( const pitem& item ){ ++( *item ); } );
            }
            else
            {
               if( !change_direction< Direction::prev >( idx ) )
                  move_impl< Position::pos_begin >( idx,
                                                   []( const pitem& item )
                                                   {
                                                      if( item->begin() )
                                                         item->change_status( true );
                                                      else
                                                         --( *item );
                                                   }
                                                );
            }
         }

      public:

         template< typename... ELEMENTS >
         concatenation_enumerator( const CMP& _cmp, ELEMENTS... elements )
         : cmp( _cmp )
         {
            add( elements... );

            idx.current = find( Direction::next );
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
            move( idx.current, Direction::next );
            idx.current = find( Direction::next );

            return *this;
         }

         concatenation_enumerator& operator--()
         {
            move( idx.current, Direction::prev );
            idx.current = find( Direction::prev );

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

      protected:

         bool find_next_with_key_search_impl( size_t key )
         {
            assert( this->idx.current != Position::pos_end );

            for( size_t i = this->idx.current + 1; i < containers.size(); ++i )
            {
               if( containers[ i ]->exists( key ) )
                  return true;
            }

            return false;
         }

         void find_next_with_key_search()
         {
            if( this->idx.current == Position::pos_end )
               return;

            size_t key;

            while( this->idx.current != Position::pos_end )
            {
               key = this->iterators[ this->idx.current ]->get_id();

               if( !find_next_with_key_search_impl( key ) )
                  break;
            
               this->move( this->idx.current, Direction::next );

               this->idx.current = this->find( Direction::next );
            }
         }

      public:

      template< typename... ELEMENTS >
      concatenation_enumerator_ex( const CMP& _cmp, ELEMENTS... elements )
                              : concatenation_enumerator< OBJECT, CMP >( _cmp, elements... )
      {
         add( elements... );
         find_next_with_key_search();
      }

      void add(){}

      template< typename TUPLE, typename... ELEMENTS >
      void add( const TUPLE& tuple, ELEMENTS... elements )
      {
         using t_without_ref = typename std::remove_reference< decltype( std::get<2>( tuple ) ) >::type;
         using t_without_ref_const = typename std::remove_const< t_without_ref >::type;

         containers.push_back( pitem( new sub_checker< t_without_ref_const >( std::get<2>( tuple ) ) ) );

         add( elements... );
      }

      concatenation_enumerator_ex& operator++()
      {
         this->move( this->idx.current, Direction::next );

         this->idx.current = this->find( Direction::next );
         find_next_with_key_search();

         return *this;
      }

   };

}

}

