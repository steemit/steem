#pragma once
#include <mira/index_converter.hpp>
#include <mira/abstract_iterator.hpp>

namespace mira {

enum index_type
{
   mira,
   bmic
};

template< typename Arg1, typename Arg2, typename Arg3 > struct multi_index_adapter;

template< typename MultiIndexAdapterType, typename IndexedBy >
struct index_adapter
{
   typedef typename MultiIndexAdapterType::value_type                                              value_type;
   typedef typename std::remove_reference< decltype(
      (((typename MultiIndexAdapterType::mira_type*)nullptr)->template get<IndexedBy>()) ) >::type mira_type;
   typedef iterator_adapter< value_type, decltype( (((mira_type*)nullptr)->begin()) ) >            mira_iter_adapter;
   typedef iterator_adapter< value_type, decltype( (((mira_type*)nullptr)->rbegin()) ) >           mira_rev_iter_adapter;
   typedef typename std::remove_reference< decltype(
      (((typename MultiIndexAdapterType::bmic_type*)nullptr)->template get<IndexedBy>()) ) >::type bmic_type;
   typedef iterator_adapter< value_type, decltype( (((bmic_type*)nullptr)->begin()) ) >            bmic_iter_adapter;
   typedef iterator_adapter< value_type, decltype( (((bmic_type*)nullptr)->rbegin()) ) >           bmic_rev_iter_adapter;
   typedef iterator_wrapper< value_type >                                                          iter_type;

   private:
      index_adapter() {}

   public:
      index_adapter( void* index_ptr, index_type type ) :
         _type( type )
      {
         switch( type )
         {
            case mira:
               _index = (void*) &((typename MultiIndexAdapterType::mira_type*) index_ptr)->template get< IndexedBy >();
               break;
            case bmic:
               _index = (void*) &((typename MultiIndexAdapterType::bmic_type*) index_ptr)->template get< IndexedBy >();
               break;
         }
      }

      index_adapter( const index_adapter& other ) :
         _index( other._index ),
         _type( other._type )
      {}

      index_adapter( const index_adapter&& other ) :
         _index( other._index ),
         _type( other._type )
      {}

      iter_type iterator_to( const value_type& v )const
      {
         assert( _index );
         iter_type result;

         switch( _type )
         {
            case mira:
               result = std::move( mira_iter_adapter( ((mira_type*)_index)->iterator_to( v ) ) );
               break;
            case bmic:
               result = std::move( bmic_iter_adapter( ((bmic_type*)_index)->iterator_to( v ) ) );
               break;
         }

         return result;
      }

      template< typename CompatibleKey >
      iter_type find( const CompatibleKey& k )const
      {
         assert( _index );
         iter_type result;

         switch( _type )
         {
            case mira:
               result = mira_iter_adapter( ((mira_type*)_index)->find( k ) );
               break;
            case bmic:
               result = bmic_iter_adapter( ((bmic_type*)_index)->find( k ) );
               break;
         }

         return result;
      }

      template< typename CompatibleKey >
      iter_type lower_bound( const CompatibleKey& k )const
      {
         assert( _index );
         iter_type result;

         switch( _type )
         {
            case mira:
               result = mira_iter_adapter( ((mira_type*)_index)->lower_bound( k ) );
               break;
            case bmic:
               result = bmic_iter_adapter( ((bmic_type*)_index)->lower_bound( k ) );
               break;
         }

         return result;
      }

      template< typename CompatibleKey >
      iter_type upper_bound( const CompatibleKey& k )const
      {
         assert( _index );
         iter_type result;

         switch( _type )
         {
            case mira:
               result = mira_iter_adapter( ((mira_type*)_index)->upper_bound( k ) );
               break;
            case bmic:
               result = bmic_iter_adapter( ((bmic_type*)_index)->upper_bound( k ) );
               break;
         }

         return result;
      }

