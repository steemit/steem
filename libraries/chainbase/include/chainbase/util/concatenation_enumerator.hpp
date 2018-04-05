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
         virtual abstract_sub_checker* create() = 0;
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

         abstract_sub_checker* create() override
         {
            return new sub_checker( collection );
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
         virtual bool operator!=( const abstract_sub_enumerator& obj ) const = 0;
         virtual bool operator<( const abstract_sub_enumerator& obj ) const = 0;
         virtual bool begin() const = 0;
         virtual bool end() const = 0;

         //Set inactive status
         virtual void change_status( bool val ) = 0;
         //This method informs, that given iterator is inactive. Something like 'end()', but from the second way.
         virtual bool is_inactive() const = 0;
         virtual abstract_sub_enumerator* create() = 0;
   };

   template< typename ITERATOR, typename OBJECT, typename CMP >
   class sub_enumerator: public abstract_sub_enumerator< OBJECT >
   {
      public:

         using reference = typename abstract_sub_enumerator< OBJECT >::reference;
         using pointer = typename abstract_sub_enumerator< OBJECT >::pointer;
 
      private:

         bool inactive;

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

         sub_enumerator( bool _inactive, const ITERATOR& _it, const ITERATOR& _begin, const ITERATOR& _end, const CMP& _cmp )
         : inactive( _inactive ), it_begin( _begin ), it_end( _end ), cmp( _cmp )
         {
            it = _it;
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

         bool operator!=( const abstract_sub_enumerator< OBJECT >& obj ) const override
         {
            return get_id() != obj.get_id();
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

         abstract_sub_enumerator< OBJECT >* create() override
         {
            return new sub_enumerator( inactive, it, it_begin, it_end, cmp );
         }
   };

   template< typename OBJECT, typename CMP >
   class concatenation_iterator
   {
      protected:

         enum class Direction : bool { prev, next };

      private:

         using self = concatenation_iterator< OBJECT, CMP >;
         using pitem = typename abstract_sub_enumerator< OBJECT >::pself;

         using reference = typename abstract_sub_enumerator< OBJECT >::reference;
         using pointer = typename abstract_sub_enumerator< OBJECT >::pointer;

         using pitem_idx = std::pair< pitem, int32_t >;

         template< bool INACTIVE, bool POSITION >
         void add(){}

         template< bool INACTIVE, bool POSITION, typename TUPLE, typename... ELEMENTS >
         void add( const TUPLE& tuple, ELEMENTS... elements )
         {
            using t_without_ref = typename std::remove_reference< decltype( std::get<0>( tuple ) ) >::type;
            using t_without_ref_const = typename std::remove_const< t_without_ref >::type;

            /*
               POSITION == true :   iterator is set on 'begin()' at the start
               POSITION == false:   iterator is set on 'end()' at the start
            */
            if( POSITION )
               iterators.push_back( pitem( new sub_enumerator< t_without_ref_const , OBJECT, CMP >( INACTIVE, std::get<0>( tuple ), std::get<0>( tuple ), std::get<1>( tuple ), cmp ) ) );
            else
               iterators.push_back( pitem( new sub_enumerator< t_without_ref_const , OBJECT, CMP >( INACTIVE, std::get<1>( tuple ), std::get<0>( tuple ), std::get<1>( tuple ), cmp ) ) );

            add< INACTIVE, POSITION >( elements... );
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

            if( idx.current < 0 )
            {
               for( auto& item : iterators )
                  action( item );

               return;
            }

            find_active_iterators( row, condition );

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
               assert( idx.current != pos_begin );
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
               assert( idx.current != pos_end );
               change_direction_impl(  []( const pitem& item ){ return !item->end(); },
                                       []( const pitem& item )
                                       {
                                          if( item->is_inactive() )
                                             item->change_status( false );
                                          else
                                             ++( *item );
                                       },
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

         template< Direction DIRECTION, int32_t position >
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
               find_impl< Direction::next, pos_end >();
            else
               find_impl< Direction::prev, pos_begin >();
         }

         template< int32_t position, typename ACTION >
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
            //Move current iterator at the end, because it is used to comparison.
            action( iterators[ idx.current ] );
         }

         template< Direction DIRECTION >
         void move()
         {
            assert( DIRECTION == Direction::prev || DIRECTION == Direction::next );

            if( DIRECTION == Direction::next )
            {
               if( !change_direction< Direction::next >() )
                  move_impl< pos_end >( []( const pitem& item ){ ++( *item ); } );
            }
            else
            {
               if( !change_direction< Direction::prev >() )
                  move_impl< pos_begin >( []( const pitem& item )
                                                   {
                                                      if( item->begin() )
                                                         item->change_status( true );
                                                      else
                                                         --( *item );
                                                   }
                                                );
            }
         }

         void copy_iterators( const concatenation_iterator& obj )
         {
            iterators.clear();

            for( auto& item : obj.iterators )
               iterators.push_back( pitem( item->create() ) );
         }

      protected:

         Direction direction = Direction::next;

         static const int32_t pos_begin = -2;
         static const int32_t pos_end   = -1;

         struct
         {
            int32_t current = 0;
         } idx;

         std::vector< pitem > iterators;

         template< Direction DIRECTION >
         void action()
         {
            move< DIRECTION >();
            find< DIRECTION >();
         }

         template< typename... ELEMENTS >
         concatenation_iterator( bool status, ELEMENTS... elements )
         {
            //This constructor is invoked only by 'reverse_iterator' class.
            if( status )
               add< true/*INACTIVE*/, true/*POSITION*/ >( elements... );
            else
               add< false/*INACTIVE*/, false/*POSITION*/ >( elements... );
         }

      public:

         CMP cmp;

         concatenation_iterator( const concatenation_iterator& obj )
         : cmp( obj.cmp )
         {
            idx.current = obj.idx.current;
            direction = obj.direction;

            copy_iterators( obj );
         }

         template< typename... ELEMENTS >
         concatenation_iterator( const CMP& _cmp, ELEMENTS... elements )
         : cmp( _cmp )
         {
            add< false/*INACTIVE*/, true/*POSITION*/ >( elements... );

            find< Direction::next >();
         }

         template< typename... ELEMENTS >
         concatenation_iterator( bool, const CMP& _cmp, ELEMENTS... elements )
         : cmp( _cmp )
         {
            add< false/*INACTIVE*/, false/*POSITION*/ >( elements... );

            idx.current = pos_end;
         }

         reference operator*()
         {
            assert( idx.current != pos_begin && idx.current != pos_end );
            return *( *( iterators[ idx.current ] ) );
         }

         const reference operator*() const
         {
            assert( idx.current != pos_begin && idx.current != pos_end );
            return *( *( iterators[ idx.current ] ) );
         }

         pointer operator->() const
         {
            assert( idx.current != pos_begin && idx.current != pos_end );
            return &( *( iterators[ idx.current ] ) );
         }

         concatenation_iterator& operator++()
         {
            action< Direction::next >();

            return *this;
         }

         concatenation_iterator operator++( int )
         {
            auto tmp( *this );

            action< Direction::next >();

            return tmp;
         }

         concatenation_iterator& operator--()
         {
            action< Direction::prev >();

            return *this;
         }

         concatenation_iterator operator--( int )
         {
            auto tmp( *this );

            action< Direction::prev >();

            return tmp;
         }

         bool operator==( const concatenation_iterator& obj ) const
         {
            if( idx.current != obj.idx.current )
               return false;

            if( iterators.size() != obj.iterators.size() )
               return false;

            if( idx.current != pos_begin && idx.current != pos_end )
            {
               assert( static_cast< size_t >( idx.current ) < iterators.size() );
               if( *iterators[ idx.current ] != *obj.iterators[ idx.current ] )
                  return false;
            }

            return true;
         }

         bool operator!=( const concatenation_iterator& obj ) const
         {
            return !( *this == obj );
         }

         concatenation_iterator& operator=( const concatenation_iterator& obj )
         {
            idx.current = obj.idx.current;

            copy_iterators( obj );

            return *this;
         }
   };

   template< typename OBJECT, typename CMP >
   class concatenation_reverse_iterator: public concatenation_iterator< OBJECT, CMP >
   {
      private:

         using base_class = concatenation_iterator< OBJECT, CMP >;
         using Direction = typename base_class::Direction;

      public:

         template< typename... ELEMENTS >
         concatenation_reverse_iterator( const CMP& _cmp, ELEMENTS... elements )
         : concatenation_iterator< OBJECT, CMP >( false/*status*/, elements... )
         {
            this->idx.current = this->pos_end;
            this-> template action< Direction::prev >();
         }

         template< typename... ELEMENTS >
         concatenation_reverse_iterator( bool, const CMP& _cmp, ELEMENTS... elements )
         : concatenation_iterator< OBJECT, CMP >( true/*status*/, elements... )
         {
            this->idx.current = this->pos_begin;
         }

         concatenation_reverse_iterator& operator++()
         {
            this-> template action< Direction::prev >();

            return *this;
         }

         concatenation_reverse_iterator operator++( int )
         {
            auto tmp( *this );

            this-> template action< Direction::prev >();

            return tmp;
         }

         concatenation_reverse_iterator& operator--()
         {
            this-> template action< Direction::next >();

            return *this;
         }

         concatenation_reverse_iterator operator--( int )
         {
            auto tmp( *this );

            this-> template action< Direction::next >();

            return tmp;
         }
   };

   template< typename OBJECT, typename CMP >
   class concatenation_iterator_ex: public concatenation_iterator< OBJECT, CMP >
   {
      protected:
         
         using base_class = concatenation_iterator< OBJECT, CMP >;
         using Direction = typename base_class::Direction;

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

         void copy_containers( const concatenation_iterator_ex& obj )
         {
            containers.clear();

            for( auto& item : obj.containers )
               containers.push_back( pitem( item->create() ) );
         }

      protected:

         bool find_with_key_search_impl( size_t key )
         {
            assert( this->idx.current != this->pos_begin && this->idx.current != this->pos_end );

            for( size_t i = this->idx.current + 1; i < containers.size(); ++i )
            {
               if( containers[ i ]->exists( key ) )
                  return true;
            }

            return false;
         }

         template< Direction DIRECTION, int32_t position >
         void find_with_key_search()
         {
            if( this->idx.current == position )
               return;

            size_t key;

            while( this->idx.current != position )
            {
               key = this->iterators[ this->idx.current ]->get_id();

               if( !find_with_key_search_impl( key ) )
                  break;
            
               this-> template action< DIRECTION >();
            }
         }

      public:

         concatenation_iterator_ex( const concatenation_iterator_ex& obj )
         : concatenation_iterator< OBJECT, CMP >( obj )
         {
            copy_containers( obj );
         }

         template< typename... ELEMENTS >
         concatenation_iterator_ex( const CMP& _cmp, ELEMENTS... elements )
                                 : concatenation_iterator< OBJECT, CMP >( _cmp, elements... )
         {
            add( elements... );
            find_with_key_search< Direction::next, this->pos_end >();
         }

         template< typename... ELEMENTS >
         concatenation_iterator_ex( bool, const CMP& _cmp, ELEMENTS... elements )
                                 : concatenation_iterator< OBJECT, CMP >( false, _cmp, elements... )
         {
            add( elements... );
         }

         concatenation_iterator_ex& operator++()
         {
            this-> template action< Direction::next >();
            find_with_key_search< Direction::next, this->pos_end >();

            return *this;
         }

         concatenation_iterator_ex operator++( int )
         {
            auto tmp( *this );

            this-> template action< Direction::next >();
            find_with_key_search< Direction::next, this->pos_end >();

            return tmp;
         }

         concatenation_iterator_ex& operator--()
         {
            this-> template action< Direction::prev >();
            find_with_key_search< Direction::prev, this->pos_begin >();

            return *this;
         }

         concatenation_iterator_ex operator--( int )
         {
            auto tmp( *this );

            this-> template action< Direction::prev >();
            find_with_key_search< Direction::prev, this->pos_begin >();

            return tmp;
         }

         concatenation_iterator_ex& operator=( const concatenation_iterator_ex& obj )
         {
            base_class::operator=( obj );

            copy_containers( obj );

            return *this;
         }
        
   };

}

}

