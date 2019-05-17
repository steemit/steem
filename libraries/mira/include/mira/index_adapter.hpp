#pragma once
#include <mira/index_converter.hpp>
#include <mira/iterator_adapter.hpp>

namespace mira {

enum index_type
{
   mira,
   bmic
};

template< typename MultiIndexAdapterType, typename IndexedBy >
struct index_adapter
{
   typedef typename MultiIndexAdapterType::value_type                                              value_type;
   typedef typename std::remove_reference< decltype(
      (((typename MultiIndexAdapterType::mira_type*)nullptr)->template get<IndexedBy>()) ) >::type mira_type;
   typedef typename std::remove_reference< decltype(
      (((typename MultiIndexAdapterType::bmic_type*)nullptr)->template get<IndexedBy>()) ) >::type bmic_type;
   typedef iterator_adapter<
      value_type,
      decltype( (((mira_type*)nullptr)->begin()) ),
      decltype( (((mira_type*)nullptr)->rbegin()) ),
      decltype( (((bmic_type*)nullptr)->begin()) ),
      decltype( (((bmic_type*)nullptr)->rbegin()) ) >                                              iter_type;
   typedef iter_type                                                                               rev_iter_type;

   typedef boost::variant< mira_type*, bmic_type* > index_variant;

   private:
      index_adapter() {}

   public:
      index_adapter( const mira_type& mira_index )
      {
         _index = const_cast< mira_type* >( &mira_index );
      }

      index_adapter( const bmic_type& bmic_index )
      {
         _index = const_cast< bmic_type* >( &bmic_index );
      }

      index_adapter( const index_adapter& other ) :
         _index( other._index )
      {}

      index_adapter( index_adapter&& other ) :
         _index( std::move( other._index ) )
      {}

      iter_type iterator_to( const value_type& v )const
      {
         return boost::apply_visitor(
         [&]( auto* index ){ return iter_type( index->iterator_to( v ) ); },
         _index
      );
      }

      template< typename CompatibleKey >
      iter_type find( const CompatibleKey& k )const
      {
         return boost::apply_visitor(
            [&k]( auto* index ){ return iter_type( index->find( k ) ); },
            _index
         );
      }

      template< typename CompatibleKey >
      iter_type lower_bound( const CompatibleKey& k )const
      {
         return boost::apply_visitor(
            [&k]( auto* index ){ return iter_type( index->lower_bound( k ) ); },
            _index
         );
      }

      template< typename CompatibleKey >
      iter_type upper_bound( const CompatibleKey& k )const
      {
         return boost::apply_visitor(
            [&k]( auto* index ){ return iter_type( index->upper_bound( k ) ); },
            _index
         );
      }

      template< typename CompatibleKey >
      std::pair< iter_type, iter_type > equal_range( const CompatibleKey& k )const
      {
         return boost::apply_visitor(
            [&k]( auto* index )
            {
               auto result = index->equal_range( k );
               return std::pair< iter_type, iter_type >(
                  std::move( result.first ),
                  std::move( result.second ) );
            },
            _index
         );
      }

      iter_type begin()const
      {
         return boost::apply_visitor(
            []( auto* index ){ return iter_type( index->begin() ); },
            _index
         );
      }

      iter_type end()const
      {
         return boost::apply_visitor(
            []( auto* index ){ return iter_type( index->end() ); },
            _index
         );
      }

      rev_iter_type rbegin()const
      {
         return boost::apply_visitor(
            []( auto* index ){ return rev_iter_type( index->rbegin() ); },
            _index
         );
      }

      rev_iter_type rend()const
      {
         return boost::apply_visitor(
            []( auto* index ){ return rev_iter_type( index->rend() ); },
            _index
         );
      }

      bool empty()const
      {
         return boost::apply_visitor(
            []( auto* index ){ return index->empty(); },
            _index
         );
      }

      size_t size()const
      {
         return boost::apply_visitor(
            []( auto* index ){ return index->size(); },
            _index
         );
      }

   private:
      index_variant _index;
};

template< typename Arg1, typename Arg2, typename Arg3 >
struct multi_index_adapter
{
   typedef Arg1                                                                                          value_type;
   typedef typename index_converter< multi_index::multi_index_container< Arg1, Arg2, Arg3 > >::mira_type mira_type;
   typedef typename mira_type::primary_iterator                                                          mira_iter_type;
   typedef typename index_converter< multi_index::multi_index_container< Arg1, Arg2, Arg3 > >::bmic_type bmic_type;
   typedef typename bmic_type::iterator                                                                  bmic_iter_type;
   typedef typename value_type::id_type id_type;

