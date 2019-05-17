#pragma once
#include <boost/variant.hpp>

#include <memory>

namespace mira {

template< typename ValueType, typename... Iters >
class iterator_adapter :
   public boost::bidirectional_iterator_helper<
      iterator_adapter< ValueType, Iters... >,
      ValueType,
      std::size_t,
      const ValueType*,
      const ValueType& >
{
   typedef boost::variant< Iters... > iter_variant;

   template< typename t > struct type {};

   public:
      iter_variant _itr;

      iterator_adapter() {}

      template< typename T >
      iterator_adapter( T& rhs )
      {
         assignment_impl( rhs, type< typename std::remove_reference< T >::type >() );
      }

      template< typename T >
      iterator_adapter( const T& rhs )
      {
         assignment_impl( rhs, type< typename std::remove_reference< T >::type >() );
      }

      template< typename T >
      iterator_adapter( T&& rhs )
      {
         assignment_impl( std::move( rhs ), type< typename std::remove_reference< T >::type >() );
      }

      iterator_adapter& operator ++()
      {
         boost::apply_visitor(
            []( auto& itr ){ ++itr; },
            _itr
         );

         return *this;
      }

      iterator_adapter operator ++(int)const
      {
         return boost::apply_visitor(
            []( auto& itr ){ return iterator_adapter( itr++ ); },
            _itr
         );
      }

      iterator_adapter& operator --()
      {
         boost::apply_visitor(
            []( auto& itr ){ --itr; },
            _itr
         );

         return *this;
      }

      iterator_adapter operator --(int)const
      {
         return boost::apply_visitor(
            []( auto& itr ){ return iterator_adapter( itr-- ); },
            _itr
         );
      }

      const ValueType& operator *()
      {
         return *(operator ->());
      }

      const ValueType* operator ->()
      {
         return boost::apply_visitor(
            []( auto& itr ){ return itr.operator->(); },
            _itr
         );
      }

      template< typename T >
      iterator_adapter& operator =( T& rhs )
      {
         assignment_impl( rhs, type< typename std::remove_reference< T >::type >() );

         return *this;
      }

      template< typename T >
      iterator_adapter& operator =( const T& rhs )
      {
         assignment_impl( rhs, type< typename std::remove_reference< T >::type >() );

         return *this;
      }

      template< typename T >
      iterator_adapter& operator =( T&& rhs )
      {
         assignment_impl( std::move( rhs ), type< typename std::remove_reference< T >::type >() );

         return *this;
      }

      template< typename IterType >
      IterType& get() { return boost::get< IterType >( _itr ); }

   private:
      void assignment_impl( iterator_adapter& rhs, type< iterator_adapter > )
      {
         _itr = rhs._itr;
      }

      template< typename T >
      void assignment_impl( T& rhs, type< T > )
      {
         _itr = rhs;
      }

      void assignment_impl( const iterator_adapter& rhs, type< iterator_adapter > )
      {
         _itr = rhs._itr;
      }

      template< typename T >
      void assignment_impl( const T& rhs, type< T > )
      {
         _itr = rhs;
      }

      void assignment_impl( iterator_adapter&& rhs, type< iterator_adapter > )
      {
         _itr = std::move( rhs._itr );
      }

      template< typename T >
      void assignment_impl( T&& rhs, type< T > )
      {
         _itr = std::move( rhs );
      }
};

template< typename ValueType, typename... Iters >
bool operator ==( const iterator_adapter< ValueType, Iters... >& lhs, const iterator_adapter< ValueType, Iters... >& rhs )
{
   return lhs._itr == rhs._itr;
}

template< typename ValueType, typename... Iters >
bool operator !=( const iterator_adapter< ValueType, Iters... >& lhs, const iterator_adapter< ValueType, Iters... >& rhs )
{
   return !( lhs == rhs );
}

} // mira
