#pragma once
#include <memory>

namespace mira {

//fwd declaration
template< typename ValueType > class iterator_wrapper;

template< typename ValueType >
class abstract_iterator
{
   public:
      typedef abstract_iterator< ValueType > abs_iter_type;

      virtual ~abstract_iterator() {}

      virtual abs_iter_type& operator ++()    = 0; //{ assert(false); return *this; }
      //virtual abs_iter_type& operator ++(int) = 0; //{ assert(false); return *this; }
      virtual abs_iter_type& operator --()    = 0; //{ assert(false); return *this; }
      //virtual abs_iter_type& operator --(int) = 0; //{ assert(false); return *this; }

      //virtual       ValueType& operator  *()       { assert(false); static ValueType v; return v; }
      //virtual       ValueType* operator ->()       { assert(false); static ValueType v; return &v; }
      virtual const ValueType& operator  *() const = 0;// { assert(false); static const ValueType v; return v; }
      virtual const ValueType* operator ->() const = 0;// { assert(false); static const ValueType v; return &v; }

      virtual bool operator ==( const abs_iter_type& )const = 0;// { assert(false); return false; }
      virtual bool operator !=( const abs_iter_type& )const = 0;// { assert(false); return false; }

      //virtual abs_iter_type& operator =( abs_iter_type& ) { assert(false); return *this; }

      template< typename IterType >
      IterType& as() { return *(IterType*)_iter; }

   protected:
      friend class iterator_wrapper< ValueType >;
      virtual abs_iter_type* copy()const { assert(false); return (abs_iter_type*)nullptr; }

      void* _iter = nullptr;
};

template< typename ValueType, typename IterType >
class iterator_adapter : public abstract_iterator< ValueType >
{
   #define ITERATOR (*(iter_type*)_iter)

   //typedef decltype( ((MultiIndexType*)nullptr)->begin() ) iter_type;
   //typedef typename MultiIndexType::value_type  value_type;

   public:
      //typedef decltype( ((MultiIndexType).begin()) ) iter_type;

      //typedef typename MultiIndexType::iterator    iter_type;
      //typedef typename iter_type::foo bar;

      typedef ValueType value_type;
      typedef IterType iter_type;
      typedef abstract_iterator< value_type >      abs_iter_type;

      using abstract_iterator< value_type >::_iter;

      iterator_adapter( iter_type&& iter )
      {
         _iter = new iter_type( iter );
      }

      iterator_adapter( const iter_type&& iter )
      {
         _iter = new iter_type( iter );
      }

      iterator_adapter( iterator_adapter& other )
      {
         _iter = new iter_type( (*(iter_type*)(other._iter)) );
      }

      iterator_adapter( const iterator_adapter& other )
      {
         _iter = new iter_type( (*(iter_type*)(other._iter)) );
      }

      iterator_adapter( iterator_adapter&& other )
      {
         _iter = other._iter;
         other._iter = nullptr;
      }

      ~iterator_adapter()
      {
         if( _iter )
         {
            delete ((iter_type*)_iter);
            _iter = nullptr;
         }
      }

      iterator_adapter& operator =( iterator_adapter&& other )
      {
         _iter = other._iter;
         other._iter = nullptr;
         return *this;
      }

      abs_iter_type& operator ++() override
      {
         assert( _iter );
         ++ITERATOR;
         return *this;
      }

      abs_iter_type& operator --() override
      {
         assert( _iter );
         --ITERATOR;
         return *this;
      }

      const value_type& operator *()const override
      {
         assert( _iter );
         return *ITERATOR;
      }

      const value_type* operator ->()const override
      {
         assert( _iter );
         return &(*ITERATOR);
      }

      bool operator ==( const abs_iter_type& other )const override
      {
         assert( _iter );
         if( dynamic_cast< const iterator_adapter* >( &other ) != nullptr )
         {
            return ITERATOR == (*(iter_type*)(static_cast< const iterator_adapter* >( &other )->_iter));
         }

         return false;
      }

      bool operator !=( const abs_iter_type& other )const override
      {
         return !(*this == other);
      }

      abs_iter_type* copy()const override
      {
         // Yeah, this is really dangerous, but should only be called via iterator_wrapper, which will
         // immediately store it in a smart pointer.
         return new iterator_adapter( *this );
      }

      iter_type& iterator()
      {
         return ITERATOR;
      }
};

template< typename ValueType >
class iterator_wrapper :
   public boost::bidirectional_iterator_helper<
      iterator_wrapper< ValueType >,
      ValueType,
      std::size_t,
      const ValueType*,
      const ValueType& >
{
   typedef abstract_iterator< ValueType > abs_iter_type;

   public:
      iterator_wrapper() {}

      iterator_wrapper( abs_iter_type& iter )
      {
         _abs_iter.reset( iter.copy() );
      }

      iterator_wrapper( abs_iter_type&& iter )
      {
         _abs_iter = std::move( iter );
      }

      iterator_wrapper( const iterator_wrapper& other )
      {
         _abs_iter.reset( other._abs_iter->copy() );
      }

      iterator_wrapper( iterator_wrapper&& other )
      {
         _abs_iter = std::move( other._abs_iter );
         other._abs_iter.reset();
      }

      abstract_iterator< ValueType >* iter()const
      {
         return &(*_abs_iter);
      }

      iterator_wrapper& operator ++()
      {
         _abs_iter->operator++();
         return *this;
      }

      iterator_wrapper operator ++(int)const
      {
         iterator_wrapper copy;
         copy._abs_iter.reset( _abs_iter->copy() );
         ++copy;
         return copy;
      }

      iterator_wrapper& operator --()
      {
         _abs_iter->operator--();
         return *this;
      }

      iterator_wrapper operator --(int)const
      {
         iterator_wrapper copy;
         copy._abs_iter.reset( _abs_iter->copy() );
         --copy;
         return copy;
      }

      const ValueType& operator *()const
      {
         return _abs_iter->operator*();
      }

      const ValueType* operator ->()const
      {
         return _abs_iter->operator->();
      }

      bool operator ==( const iterator_wrapper& other )
      {
         return _abs_iter->operator==( *other._abs_iter );
      }

      bool operator !=( const iterator_wrapper& other )
      {
         return _abs_iter->operator!=( *other._abs_iter );
      }

      iterator_wrapper& operator =( iterator_wrapper& other )
      {
         _abs_iter = _abs_iter.reset( other._abs_iter->copy() );
         return *this;
      }

      iterator_wrapper& operator =( iterator_wrapper&& other )
      {
         _abs_iter = std::move( other._abs_iter );
         other._abs_iter.reset();
         return *this;
      }

      template< typename IterType >
      iterator_wrapper& operator =( iterator_adapter< ValueType, IterType >&& adapter )
      {
         _abs_iter.reset( new iterator_adapter< ValueType, IterType >( adapter ) );
         return *this;
      }

      template< typename IterType >
      IterType& as() { return _abs_iter->template as< IterType >(); }

   private:
      std::unique_ptr< abstract_iterator< ValueType > > _abs_iter;
};

}
