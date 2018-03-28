#pragma once

#include <chainbase/util/object_id.hpp>

#include <boost/multi_index/composite_key.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/iterator/filter_iterator.hpp>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <random>
#include <string>

using namespace boost::multi_index;

namespace ce_tests
{

//===================test_object===================
   struct test_object
   {
      template <class Constructor, class Allocator>
      test_object( Constructor&& c, Allocator a )
      : id( 0 ), val( 0 ), val2( 0 ), val3( 0 )
      {
         c( *this );
      }

      chainbase::oid<test_object> id;

      uint32_t val;
      uint32_t val2;
      uint32_t val3;

      bool operator==( const test_object& src ) const
      {
         return ( id == src.id ) && ( val == src.val ) && ( val2 == src.val2 ) && ( val3 == src.val3 );
      }
   };

   std::ostream& operator<<( std::ostream &os, const test_object &obj )
   {
      os << "obj.id: " << obj.id << " ";
      os << "obj.val: " << obj.val << " ";
      os << "obj.val2: " << obj.val2 << " ";
      os << "obj.val3: " << obj.val3 << " \n";

      return os;
   }

   struct cmp1
   {
      bool operator()( const test_object& obj1, const test_object& obj2 ) const
      {
         return obj1.id < obj2.id;
      }
   };

   struct cmp2
   {
      bool operator()( const test_object& obj1, const test_object& obj2 ) const
      {
         if( obj1.val == obj2.val )
            return obj1.val2 < obj2.val2;
         else
            return obj1.val < obj2.val;
      }
   };

   struct cmp3
   {
      bool operator()( const test_object& obj1, const test_object& obj2 ) const
      {
         if( obj1.val3 == obj2.val3 )
            return obj1.val < obj2.val;
         else
            return obj1.val3 < obj2.val3;
      }
   };

   struct OrderedIndex;
   struct CompositeOrderedIndexA;
   struct CompositeOrderedIndexB;

   using test_object_index = boost::multi_index_container<test_object,
   indexed_by<
      ordered_unique<
            tag<OrderedIndex>, member<test_object, chainbase::oid<test_object>, &test_object::id>>,
      ordered_unique< tag< CompositeOrderedIndexA>,
            composite_key< test_object,
               member< test_object, uint32_t, &test_object::val >,
               member< test_object, uint32_t, &test_object::val2 >
               >
            >,
      ordered_unique< tag< CompositeOrderedIndexB>,
            composite_key< test_object,
               member< test_object, uint32_t, &test_object::val3 >,
               member< test_object, uint32_t, &test_object::val >
               >
            >
      >,
   std::allocator<test_object>
   >;

//===================test_object2===================
   struct test_object2
   {
      template <class Constructor, class Allocator>
      test_object2( Constructor&& c, Allocator a )
      : id( 0 )
      {
         c( *this );
      }

      chainbase::oid<test_object2> id;

      std::string s1;
      std::string s2;

      bool operator==( const test_object2& src ) const
      {
         return ( id == src.id ) && ( s1 == src.s1 ) && ( s2 == src.s2 );
      }
   };

   std::ostream& operator<<( std::ostream &os, const test_object2 &obj )
   {
      os << "obj.id: " << obj.id << " ";
      os << "obj.s1: " << obj.s1 << " ";
      os << "obj.s2: " << obj.s2 << " \n";

      return os;
   }

   struct cmp4
   {
      bool operator()( const test_object2& obj1, const test_object2& obj2 ) const
      {
         if( obj1.s1 == obj2.s1 )
            return obj1.s2 < obj2.s2;
         else
            return obj1.s1 < obj2.s1;
      }
   };

   struct OrderedIndex2;
   struct CompositeOrderedIndex2;

   using test_object_index2 = boost::multi_index_container<test_object2,
   indexed_by<
      ordered_unique<
            tag<OrderedIndex2>, member<test_object2, chainbase::oid<test_object2>, &test_object2::id>>,
      ordered_unique< tag< CompositeOrderedIndex2>,
            composite_key< test_object2,
               member< test_object2, std::string, &test_object2::s1 >,
               member< test_object2, std::string, &test_object2::s2 >
               >
            >
      >,
   std::allocator<test_object2>
   >;

//===================data for testing===================

