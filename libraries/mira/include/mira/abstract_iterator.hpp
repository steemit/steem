namespace mira {

struct null_type{};

template< typename ValueType >
class abstract_iterator
{
   public:
      typedef abstract_iterator< ValueType > abs_iter_type;

      virtual ~abstract_iterator() {}

      virtual abs_iter_type& operator ++()    { assert(false); return *this; }
      virtual abs_iter_type  operator ++(int) { assert(false); return *this; }
      virtual abs_iter_type& operator --()    { assert(false); return *this; }
      virtual abs_iter_type  operator --(int) { assert(false); return *this; }

      //virtual       ValueType& operator  *()       { assert(false); static ValueType v; return v; }
      //virtual       ValueType* operator ->()       { assert(false); static ValueType v; return &v; }
      virtual const ValueType& operator  *() const { assert(false); static const ValueType v; return v; }
      virtual const ValueType* operator ->() const { assert(false); static const ValueType v; return &v; }

      virtual bool operator ==( const abs_iter_type& ) { assert(false); return false; }
      virtual bool operator !=( const abs_iter_type& ) { assert(false); return false; }

      //virtual abs_iter_type& operator =( abs_iter_type& ) { assert(false); return *this; }

      template< typename IterType >
      IterType& as() { return *(IterType*)_iter; }

   protected:
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

      /*abs_iter_type& operator =( abs_iter_type& other )override
      {
         assert( dynamic_cast< const iterator_adapter* >( &other ) != nullptr );
         auto o = static_cast< const iterator_adapter* >( &other );
         _iter = o->_iter;
         o->_iter = nullptr;
         return *this;
      }*/

      iterator_adapter& operator =( iterator_adapter&& other )
      {
         _iter = other._iter;
         other._iter = nullptr;
         return *this;
      }

      abs_iter_type& operator ++() override
      {
         ++ITERATOR;
         return *this;
      }

      abs_iter_type operator ++(int) override
      {
         return iterator_adapter( ITERATOR++ );
      }

      abs_iter_type& operator --() override
      {
         --ITERATOR;
         return *this;
      }

      abs_iter_type operator --(int) override
      {
         return iterator_adapter( ITERATOR-- );
      }

      /*
      value_type& operator *() override
      {
         return *ITERATOR;
      }

      value_type* operator ->() override
      {
         return &(*ITERATOR);
      }
      */

      const value_type& operator *()const override
      {
         return *ITERATOR;
      }

      const value_type* operator ->()const override
      {
         return &(*ITERATOR);
      }

      bool operator ==( const abs_iter_type& other ) override
      {
         if( dynamic_cast< const iterator_adapter* >( &other ) != nullptr )
         {
            return ITERATOR == (*(iter_type*)(static_cast< const iterator_adapter* >( &other )->_iter));
         }

         return false;
      }

      bool operator !=( const abs_iter_type& other ) override
      {
         return !(*this == other);
      }

      iter_type& iterator()
      {
         return ITERATOR;
      }
};
/*
template< typename ValueType, typename IndexedBy = null_type >
class iterator_wrapper
{
   public:
      iterator_wrapper() = delete;
      iterator_wrapper( abstract_iterator< ValueType, IndexedBy >&& iter )
      {
         _iter = new abstract_iterator< ValueType, IndexedBy >
      }

      iterator_wrapper< ValueType >& operator ++() { _iter->++(); return *this; }
      iterator_wrapper< ValueType >& operator --() { _iter->--(); return *this;}

      const ValueType& operator  *() { return _iter->*(); }
      const ValueType* operator ->() { return _iter->->(); }

      bool operator ==( const iterator_wrapper< ValueType >& other ) { return _iter->==( *(other._iter) ); }
      bool operator !=( const iterator_wrapper< ValueType >& other ) { return *this != other; }

   private:
      abstract_iterator< ValueType, IndexedBy >* _iter;
};
*/

}