      template< typename CompatibleKey >
      std::pair< iter_type, iter_type > equal_range( const CompatibleKey& k )const
      {
         assert( _index );
         std::pair< iter_type, iter_type > result;

         switch( _type )
         {
            case mira:
            {
               auto mira_result = ((mira_type*)_index)->equal_range( k );
               result.first = mira_iter_adapter( std::move( mira_result.first ) );
               result.second = mira_iter_adapter( std::move( mira_result.second ) );
               break;
            }
            case bmic:
            {
               auto bmic_result = ((bmic_type*)_index)->equal_range( k );
               result.first = bmic_iter_adapter( std::move( bmic_result.first ) );
               result.second = bmic_iter_adapter( std::move( bmic_result.second ) );
               break;
            }
         }

         return result;
      }

      iter_type begin()const
      {
         assert( _index );
         iter_type result;

         switch( _type )
         {
            case mira:
               result = mira_iter_adapter( ((mira_type*)_index)->begin() );
               break;
            case bmic:
               result = bmic_iter_adapter( ((bmic_type*)_index)->begin() );
               break;
         }

         return result;
      }

      iter_type end()const
      {
         assert( _index );
         iter_type result;

         switch( _type )
         {
            case mira:
               result = mira_iter_adapter( ((mira_type*)_index)->end() );
               break;
            case bmic:
               result = bmic_iter_adapter( ((bmic_type*)_index)->end() );
               break;
         }

         return result;
      }

      iter_type rbegin()const
      {
         assert( _index );
         iter_type result;

         switch( _type )
         {
            case mira:
               result = mira_rev_iter_adapter( ((mira_type*)_index)->rbegin() );
               break;
            case bmic:
               result = bmic_rev_iter_adapter( ((bmic_type*)_index)->rbegin() );
               break;
         }

         return result;
      }

      iter_type rend()const
      {
         assert( _index );
         iter_type result;

         switch( _type )
         {
            case mira:
               result = mira_rev_iter_adapter( ((mira_type*)_index)->rend() );
               break;
            case bmic:
               result = bmic_rev_iter_adapter( ((bmic_type*)_index)->rend() );
               break;
         }

         return result;
      }

      bool empty()const
      {
         assert( _index );
         bool result = true;

         switch( _type )
         {
            case mira:
               result = ((mira_type*)_index)->empty();
               break;
            case bmic:
               result = ((bmic_type*)_index)->empty();
               break;
         }

         return result;
      }

      size_t size()const
      {
         assert( _index );
         size_t result = 0;

         switch( _type )
         {
            case mira:
               result = ((mira_type*)_index)->size();
               break;
            case bmic:
               result = ((bmic_type*)_index)->size();
               break;
         }

         return result;
      }

   private:
      void*       _index = nullptr;
      index_type  _type  = mira;
};

template< typename Arg1, typename Arg2, typename Arg3 >
struct multi_index_adapter
{
   typedef Arg1                                                                                          value_type;
   typedef typename index_converter< multi_index::multi_index_container< Arg1, Arg2, Arg3 > >::mira_type mira_type;
   typedef typename mira_type::primary_iterator                                                          mira_iter_type;
   typedef iterator_adapter< value_type, decltype( (((mira_type*)nullptr)->begin()) ) >                  mira_iter_adapter;
   typedef typename index_converter< multi_index::multi_index_container< Arg1, Arg2, Arg3 > >::bmic_type bmic_type;
   typedef typename bmic_type::iterator                                                                  bmic_iter_type;
   typedef iterator_adapter< value_type, decltype( (((bmic_type*)nullptr)->begin()) ) >                  bmic_iter_adapter;
   typedef iterator_wrapper< value_type >                                                                iter_type;
   typedef typename bmic_type::allocator_type allocator_type;

   multi_index_adapter()
   {
      _index = (void*) new mira_type();
   }

   multi_index_adapter( allocator_type& a )
   {
      _index = (void*) new mira_type();
   }

   multi_index_adapter( index_type type )
   {
      switch( type )
      {
         case mira:
            _index = (void*) new mira_type();
            _type = mira;
            break;
         case bmic:
            _index = (void*) new bmic_type();
            _type = bmic;
            break;
      }
   }

   multi_index_adapter( index_type type, allocator_type& a )
   {
      switch( type )
      {
         case mira:
            _index = (void*) new mira_type();
            _type = mira;
            break;
         case bmic:
            _index = (void*) new bmic_type( a );
            _type = bmic;
            break;
      }
   }

