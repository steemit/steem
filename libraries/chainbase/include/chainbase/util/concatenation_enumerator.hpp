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

         virtual void inc() = 0;

      public:

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
         virtual bool finished() const = 0;
   };

   template< typename ITERATOR, typename OBJECT, typename CMP >
   class sub_enumerator: public abstract_sub_enumerator< OBJECT >
   {
      public:

         using reference = typename abstract_sub_enumerator< OBJECT >::reference;
         using pointer = typename abstract_sub_enumerator< OBJECT >::pointer;
 
      protected:

         void inc() override
         {
            assert( it != end );
            ++it;
         }

      public:

         ITERATOR it;
         ITERATOR end;

         const CMP cmp;

         sub_enumerator( const ITERATOR& _it, const ITERATOR& _end, const CMP& _cmp )
         : end( _end ), cmp( _cmp )
         {
            it = _it;
         }

         size_t get_id() const override
         {
            assert( it != end );
            return size_t( it->id );
         }

         sub_enumerator operator++()
         {
            inc();
            return *this;
         }

         reference operator*() const override
         {
            assert( it != end );
            return *it;
         }

         pointer operator->() const override
         {
            assert( it != end );
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

         bool finished() const override
         {
            return it == end;
         }
   };

   template< typename OBJECT, typename CMP >
   class concatenation_enumerator
   {
      private:

         using pitem = typename abstract_sub_enumerator< OBJECT >::pself;

         using reference = typename abstract_sub_enumerator< OBJECT >::reference;
         using pointer = typename abstract_sub_enumerator< OBJECT >::pointer;

         CMP cmp;

      protected:

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

         int32_t find_next()
         {
            using pair = std::pair< pitem, int32_t >;

            std::vector< pair > row;

            int32_t cnt = 0;
            for( auto& item : iterators )
            {
               if( !item->finished() )
                  row.emplace_back( std::make_pair( item, cnt ) );
               ++cnt;
            }

            switch( row.size() )
            {
               case 0: return -1;
               case 1: return row[0].second;
               default:
               {
                  std::sort( row.begin(), row.end(),
                              [ this ]( const pair& a, const pair& b )
                              {
                                 if( *( a.first ) == *( b.first ) )
                                    return a.second > b.second;
                                 else
                                    return cmp( *( *a.first ), *( *b.first ) );
                              }
                           );
               } break;
            }

            auto ret = row.begin()->second;
            assert( !iterators[ ret ]->finished() );

            return ret;
         }

         void move( int32_t idx )
         {
            if( idx < 0 )
               return;

            assert( static_cast< size_t >( idx ) < iterators.size() && !iterators[ idx ]->finished() );

            int32_t cnt = 0;
            for( size_t i = 0 ; i < iterators.size(); ++i )
            {
               if( cnt != idx && !iterators[ cnt ]->finished() && ( *iterators[ cnt ] ) == ( *iterators[ idx ] ) )
                  ++( *iterators[ cnt ] );

               ++cnt;
            }
            //Move given iterator at the end, because it is used to comparision.
            ++( *iterators[ idx ] );
         }

      public:

         template< typename... ELEMENTS >
         concatenation_enumerator( const CMP& _cmp, ELEMENTS... elements )
         : cmp( _cmp )
         {
            add( elements... );

            idx.current = find_next();
         }

         reference operator*() const
         {
            assert( idx.current != -1 );
            return *( *( iterators[ idx.current ] ) );
         }

         pointer operator->() const
         {
            assert( idx.current != -1 );
            return &( *( iterators[ idx.current ] ) );
         }

         concatenation_enumerator& operator++()
         {
            move( idx.current );
            idx.current = find_next();

            return *this;
         }

         bool finished() const
         {
            return idx.current == -1;
         }
   };

   template< typename OBJECT, typename CMP >
   class concatenation_enumerator_ex: public concatenation_enumerator< OBJECT, CMP >
   {
      private:

         using pitem = typename abstract_sub_checker::pself;

         std::vector< pitem > containers;

      protected:

         bool find_next_with_key_search_impl( size_t key )
         {
            assert( this->idx.current != -1 );

            for( size_t i = this->idx.current + 1; i < containers.size(); ++i )
            {
               if( containers[ i ]->exists( key ) )
                  return true;
            }

            return false;
         }

         void find_next_with_key_search()
         {
            if( this->idx.current == -1 )
               return;

            size_t key;

            while( this->idx.current != -1 )
            {
               key = this->iterators[ this->idx.current ]->get_id();

               if( !find_next_with_key_search_impl( key ) )
                  break;
            
               this->move( this->idx.current );

               this->idx.current = this->find_next();
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
         this->move( this->idx.current );

         this->idx.current = this->find_next();
         find_next_with_key_search();

         return *this;
      }

   };

}

}

