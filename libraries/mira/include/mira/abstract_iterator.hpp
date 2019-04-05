#pragma once
#include <memory>

namespace mira {

enum index_type
{
   mira,
   bmic
};

//fwd declaration
//template< typename ValueType > class iterator_wrapper;
/*
template< typename ValueType >
class abstract_iterator
{
   public:
      typedef abstract_iterator< ValueType > abs_iter_type;

      virtual ~abstract_iterator() {}

      virtual abs_iter_type& operator ++()    = 0;
      virtual abs_iter_type& operator --()    = 0;

      virtual const ValueType& operator  *() const = 0;
      virtual const ValueType* operator ->() const = 0;

      virtual bool operator ==( const abs_iter_type& )const = 0;
      virtual bool operator !=( const abs_iter_type& )const = 0;

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

   public:
      typedef ValueType                         value_type;
      typedef IterType                          iter_type;
      typedef abstract_iterator< value_type >   abs_iter_type;

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
};*/

template< typename ValueType, typename MiraType, typename BmicType >
class iterator_wrapper :
   public boost::bidirectional_iterator_helper<
      iterator_wrapper< ValueType, MiraType, BmicType >,
      ValueType,
      std::size_t,
      const ValueType*,
      const ValueType& >
{
   union iter_union
   {
      MiraType mira_iter;
      BmicType bmic_iter;
   };

   iter_union _iter;
   index_type _type = mira;

   //typedef abstract_iterator< ValueType > abs_iter_type;

   iterator_wrapper() {}

   public:
      iterator_wrapper( const MiraType& mira_iter )
      {
         _iter.mira_iter = mira_iter;
         _type = mira;
      }

      iterator_wrapper( MiraType&& mira_iter )
      {
         _iter.mira_iter = std::move( mira_iter );
         _type = mira;
      }

      iterator_wrapper( const BmicType& bmic_iter )
      {
         _iter.bmic_iter = bmic_iter;
         _type = bmic;
      }

      iterator_wrapper( BmicType&& bmic_iter )
      {
         _iter.bmic_iter = std::move( bmic_iter );
         _type = bmic;
      }

      iterator_wrapper( const iterator_wrapper& other )
      {
         switch( other._type )
         {
            case mira:
               _iter.mira_iter = other._iter.mira_iter;
               _type = mira;
               break;
            case bmic:
               _iter.bmic_iter = other._iter.bmic_iter;
               _type = bmic;
               break;
         }
      }

      iterator_wrapper( iterator_wrapper&& other )
      {
         switch( other._type )
         {
            case mira:
               _iter.mira_iter = std::move( other._iter.mira_iter );
               _type = mira;
               break;
            case bmic:
               _iter.bmic_iter = std::move( other._iter.bmic_iter );
               _type = bmic;
               break;
         }
      }

      iterator_wrapper& operator ++()
      {
         switch( _type )
         {
            case mira:
               _iter.mira_iter++();
               break;
            case bmic:
               _iter.bmic_iter++();
               break;
         }

         return *this;
      }

      iterator_wrapper operator ++(int)const
      {
         iterator_wrapper copy( *this );
         ++copy;
         return copy;
      }

      iterator_wrapper& operator --()
      {
         switch( _type )
         {
            case mira:
               _iter.mira_iter--();
               break;
            case bmic:
               _iter.bmic_iter--();
               break;
         }

         return *this;
      }

      iterator_wrapper operator --(int)const
      {
         iterator_wrapper copy( *this );
         --copy;
         return copy;
      }

      const ValueType& operator *()const
      {
         return *(operator->());
      }

      const ValueType* operator ->()const
      {
         ValueType* result = nullptr;

         switch( _type )
         {
            case mira:
               result = _iter.mira_iter.operator->();
               break;
            case bmic:
               result = _iter.bmic_iter.operator->();
               break;
         }

         return result;
      }

      bool operator ==( const iterator_wrapper& other )
      {
         bool result = false;

         if( _type == other._type )
         {
            switch( _type )
            {
               case mira:
                  result = _iter.mira_iter == other._iter.mira_iter;
                  break;
               case bmic:
                  result = _iter.bmic_iter == other._iter.mira_iter;
                  break;
            }
         }

         return result;
      }

      bool operator !=( const iterator_wrapper& other )
      {
         bool result = false;

         if( _type == other._type )
         {
            switch( _type )
            {
               case mira:
                  result = _iter.mira_iter != other._iter.mira_iter;
                  break;
               case bmic:
                  result = _iter.bmic_iter != other._iter.mira_iter;
                  break;
            }
         }

         return result;
      }

      iterator_wrapper& operator =( const iterator_wrapper& other )
      {
         switch( other._type )
         {
            case mira:
               _iter.mira_iter = other._iter.mira_iter;
               _type = mira;
               break;
            case bmic:
               _iter.bmic_iter = other._iter.bmic_iter;
               _type = bmic;
               break;
         }

         return *this;
      }

      iterator_wrapper& operator =( iterator_wrapper&& other )
      {
         switch( other._type )
         {
            case mira:
               _iter.mira_iter = std::move( other._iter.mira_iter );
               _type = mira;
               break;
            case bmic:
               _iter.bmic_iter = std::move( other._iter.bmic_iter );
               _type = bmic;
               break;
         }

         return *this;
      }

      iterator_wrapper& operator =( const MiraType& mira_iter )
      {
         _iter.mira_iter = mira_iter;
         _type = mira;
      }

      iterator_wrapper& operator =( MiraType&& mira_iter )
      {
         _iter.mira_iter = std::move( mira_iter );
         _type = mira;
      }

      iterator_wrapper& operator =( const BmicType& bmic_iter )
      {
         _iter.bmic_iter = bmic_iter;
         _type = bmic;
      }

      iterator_wrapper& operator =( BmicType&& bmic_iter )
      {
         _iter.bmic_iter = std::move( bmic_iter );
         _type = bmic;
      }

      //template< typename IndexType > IndexType& as();
};
/*
template< typename ValueType, typename MiraType, typename BmicType >
MiraType& iterator_wrapper< ValueType, MiraType, BmicType >::as()
{
   assert( _type = mira );
   return _iter.mira_iter;
}

template< typename ValueType, typename MiraType, typename BmicType >
BmicType& iterator_wrapper< ValueType, MiraType, BmicType >as()
{
   assert( _type = bmic );
   return _iter.bmic_iter;
}
*/
}