   typedef iterator_adapter<
      value_type,
      decltype( (((mira_type*)nullptr)->begin()) ),
      decltype( (((mira_type*)nullptr)->rbegin()) ),
      decltype( (((bmic_type*)nullptr)->begin()) ),
      decltype( (((bmic_type*)nullptr)->rbegin()) ) >                                                    iter_type;
   typedef iter_type                                                                                     rev_iter_type;

   typedef typename bmic_type::allocator_type                                                         allocator_type;

   typedef boost::variant<
      mira_type,
      bmic_type > index_variant;

   multi_index_adapter()
   {
      _index = mira_type();
   }

   multi_index_adapter( allocator_type& a )
   {
      _index = mira_type();
   }

   multi_index_adapter( index_type type ) :
      _type( type )
   {
      switch( _type )
      {
         case mira:
            _index = mira_type();
            break;
         case bmic:
            _index = bmic_type();
            break;
      }
   }

   multi_index_adapter( index_type type, allocator_type& a ) :
      _type( type )
   {
      switch( _type )
      {
         case mira:
            _index = mira_type();
            break;
         case bmic:
            _index = bmic_type( a );
            break;
      }
   }

   ~multi_index_adapter() {}

   template< typename IndexedBy >
   index_adapter< multi_index_adapter< Arg1, Arg2, Arg3 >, IndexedBy > get()
   {
      return boost::apply_visitor(
         []( auto& index )
         {
            return index_adapter< multi_index_adapter< Arg1, Arg2, Arg3 >, IndexedBy >(
               index.template get< IndexedBy >()
            );
         },
         _index
      );
   }

   template< typename IndexedBy >
   index_adapter< multi_index_adapter< Arg1, Arg2, Arg3 >, IndexedBy > mutable_get()
   {
      return boost::apply_visitor(
         []( auto& index )
         {
            return index_adapter< multi_index_adapter< Arg1, Arg2, Arg3 >, IndexedBy >(
               index.template get< IndexedBy >()
            );
         },
         _index
      );
   }

   template< typename IndexedBy >
   const index_adapter< multi_index_adapter< Arg1, Arg2, Arg3 >, IndexedBy > get()const
   {
      return boost::apply_visitor(
         []( const auto& index )
         {
            return index_adapter< multi_index_adapter< Arg1, Arg2, Arg3 >, IndexedBy >(
               index.template get< IndexedBy >()
            );
         },
         _index
      );
   }

   void set_index_type( index_type type, const boost::filesystem::path& p, const boost::any& cfg )
   {
      if( type == _type ) return;

      index_variant new_index;

      auto id = next_id();
      auto rev = revision();

      {
         auto first = begin();
         auto last = end();

         switch( type )
         {
            case mira:
               new_index = std::move( mira_type( first, last, p, cfg ) );
               break;
            case bmic:
               new_index = std::move( bmic_type( first, last ) );
               break;
         }
      }
      close();
      wipe( p );

      _index = std::move( new_index );
      _type = type;

      set_revision( rev );
      set_next_id( id );
   }

   id_type next_id()
   {
      return boost::apply_visitor(
         []( auto& index ){ return index.next_id(); },
         _index
      );
   }

   void set_next_id( id_type id )
   {
      return boost::apply_visitor(
         [=]( auto& index ){ return index.set_next_id( id ); },
         _index
      );
   }

   int64_t revision()
   {
      return boost::apply_visitor(
         []( auto& index ){ return index.revision(); },
         _index
      );
   }

   int64_t set_revision( int64_t rev )
   {
      return boost::apply_visitor(
         [&rev]( auto& index ){ return index.set_revision( rev ); },
         _index
      );
   }

   template< typename Constructor >
   std::pair< iter_type, bool >
   emplace( Constructor&& con, allocator_type alloc )
   {
      return boost::apply_visitor(
         [&]( auto& index )
         {
            auto result = index.emplace( std::forward< Constructor >( con ), alloc );
            return std::pair< iter_type, bool >( iter_type( std::move( result.first ) ), result.second );
         },
         _index
      );
   }

   template< typename Constructor >
   std::pair< iter_type, bool >
   emplace( Constructor&& con )
   {
      return boost::apply_visitor(
         [&]( auto& index )
         {
            auto result = index.emplace( std::forward< Constructor >( con ) );
            return std::pair< iter_type, bool >( iter_type( std::move( result.first ) ), result.second );
         },
         _index
      );
   }

   template< typename Modifier >
   bool modify( iter_type position, Modifier&& mod )
   {
      bool result = false;

      switch( _type )
      {
         case mira:
            result = boost::get< mira_type >( _index ).modify( position.template get< mira_iter_type >(), mod );
            break;
         case bmic:
            result = boost::get< bmic_type >( _index ).modify( position.template get< bmic_iter_type >(), mod );
            break;
      }

      return result;
   }