   struct t_input_data_core
   {
      bool incremented = false;
      std::string c = ".";
      mutable size_t idx = 0;

      t_input_data_core( bool _incremented, const std::string& _c, size_t _idx )
      :  incremented( _incremented ), c( _c ), idx( _idx )
      {

      }

      void change() const
      {
         if( incremented )
            ++idx;
      }
   };

   struct t_input_data
   {
      size_t length;
      t_input_data_core obj1;
      t_input_data_core obj2;

      t_input_data( size_t _length,
                     bool incremented1, const std::string& c1, size_t idx1,
                     bool incremented2, const std::string& c2, size_t idx2
                  )
                  :  length( _length ),
                     obj1( incremented1, c1, idx1 ),
                     obj2( incremented2, c2, idx2 )
      {

      }
   };

   template< typename Object, typename Collection >
   void fill_with_many_objects( int64_t& id_cnt, Collection& col, const std::vector< t_input_data >& items )
   {
      for( auto& item : items )
      {
         for( size_t i = 0; i < item.length; ++i )
         {
            auto c = [&]( Object& obj )
            {
               obj.id = id_cnt++;

               std::string _s1 = std::string( item.obj1.c ) + std::to_string( item.obj1.idx );
               std::string _s2 = std::string( item.obj2.c ) + std::to_string( item.obj2.idx );
               obj.s1 = _s1;
               obj.s2 = _s2;
            };

            item.obj1.change();
            item.obj2.change();

            col.emplace( Object( c, std::allocator< Object >() ) );
         }
      }

   }

   template< typename Object, typename Collection >
   void fill1( Collection& col )
   {
      col.clear();

      auto a1 = []( Object& obj ){ obj.id = 0; obj.val = 20; obj.val2 = 70; obj.val3 = 1; };
      auto b1 = []( Object& obj ){ obj.id = 1; obj.val = 21; obj.val2 = 69; obj.val3 = 2; };
      auto c1 = []( Object& obj ){ obj.id = 2; obj.val = 22; obj.val2 = 68; obj.val3 = 1; };
      auto d1 = []( Object& obj ){ obj.id = 3; obj.val = 23; obj.val2 = 67; obj.val3 = 2; };
      col.emplace( Object( a1, std::allocator< Object >() ) );
      col.emplace( Object( b1, std::allocator< Object >() ) );
      col.emplace( Object( c1, std::allocator< Object >() ) );
      col.emplace( Object( d1, std::allocator< Object >() ) );
   }

   template< typename Object, typename Collection >
   void fill2( Collection& col )
   {
      col.clear();

      auto b2 = []( Object& obj ){ obj.id = 1; obj.val = 121; obj.val2 = 169; obj.val3 = 12; };
      auto c2 = []( Object& obj ){ obj.id = 2; obj.val = 7; obj.val2 = 8; obj.val3 = 11; };
      col.emplace( Object( b2, std::allocator< Object >() ) );
      col.emplace( Object( c2, std::allocator< Object >() ) );
   }

   template< typename Object, typename Collection >
   void fill2a( Collection& col )
   {
      col.clear();

      auto b2 = []( Object& obj ){ obj.id = 2; obj.val = 17; obj.val2 = 18; obj.val3 = 12; };
      col.emplace( Object( b2, std::allocator< Object >() ) );
   }

   template< typename Object, typename Collection >
   void fill2b( Collection& col )
   {
      col.clear();

      auto b2 = []( Object& obj ){ obj.id = 0; obj.val = 8; obj.val2 = 18; obj.val3 = 12; };
      auto c2 = []( Object& obj ){ obj.id = 1; obj.val = 9; obj.val2 = 18; obj.val3 = 13; };
      auto d2 = []( Object& obj ){ obj.id = 2; obj.val = 10; obj.val2 = 18; obj.val3 = 14; };
      col.emplace( Object( b2, std::allocator< Object >() ) );
      col.emplace( Object( c2, std::allocator< Object >() ) );
      col.emplace( Object( d2, std::allocator< Object >() ) );
   }