   ~multi_index_adapter()
   {
      if( _index )
      {
         switch( _type )
         {
            case mira:
               delete ((mira_type*)_index);
               break;
            case bmic:
               delete ((bmic_type*)_index);
               break;
         }

         _index = nullptr;
      }
   }

   template< typename IndexedBy >
   index_adapter< multi_index_adapter< Arg1, Arg2, Arg3 >, IndexedBy > get()
   {
      assert( _index );
      return index_adapter< multi_index_adapter< Arg1, Arg2, Arg3 >, IndexedBy >( _index, _type );
   }

   template< typename IndexedBy >
   const index_adapter< multi_index_adapter< Arg1, Arg2, Arg3 >, IndexedBy > get()const
   {
      assert( _index );
      return index_adapter< multi_index_adapter< Arg1, Arg2, Arg3 >, IndexedBy >( _index, _type );
   }

   void set_index_type( index_type type, const boost::filesystem::path& p )
   {
      if( type == _type ) return;

      if( _index )
      {
         void* new_index = nullptr;

         switch( type )
         {
            case mira:
            {
               auto first = ((bmic_type*)_index)->begin();
               auto last = ((bmic_type*)_index)->end();
               new_index = (void*) new mira_type( first, last, p );
               delete ((bmic_type*)_index);
               break;
            }
            case bmic:
            {
               auto first = ((mira_type*)_index)->begin();
               auto last = ((mira_type*)_index)->end();
               new_index = (void*) new bmic_type( first, last );
               delete ((mira_type*)_index);
               break;
            }
         }

         _index = new_index;
      }
      else
      {
         switch( type )
         {
            case mira:
               _index = (void*) new mira_type();
               break;
            case bmic:
               _index = (void*) new bmic_type();
               break;
         }
      }

      _type = type;
   }

   int64_t revision()
   {
      assert( _index );
      int64_t result = 0;

      switch( _type )
      {
         case mira:
            result = ((mira_type*)_index)->revision();
            break;
         case bmic:
            result = ((bmic_type*)_index)->revision();
            break;
      }

      return result;
   }

   int64_t set_revision( int64_t rev )
   {
      assert( _index );
      int64_t result = 0;

      switch( _type )
      {
         case mira:
            result = ((mira_type*)_index)->set_revision( rev );
            break;
         case bmic:
            result = ((bmic_type*)_index)->set_revision( rev );
            break;
      }

      return result;
   }

   template< typename Constructor >
   std::pair< iter_type, bool >
   emplace( Constructor&& con, allocator_type alloc )
   {
      assert( _index );
      std::pair< iter_type, bool > result;

      switch( _type )
      {
         case mira:
         {
            auto mira_result = ((mira_type*)_index)->emplace( std::forward< Constructor>( con ), alloc );
            result.first = mira_iter_adapter( std::move( mira_result.first ) );
            result.second = mira_result.second;
            break;
         }
         case bmic:
         {
            auto bmic_result = ((bmic_type*)_index)->emplace( std::forward< Constructor>( con ), alloc );
            result.first = bmic_iter_adapter( std::move( bmic_result.first ) );
            result.second = bmic_result.second;
            break;
         }
      }

      return result;
   }

   template< typename Constructor >
   std::pair< iter_type, bool >
   emplace( Constructor&& con )
   {
      assert( _index );
      std::pair< iter_type, bool > result;

      switch( _type )
      {
         case mira:
         {
            auto mira_result = ((mira_type*)_index)->emplace( std::forward< Constructor>( con ) );
            result.first = mira_iter_adapter( std::move( mira_result.first ) );
            result.second = mira_result.second;
            break;
         }
         case bmic:
         {
            auto bmic_result = ((bmic_type*)_index)->emplace( std::forward< Constructor>( con ) );
            result.first = bmic_iter_adapter( std::move( bmic_result.first ) );
            result.second = bmic_result.second;
            break;
         }
      }

      return result;
   }

   template< typename Modifier >
   bool modify( iter_type position, Modifier&& mod )
   {
      assert( _index );
      bool result = false;

      switch( _type )
      {
         case mira:
            result = ((mira_type*)_index)->modify( position.template as< mira_iter_type >(), mod );
            break;
         case bmic:
            result = ((bmic_type*)_index)->modify( position.template as< bmic_iter_type >(), mod );
            break;
      }

      return result;
   }