   iter_type erase( iter_type position )
   {
      iter_type result;

      switch( _type )
      {
         case mira:
            result = boost::get< mira_type >( _index ).erase( position.template get< mira_iter_type >() );
            break;
         case bmic:
            result = boost::get< bmic_type >( _index ).erase( position.template get< bmic_iter_type >() );
            break;
      }

      return result;
   }

   iter_type iterator_to( const value_type& v )
   {
      return boost::apply_visitor(
         [&]( auto& index ){ return iter_type( index.iterator_to( v ) ); },
         _index
      );
   }

   template< typename CompatibleKey >
   iter_type find( const CompatibleKey& k )const
   {
      return boost::apply_visitor(
         [&k]( auto& index ){ return iter_type( index.find( k ) ); },
         _index
      );
   }

   template< typename CompatibleKey >
   iter_type lower_bound( const CompatibleKey& k )const
   {
      return boost::apply_visitor(
         [&k]( auto& index ){ return iter_type( index.lower_bound( k ) ); },
         _index
      );
   }

   template< typename CompatibleKey >
   iter_type upper_bound( const CompatibleKey& k )const
   {
      return boost::apply_visitor(
         [&k]( auto& index ){ return iter_type( index.upper_bound( k ) ); },
         _index
      );
   }

   template< typename CompatibleKey >
   std::pair< iter_type, iter_type > equal_range( const CompatibleKey& k )const
   {
      return boost::apply_visitor(
         [&k]( auto& index )
         {
            auto result = index.equal_range( k );
            return std::pair< iter_type, iter_type >(
               std::move( result.first ),
               std::move( result.second ) );
         },
         _index
      );
   }

   iter_type begin()const
   {
      return boost::apply_visitor(
         []( auto& index ){ return iter_type( index.begin() ); },
         _index
      );
   }

   iter_type end()const
   {
      return boost::apply_visitor(
         []( auto& index ){ return iter_type( index.end() ); },
         _index
      );
   }

   rev_iter_type rbegin()const
   {
      return boost::apply_visitor(
         []( auto& index ){ return rev_iter_type( index.rbegin() ); },
         _index
      );
   }

   rev_iter_type rend()const
   {
      return boost::apply_visitor(
         []( auto& index ){ return rev_iter_type( index.rend() ); },
         _index
      );
   }

   bool open( const boost::filesystem::path& p, const boost::any& o )
   {
      return boost::apply_visitor(
         [&p, &o]( auto& index ){ return index.open( p, o ); },
         _index
      );
   }

   void close()
   {
      boost::apply_visitor(
         []( auto& index ){ index.close(); },
         _index
      );
   }

   void wipe( const boost::filesystem::path& p )
   {
      boost::apply_visitor(
         [&]( auto& index ){ index.wipe( p ); },
         _index
      );
   }

   void clear()
   {
      boost::apply_visitor(
         []( auto& index ){ index.clear(); },
         _index
      );
   }

   void flush()
   {
      boost::apply_visitor(
         []( auto& index ){ index.flush(); },
         _index
      );
   }

   size_t size()const
   {
      return boost::apply_visitor(
         []( auto& index ){ return index.size(); },
         _index
      );
   }

   allocator_type get_allocator()const
   {
      return allocator_type();
   }

   template< typename MetaKey, typename MetaValue >
   bool put_metadata( const MetaKey& k, const MetaValue& v )
   {
      return boost::apply_visitor(
         [&k, &v]( auto& index ){ return index.put_metadata( k, v ); },
         _index
      );
   }

   template< typename MetaKey, typename MetaValue >
   bool get_metadata( const MetaKey& k, MetaValue& v )
   {
      return boost::apply_visitor(
         [&k, &v]( auto& index ){ return index.get_metadata( k, v ); },
         _index
      );
   }

   size_t get_cache_usage()const
   {
      return boost::apply_visitor(
         []( auto& index ){ return index.get_cache_usage(); },
         _index
      );
   }

   size_t get_cache_size()const
   {
      return boost::apply_visitor(
         []( auto& index ){ return index.get_cache_size(); },
         _index
      );
   }

   void dump_lb_call_counts()
   {
      boost::apply_visitor(
         []( auto& index ){ index.dump_lb_call_counts(); },
         _index
      );
   }

   void trim_cache()
   {
      boost::apply_visitor(
         []( auto& index ){ index.trim_cache(); },
         _index
      );
   }

   void print_stats()const
   {
      boost::apply_visitor(
         []( auto& index ){ index.print_stats(); },
         _index
      );
   }

   private:
      index_variant  _index;
      index_type     _type = mira;
};

}