   template< typename Object, typename Collection >
   void fill3( Collection& col )
   {
      col.clear();

      auto b3 = []( Object& obj ){ obj.id = 1; obj.val = 11; obj.val2 = 59; obj.val3 = 2; };
      auto c3 = []( Object& obj ){ obj.id = 2; obj.val = 20; obj.val2 = 48; obj.val3 = 1; };
      auto d3 = []( Object& obj ){ obj.id = 3; obj.val = 3; obj.val2 = 37; obj.val3 = 2; };
   
      col.emplace( Object ( b3, std::allocator< Object >() ) );
      col.emplace( Object ( c3, std::allocator< Object >() ) );
      col.emplace( Object ( d3, std::allocator< Object >() ) );
   }

   template< typename Object, typename Collection >
   void fill4( Collection& col )
   {
      col.clear();

      auto c0 = []( Object& obj ){ obj.id = 0; obj.val = 10; obj.val2 = 24; obj.val3 = 33; };
      auto c1 = []( Object& obj ){ obj.id = 2; obj.val = 9; obj.val2 = 23; obj.val3 = 35; };
      auto c2 = []( Object& obj ){ obj.id = 3; obj.val = 8; obj.val2 = 24; obj.val3 = 34; };
      auto c3 = []( Object& obj ){ obj.id = 1; obj.val = 10; obj.val2 = 23; obj.val3 = 32; };
   
      col.emplace( Object ( c0, std::allocator< Object >() ) );
      col.emplace( Object ( c1, std::allocator< Object >() ) );
      col.emplace( Object ( c2, std::allocator< Object >() ) );
      col.emplace( Object ( c3, std::allocator< Object >() ) );
   }

   template< typename Object, typename Collection >
   void fill4a( Collection& col )
   {
      col.clear();

      auto c0 = []( Object& obj ){ obj.id = 4; obj.val = 8; obj.val2 = 23; obj.val3 = 35; };
      auto c1 = []( Object& obj ){ obj.id = 5; obj.val = 7; obj.val2 = 24; obj.val3 = 35; };
      auto c2 = []( Object& obj ){ obj.id = 0; obj.val = 3; obj.val2 = 22; obj.val3 = 36; };
      auto c3 = []( Object& obj ){ obj.id = 2; obj.val = 5; obj.val2 = 22; obj.val3 = 35; };
      auto c4 = []( Object& obj ){ obj.id = 6; obj.val = 8; obj.val2 = 21; obj.val3 = 36; };
   
      col.emplace( Object ( c0, std::allocator< Object >() ) );
      col.emplace( Object ( c1, std::allocator< Object >() ) );
      col.emplace( Object ( c2, std::allocator< Object >() ) );
      col.emplace( Object ( c3, std::allocator< Object >() ) );
      col.emplace( Object ( c4, std::allocator< Object >() ) );
   }

   template< typename Object, typename Collection >
   void fill4b( Collection& col )
   {
      col.clear();

      auto c0 = []( Object& obj ){ obj.id = 2; obj.val = 9; obj.val2 = 21; obj.val3 = 30; };
      auto c1 = []( Object& obj ){ obj.id = 6; obj.val = 8; obj.val2 = 22; obj.val3 = 36; };
      auto c2 = []( Object& obj ){ obj.id = 1; obj.val = 10; obj.val2 = 27; obj.val3 = 33; };
      auto c3 = []( Object& obj ){ obj.id = 7; obj.val = 8; obj.val2 = 20; obj.val3 = 37; };
      auto c4 = []( Object& obj ){ obj.id = 8; obj.val = 8; obj.val2 = 27; obj.val3 = 30; };
   
      col.emplace( Object ( c0, std::allocator< Object >() ) );
      col.emplace( Object ( c1, std::allocator< Object >() ) );
      col.emplace( Object ( c2, std::allocator< Object >() ) );
      col.emplace( Object ( c3, std::allocator< Object >() ) );
      col.emplace( Object ( c4, std::allocator< Object >() ) );
   }