   iter_type erase( iter_type position )
   {
      assert( _index );
      iter_type result;

      switch( _type )
      {
         case mira:
            result = mira_iter_adapter( ((mira_type*)_index)->erase( position.template as< mira_iter_type >() ) );
            break;
         case bmic:
            result = bmic_iter_adapter( ((bmic_type*)_index)->erase( position.template as< bmic_iter_type >() ) );
            break;
      }

      return result;
   }

   iter_type iterator_to( const value_type& v )const
   {
      assert( _index );
      iter_type result;

      switch( _type )
      {
         case mira:
            result = mira_iter_adapter( ((mira_type*)_index)->iterator_to( v ) );
            break;
         case bmic:
            result = bmic_iter_adapter( ((bmic_type*)_index)->iterator_to( v ) );
            break;
      }

      return result;
   }

   template< typename CompatibleKey >
   iter_type find( const CompatibleKey& k )const
   {
      assert( _index );
      iter_type result;

      switch( _type )
      {
         case mira:
            result = mira_iter_adapter( ((mira_type*)_index)->find( k ) );
            break;
         case bmic:
            result = bmic_iter_adapter( ((bmic_type*)_index)->find( k ) );
            break;
      }

      return result;
   }

   template< typename CompatibleKey >
   iter_type lower_bound( const CompatibleKey& k )const
   {
      assert( _index );
      iter_type result;

      switch( _type )
      {
         case mira:
            result = mira_iter_adapter( ((mira_type*)_index)->lower_bound( k ) );
            break;
         case bmic:
            result = bmic_iter_adapter( ((bmic_type*)_index)->lower_bound( k ) );
            break;
      }

      return result;
   }

   template< typename CompatibleKey >
   iter_type upper_bound( const CompatibleKey& k )const
   {
      assert( _index );
      iter_type result;

      switch( _type )
      {
         case mira:
            result = mira_iter_adapter( ((mira_type*)_index)->upper_bound( k ) );
            break;
         case bmic:
            result = bmic_iter_adapter( ((bmic_type*)_index)->upper_bound( k ) );
            break;
      }

      return result;
   }

   template< typename CompatibleKey >
   std::pair< iter_type, iter_type > equal_range( const CompatibleKey& k )const
   {
      assert( _index );
      std::pair< iter_type, iter_type > result;

      switch( _type )
      {
         case mira:
         {
            auto mira_result = ((mira_type*)_index)->equal_range( k );
            result.first = mira_iter_adapter( std::move( mira_result.first ) );
            result.second = mira_iter_adapter( std::move( mira_result.second ) );
            break;
         }
         case bmic:
         {
            auto bmic_result = ((bmic_type*)_index)->equal_range( k );
            result.first = bmic_iter_adapter( std::move( bmic_result.first ) );
            result.second = bmic_iter_adapter( std::move( bmic_result.second ) );
            break;
         }
      }

      return result;
   }

   iter_type begin()const
   {
      assert( _index );
      iter_type result;

      switch( _type )
      {
         case mira:
            result = mira_iter_adapter( ((mira_type*)_index)->begin() );
            break;
         case bmic:
            result = bmic_iter_adapter( ((bmic_type*)_index)->begin() );
            break;
      }

      return result;
   }

   iter_type end()const
   {
      assert( _index );
      iter_type result;

      switch( _type )
      {
         case mira:
            result = mira_iter_adapter( ((mira_type*)_index)->end() );
            break;
         case bmic:
            result = bmic_iter_adapter( ((bmic_type*)_index)->end() );
            break;
      }

      return result;
   }

   iter_type rbegin()const
   {
      assert( _index );
      iter_type result;

      switch( _type )
      {
         case mira:
            result = mira_rev_iter_adapter( ((mira_type*)_index)->rbegin() );
            break;
         case bmic:
            result = bmic_rev_iter_adapter( ((bmic_type*)_index)->rbegin() );
            break;
      }

      return result;
   }

