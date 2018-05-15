#pragma once

#include <iostream>

#include <boost/operators.hpp>
#include <boost/multi_index/detail/bidir_node_iterator.hpp>
#include <chainbase/util/concatenation_transform.hpp>

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
         decltype( collection->end() ) it_end;

      public:

         sub_checker( const COLLECTION_POINTER _collection )
                  : collection( _collection ), it_end( _collection->end() )
         {
         }

         bool exists( size_t key ) override
         {
            return collection->find( key ) != it_end;
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

         virtual int32_t get_tag() const = 0;

         virtual void set_iterator( bool begin_pos ) = 0;
   };

   template< typename COLLECTION_POINTER, typename ITERATOR, typename OBJECT, typename CMP >
   class sub_enumerator: public abstract_sub_enumerator< OBJECT >
   {
      public:

         using reference = typename abstract_sub_enumerator< OBJECT >::reference;
         using pointer = typename abstract_sub_enumerator< OBJECT >::pointer;
 
      private:

         bool inactive;
         const int32_t tag;

         ITERATOR it;
         ITERATOR it_end;
         const COLLECTION_POINTER collection;

      protected:

         void dec() override
         {
            assert( it != collection->begin() );
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

         const CMP cmp;

         sub_enumerator( bool _inactive, int32_t _tag, const ITERATOR& _it, const COLLECTION_POINTER _collection, const CMP& _cmp )
         : inactive( _inactive ), tag( _tag ), it_end( _collection->end() ), collection( _collection ), cmp( _cmp )
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
            return it == collection->begin();
         }

         bool end() const override
         {
            return it == it_end;
         }

         void change_status( bool val ) override
         {
            inactive = val;
         }

         bool is_inactive() const override
         {
            return inactive;
         }

         abstract_sub_enumerator< OBJECT >* create() override
         {
            return new sub_enumerator( inactive, tag, it, collection, cmp );
         }

         int32_t get_tag() const{ return tag; };

         void set_iterator( bool begin_pos ) override
         {
            if( begin_pos )
               it = collection->begin();
            else
               it = it_end;
         }
   };

   template< typename CMP, typename DIRECTION >
   struct t_complex_sorter
   {
      bool equal_allowed;
      CMP cmp;
      DIRECTION direction;

      t_complex_sorter( bool _equal_allowed, CMP _cmp, DIRECTION _direction ): equal_allowed( _equal_allowed ), cmp( _cmp ), direction( _direction ) {}

      template< typename T >
      bool operator()( const T& a, const T& b )
      {
         if( equal_allowed && ( *a == *b ) )
            return false;
         else
         {
            if( direction == DIRECTION::next )
               return cmp( *( *a ), *( *b ) );
            else
               return !cmp( *( *a ), *( *b ) );
         }
      };
   };

   template< typename OBJECT, typename CMP >
   class concatenation_iterator: public boost::bidirectional_iterator_helper
                                 <
                                    boost::multi_index::detail::bidir_node_iterator< concatenation_iterator< OBJECT, CMP > >,
                                    concatenation_iterator< OBJECT, CMP >,
                                    std::ptrdiff_t,
                                    const concatenation_iterator< OBJECT, CMP >*,
                                    const concatenation_iterator< OBJECT, CMP >&
                                 >
   {
      private:

      using base_type = boost::bidirectional_iterator_helper
                        <
                           boost::multi_index::detail::bidir_node_iterator< concatenation_iterator< OBJECT, CMP > >,
                           concatenation_iterator< OBJECT, CMP >,
                           std::ptrdiff_t,
                           const concatenation_iterator< OBJECT, CMP >*,
                           const concatenation_iterator< OBJECT, CMP >&
                        >;

         using pitem = typename abstract_sub_enumerator< OBJECT >::pself;

         modifier _modifier;

      public:

         using value_type = typename base_type::value_type;
         using difference_type = typename base_type::difference_type;

         using reference = typename abstract_sub_enumerator< OBJECT >::reference;
         using pointer = typename abstract_sub_enumerator< OBJECT >::pointer;

      private:

         using _t_complex_sorter = t_complex_sorter< CMP, Direction >;

         template< bool INACTIVE, bool POSITION >
         void add(){}

         template< bool INACTIVE, bool POSITION, typename TUPLE, typename... ELEMENTS >
         void add( const TUPLE& tuple, ELEMENTS... elements )
         {
            using t_collection_without_ref = typename std::remove_reference< decltype( std::get<0>( tuple ) ) >::type;
            using t_collection_without_ref_const = typename std::remove_const< t_collection_without_ref >::type;

            using t_without_ref = typename std::remove_reference< decltype( std::get<0>( tuple )->begin() ) >::type;
            using t_without_ref_const = typename std::remove_const< t_without_ref >::type;

            /*
               POSITION == true :   iterator is set on 'begin()' at the start
               POSITION == false:   iterator is set on 'end()' at the start
            */
            if( POSITION )
               iterators.push_back( pitem( new sub_enumerator< t_collection_without_ref_const, t_without_ref_const , OBJECT, CMP >( INACTIVE, iterators.size(), std::get<0>( tuple )->begin(), std::get<0>( tuple ), cmp ) ) );
            else
               iterators.push_back( pitem( new sub_enumerator< t_collection_without_ref_const, t_without_ref_const , OBJECT, CMP >( INACTIVE, iterators.size(), std::get<0>( tuple )->end(), std::get<0>( tuple ), cmp ) ) );

            add< INACTIVE, POSITION >( elements... );
         }

         template< typename CONTAINER, typename CONDITION >
         void find_active_iterators( CONTAINER& dst, CONDITION&& condition )
         {
            auto it = iterators.rbegin();
            auto it_end = iterators.rend();

            while( it != it_end )
            {
               if( condition( *it ) )
                  dst.emplace( *it );

               ++it;
            }
         }

         template< typename CONDITION, typename ACTION, typename COMPARISION, typename CHECKER >
         void change_direction_impl( CONDITION&& condition, ACTION&& action, COMPARISION&& comparision, CHECKER&& checker )
         {
            struct _sorter
            {
               bool operator()( const pitem& a, const pitem& b ){ return a->get_tag() < b->get_tag(); };
            };
            std::set< pitem, _sorter > row;

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
               if( item->get_tag() == idx.current )
                  current_it_active = true;
               else
               {
                  if( checker( item, current_it ) )
                     action( item );
                  else
                  {
                     auto res = comparision( item, current_it );
                     if( res )
                        action( item );
                  }
               }
            }

            if( current_it_active )
               action( iterators[ idx.current ] );
         }

         template< Direction DIRECTION >
         bool is_changed()
         {
            //Direction is still the same.
            if( direction == DIRECTION )
               return false;

            direction = DIRECTION;

            return true;
         }

         template< Direction DIRECTION >
         void change_direction()
         {
            assert( direction == DIRECTION );

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
            }
         }

         template< int32_t position, typename COLLECTION >
         void generate_idx( COLLECTION& row )
         {
            row.clear();
            find_active_iterators( row, []( const pitem& item ){ return !item->end() && !item->is_inactive(); } );

            if( row.size() < 2 )
            {
               idx.current = ( row.empty() ) ? position : ( ( *row.begin() )->get_tag() );
               return;
            }

            idx.current = ( *row.begin() )->get_tag();
            assert( !iterators[ idx.current ]->end() && !iterators[ idx.current ]->is_inactive() );
         }

         template< int32_t position, typename Comparator >
         void simple_generate_idx( const Comparator& comparator )
         {
            assert( iterators.size() == 2 );

            static auto checker =[]( const pitem& item ){ return !item->end() && !item->is_inactive(); };

            if( checker( iterators[0] ) && checker( iterators[1] ) )
            {
               if( *iterators[0] == *iterators[1] )
                  idx.current = 1;
               else
                  idx.current = comparator( iterators[0], iterators[1] )?0:1;
            }
            else if( checker( iterators[0] ) )
               idx.current = 0;
            else if( checker( iterators[1] ) )
               idx.current = 1;
            else
            {
               idx.current = position;
               return;
            }

            assert( !iterators[ idx.current ]->end() && !iterators[ idx.current ]->is_inactive() );
         }

         template< Direction DIRECTION, int32_t position, bool EQUAL_ALLOWED, template< typename... > typename COLLECTION >
         void find_impl()
         {
            assert( DIRECTION == Direction::prev || DIRECTION == Direction::next );

            if( iterators.size() == 2 )
            {
               if( DIRECTION == Direction::prev )
               {
                  static auto comparator = [this]( const pitem& a, const pitem& b ){ return !cmp( *( *a ), *( *b ) ); };
                  simple_generate_idx< position >( comparator );
               }
               else
               {
                  static auto comparator = [this]( const pitem& a, const pitem& b ){ return cmp( *( *a ), *( *b ) ); };
                  simple_generate_idx< position >( comparator );
               }
            }
            else
            {
               if( DIRECTION == Direction::prev )
               {
                  static COLLECTION< pitem, _t_complex_sorter > row( _t_complex_sorter( EQUAL_ALLOWED, cmp, Direction::prev ) );
                  generate_idx< position >( row );
               }
               else
               {
                  static COLLECTION< pitem, _t_complex_sorter > row( _t_complex_sorter( EQUAL_ALLOWED, cmp, Direction::next ) );
                  generate_idx< position >( row );
               }
            }
         }

         template< Direction DIRECTION, bool EQUAL_ALLOWED, template< typename... > typename COLLECTION >
         void find()
         {
            assert( DIRECTION == Direction::prev || DIRECTION == Direction::next );

            if( DIRECTION == Direction::next )
               find_impl< Direction::next, pos_end, EQUAL_ALLOWED, COLLECTION >();
            else
               find_impl< Direction::prev, pos_begin, EQUAL_ALLOWED, COLLECTION >();
         }

         template< bool MOVE_ALL, int32_t position, typename ACTION >
         void move_impl( ACTION&& action )
         {
            if( idx.current == position )
               return;

            assert( static_cast< size_t >( idx.current ) < iterators.size() && !iterators[ idx.current ]->end() && !iterators[ idx.current ]->is_inactive() );

            if( MOVE_ALL )
               for( size_t i = 0 ; i < iterators.size(); ++i )
               {
                  if( static_cast< int32_t >( i ) != idx.current && !iterators[ i ]->end() && !iterators[ i ]->is_inactive() && ( *iterators[ i ] ) == ( *iterators[ idx.current ] ) )
                     action( iterators[ i ] );
               }
            //Move current iterator at the end, because it is used to comparison.
            action( iterators[ idx.current ] );
         }

         template< int32_t position, Direction DIRECTION >
         void reorder()
         {
            if( idx.current == position )
            {
               if( !_modifier.empty() )
               {
                  for( size_t i = 0 ; i < iterators.size(); ++i )
                     iterators[ i ]->set_iterator( position == pos_begin );

                  _modifier.end();
               }

               return;
            }

            assert( static_cast< size_t >( idx.current ) < iterators.size() && !iterators[ idx.current ]->end() && !iterators[ idx.current ]->is_inactive() );

            if( !_modifier.start( iterators[ idx.current ], idx.current ) )
               return;

            for( size_t i = 0 ; i < iterators.size(); ++i )
            {
               if( static_cast< int32_t >( i ) != idx.current )
                  _modifier.run< DIRECTION >( cmp, iterators[ i ], iterators[ idx.current ], i, idx.current );
            }

            _modifier.end();
         }

         template< bool MOVE_ALL, Direction DIRECTION, bool REORDER >
         void move()
         {
            assert( DIRECTION == Direction::prev || DIRECTION == Direction::next );

            bool changed = is_changed< DIRECTION >();

            if( DIRECTION == Direction::next )
            {
               if( REORDER )
                  reorder< pos_begin, DIRECTION >();

               if( !changed )
                  move_impl< MOVE_ALL, pos_end >( []( const pitem& item ){ ++( *item ); } );
               else
                  change_direction< Direction::next >();
            }
            else
            {
               if( REORDER )
                  reorder< pos_end, DIRECTION >();

               if( !changed )
                  move_impl< MOVE_ALL, pos_begin >( []( const pitem& item )
                                                   {
                                                      if( item->begin() )
                                                         item->change_status( true );
                                                      else
                                                         --( *item );
                                                   }
                                                );
               else
                  change_direction< Direction::prev >();
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

         template< bool MOVE_ALL, bool EQUAL_ALLOWED, template< typename... > typename COLLECTION, Direction DIRECTION, bool REORDER >
         void action()
         {
            move< MOVE_ALL, DIRECTION, REORDER >();
            find< DIRECTION, EQUAL_ALLOWED, COLLECTION >();
         }

         concatenation_iterator( const CMP& _cmp )
         : cmp( _cmp )
         {
         }

         template< typename... ELEMENTS >
         static void prepare( concatenation_iterator& ci, bool status, Direction _direction, int32_t start_pos, ELEMENTS... elements )
         {
            ci.idx.current = start_pos;
            ci.direction = _direction;

            if( status )
               ci.add< true/*INACTIVE*/, true/*POSITION*/ >( elements... );
            else
               ci.add< false/*INACTIVE*/, false/*POSITION*/ >( elements... );
         }

         template< typename... ELEMENTS >
         static void create_end( concatenation_iterator& ci, ELEMENTS... elements )
         {
            prepare( ci, false/*status*/, Direction::next, pos_end, elements... );
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

            find< Direction::next, true/*EQUAL_ALLOWED*/, std::set/*COLLECTION*/ >();
         }

         template< typename... ELEMENTS >
         static concatenation_iterator create_end( const CMP& _cmp, ELEMENTS... elements )
         {
            concatenation_iterator ret = concatenation_iterator( _cmp );
            create_end( ret, elements... );

            return ret;
         }

         modifier& get_modifier()
         {
            return _modifier;
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
            return &( *( *( iterators[ idx.current ] ) ) );
         }

         concatenation_iterator& operator++()
         {
            action< true/*MOVE_ALL*/, true/*EQUAL_ALLOWED*/, std::set/*COLLECTION*/, Direction::next, true/*REORDER*/ >();

            return *this;
         }

         concatenation_iterator operator++( int )
         {
            auto tmp( *this );

            action< true/*MOVE_ALL*/, true/*EQUAL_ALLOWED*/, std::set/*COLLECTION*/, Direction::next, true/*REORDER*/ >();

            return tmp;
         }

         concatenation_iterator& operator--()
         {
            action< true/*MOVE_ALL*/, true/*EQUAL_ALLOWED*/, std::set/*COLLECTION*/, Direction::prev, true/*REORDER*/ >();

            return *this;
         }

         concatenation_iterator operator--( int )
         {
            auto tmp( *this );

            action< true/*MOVE_ALL*/, true/*EQUAL_ALLOWED*/, std::set/*COLLECTION*/, Direction::prev, true/*REORDER*/ >();

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

            _modifier = obj._modifier;

            return *this;
         }
   };

   template< typename OBJECT, typename CMP >
   class concatenation_reverse_iterator: public concatenation_iterator< OBJECT, CMP >
   {
      private:

         using base_class = concatenation_iterator< OBJECT, CMP >;

      protected:

         concatenation_reverse_iterator( const CMP& _cmp )
         : base_class( _cmp )
         {
         }

         template< typename... ELEMENTS >
         static void create_end( concatenation_reverse_iterator& ci, ELEMENTS... elements )
         {
            base_class::prepare( ci, true/*status*/, Direction::prev, base_class::pos_begin, elements... );
         }

      public:

         concatenation_reverse_iterator( const concatenation_iterator< OBJECT, CMP >& obj )
         : base_class( obj )
         {
            this-> template action< true/*MOVE_ALL*/, true/*EQUAL_ALLOWED*/, std::set/*COLLECTION*/, Direction::prev, false/*REORDER*/ >();
         }

         template< typename... ELEMENTS >
         concatenation_reverse_iterator( const CMP& _cmp, ELEMENTS... elements )
         : base_class( _cmp )
         {
            base_class::prepare( *this, false/*status*/, Direction::next, base_class::pos_end, elements... );

            this-> template action< true/*MOVE_ALL*/, true/*EQUAL_ALLOWED*/, std::set/*COLLECTION*/, Direction::prev, false/*REORDER*/ >();
         }

         template< typename... ELEMENTS >
         static concatenation_reverse_iterator create_end( const CMP& _cmp, ELEMENTS... elements )
         {
            concatenation_reverse_iterator ret = concatenation_reverse_iterator( _cmp );
            create_end( ret, elements... );

            return ret;
         }

         concatenation_reverse_iterator& operator++()
         {
            this-> template action< true/*MOVE_ALL*/, true/*EQUAL_ALLOWED*/, std::set/*COLLECTION*/, Direction::prev, true/*REORDER*/ >();

            return *this;
         }

         concatenation_reverse_iterator operator++( int )
         {
            auto tmp( *this );

            this-> template action< true/*MOVE_ALL*/, true/*EQUAL_ALLOWED*/, std::set/*COLLECTION*/, Direction::prev, true/*REORDER*/ >();

            return tmp;
         }

         concatenation_reverse_iterator& operator--()
         {
            this-> template action< true/*MOVE_ALL*/, true/*EQUAL_ALLOWED*/, std::set/*COLLECTION*/, Direction::next, true/*REORDER*/ >();

            return *this;
         }

         concatenation_reverse_iterator operator--( int )
         {
            auto tmp( *this );

            this-> template action< true/*MOVE_ALL*/, true/*EQUAL_ALLOWED*/, std::set/*COLLECTION*/, Direction::next, true/*REORDER*/ >();

            return tmp;
         }
   };

   template< typename OBJECT, typename CMP, template< typename ... > typename CONCATENATION_ITERATOR >
   class concatenation_iterator_proxy: public CONCATENATION_ITERATOR< OBJECT, CMP >
   {
      private:

         using base_class = CONCATENATION_ITERATOR< OBJECT, CMP >;

      private:

         using pitem = typename abstract_sub_checker::pself;

      public:

         std::vector< pitem > containers;

      private:

         void add(){}

         template< typename TUPLE, typename... ELEMENTS >
         void add( const TUPLE& tuple, ELEMENTS... elements )
         {
            using t_without_ref = typename std::remove_reference< decltype( std::get<1>( tuple ) ) >::type;
            using t_without_ref_const = typename std::remove_const< t_without_ref >::type;

            containers.push_back( pitem( new sub_checker< t_without_ref_const >( std::get<1>( tuple ) ) ) );

            add( elements... );
         }

         template< typename SRC >
         void copy_containers( const SRC& obj )
         {
            containers.clear();

            for( auto& item : obj.containers )
               containers.push_back( pitem( item->create() ) );
         }

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

      protected:

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
            
               this-> template action< false/*MOVE_ALL*/, false/*EQUAL_ALLOWED*/, std::multiset/*COLLECTION*/, DIRECTION, false/*REORDER*/ >();
            }
         }

      protected:

         template< typename T >
         concatenation_iterator_proxy( const T& obj )
         : base_class( obj )
         {
            copy_containers( obj );
         }

         concatenation_iterator_proxy( const concatenation_iterator_proxy& obj )
         : base_class( obj )
         {
            copy_containers( obj );
         }

         template< typename... ELEMENTS >
         concatenation_iterator_proxy( const CMP& _cmp, ELEMENTS... elements )
                                 : base_class( _cmp, elements... )
         {
            add( elements... );
         }

         concatenation_iterator_proxy& operator=( const concatenation_iterator_proxy& obj )
         {
            base_class::operator=( obj );

            copy_containers( obj );

            return *this;
         }

         template< Direction DIRECTION, int32_t position >
         void action_ex()
         {
            this-> template action< true/*MOVE_ALL*/, false/*EQUAL_ALLOWED*/, std::multiset/*COLLECTION*/, DIRECTION, true/*REORDER*/ >();
            find_with_key_search< DIRECTION, position >();
         }

      public:

         template< typename... ELEMENTS >
         static concatenation_iterator_proxy create_end( const CMP& _cmp, ELEMENTS... elements )
         {
            concatenation_iterator_proxy ret = concatenation_iterator_proxy( _cmp );
            base_class::create_end( ret, elements... );

            ret.add( elements... );

            return ret;
         }
   };

   template< typename OBJECT, typename CMP >
   class concatenation_iterator_ex: public concatenation_iterator_proxy< OBJECT, CMP, concatenation_iterator >
   {
      protected:

         using base_class = concatenation_iterator_proxy< OBJECT, CMP, concatenation_iterator >;
         using base_class::base_class;

      public:

         template< typename... ELEMENTS >
         concatenation_iterator_ex( const CMP& _cmp, ELEMENTS... elements )
                                 : base_class( _cmp, elements... )
         {
            this-> template find_with_key_search< Direction::next, this->pos_end >();
         }

         concatenation_iterator_ex& operator++()
         {
            this-> template action_ex< Direction::next, this->pos_end >();
            return *this;
         }

         concatenation_iterator_ex operator++( int )
         {
            auto tmp( *this );

            this-> template action_ex< Direction::next, this->pos_end >();

            return tmp;
         }

         concatenation_iterator_ex& operator--()
         {
            this-> template action_ex< Direction::prev, this->pos_begin >();
            return *this;
         }

         concatenation_iterator_ex operator--( int )
         {
            auto tmp( *this );

            this-> template action_ex< Direction::prev, this->pos_begin >();

            return tmp;
         }
   };


   template< typename OBJECT, typename CMP >
   class concatenation_reverse_iterator_ex: public concatenation_iterator_proxy< OBJECT, CMP, concatenation_reverse_iterator >
   {
      protected:

         using base_class = concatenation_iterator_proxy< OBJECT, CMP, concatenation_reverse_iterator >;
         using base_class::base_class;

      public:

         template< typename... ELEMENTS >
         concatenation_reverse_iterator_ex( const CMP& _cmp, ELEMENTS... elements )
                                 : base_class( _cmp, elements... )
         {
            this-> template find_with_key_search< Direction::prev, this->pos_begin >();
         }

         concatenation_reverse_iterator_ex& operator++()
         {
            this-> template action_ex< Direction::prev, this->pos_begin >();
            return *this;
         }

         concatenation_reverse_iterator_ex operator++( int )
         {
            auto tmp( *this );

            this-> template action_ex< Direction::prev, this->pos_begin >();

            return tmp;
         }

         concatenation_reverse_iterator_ex& operator--()
         {
            this-> template action_ex< Direction::next, this->pos_end >();
            return *this;
         }

         concatenation_reverse_iterator_ex operator--( int )
         {
            auto tmp( *this );

            this-> template action_ex< Direction::next, this->pos_end >();

            return tmp;
         }
   };

}

}