   template< typename Collection >
   void fill4b_helper( Collection& col )
   {
      col.clear();

      //The same id-s as in 'fill4b' function.
      col = { 2, 6, 1, 7, 8 };
   }

   template< typename Object, typename Collection >
   void fill5( Collection& col )
   {
      col.clear();

      auto c = []( Object& obj ){ obj.id = 0; obj.val = 2; obj.val2 = 7; obj.val3 = 50; };
   
      col.emplace( Object ( c, std::allocator< Object >() ) );
   }

   template< typename Object, typename Collection >
   void fill5b( Collection& col )
   {
      col.clear();

      auto c0 = []( Object& obj ){ obj.id = 1; obj.val = 2; obj.val2 = 9; obj.val3 = 51; };
      auto c1 = []( Object& obj ){ obj.id = 2; obj.val = 2; obj.val2 = 8; obj.val3 = 52; };
      auto c2 = []( Object& obj ){ obj.id = 3; obj.val = 2; obj.val2 = 6; obj.val3 = 53; };
   
      col.emplace( Object ( c0, std::allocator< Object >() ) );
      col.emplace( Object ( c1, std::allocator< Object >() ) );
      col.emplace( Object ( c2, std::allocator< Object >() ) );
   }

   template< typename Object, typename Collection >
   void fill6( Collection& col )
   {
      col.clear();

      auto c0 = []( Object& obj ){ obj.id = 0; obj.val = 30; obj.val2 = 50; obj.val3 = 100; };
      auto c1 = []( Object& obj ){ obj.id = 1; obj.val = 31; obj.val2 = 51; obj.val3 = 101; };
      auto c2 = []( Object& obj ){ obj.id = 2; obj.val = 32; obj.val2 = 52; obj.val3 = 102; };
      auto c3 = []( Object& obj ){ obj.id = 3; obj.val = 33; obj.val2 = 53; obj.val3 = 102; };
   
      col.emplace( Object ( c0, std::allocator< Object >() ) );
      col.emplace( Object ( c1, std::allocator< Object >() ) );
      col.emplace( Object ( c2, std::allocator< Object >() ) );
      col.emplace( Object ( c3, std::allocator< Object >() ) );
   }

   template< typename Object, typename Collection >
   void fill7( Collection& col )
   {
      col.clear();

      auto c0 = []( Object& obj ){ obj.id = 0; obj.val = 30; obj.val2 = 50; obj.val3 = 0; };
      auto c1 = []( Object& obj ){ obj.id = 4; obj.val = 31; obj.val2 = 51; obj.val3 = 1; };
      auto c2 = []( Object& obj ){ obj.id = 5; obj.val = 32; obj.val2 = 52; obj.val3 = 2; };
   
      col.emplace( Object ( c0, std::allocator< Object >() ) );
      col.emplace( Object ( c1, std::allocator< Object >() ) );
      col.emplace( Object ( c2, std::allocator< Object >() ) );
   }

   template< typename Object, typename Collection >
   void fill7a( Collection& col )
   {
      col.clear();

      auto c0 = []( Object& obj ){ obj.id = 1; obj.val = 130; obj.val2 = 150; obj.val3 = 100; };
      auto c1 = []( Object& obj ){ obj.id = 2; obj.val = 131; obj.val2 = 151; obj.val3 = 101; };
      auto c2 = []( Object& obj ){ obj.id = 3; obj.val = 132; obj.val2 = 152; obj.val3 = 102; };
   
      col.emplace( Object ( c0, std::allocator< Object >() ) );
      col.emplace( Object ( c1, std::allocator< Object >() ) );
      col.emplace( Object ( c2, std::allocator< Object >() ) );
   }

   template< typename Object, typename Collection >
   void fill7b( Collection& col )
   {
      col.clear();

      auto c0 = []( Object& obj ){ obj.id = 2; obj.val = 230; obj.val2 = 250; obj.val3 = 200; };
      auto c1 = []( Object& obj ){ obj.id = 4; obj.val = 231; obj.val2 = 251; obj.val3 = 201; };
   
      col.emplace( Object ( c0, std::allocator< Object >() ) );
      col.emplace( Object ( c1, std::allocator< Object >() ) );
   }