   iter_type rend()const
   {
      assert( _index );
      iter_type result;

      switch( _type )
      {
         case mira:
            result = mira_rev_iter_adapter( ((mira_type*)_index)->rend() );
            break;
         case bmic:
            result = bmic_rev_iter_adapter( ((bmic_type*)_index)->rend() );
            break;
      }

      return result;
   }

   bool open( const boost::filesystem::path& p )
   {
      assert( _index );
      bool result = false;

      switch( _type )
      {
         case mira:
            result = ((mira_type*)_index)->open( p );
            break;
         case bmic:
            result = ((bmic_type*)_index)->open( p );
            break;
      }

      return result;
   }

   void close()
   {
      assert( _index );
      switch( _type )
      {
         case mira:
            ((mira_type*)_index)->close();
            break;
         case bmic:
            ((bmic_type*)_index)->close();
            break;
      }
   }

   void wipe( const boost::filesystem::path& p )
   {
      assert( _index );
      switch( _type )
      {
         case mira:
            ((mira_type*)_index)->wipe( p );
            break;
         case bmic:
            ((bmic_type*)_index)->wipe( p );
            break;
      }
   }

   void clear()
   {
      assert( _index );
      switch( _type )
      {
         case mira:
            ((mira_type*)_index)->close();
            break;
         case bmic:
            ((bmic_type*)_index)->close();
            break;
      }
   }

   void flush()
   {
      switch( _type )
      {
         case mira:
            ((mira_type*)_index)->flush();
            break;
         case bmic:
            ((bmic_type*)_index)->flush();
            break;
      }
   }

   size_t size()const
   {
      assert( _index );
      size_t result = 0;

      switch( _type )
      {
         case mira:
            result = ((mira_type*)_index)->size();
            break;
         case bmic:
            result = ((bmic_type*)_index)->size();
            break;
      }

      return result;
   }

   allocator_type get_allocator()const
   {
      return allocator_type();
   }

   template< typename MetaKey, typename MetaValue >
   bool put_metadata( const MetaKey& k, const MetaValue& v )
   {
      assert( _index );
      bool result = false;

      switch( _type )
      {
         case mira:
            result = ((mira_type*)_index)->put_metadata( k, v );
            break;
         case bmic:
            result = ((bmic_type*)_index)->put_metadata( k, v );
            break;
      }

      return result;
   }

   template< typename MetaKey, typename MetaValue >
   bool get_metadata( const MetaKey& k, MetaValue& v )
   {
      assert( _index );
      bool result = false;

      switch( _type )
      {
         case mira:
            result = ((mira_type*)_index)->get_metadata( k, v );
            break;
         case bmic:
            result = ((bmic_type*)_index)->get_metadata( k, v );
            break;
      }

      return result;
   }

   size_t get_cache_usage()const
   {
      assert( _index );
      size_t result = 0;

      switch( _type )
      {
         case mira:
            result = ((mira_type*)_index)->get_cache_usage();
            break;
         case bmic:
            result = ((bmic_type*)_index)->get_cache_usage();
            break;
      }

      return result;
   }

   size_t get_cache_size()const
   {
      assert( _index );
      size_t result = 0;

      switch( _type )
      {
         case mira:
            result = ((mira_type*)_index)->get_cache_size();
            break;
         case bmic:
            result = ((bmic_type*)_index)->get_cache_size();
            break;
      }

      return result;
   }

   void dump_lb_call_counts()
   {
      assert( _index );
      switch( _type )
      {
         case mira:
            ((mira_type*)_index)->dump_lb_call_counts();
            break;
         case bmic:
            ((bmic_type*)_index)->dump_lb_call_counts();
            break;
      }
   }

   void trim_cache( size_t cap )
   {
      assert( _index );
      switch( _type )
      {
         case mira:
            ((mira_type*)_index)->trim_cache( cap );
            break;
         case bmic:
            ((bmic_type*)_index)->trim_cache( cap );
            break;
      }
   }

   void print_stats()const
   {
      assert( _index );
      switch( _type )
      {
         case mira:
            ((mira_type*)_index)->print_stats();
            break;
         case bmic:
            ((bmic_type*)_index)->print_stats();
            break;
      }
   }

   private:
      void*       _index = nullptr;
      index_type  _type  = mira;
};



}
