#pragma once
#include <boost/variant.hpp>

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

template< typename ValueType, typename MiraType, typename RevMiraType, typename BmicType, typename RevBmicType >
class iterator_adapter :
   public boost::bidirectional_iterator_helper<
      iterator_adapter< ValueType, MiraType, RevMiraType, BmicType, RevBmicType >,
      ValueType,
      std::size_t,
      const ValueType*,
      const ValueType& >
{

   typedef boost::variant< MiraType, RevMiraType, BmicType, RevBmicType > iter_variant;
/*
   struct equality_visitor
   {
      const iter_variant& a;

      typedef bool result_type;

      equality_visitor( const iter_variant& _a ) : a( _a ) {}

      template< typename U >
      result_type operator()( U& b )
      {
         return b == boost::get< U >( a );
      }
   };
*/
/*
   union iter_union
   {
      MiraType mira_itr;
      BmicType bmic_itr;

      iter_union()
      {
         bmic_itr = BmicType();
      }
   };
*/
   iter_variant   _itr;
   index_type     _type = bmic;

   public:
      iterator_adapter() {}
/*
      template< typename IterType >
      iterator_adapter( const IterType& itr )
      {
         _itr = itr;
      }

      template< typename IterType >
      iterator_adapter( IterType&& itr )
      {
         _itr = std::move( itr );
      }
*/
//*
      iterator_adapter( const MiraType& mira_itr )
      {
         _itr = mira_itr;
      }

      iterator_adapter( MiraType&& mira_itr )
      {
         _itr = std::move( mira_itr );
      }

      iterator_adapter( const RevMiraType& rev_mira_itr )
      {
         _itr = rev_mira_itr;
      }

      iterator_adapter( RevMiraType&& rev_mira_itr )
      {
         _itr = std::move( rev_mira_itr );
      }

      iterator_adapter( const BmicType& bmic_itr )
      {
         _itr = bmic_itr;
      }

      iterator_adapter( BmicType&& bmic_itr )
      {
         _itr = std::move( bmic_itr );
      }

      iterator_adapter( const RevBmicType& rev_bmic_itr )
      {
         _itr = rev_bmic_itr;
      }

      iterator_adapter( RevBmicType&& rev_bmic_itr )
      {
         _itr = std::move( rev_bmic_itr );
      }
//*/
      iterator_adapter( const iterator_adapter& other )
      {
         switch( other._type )
         {
            case mira:
               _itr = other._itr;
               _type = mira;
               break;
            case bmic:
               _itr = other._itr;
               _type = bmic;
               break;
         }
      }

      iterator_adapter( iterator_adapter&& other )
      {
         switch( other._type )
         {
            case mira:
               _itr = std::move( other._itr );
               _type = mira;
               break;
            case bmic:
               _itr = std::move( other._itr );
               _type = bmic;
               break;
         }
      }

/*
      ~iterator_adapter()
      {
         switch( _type )
         {
            case mira:
               _itr.mira_itr.~MiraType();
               break;
            case bmic:
               _itr.bmic_itr.~BmicType();
               break;
         }
      }
*/
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

      bool operator ==( const iterator_adapter& other )
      {
         return _itr == other._itr;
         //return _itr.which() != other._itr.which();
/*
         if( _itr.which() != other._itr.which() ) return false;

         return boost::apply_visitor(
            equality_visitor( other ),
            _itr
         );
*/
      }

      bool operator !=( const iterator_adapter& other )
      {
         return _itr != other._itr;
         //return !( *this == other );
      }

      iterator_adapter& operator =( const iterator_adapter& other )
      {
         switch( other._type )
         {
            case mira:
               _itr = other._itr;
               _type = mira;
               break;
            case bmic:
               _itr = other._itr;
               _type = bmic;
               break;
         }

         return *this;
      }

      iterator_adapter& operator =( iterator_adapter&& other )
      {
         switch( other._type )
         {
            case mira:
               _itr = std::move( other._itr );
               _type = mira;
               break;
            case bmic:
               _itr = std::move( other._itr );
               _type = bmic;
               break;
         }

         return *this;
      }
/*
      template< typename IterType >
      iterator_adapter& operator =( const IterType& itr )
      {
         _itr = itr;
         return *this;
      }

      template< typename IterType >
      iterator_adapter& operator =( IterType&& itr )
      {
         _itr = std::move( itr );
         return *this;
      }
//*/
//*
      iterator_adapter& operator =( MiraType& mira_itr )
      {
         _itr = mira_itr;
         return *this;
      }

      iterator_adapter& operator =( MiraType&& mira_itr )
      {
         _itr = std::move( mira_itr );
         return *this;
      }

      iterator_adapter& operator =( RevMiraType& rev_mira_itr )
      {
         _itr = rev_mira_itr;
         return *this;
      }

      iterator_adapter& operator =( RevMiraType&& rev_mira_itr )
      {
         _itr = std::move( rev_mira_itr );
         return *this;
      }

      iterator_adapter& operator =( const BmicType& bmic_itr )
      {
         _itr = bmic_itr;
         _type = bmic;
         return *this;
      }

      iterator_adapter& operator =( BmicType&& bmic_itr )
      {
         _itr = std::move( bmic_itr );
         _type = bmic;
         return *this;
      }

      iterator_adapter& operator =( const RevBmicType& rev_bmic_itr )
      {
         _itr = rev_bmic_itr;
         return *this;
      }

      iterator_adapter& operator =( RevBmicType&& rev_bmic_itr )
      {
         _itr = std::move( rev_bmic_itr );
         return *this;
      }
//*/

      template< typename IterType >
      IterType& get() { return boost::get< IterType >( _itr ); }
};
/*
template< typename ValueType, typename MiraType, typename BmicType >
MiraType& iterator_adapter< ValueType, MiraType, BmicType >::as()
{
   assert( _type = mira );
   return _itr.mira_itr;
}

template< typename ValueType, typename MiraType, typename BmicType >
BmicType& iterator_adapter< ValueType, MiraType, BmicType >as()
{
   assert( _type = bmic );
   return _itr.bmic_itr;
}
*/
}