   template< typename Object, typename Collection >
   void sort7( Collection& col )
   {
      col.clear();

      auto c0 = []( Object& obj ){ obj.id = 0; obj.val = 30; obj.val2 = 50; obj.val3 = 0; };
      auto c1 = []( Object& obj ){ obj.id = 1; obj.val = 130; obj.val2 = 150; obj.val3 = 100; };
      auto c2 = []( Object& obj ){ obj.id = 2; obj.val = 230; obj.val2 = 250; obj.val3 = 200; };
      auto c3 = []( Object& obj ){ obj.id = 3; obj.val = 132; obj.val2 = 152; obj.val3 = 102; };
      auto c4 = []( Object& obj ){ obj.id = 4; obj.val = 231; obj.val2 = 251; obj.val3 = 201; };
      auto c5 = []( Object& obj ){ obj.id = 5; obj.val = 32; obj.val2 = 52; obj.val3 = 2; };

      col.emplace( Object ( c0, std::allocator< Object >() ) );
      col.emplace( Object ( c1, std::allocator< Object >() ) );
      col.emplace( Object ( c2, std::allocator< Object >() ) );
      col.emplace( Object ( c3, std::allocator< Object >() ) );
      col.emplace( Object ( c4, std::allocator< Object >() ) );
      col.emplace( Object ( c5, std::allocator< Object >() ) );
   }

   template< typename Object, typename Collection >
   void sort5( Collection& col )
   {
      col.clear();

      auto c0 = []( Object& obj ){ obj.id = 3; obj.val = 2; obj.val2 = 6; obj.val3 = 53; };
      auto c1 = []( Object& obj ){ obj.id = 0; obj.val = 2; obj.val2 = 7; obj.val3 = 50; };
      auto c2 = []( Object& obj ){ obj.id = 2; obj.val = 2; obj.val2 = 8; obj.val3 = 52; };
      auto c3 = []( Object& obj ){ obj.id = 1; obj.val = 2; obj.val2 = 9; obj.val3 = 51; };

      col.emplace_back( Object ( c0, std::allocator< Object >() ) );
      col.emplace_back( Object ( c1, std::allocator< Object >() ) );
      col.emplace_back( Object ( c2, std::allocator< Object >() ) );
      col.emplace_back( Object ( c3, std::allocator< Object >() ) );
   }

   template< typename Object, typename Collection >
   void sort1( Collection& col )
   {
      col.clear();

      auto c1 = []( Object& obj ){ obj.id = 0; obj.val = 20; obj.val2 = 70; obj.val3 = 1; };
      auto c2 = []( Object& obj ){ obj.id = 1; obj.val = 11; obj.val2 = 59; obj.val3 = 2; };
      auto c3 = []( Object& obj ){ obj.id = 2; obj.val = 20; obj.val2 = 48; obj.val3 = 1; };
      auto c4 = []( Object& obj ){ obj.id = 3; obj.val = 3; obj.val2 = 37; obj.val3 = 2; };
   
      col.emplace_back( Object ( c1, std::allocator< Object >() ) );
      col.emplace_back( Object ( c2, std::allocator< Object >() ) );
      col.emplace_back( Object ( c3, std::allocator< Object >() ) );
      col.emplace_back( Object ( c4, std::allocator< Object >() ) );
   }

   template< typename Object, typename Collection >
   void sort2( Collection& col )
   {
      col.clear();

      auto c1 = []( Object& obj ){ obj.id = 0; obj.val = 8; obj.val2 = 18; obj.val3 = 12; };
      auto c2 = []( Object& obj ){ obj.id = 1; obj.val = 9; obj.val2 = 18; obj.val3 = 13; };
      auto c3 = []( Object& obj ){ obj.id = 2; obj.val = 10; obj.val2 = 18; obj.val3 = 14; };

      col.emplace_back( Object ( c1, std::allocator< Object >() ) );
      col.emplace_back( Object ( c2, std::allocator< Object >() ) );
      col.emplace_back( Object ( c3, std::allocator< Object >() ) );
   }

   template< typename Object, typename Collection >
   void sort4( Collection& col )
   {
      col.clear();

      auto c0 = []( Object& obj ){ obj.id = 0; obj.val = 3; obj.val2 = 22; obj.val3 = 36; };
      auto c1 = []( Object& obj ){ obj.id = 5; obj.val = 7; obj.val2 = 24; obj.val3 = 35; };
      auto c2 = []( Object& obj ){ obj.id = 7; obj.val = 8; obj.val2 = 20; obj.val3 = 37; };
      auto c3 = []( Object& obj ){ obj.id = 6; obj.val = 8; obj.val2 = 22; obj.val3 = 36; };
      auto c4 = []( Object& obj ){ obj.id = 4; obj.val = 8; obj.val2 = 23; obj.val3 = 35; };
      auto c5 = []( Object& obj ){ obj.id = 3; obj.val = 8; obj.val2 = 24; obj.val3 = 34; };
      auto c6 = []( Object& obj ){ obj.id = 8; obj.val = 8; obj.val2 = 27; obj.val3 = 30; };
      auto c7 = []( Object& obj ){ obj.id = 2; obj.val = 9; obj.val2 = 21; obj.val3 = 30; };
      auto c8 = []( Object& obj ){ obj.id = 1; obj.val = 10; obj.val2 = 27; obj.val3 = 33; };

      col.emplace_back( Object ( c0, std::allocator< Object >() ) );
      col.emplace_back( Object ( c1, std::allocator< Object >() ) );
      col.emplace_back( Object ( c2, std::allocator< Object >() ) );
      col.emplace_back( Object ( c3, std::allocator< Object >() ) );
      col.emplace_back( Object ( c4, std::allocator< Object >() ) );
      col.emplace_back( Object ( c5, std::allocator< Object >() ) );
      col.emplace_back( Object ( c6, std::allocator< Object >() ) );
      col.emplace_back( Object ( c7, std::allocator< Object >() ) );
      col.emplace_back( Object ( c8, std::allocator< Object >() ) );
   }

   template< typename Object, typename Collection >
   void sort4a( Collection& col )
   {
      col.clear();

      auto c0 = []( Object& obj ){ obj.id = 8; obj.val = 8; obj.val2 = 27; obj.val3 = 30; };
      auto c1 = []( Object& obj ){ obj.id = 2; obj.val = 9; obj.val2 = 21; obj.val3 = 30; };
      auto c2 = []( Object& obj ){ obj.id = 1; obj.val = 10; obj.val2 = 27; obj.val3 = 33; };
      auto c3 = []( Object& obj ){ obj.id = 3; obj.val = 8; obj.val2 = 24; obj.val3 = 34; };
      auto c4 = []( Object& obj ){ obj.id = 5; obj.val = 7; obj.val2 = 24; obj.val3 = 35; };
      auto c5 = []( Object& obj ){ obj.id = 4; obj.val = 8; obj.val2 = 23; obj.val3 = 35; };
      auto c6 = []( Object& obj ){ obj.id = 0; obj.val = 3; obj.val2 = 22; obj.val3 = 36; };
      auto c7 = []( Object& obj ){ obj.id = 6; obj.val = 8; obj.val2 = 22; obj.val3 = 36; };
      auto c8 = []( Object& obj ){ obj.id = 7; obj.val = 8; obj.val2 = 20; obj.val3 = 37; };

      col.emplace_back( Object ( c0, std::allocator< Object >() ) );
      col.emplace_back( Object ( c1, std::allocator< Object >() ) );
      col.emplace_back( Object ( c2, std::allocator< Object >() ) );
      col.emplace_back( Object ( c3, std::allocator< Object >() ) );
      col.emplace_back( Object ( c4, std::allocator< Object >() ) );
      col.emplace_back( Object ( c5, std::allocator< Object >() ) );
      col.emplace_back( Object ( c6, std::allocator< Object >() ) );
      col.emplace_back( Object ( c7, std::allocator< Object >() ) );
      col.emplace_back( Object ( c8, std::allocator< Object >() ) );
   }

}
