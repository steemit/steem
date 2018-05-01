#include <boost/test/unit_test.hpp>

#include "../concatenation_enumerator_tests/concatenation_enumerator_tests_mgr.hpp"
#include <chainbase/util/concatenation_enumerator.hpp>

#include <chrono>

using namespace chainbase;

template< typename Collection, typename Object >
void basic_test()
{
   int cnt;

   Collection s1;
   Collection s2;

   auto p1 = std::make_tuple( s1.begin(), s1.end() );
   auto p2 = std::make_tuple( s2.begin(), s2.end() );

   BOOST_TEST_MESSAGE( "1 empty source" );
   cnt = 0;
   using Iterator = ce::concatenation_iterator< Object, ce_tests::cmp1 >;
   Iterator it01( ce_tests::cmp1(), p1 );
   Iterator it_end_01 = Iterator::create_end( ce_tests::cmp1(), p1 );
   while( it01 != it_end_01 )
   {
      ++it01;
      ++cnt;
   }
   BOOST_REQUIRE( cnt == 0 );

   BOOST_TEST_MESSAGE( "2 empty sources" );
   cnt = 0;
   Iterator it02( ce_tests::cmp1(), p1, p2 );
   Iterator it_end_02 = Iterator::create_end( ce_tests::cmp1(), p1, p2 );
   while( it02 != it_end_02 )
   {
      ++it02;
      ++cnt;
   }
   BOOST_REQUIRE( cnt == 0 );

   BOOST_TEST_MESSAGE( "1 filled source" );
   ce_tests::fill1< Object >( s1 );
   Iterator it03( ce_tests::cmp1(), std::make_tuple( s1.begin(), s1.end() ) );
   Iterator it_end_03 = Iterator::create_end( ce_tests::cmp1(), std::make_tuple( s1.begin(), s1.end() ) );
   auto it_src03 = s1.begin();
   while( it03 != it_end_03 )
   {
      BOOST_REQUIRE( ( *it03 ) == ( *it_src03 ) );

      ++it03;
      ++it_src03;
   }

   BOOST_TEST_MESSAGE( "2 filled sources: one has content, second is empty" );
   ce_tests::fill1< Object >( s1 );
   Iterator it04 (  ce_tests::cmp1(),
                     std::make_tuple( s1.begin(), s1.end() ),
                     std::make_tuple( s2.begin(), s2.end() )
                  );

   Iterator it_end_04 = Iterator::create_end( ce_tests::cmp1(),
                                                std::make_tuple( s1.begin(), s1.end() ),
                                                std::make_tuple( s2.begin(), s2.end() )
                                             );
   auto it_src04_1 = s1.begin();
   while( it04 != it_end_04 )
   {
      BOOST_REQUIRE( ( *it04 ) == ( *it_src04_1 ) );

      ++it04;
      ++it_src04_1;
   }

   BOOST_TEST_MESSAGE( "2 filled sources with the same content" );
   ce_tests::fill1< Object >( s1 );
   ce_tests::fill1< Object >( s2 );
   Iterator it05 ( ce_tests::cmp1(),
                     std::make_tuple( s1.begin(), s1.end() ),
                     std::make_tuple( s2.begin(), s2.end() )
                  );
   Iterator it_end_05 = Iterator::create_end( ce_tests::cmp1(),
                           std::make_tuple( s1.begin(), s1.end() ),
                           std::make_tuple( s2.begin(), s2.end() )
                        );
   auto it_src05_1 = s1.begin();
   auto it_src05_2 = s2.begin();
   while( it05 != it_end_05 )
   {
      BOOST_REQUIRE( ( *it05 ) == ( *it_src05_1 ) );
      BOOST_REQUIRE( ( *it05 ) == ( *it_src05_2 ) );

      ++it05;
      ++it_src05_1;
      ++it_src05_2;
   }

   BOOST_TEST_MESSAGE( "2 filled sources with different content" );
   ce_tests::fill2< Object >( s1 );
   ce_tests::fill2a< Object >( s2 );
   Iterator it06 ( ce_tests::cmp1(),
                     std::make_tuple( s1.begin(), s1.end() ),
                     std::make_tuple( s2.begin(), s2.end() )
                  );
   Iterator it_end_06 = Iterator::create_end( ce_tests::cmp1(),
                        std::make_tuple( s1.begin(), s1.end() ),
                        std::make_tuple( s2.begin(), s2.end() )
                     );
   cnt = 0;
   while( it06 != it_end_06 )
   {
      if( cnt == 0 )
         BOOST_REQUIRE( ( *it06 ) == ( *s1.begin() ) );
      else if( cnt == 1 )
         BOOST_REQUIRE( ( *it06 ) == ( *s2.begin() ) );
      else
         BOOST_REQUIRE( 0 && "Unknown state of enumerator" );

      ++it06;
      ++cnt;
   }
}

template< typename Collection1, typename Collection2, typename Index, typename Object, typename Cmp, typename SortedCollection >
void test_different_sources( const SortedCollection& sorted )
{
   Collection1 bmic1;
   Collection1 bmic2;
   Collection2 s1;

   BOOST_TEST_MESSAGE( "3 filled sources with different content" );
   ce_tests::fill1< Object >( bmic1 );
   ce_tests::fill2< Object >( bmic2 );
   ce_tests::fill3< Object >( s1 );

   const auto& idx1 = bmic1.template get< Index >();
   const auto& idx2 = bmic2.template get< Index >();

   auto p1 = std::make_tuple( idx1.begin(), idx1.end() );
   auto p2 = std::make_tuple( idx2.begin(), idx2.end() );
   auto p3 = std::make_tuple( s1.begin(), s1.end() );

   using Iterator = ce::concatenation_iterator< Object, Cmp >;

   Iterator it( Cmp(), p1, p2, p3 );
   Iterator it_end = Iterator::create_end( Cmp(), p1, p2, p3 );

   auto sorted_it = sorted.begin();
   while( it != it_end )
   {
      BOOST_REQUIRE( ( *it ) == ( *sorted_it ) );

      ++it;
      ++sorted_it;
   }

}

template< typename Call1, typename Call2, typename Collection, typename ID_Index, typename Index, typename Object, typename Cmp, typename SortedCollection >
void test_with_sub_index( Call1& call1, Call2& call2, const SortedCollection& sorted )
{
   Collection bmic1;
   Collection bmic2;

   BOOST_TEST_MESSAGE( "2 filled sources with different content ( sub-index is active )" );
   call1( bmic1 );
   call2( bmic2 );

   const auto& id_idx1 = bmic1.template get< ID_Index >();
   const auto& id_idx2 = bmic2.template get< ID_Index >();

   const auto& idx1 = bmic1.template get< Index >();
   const auto& idx2 = bmic2.template get< Index >();

   auto p1 = std::make_tuple( idx1.begin(), idx1.end(), &id_idx1 );
   auto p2 = std::make_tuple( idx2.begin(), idx2.end(), &id_idx2 );

   using Iterator = ce::concatenation_iterator_ex< Object, Cmp >;

   Iterator it( Cmp(), p1, p2 );
   Iterator it_end = Iterator::create_end( Cmp(), p1, p2 );

   auto sorted_it = sorted.begin();
   while( it != it_end )
   {
      BOOST_REQUIRE( ( *it ) == ( *sorted_it ) );

      ++it;
      ++sorted_it;
   }

}

template< typename Collection1, typename Collection2, typename ID_Index, typename Index, typename Object, typename Cmp, typename SortedCollection >
void test_with_sub_index_3_sources( const SortedCollection& sorted )
{
   Collection1 bmic1;
   Collection1 bmic2;
   Collection2 s;
   std::set< size_t > s_helper;

   BOOST_TEST_MESSAGE( "3 filled sources with different content ( sub-index is active )" );
   ce_tests::fill4< Object >( bmic1 );
   ce_tests::fill4a< Object >( bmic2 );
   ce_tests::fill4b< Object >( s );

   ce_tests::fill4b_helper( s_helper );

   const auto& id_idx1 = bmic1.template get< ID_Index >();
   const auto& id_idx2 = bmic2.template get< ID_Index >();

   const auto& idx1 = bmic1.template get< Index >();
   const auto& idx2 = bmic2.template get< Index >();

   auto p1 = std::make_tuple( idx1.begin(), idx1.end(), &id_idx1 );
   auto p2 = std::make_tuple( idx2.begin(), idx2.end(), &id_idx2 );
   auto p3 = std::make_tuple( s.begin(), s.end(), &s_helper );

   using Iterator = ce::concatenation_iterator_ex< Object, Cmp >;

   Iterator it( Cmp(), p1, p2, p3 );
   Iterator it_end = Iterator::create_end( Cmp(), p1, p2, p3 );

   auto sorted_it = sorted.begin();
   while( it != it_end )
   {
      BOOST_REQUIRE( ( *it ) == ( *sorted_it ) );

      ++it;
      ++sorted_it;
   }

}

template< typename ID_Index, typename Iterator, typename ReverseIterator, typename CmpIterator, typename CmpReverseIterator >
void benchmark_internal( Iterator begin, Iterator end, CmpIterator cmp_begin, CmpIterator cmp_end,
                         ReverseIterator r_begin, ReverseIterator r_end, CmpReverseIterator r_cmp_begin, CmpReverseIterator r_cmp_end,
                         const ID_Index& id_index
                         )
{
   //start benchmark - classical sorted collection
   uint64_t start_time1 = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now().time_since_epoch() ).count();

   while( cmp_begin != cmp_end )
      ++cmp_begin;

   uint64_t end_time1 = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now().time_since_epoch() ).count();

   std::cout<<"iterator: bmic iterator: "<< end_time1 - start_time1<<" ms"<<std::endl;
   //end benchmark

   //start benchmark - concatenation iterator
   uint64_t start_time2 = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now().time_since_epoch() ).count();

   while( begin != end )
      ++begin;

   uint64_t end_time2 = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now().time_since_epoch() ).count();

   std::cout<<"iterator: concatenation iterator: "<< end_time2 - start_time2<<" ms"<<std::endl;
   std::cout<<"ratio: "<<( static_cast< double >( end_time2 - start_time2 ) )/( end_time1 - start_time1 )<<std::endl;
   //end benchmark

   BOOST_TEST_MESSAGE( "reverse_iterator" );
   //start benchmark - classical sorted collection
   start_time1 = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now().time_since_epoch() ).count();

   while( r_cmp_begin != r_cmp_end )
      ++r_cmp_begin;

   end_time1 = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now().time_since_epoch() ).count();

   std::cout<<"reverse_iterator: bmic iterator: "<< end_time1 - start_time1<<" ms"<<std::endl;
   //end benchmark

   //start benchmark - concatenation iterator
   start_time2 = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now().time_since_epoch() ).count();

   while( r_begin != r_end )
      ++r_begin;

   end_time2 = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now().time_since_epoch() ).count();

   std::cout<<"reverse_iterator: concatenation iterator: "<< end_time2 - start_time2<<" ms"<<std::endl;
   std::cout<<"ratio: "<<( static_cast< double >( end_time2 - start_time2 ) )/( end_time1 - start_time1 )<<std::endl;
   //end benchmark
}

template< bool Is_Another_Source, typename Collection, typename ID_Index, typename Index, typename Object, typename Cmp >
void benchmark_test_2_sources()
{
   Collection another;
   Collection bmic1;
   Collection bmic2;
   Collection sorted;

   using t_src = ce_tests::t_input_data;

   const int32_t ratio = 10;
   const int32_t size1 = 1000000;
   const int32_t size2 = size1/ratio;

   std::vector< t_src > v1 = 
   {
      // Example:
      // t_src(
      //       3      /*_length*/,
      //       true   /*bool incremented1*/,    "a"   /*std::string c1*/,    0   /*size_t idx1*/,
      //       true   /*bool incremented2*/,    "b"   /*std::string c2*/,    0   /*size_t idx2*/
      //       )

      t_src( size1, true, "a", 0, true, "b", 0 )
   };
   std::vector< t_src > v2 = 
   {
      t_src( size2, true, "x", 0, true, "y", 0 )
   };

   size_t total_size = size1 + size2;
   int64_t id_cnt = 0;
   int64_t id_cnt_copy = id_cnt;

   BOOST_TEST_MESSAGE( "benchmark test with 2 sources - ( sub-index is active )" );

   //Making sorted collection
   ce_tests::fill_with_many_objects< Object >( id_cnt_copy, sorted, std::vector< t_src >( v1 ) );
   ce_tests::fill_with_many_objects< Object >( id_cnt_copy, sorted, std::vector< t_src >( v2 ) );
   size_t sorted_size = sorted.size();
   //Making sorted collection

   BOOST_REQUIRE( total_size == sorted_size );

   ce_tests::fill_with_many_objects< Object >( id_cnt, bmic1, v1 );
   ce_tests::fill_with_many_objects< Object >( id_cnt, bmic2, v2 );

   const auto& id_idx1 = bmic1.template get< ID_Index >();
   const auto& id_idx2 = bmic2.template get< ID_Index >();
   const auto& idx_another = another.template get< ID_Index >();

   const auto& idx1 = bmic1.template get< Index >();
   const auto& idx2 = bmic2.template get< Index >();
   const auto& idx_sorted = sorted.template get< Index >();

   auto p1 = std::make_tuple( idx1.begin(), idx1.end(), Is_Another_Source?(&idx_another):(&id_idx1) );
   auto p2 = std::make_tuple( idx2.begin(), idx2.end(), Is_Another_Source?(&idx_another):(&id_idx2) );

   using Iterator = ce::concatenation_iterator_ex< Object, Cmp >;
   using ReverseIterator = ce::concatenation_reverse_iterator_ex< Object, Cmp >;

   Iterator it( Cmp(), p1, p2 );
   Iterator it_end = Iterator::create_end( Cmp(), p1, p2 );

   auto sorted_it = idx_sorted.begin();
   while( it != it_end )
   {
      BOOST_REQUIRE( ( *it ) == ( *sorted_it ) );

      ++it;
      ++sorted_it;
   }

   BOOST_TEST_MESSAGE( "iterator" );
   std::cout<<std::endl<<"***2 sources***"<<std::endl;

   Iterator _begin( Cmp(), p1, p2 );
   Iterator _end = Iterator::create_end( Cmp(), p1, p2 );
   auto cmp_begin = idx_sorted.begin();
   auto cmp_end = idx_sorted.end();

   ReverseIterator _r_begin( Cmp(), p1, p2 );
   ReverseIterator _r_end = ReverseIterator::create_end( Cmp(), p1, p2 );
   auto r_cmp_begin = idx_sorted.rbegin();
   auto r_cmp_end = idx_sorted.rend();

   benchmark_internal( _begin, _end, cmp_begin, cmp_end, _r_begin, _r_end, r_cmp_begin, r_cmp_end,
                        sorted.template get< ID_Index >() );
}

template< bool Is_Another_Source, typename Collection, typename ID_Index, typename Index, typename Object, typename Cmp >
void benchmark_test_3_sources()
{
   Collection another;
   Collection bmic1;
   Collection bmic2;
   Collection bmic3;
   Collection sorted;

   using t_src = ce_tests::t_input_data;

   const int32_t ratio = 10;
   const int32_t size1 = 1000000;
   const int32_t size2 = size1/ratio;
   const int32_t size3 = size1/ratio;

   std::vector< t_src > v1 = 
   {
      // Example:
      // t_src(
      //       3      /*_length*/,
      //       true   /*bool incremented1*/,    "a"   /*std::string c1*/,    0   /*size_t idx1*/,
      //       true   /*bool incremented2*/,    "b"   /*std::string c2*/,    0   /*size_t idx2*/
      //       )

      t_src( size1, true, "a", 0, true, "b", 0 )
   };
   std::vector< t_src > v2 = 
   {
      t_src( size2, true, "x", 0, true, "y", 0 )
   };
   std::vector< t_src > v3 = 
   {
      t_src( size3, true, "p", 0, true, "q", 0 )
   };

   size_t total_size = size1 + size2 + size3;
   int64_t id_cnt = 0;
   int64_t id_cnt_copy = id_cnt;

   BOOST_TEST_MESSAGE( "benchmark test with 3 sources - ( sub-index is active )" );

   //Making sorted collection
   ce_tests::fill_with_many_objects< Object >( id_cnt_copy, sorted, std::vector< t_src >( v1 ) );
   ce_tests::fill_with_many_objects< Object >( id_cnt_copy, sorted, std::vector< t_src >( v2 ) );
   ce_tests::fill_with_many_objects< Object >( id_cnt_copy, sorted, std::vector< t_src >( v3 ) );
   size_t sorted_size = sorted.size();
   //Making sorted collection

   BOOST_REQUIRE( total_size == sorted_size );

   ce_tests::fill_with_many_objects< Object >( id_cnt, bmic1, v1 );
   ce_tests::fill_with_many_objects< Object >( id_cnt, bmic2, v2 );
   ce_tests::fill_with_many_objects< Object >( id_cnt, bmic2, v3 );

   const auto& id_idx1 = bmic1.template get< ID_Index >();
   const auto& id_idx2 = bmic2.template get< ID_Index >();
   const auto& id_idx3 = bmic2.template get< ID_Index >();
   const auto& idx_another = another.template get< ID_Index >();

   const auto& idx1 = bmic1.template get< Index >();
   const auto& idx2 = bmic2.template get< Index >();
   const auto& idx3 = bmic2.template get< Index >();
   const auto& idx_sorted = sorted.template get< Index >();

   auto p1 = std::make_tuple( idx1.begin(), idx1.end(), Is_Another_Source?(&idx_another):(&id_idx1) );
   auto p2 = std::make_tuple( idx2.begin(), idx2.end(), Is_Another_Source?(&idx_another):(&id_idx2) );
   auto p3 = std::make_tuple( idx3.begin(), idx3.end(), Is_Another_Source?(&idx_another):(&id_idx3) );

   using Iterator = ce::concatenation_iterator_ex< Object, Cmp >;
   using ReverseIterator = ce::concatenation_reverse_iterator_ex< Object, Cmp >;

   Iterator it( Cmp(), p1, p2, p3 );
   Iterator it_end = Iterator::create_end( Cmp(), p1, p2, p3 );

   auto sorted_it = idx_sorted.begin();
   while( it != it_end )
   {
      BOOST_REQUIRE( ( *it ) == ( *sorted_it ) );

      ++it;
      ++sorted_it;
   }

   BOOST_TEST_MESSAGE( "iterator" );
   std::cout<<std::endl<<"***3 sources***"<<std::endl;

   Iterator _begin( Cmp(), p1, p2, p3 );
   Iterator _end = Iterator::create_end( Cmp(), p1, p2, p3 );
   auto cmp_begin = idx_sorted.begin();
   auto cmp_end = idx_sorted.end();

   ReverseIterator _r_begin( Cmp(), p1, p2, p3 );
   ReverseIterator _r_end = ReverseIterator::create_end( Cmp(), p1, p2, p3 );
   auto r_cmp_begin = idx_sorted.rbegin();
   auto r_cmp_end = idx_sorted.rend();

   benchmark_internal( _begin, _end, cmp_begin, cmp_end, _r_begin, _r_end, r_cmp_begin, r_cmp_end,
                        sorted.template get< ID_Index >() );
}

template< typename Collection, typename ID_Index, typename Index, typename Object, typename Cmp >
void test_with_sub_index_2_sources_many_objects_without_id_repeat()
{
   Collection bmic1;
   Collection bmic2;

   using t_src = ce_tests::t_input_data;

   std::vector< t_src > v1 = 
   {
      /*
         size_t _length
         bool incremented1, std::string c1, size_t idx1
         bool incremented2, std::string c2, size_t idx2
      */
      t_src( 3, true,  "a", 0, true,  "b", 0 ),
      t_src( 5,  false, "c", 1, true,  "d", 10 ),
      t_src( 4,  true,  "e", 2, false, "b", 2 )
   };
   std::vector< t_src > v2 = 
   {
      t_src( 3, true,  "a", 10, true, "b", 10 ),
      t_src( 5,  false, "c", 1, true,  "d", 15 ),
      t_src( 4,  true,  "e", 9, false, "b", 2 ),

      t_src( 100, true,  "b", 1000, true, "a", 100 ),
      t_src( 100,  false, "d", 1000, true,  "c", 150 ),
      t_src( 100,  true,  "b", 1000, false, "e", 200 ),
      t_src( 1000,  true,  "x1000-start", 1000, false, "x1000-end", 200 ),
   };

   size_t total_size = (3+5+4)*2 + 3*100 + 1000;
   int64_t id_cnt = 0;
   int64_t id_cnt_copy = id_cnt;

   BOOST_TEST_MESSAGE( "2 filled sources with different content - many objects without key repeating ( sub-index is active )" );

   //Making sorted collection
   using t_sorted = std::set< Object, Cmp >;
   t_sorted sorted;
   ce_tests::fill_with_many_objects< Object >( id_cnt_copy, sorted, std::vector< t_src >( v1 ) );
   ce_tests::fill_with_many_objects< Object >( id_cnt_copy, sorted, std::vector< t_src >( v2 ) );
   size_t sorted_size = sorted.size();
   //Making sorted collection

   BOOST_REQUIRE( total_size == sorted_size );

   ce_tests::fill_with_many_objects< Object >( id_cnt, bmic1, v1 );
   ce_tests::fill_with_many_objects< Object >( id_cnt, bmic2, v2 );

   const auto& id_idx1 = bmic1.template get< ID_Index >();
   const auto& id_idx2 = bmic2.template get< ID_Index >();

   const auto& idx1 = bmic1.template get< Index >();
   const auto& idx2 = bmic2.template get< Index >();

   auto p1 = std::make_tuple( idx1.begin(), idx1.end(), &id_idx1 );
   auto p2 = std::make_tuple( idx2.begin(), idx2.end(), &id_idx2 );

   using Iterator = ce::concatenation_iterator_ex< Object, Cmp >;

   Iterator it( Cmp(), p1, p2 );
   Iterator it_end = Iterator::create_end( Cmp(), p1, p2 );

   auto sorted_it = sorted.begin();
   while( it != it_end )
   {
      BOOST_REQUIRE( ( *it ) == ( *sorted_it ) );

      ++it;
      ++sorted_it;
   }
}

template< typename Collection, typename ID_Index, typename Index, typename Object, typename Cmp >
void test_with_sub_index_2_sources_many_objects_with_id_repeat()
{
   Collection bmic1;
   Collection bmic2;

   using t_src = ce_tests::t_input_data;

   std::vector< t_src > v1 = 
   {
      /*
         size_t _length
         bool incremented1, std::string c1, size_t idx1
         bool incremented2, std::string c2, size_t idx2
      */
      t_src( 3, true,  "a", 0, true,  "b", 0 ),
      t_src( 5,  false, "c", 1, true,  "d", 10 ),
      t_src( 4,  true,  "e", 2, false, "b", 2 ),
      t_src( 100, true,  "b", 1000, true, "a", 100 ),
      t_src( 100,  false, "d", 1000, true,  "c", 150 ),
      t_src( 100,  true,  "b", 1000, false, "e", 200 ),
      t_src( 1000,  true,  "x1000-start", 1000, false, "x1000-end", 200 )
   };
   std::vector< t_src > v2 = 
   {
      t_src( 3, true,  ".a", 0, true,  ".b", 0 ),
      t_src( 5,  false, ".c", 1, true,  ".d", 10 ),
      t_src( 4,  true,  ".e", 2, false, ".b", 2 ),
      t_src( 100, true,  ".b", 1000, true, ".a", 100 ),
      t_src( 100,  false, ".d", 1000, true,  ".c", 150 ),
      t_src( 100,  true,  ".b", 1000, false, ".e", 200 ),
      t_src( 1000,  true,  ".x1000-start", 1000, false, ".x1000-end", 200 )
   };

   size_t total_size = 3+5+4 + 3*100 + 1000;

   int64_t id_cnt = 0;

   int64_t id_cnt_copy = id_cnt;
   int64_t id_cnt_copy1 = id_cnt;
   int64_t id_cnt_copy2 = id_cnt;

   BOOST_TEST_MESSAGE( "2 filled sources with different content - many objects with key repeating ( sub-index is active )" );

   //Making sorted collection
   using t_sorted = std::set< Object, Cmp >;
   t_sorted sorted;
   ce_tests::fill_with_many_objects< Object >( id_cnt_copy, sorted, std::vector< t_src >( v2 ) );
   size_t sorted_size = sorted.size();
   //Making sorted collection

   BOOST_REQUIRE( total_size == sorted_size );

   ce_tests::fill_with_many_objects< Object >( id_cnt_copy1, bmic1, v1 );
   ce_tests::fill_with_many_objects< Object >( id_cnt_copy2, bmic2, v2 );

   const auto& id_idx1 = bmic1.template get< ID_Index >();
   const auto& id_idx2 = bmic2.template get< ID_Index >();

   const auto& idx1 = bmic1.template get< Index >();
   const auto& idx2 = bmic2.template get< Index >();

   auto p1 = std::make_tuple( idx1.begin(), idx1.end(), &id_idx1 );
   auto p2 = std::make_tuple( idx2.begin(), idx2.end(), &id_idx2 );

   using Iterator = ce::concatenation_iterator_ex< Object, Cmp >;

   Iterator it( Cmp(), p1, p2 );
   Iterator it_end = Iterator::create_end( Cmp(), p1, p2 );

   auto sorted_it = sorted.begin();
   while( it != it_end )
   {
      BOOST_REQUIRE( ( *it ) == ( *sorted_it ) );

      ++it;
      ++sorted_it;
   }
}

template< typename Collection, typename ID_Index, typename Index, typename Object, typename Cmp >
void test_with_sub_index_3_sources_many_objects_with_id_repeat()
{
   Collection bmic1;
   Collection bmic2;
   Collection bmic3;

   using t_src = ce_tests::t_input_data;

   std::vector< t_src > v1 = 
   {
      /*
         size_t _length
         bool incremented1, std::string c1, size_t idx1
         bool incremented2, std::string c2, size_t idx2
      */
      t_src( 3, true,  "a", 0, true,  "b", 0 ),
      t_src( 5,  false, "c", 1, true,  "d", 10 ),
      t_src( 4,  true,  "e", 2, false, "b", 2 ),
      t_src( 100, true,  "b", 1000, true, "a", 100 ),
      t_src( 100,  false, "d", 1000, true,  "c", 150 ),
      t_src( 100,  true,  "b", 1000, false, "e", 200 ),
      t_src( 1000,  true,  "x1000-start", 1000, false, "x1000-end", 200 )
   };
   std::vector< t_src > v2 = 
   {
      t_src( 3, true,  ".a", 0, true,  ".b", 0 ),
      t_src( 5,  false, ".c", 1, true,  ".d", 10 ),
      t_src( 4,  true,  ".e", 2, false, ".b", 2 ),
      t_src( 100, true,  ".b", 1000, true, ".a", 100 ),
      t_src( 100,  false, ".d", 1000, true,  ".c", 150 ),
      t_src( 100,  true,  ".b", 1000, false, ".e", 200 ),
      t_src( 1000,  true,  ".x1000-start", 1000, false, ".x1000-end", 200 )
   };
   std::vector< t_src > v3 = 
   {
      t_src( 3, true,  "!.a", 0, true,  "!.b", 0 ),
      t_src( 5,  false, "!.c", 1, true,  "!.d", 10 ),
      t_src( 4,  true,  "!.e", 2, false, "!.b", 2 ),
      t_src( 100, true,  "!.b", 1000, true, "!.a", 100 ),
      t_src( 100,  false, "!.d", 1000, true,  "!.c", 150 ),
      t_src( 100,  true,  "!.b", 1000, false, "!.e", 200 ),
      t_src( 1000,  true,  "!.x1000-start", 1000, false, "!.x1000-end", 200 )
   };

   size_t total_size = 3+5+4 + 3*100 + 1000;

   int64_t id_cnt = 0;

   int64_t id_cnt_copy = id_cnt;
   int64_t id_cnt_copy1 = id_cnt;
   int64_t id_cnt_copy2 = id_cnt;
   int64_t id_cnt_copy3 = id_cnt;

   BOOST_TEST_MESSAGE( "3 filled sources with different content - many objects with key repeating ( sub-index is active )" );

   //Making sorted collection
   using t_sorted = std::set< Object, Cmp >;
   t_sorted sorted;
   ce_tests::fill_with_many_objects< Object >( id_cnt_copy, sorted, std::vector< t_src >( v3 ) );
   size_t sorted_size = sorted.size();
   //Making sorted collection

   BOOST_REQUIRE( total_size == sorted_size );

   ce_tests::fill_with_many_objects< Object >( id_cnt_copy1, bmic1, v1 );
   ce_tests::fill_with_many_objects< Object >( id_cnt_copy2, bmic2, v2 );
   ce_tests::fill_with_many_objects< Object >( id_cnt_copy3, bmic3, v3 );

   const auto& id_idx1 = bmic1.template get< ID_Index >();
   const auto& id_idx2 = bmic2.template get< ID_Index >();
   const auto& id_idx3 = bmic2.template get< ID_Index >();

   const auto& idx1 = bmic1.template get< Index >();
   const auto& idx2 = bmic2.template get< Index >();
   const auto& idx3 = bmic3.template get< Index >();

   auto p1 = std::make_tuple( idx1.begin(), idx1.end(), &id_idx1 );
   auto p2 = std::make_tuple( idx2.begin(), idx2.end(), &id_idx2 );
   auto p3 = std::make_tuple( idx3.begin(), idx3.end(), &id_idx3 );

   using Iterator = ce::concatenation_iterator_ex< Object, Cmp >;

   Iterator it( Cmp(), p1, p2, p3 );
   Iterator it_end = Iterator::create_end( Cmp(), p1, p2, p3 );

   auto sorted_it = sorted.begin();
   while( it != it_end )
   {
      BOOST_REQUIRE( ( *it ) == ( *sorted_it ) );

      ++it;
      ++sorted_it;
   }
}

template< typename Iterator, typename ReverseIterator, typename Collection, typename Object, typename Index, typename Cmp, typename Filler >
void different_test( Filler&& filler )
{
   Collection bmic1;

   BOOST_TEST_MESSAGE( "1 source - different tests" );
   filler( bmic1 );

   const auto& idx1 = bmic1.template get< Index >();

   auto p1 = std::make_tuple( idx1.begin(), idx1.end() );

   {
      Iterator it( Cmp(), p1 );
      Iterator it_begin( it );
      Iterator it_end = Iterator::create_end( Cmp(), p1 );

      ReverseIterator it_r( Cmp(), p1 );
      ReverseIterator it_r_begin( it_r );
      ReverseIterator it_r_end = Iterator::create_end( Cmp(), p1 );

      auto it_comparer = idx1.begin();
      auto it_r_comparer = idx1.rbegin();

      while( it != it_end )
      {
         BOOST_REQUIRE( *it == *it_comparer );
         BOOST_REQUIRE( *it_r == *it_r_comparer );

         it = std::next( it, 1 );
         it_r = std::next( it_r, 1 );
         ++it_comparer;
         ++it_r_comparer;
      }

      for( size_t i = 0; i<idx1.size(); ++i )
      {
         it = std::prev( it, 1 );
         it_r = std::prev( it_r, 1 );
         it_comparer = std::prev( it_comparer, 1 );
         it_r_comparer = std::prev( it_r_comparer, 1 );

         BOOST_REQUIRE( *it == *it_comparer );
         BOOST_REQUIRE( *it_r == *it_r_comparer );
      }

      it = it_begin;
      it_r = it_r_begin;
      it_comparer = idx1.begin();
      it_r_comparer = idx1.rbegin();

      it = std::next( it, 3 );
      it_r = std::next( it_r, 3 );
      it_comparer = std::next( it_comparer, 3 );
      it_r_comparer = std::next( it_r_comparer, 3 );
      BOOST_REQUIRE( *it == *it_comparer );
      BOOST_REQUIRE( *it_r == *it_r_comparer );

      it = std::prev( it, 2 );
      it_r = std::prev( it_r, 2 );
      it_comparer = std::prev( it_comparer, 2 );
      it_r_comparer = std::prev( it_r_comparer, 2 );
      BOOST_REQUIRE( *it == *it_comparer );
      BOOST_REQUIRE( *it_r == *it_r_comparer );

      it = std::prev( it, 1 );
      it_r = std::prev( it_r, 1 );
      it_comparer = std::prev( it_comparer, 1 );
      it_r_comparer = std::prev( it_r_comparer, 1 );
      BOOST_REQUIRE( *it == *it_comparer );
      BOOST_REQUIRE( *it_r == *it_r_comparer );
   }

   {
      ReverseIterator it_r_end = ReverseIterator::create_end( Cmp(), p1 );
      it_r_end = std::prev( it_r_end, 1 );
      auto it_comparer = idx1.begin();

      while( it_comparer != idx1.end() )
      {
         BOOST_REQUIRE( *it_r_end == *it_comparer );
         --it_r_end;
         it_comparer++;
      }
   }

   {
      ReverseIterator it_r_end = ReverseIterator::create_end( Cmp(), p1 );

      ReverseIterator it_r( Iterator::create_end( Cmp(), p1 ) );
      decltype( idx1.rbegin() ) it_r_comparer( idx1.end() );

      BOOST_REQUIRE( it_r == ReverseIterator( Cmp(), p1 ) );
      BOOST_REQUIRE( *it_r == *it_r_comparer );

      while( it_r != it_r_end )
      {
         BOOST_REQUIRE( *it_r == *it_r_comparer );

         it_r = std::next( it_r, 1 );
         it_r_comparer = std::next( it_r_comparer, 1 );
      }
   }

   {
      auto it_comparer = idx1.end();
      it_comparer = std::prev( it_comparer, 1 );

      ReverseIterator it_r( Cmp(), p1 );
      std::reverse_iterator< Iterator > std_it_r ( it_r );
      Iterator it = std_it_r.base();
      BOOST_REQUIRE( *it == *it_comparer );

      it_comparer = std::prev( it_comparer, 1 );
      it = std::prev( it, 1 );
      BOOST_REQUIRE( *it == *it_comparer );
   }

   {
      auto it_comparer = idx1.end();
      Iterator it = Iterator::create_end( Cmp(), p1 );
      --it;
      --it_comparer;
      BOOST_REQUIRE( *it == *it_comparer );
   }

   {
      auto it_comparer = idx1.begin();

      ReverseIterator it_r = ReverseIterator::create_end( Cmp(), p1 );
      it_r--;
      std::reverse_iterator< Iterator > std_it_r ( it_r );
      Iterator it = std_it_r.base();
      BOOST_REQUIRE( *it == *it_comparer );
   }
}

template< typename Iterator, typename ReverseIterator, typename Collection, typename Object, typename ID_Index, typename Index, typename Cmp, typename Filler1, typename Filler2, typename Filler3, typename SortedFiller >
void different_test_sub_index( Filler1& filler1, Filler2& filler2, Filler3& filler3, SortedFiller& sorted_filler )
{
   Collection bmic1;
   Collection bmic2;
   Collection bmic3;

   Collection comparer;

   BOOST_TEST_MESSAGE( "3 sources - different tests ( sub-index is active )" );
   filler1( bmic1 );
   filler2( bmic2 );
   filler3( bmic3 );
   sorted_filler( comparer );

   const auto& id_idx1 = bmic1.template get< ID_Index >();
   const auto& id_idx2 = bmic2.template get< ID_Index >();
   const auto& id_idx3 = bmic3.template get< ID_Index >();

   const auto& idx1 = bmic1.template get< Index >();
   const auto& idx2 = bmic2.template get< Index >();
   const auto& idx3 = bmic3.template get< Index >();

   const auto& comparer_idx = comparer.template get< Index >();
   auto it_comparer = comparer_idx.begin();
   auto it_r_comparer = comparer_idx.rbegin();

   {
      Iterator it( Cmp(),
            std::make_tuple( idx1.begin(), idx1.end(), &id_idx1 ),
            std::make_tuple( idx2.begin(), idx2.end(), &id_idx2 ),
            std::make_tuple( idx3.begin(), idx3.end(), &id_idx3 )
         );
      Iterator it_begin( it );
      Iterator it_end = Iterator::create_end( Cmp(),
            std::make_tuple( idx1.begin(), idx1.end(), &id_idx1 ),
            std::make_tuple( idx2.begin(), idx2.end(), &id_idx2 ),
            std::make_tuple( idx3.begin(), idx3.end(), &id_idx3 )
         );

      ReverseIterator it_r( Cmp(),
            std::make_tuple( idx1.begin(), idx1.end(), &id_idx1 ),
            std::make_tuple( idx2.begin(), idx2.end(), &id_idx2 ),
            std::make_tuple( idx3.begin(), idx3.end(), &id_idx3 )
         );
      ReverseIterator it_r_begin( it_r );
      ReverseIterator it_r_end = ReverseIterator::create_end( Cmp(),
            std::make_tuple( idx1.begin(), idx1.end(), &id_idx1 ),
            std::make_tuple( idx2.begin(), idx2.end(), &id_idx2 ),
            std::make_tuple( idx3.begin(), idx3.end(), &id_idx3 )
         );

      while( it != it_end )
      {
         BOOST_REQUIRE( *it == *it_comparer );
         BOOST_REQUIRE( *it_r == *it_r_comparer );

         it = std::next( it, 1 );
         it_r = std::next( it_r, 1 );
         ++it_comparer;
         ++it_r_comparer;
      }

      for( size_t i = 0; i<comparer_idx.size(); ++i )
      {
         it = std::prev( it, 1 );
         it_r = std::prev( it_r, 1 );
         it_comparer = std::prev( it_comparer, 1 );
         it_r_comparer = std::prev( it_r_comparer, 1 );

         BOOST_REQUIRE( *it == *it_comparer );
         BOOST_REQUIRE( *it_r == *it_r_comparer );
      }

      it = it_begin;
      it_r = it_r_begin;
      it_comparer = comparer_idx.begin();
      it_r_comparer = comparer_idx.rbegin();

      it = std::next( it, 3 );
      it_r = std::next( it_r, 3 );
      it_comparer = std::next( it_comparer, 3 );
      it_r_comparer = std::next( it_r_comparer, 3 );
      BOOST_REQUIRE( *it == *it_comparer );
      BOOST_REQUIRE( *it_r == *it_r_comparer );

      it = std::prev( it, 2 );
      it_r = std::prev( it_r, 2 );
      it_comparer = std::prev( it_comparer, 2 );
      it_r_comparer = std::prev( it_r_comparer, 2 );
      BOOST_REQUIRE( *it == *it_comparer );
      BOOST_REQUIRE( *it_r == *it_r_comparer );

      it = std::prev( it, 1 );
      it_r = std::prev( it_r, 1 );
      it_comparer = std::prev( it_comparer, 1 );
      it_r_comparer = std::prev( it_r_comparer, 1 );
      BOOST_REQUIRE( *it == *it_comparer );
      BOOST_REQUIRE( *it_r == *it_r_comparer );
   }

   {
      ReverseIterator it_r_end = ReverseIterator::create_end( Cmp(),
            std::make_tuple( idx1.begin(), idx1.end(), &id_idx1 ),
            std::make_tuple( idx2.begin(), idx2.end(), &id_idx2 ),
            std::make_tuple( idx3.begin(), idx3.end(), &id_idx3 )
         );

      ReverseIterator it_r( Iterator::create_end( Cmp(),
            std::make_tuple( idx1.begin(), idx1.end(), &id_idx1 ),
            std::make_tuple( idx2.begin(), idx2.end(), &id_idx2 ),
            std::make_tuple( idx3.begin(), idx3.end(), &id_idx3 )
         ) );
      decltype( comparer_idx.rbegin() ) it_r_comparer( comparer_idx.end() );

      BOOST_REQUIRE( it_r == ReverseIterator( Cmp(), 
            std::make_tuple( idx1.begin(), idx1.end(), &id_idx1 ),
            std::make_tuple( idx2.begin(), idx2.end(), &id_idx2 ),
            std::make_tuple( idx3.begin(), idx3.end(), &id_idx3 ) ) );
      BOOST_REQUIRE( *it_r == *it_r_comparer );

      while( it_r != it_r_end )
      {
         BOOST_REQUIRE( *it_r == *it_r_comparer );

         it_r = std::next( it_r, 1 );
         it_r_comparer = std::next( it_r_comparer, 1 );
      }
   }
}

template< typename ReverseIterator, typename Collection, typename Object, typename Index, typename Cmp, typename Filler >
void inc_dec_basic_reverse_test( Filler&& filler )
{
   Collection bmic1;

   BOOST_TEST_MESSAGE( "1 source - reverse_iterator: operations: '++'" );
   filler( bmic1 );

   const auto& idx1 = bmic1.template get< Index >();

   auto p1 = std::make_tuple( idx1.begin(), idx1.end() );
   ReverseIterator it_r( Cmp(), p1 );
   ReverseIterator it_end_r = ReverseIterator::create_end( Cmp(), p1 );

   auto it_comparer_r = idx1.rbegin();
   auto it01_r( it_r );

   while( it_r != it_end_r )
   {
      BOOST_REQUIRE( *it_r == *it_comparer_r );
      ++it_r;
      ++it_comparer_r;
   }

   BOOST_REQUIRE( it_r == it_end_r );

   it_comparer_r = idx1.rbegin();
   while( it_comparer_r != idx1.rend() )
   {
      BOOST_REQUIRE( *it01_r == *it_comparer_r );
      ++it01_r;
      ++it_comparer_r;
   }
}

template< typename Iterator, typename ReverseIterator, typename Collection, typename Object, typename Index, typename Cmp, typename Filler >
void inc_dec_basic_1_source_test( Filler&& filler )
{
   Collection bmic1;

   BOOST_TEST_MESSAGE( "1 source - operations: '++' and '--'" );
   filler( bmic1 );

   const auto& idx1 = bmic1.template get< Index >();

   auto p1 = std::make_tuple( idx1.begin(), idx1.end() );
   Iterator it( Cmp(), p1 );
   ReverseIterator it_r( Cmp(), p1 );
   ReverseIterator it_end_r = ReverseIterator::create_end( Cmp(), p1 );

   auto it_comparer = idx1.begin();
   auto it_comparer_r = idx1.rbegin();

   BOOST_REQUIRE( *it_r == *it_comparer_r );
   BOOST_REQUIRE( *it == *it_comparer );

   ++it;
   ++it_r;
   ++it_comparer;
   ++it_comparer_r;

   --it;
   --it_r;
   --it_comparer;
   --it_comparer_r;

   BOOST_REQUIRE( *it_r == *it_comparer_r );
   BOOST_REQUIRE( *it == *it_comparer );

   ++it;
   ++it_r;
   ++it_comparer;
   ++it_comparer_r;

   BOOST_REQUIRE( *it_r == *it_comparer_r );
   BOOST_REQUIRE( *it == *it_comparer );

   ++it;
   ++it_r;
   ++it_comparer;
   ++it_comparer_r;

   --it;
   --it_r;
   --it_comparer;
   --it_comparer_r;

   BOOST_REQUIRE( *it_r == *it_comparer_r );
   BOOST_REQUIRE( *it == *it_comparer );

   ++it;
   ++it_r;
   ++it_comparer;
   ++it_comparer_r;

   BOOST_REQUIRE( *it_r == *it_comparer_r );
   BOOST_REQUIRE( *it == *it_comparer );

   ++it;
   ++it_r;
   ++it_comparer;
   ++it_comparer_r;

   --it;
   --it_r;
   --it_comparer;
   --it_comparer_r;

   BOOST_REQUIRE( *it_r == *it_comparer_r );
   BOOST_REQUIRE( *it == *it_comparer );

   ++it;
   ++it_r;
   ++it_comparer;
   ++it_comparer_r;

   BOOST_REQUIRE( *it_r == *it_comparer_r );
   BOOST_REQUIRE( *it == *it_comparer );

   ++it;
   ++it_r;
   ++it_comparer;
   ++it_comparer_r;

   BOOST_REQUIRE( it_r == it_end_r );

   --it;
   --it_r;
   --it_comparer;
   --it_comparer_r;

   BOOST_REQUIRE( *it_r == *it_comparer_r );
   BOOST_REQUIRE( *it == *it_comparer );

   --it;
   --it_r;
   --it_comparer;
   --it_comparer_r;

   BOOST_REQUIRE( *it_r == *it_comparer_r );
   BOOST_REQUIRE( *it == *it_comparer );

   --it;
   --it_r;
   --it_comparer;
   --it_comparer_r;

   BOOST_REQUIRE( *it_r == *it_comparer_r );
   BOOST_REQUIRE( *it == *it_comparer );

   --it;
   --it_r;
   --it_comparer;
   --it_comparer_r;

   BOOST_REQUIRE( *it_r == *it_comparer_r );
   BOOST_REQUIRE( *it == *it_comparer );
}

template< typename Collection, typename Object, typename Index, typename Cmp, typename Filler1, typename Filler2, typename Filler3, typename SortedFiller >
void inc_dec_basic_3_sources_test( Filler1& filler1, Filler2& filler2, Filler3& filler3, SortedFiller& sorted_filler )
{
   Collection bmic1;
   Collection bmic2;
   Collection bmic3;

   Collection comparer;

   BOOST_TEST_MESSAGE( "3 sources - operations: '++' and '--'" );
   filler1( bmic1 );
   filler2( bmic2 );
   filler3( bmic3 );
   sorted_filler( comparer );

   const auto& idx1 = bmic1.template get< Index >();
   const auto& idx2 = bmic2.template get< Index >();
   const auto& idx3 = bmic3.template get< Index >();

   ce::concatenation_iterator< Object, Cmp > it( Cmp(),
                                                   std::make_tuple( idx1.begin(), idx1.end() ),
                                                   std::make_tuple( idx2.begin(), idx2.end() ),
                                                   std::make_tuple( idx3.begin(), idx3.end() )
                                                );

   auto it_comparer = comparer.begin();

   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it;
   ++it_comparer;

   --it;
   --it_comparer;

   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it;
   ++it_comparer;

   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it;
   ++it_comparer;

   --it;
   --it_comparer;

   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it;
   ++it_comparer;

   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it;
   ++it_comparer;

   --it;
   --it_comparer;

   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it;
   ++it_comparer;

   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it;
   ++it_comparer;

   --it;
   --it_comparer;

   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it;
   ++it_comparer;

   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it;
   ++it_comparer;

   --it;
   --it_comparer;

   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it;
   ++it_comparer;

   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it;
   ++it_comparer;

   --it;
   --it_comparer;

   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it;
   --it_comparer;

   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it;
   --it_comparer;

   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it;
   --it_comparer;

   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it;
   --it_comparer;

   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it;
   --it_comparer;

   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
}

template< typename Collection, typename Object, typename Index, typename Cmp,
typename Filler1, typename Filler2, typename Filler3, typename Filler4, typename Filler5, typename Filler6, typename SortedFiller >
void inc_dec_basic_6_sources_test( Filler1& f1, Filler2& f2, Filler3& f3, Filler4& f4, Filler5& f5, Filler6& f6, SortedFiller& sorted_filler )
{
   Collection bmic1;
   Collection bmic2;
   Collection bmic3;
   Collection bmic4;
   Collection bmic5;
   Collection bmic6;

   Collection comparer;

   BOOST_TEST_MESSAGE( "6 sources - operations: '++' and '--'" );
   f1( bmic1 );
   f2( bmic2 );
   f3( bmic3 );
   f4( bmic4 );
   f5( bmic5 );
   f6( bmic6 );
   sorted_filler( comparer );

   const auto& idx1 = bmic1.template get< Index >();
   const auto& idx2 = bmic2.template get< Index >();
   const auto& idx3 = bmic3.template get< Index >();
   const auto& idx4 = bmic4.template get< Index >();
   const auto& idx5 = bmic5.template get< Index >();
   const auto& idx6 = bmic6.template get< Index >();

   ce::concatenation_iterator< Object, Cmp > it( Cmp(),
                                                   std::make_tuple( idx1.begin(), idx1.end() ),
                                                   std::make_tuple( idx2.begin(), idx2.end() ),
                                                   std::make_tuple( idx3.begin(), idx3.end() ),
                                                   std::make_tuple( idx4.begin(), idx4.end() ),
                                                   std::make_tuple( idx5.begin(), idx5.end() ),
                                                   std::make_tuple( idx6.begin(), idx6.end() )
                                                );

   auto it_comparer = comparer.begin();
   size_t size = comparer.size();

   for( size_t i = 0 ; i < size; ++i )
   {
      BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
      ++it;
      ++it_comparer;
   }

   --it;
   --it_comparer;

   for( size_t i = 1 ; i < size; ++i )
   {
      BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
      --it;
      --it_comparer;
   }

   ++it; ++it_comparer;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
}

template< typename Iterator, typename ReverseIterator, typename Collection, typename Object, typename ID_Index, typename Index, typename Cmp,
typename Filler1, typename Filler2, typename Filler3, typename SortedFiller >
void inc_dec_basic_3_sources_sub_index_test( Filler1& f1, Filler2& f2, Filler3& f3, SortedFiller& sorted_filler )
{
   Collection bmic1;
   Collection bmic2;
   Collection bmic3;

   Collection comparer;

   BOOST_TEST_MESSAGE( "3 sources - operations: '++' and '--' ( sub-index is active )" );
   f1( bmic1 );
   f2( bmic2 );
   f3( bmic3 );
   sorted_filler( comparer );

   const auto& id_idx1 = bmic1.template get< ID_Index >();
   const auto& id_idx2 = bmic2.template get< ID_Index >();
   const auto& id_idx3 = bmic3.template get< ID_Index >();

   const auto& idx1 = bmic1.template get< Index >();
   const auto& idx2 = bmic2.template get< Index >();
   const auto& idx3 = bmic3.template get< Index >();

   Iterator it( Cmp(),
                  std::make_tuple( idx1.begin(), idx1.end(), &id_idx1 ),
                  std::make_tuple( idx2.begin(), idx2.end(), &id_idx2 ),
                  std::make_tuple( idx3.begin(), idx3.end(), &id_idx3 )
               );

   ReverseIterator it_r( Cmp(),
                           std::make_tuple( idx1.begin(), idx1.end(), &id_idx1 ),
                           std::make_tuple( idx2.begin(), idx2.end(), &id_idx2 ),
                           std::make_tuple( idx3.begin(), idx3.end(), &id_idx3 )
                        );

   const auto& comparer_idx = comparer.template get< Index >();
   auto it_comparer = comparer_idx.begin();
   auto it_comparer_r = comparer_idx.rbegin();

   size_t size = comparer_idx.size();

   for( size_t i = 0 ; i < size; ++i )
   {
      BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
      BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );
      ++it;
      ++it_comparer;
      ++it_r;
      ++it_comparer_r;
   }

   for( size_t i = 0 ; i < size; ++i )
   {
      --it;
      --it_comparer;
      --it_r;
      --it_comparer_r;
      BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
      BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );
   }

   ++it; ++it_comparer;
   ++it_r; ++it_comparer_r;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );

   ++it; ++it_comparer;
   ++it_r; ++it_comparer_r;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );

   --it; --it_comparer;
   --it_r; --it_comparer_r;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );

   ++it; ++it_comparer;
   ++it_r; ++it_comparer_r;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );

   ++it; ++it_comparer;
   ++it_r; ++it_comparer_r;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );

   ++it; ++it_comparer;
   ++it_r; ++it_comparer_r;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );

   ++it; ++it_comparer;
   ++it_r; ++it_comparer_r;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );

   --it; --it_comparer;
   --it_r; --it_comparer_r;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );

   --it; --it_comparer;
   --it_r; --it_comparer_r;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );

   --it; --it_comparer;
   --it_r; --it_comparer_r;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );

   --it; --it_comparer;
   --it_r; --it_comparer_r;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );

   --it; --it_comparer;
   --it_r; --it_comparer_r;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );

   ++it; ++it_comparer;
   ++it_r; ++it_comparer_r;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );

   ++it; ++it_comparer;
   ++it_r; ++it_comparer_r;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );

   ++it; ++it_comparer;
   ++it_r; ++it_comparer_r;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );

   ++it; ++it_comparer;
   ++it_r; ++it_comparer_r;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );

   ++it; ++it_comparer;
   ++it_r; ++it_comparer_r;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );

   ++it; ++it_comparer;
   ++it_r; ++it_comparer_r;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );

   --it; --it_comparer;
   --it_r; --it_comparer_r;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );

   ++it; ++it_comparer;
   ++it_r; ++it_comparer_r;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );

   --it; --it_comparer;
   --it_r; --it_comparer_r;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );

   ++it; ++it_comparer;
   ++it_r; ++it_comparer_r;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );

   --it; --it_comparer;
   --it_r; --it_comparer_r;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );

   --it; --it_comparer;
   --it_r; --it_comparer_r;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );

   --it; --it_comparer;
   --it_r; --it_comparer_r;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );

   --it; --it_comparer;
   --it_r; --it_comparer_r;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );

   ++it; ++it_comparer;
   ++it_r; ++it_comparer_r;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );

   --it; --it_comparer;
   --it_r; --it_comparer_r;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );

   --it; --it_comparer;
   --it_r; --it_comparer_r;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );

   ++it; ++it_comparer;
   ++it_r; ++it_comparer_r;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );

   --it; --it_comparer;
   --it_r; --it_comparer_r;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );

   --it; --it_comparer;
   --it_r; --it_comparer_r;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );

   ++it; ++it_comparer;
   ++it_r; ++it_comparer_r;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );

   --it; --it_comparer;
   --it_r; --it_comparer_r;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   BOOST_REQUIRE( ( *it_r ) == ( *it_comparer_r ) );
}

template< typename Iterator, typename Collection, typename Object, typename Index, typename Cmp, typename Filler1, typename Filler2, typename Filler3, typename SortedFiller >
void comparision_assignment_test( Filler1& filler1, Filler2& filler2, Filler3& filler3, SortedFiller& sorted_filler )
{
   Collection bmic1;
   Collection bmic2;
   Collection bmic3;

   Collection comparer;

   BOOST_TEST_MESSAGE( "3 sources - assignments, comparisions" );
   filler1( bmic1 );
   filler2( bmic2 );
   filler3( bmic3 );
   sorted_filler( comparer );

   const auto& idx1 = bmic1.template get< Index >();
   const auto& idx2 = bmic2.template get< Index >();
   const auto& idx3 = bmic3.template get< Index >();

   using _t = Iterator;

   _t _it( Cmp(),
            std::make_tuple( idx1.begin(), idx1.end() ),
            std::make_tuple( idx2.begin(), idx2.end() ),
            std::make_tuple( idx3.begin(), idx3.end() )
         );

   _t it_end = _t::create_end( Cmp(),
            std::make_tuple( idx1.begin(), idx1.end() ),
            std::make_tuple( idx2.begin(), idx2.end() ),
            std::make_tuple( idx3.begin(), idx3.end() )
         );

   _t it_begin( _it );
   _t it( _it );

   BOOST_REQUIRE( it == _it );
   BOOST_REQUIRE( it == it_begin );
   BOOST_REQUIRE( _it == it_begin );
   BOOST_REQUIRE( it != it_end );

   ++it;

   BOOST_REQUIRE( _it == it_begin );

   --it;
   BOOST_REQUIRE( it == it_begin );

   ++_it;
   ++_it;
   --_it;
   --_it;
   BOOST_REQUIRE( _it == it_begin );
   BOOST_REQUIRE( _it == it );

   auto it_comparer = comparer.begin();
   while( it != it_end )
   {
      BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
      ++it;
      ++it_comparer;
   }

   it = it_begin;
   it_comparer = comparer.begin();
   while( it != it_end )
   {
      BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
      ++it;
      ++it_comparer;
   }

   it = it_end;
   BOOST_REQUIRE( it == it_end );

   BOOST_REQUIRE( _it == it_begin );
   ++_it;
   ++_it;
   it_comparer = comparer.begin();
   ++it_comparer;
   ++it_comparer;
   BOOST_REQUIRE( ( *_it ) == ( *it_comparer ) );
   it = _it;
   BOOST_REQUIRE( it == _it );
   while( it != it_end )
   {
      BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
      ++it;
      ++it_comparer;
   }
}

template< typename Iterator, typename Collection, typename Object, typename ID_Index, typename Index, typename Cmp, typename Filler1, typename Filler2, typename Filler3, typename SortedFiller >
void comparision_assignment_sub_index_test( Filler1& filler1, Filler2& filler2, Filler3& filler3, SortedFiller& sorted_filler )
{
   Collection bmic1;
   Collection bmic2;
   Collection bmic3;

   Collection comparer;

   BOOST_TEST_MESSAGE( "3 sources - assignments, comparisions ( sub-index is active )" );
   filler1( bmic1 );
   filler2( bmic2 );
   filler3( bmic3 );
   sorted_filler( comparer );

   using _t = Iterator;

   const auto& id_idx1 = bmic1.template get< ID_Index >();
   const auto& id_idx2 = bmic2.template get< ID_Index >();
   const auto& id_idx3 = bmic3.template get< ID_Index >();

   const auto& idx1 = bmic1.template get< Index >();
   const auto& idx2 = bmic2.template get< Index >();
   const auto& idx3 = bmic3.template get< Index >();

   _t _it( Cmp(),
            std::make_tuple( idx1.begin(), idx1.end(), &id_idx1 ),
            std::make_tuple( idx2.begin(), idx2.end(), &id_idx2 ),
            std::make_tuple( idx3.begin(), idx3.end(), &id_idx3 )
         );

   _t it_end = _t::create_end( Cmp(),
            std::make_tuple( idx1.begin(), idx1.end(), &id_idx1 ),
            std::make_tuple( idx2.begin(), idx2.end(), &id_idx2 ),
            std::make_tuple( idx3.begin(), idx3.end(), &id_idx3 )
         );

   _t it_begin( _it );
   _t it( _it );

   BOOST_REQUIRE( it == _it );
   BOOST_REQUIRE( it == it_begin );
   BOOST_REQUIRE( _it == it_begin );
   BOOST_REQUIRE( it != it_end );

   ++it;

   BOOST_REQUIRE( _it == it_begin );

   --it;
   BOOST_REQUIRE( it == it_begin );

   ++_it;
   ++_it;
   --_it;
   --_it;
   BOOST_REQUIRE( _it == it_begin );
   BOOST_REQUIRE( _it == it );

   const auto& comparer_idx = comparer.template get< Index >();
   auto it_comparer = comparer_idx.begin();

   while( it != it_end )
   {
      BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
      ++it;
      ++it_comparer;
   }

   it = it_begin;
   it_comparer = comparer_idx.begin();
   while( it != it_end )
   {
      BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
      ++it;
      ++it_comparer;
   }

   it = it_end;
   BOOST_REQUIRE( it == it_end );

   BOOST_REQUIRE( _it == it_begin );
   ++_it;
   ++_it;
   it_comparer = comparer_idx.begin();
   ++it_comparer;
   ++it_comparer;
   BOOST_REQUIRE( ( *_it ) == ( *it_comparer ) );
   it = _it;
   BOOST_REQUIRE( it == _it );
   while( it != it_end )
   {
      BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
      ++it;
      ++it_comparer;
   }
}

template< typename Iterator, typename Collection, typename Object, typename ID_Index, typename Index, typename Cmp, typename Filler1, typename Filler2, typename Filler3, typename SortedFiller >
void misc_operations_sub_index_test( Filler1& filler1, Filler2& filler2, Filler3& filler3, SortedFiller& sorted_filler )
{
   Collection bmic1;
   Collection bmic2;
   Collection bmic3;

   Collection comparer;

   BOOST_TEST_MESSAGE( "3 sources - miscellaneous operations ( sub-index is active )" );
   filler1( bmic1 );
   filler2( bmic2 );
   filler3( bmic3 );
   sorted_filler( comparer );

   using _t = Iterator;

   const auto& id_idx1 = bmic1.template get< ID_Index >();
   const auto& id_idx2 = bmic2.template get< ID_Index >();
   const auto& id_idx3 = bmic3.template get< ID_Index >();

   const auto& idx1 = bmic1.template get< Index >();
   const auto& idx2 = bmic2.template get< Index >();
   const auto& idx3 = bmic3.template get< Index >();

   _t begin( Cmp(),
            std::make_tuple( idx1.begin(), idx1.end(), &id_idx1 ),
            std::make_tuple( idx2.begin(), idx2.end(), &id_idx2 ),
            std::make_tuple( idx3.begin(), idx3.end(), &id_idx3 )
         );

   _t end = _t::create_end( Cmp(),
            std::make_tuple( idx1.begin(), idx1.end(), &id_idx1 ),
            std::make_tuple( idx2.begin(), idx2.end(), &id_idx2 ),
            std::make_tuple( idx3.begin(), idx3.end(), &id_idx3 )
         );

   const auto& comparer_idx = comparer.template get< Index >();
   auto comp = comparer_idx.begin();

   _t it_01( begin );
   BOOST_REQUIRE( it_01 == begin );
  
   it_01++;
   ++comp;
   BOOST_REQUIRE( *it_01 == *comp );

   it_01--;
   --comp;
   BOOST_REQUIRE( *it_01 == *comp );
   BOOST_REQUIRE( it_01 == begin );

   auto it_02( it_01++ );
   BOOST_REQUIRE( *it_02 == *comp );
   BOOST_REQUIRE( it_02 == begin );

   it_01--;
   BOOST_REQUIRE( it_02 == it_01 );

   it_01++;
   it_01++;
   it_01++;
   comp = std::next( comp, 3 );
   BOOST_REQUIRE( *it_01 == *comp );

   auto it_03( it_01-- );
   BOOST_REQUIRE( *it_03 == *comp );
   it_01++;
   BOOST_REQUIRE( it_03 == it_01 );

   auto it_04( it_01 );
   auto it_05( it_01 );

   ++it_04;
   it_05++;
   BOOST_REQUIRE( it_04 == it_05 );
   --it_05;

   auto it_06( it_05-- );
   ++it_05;
   BOOST_REQUIRE( it_06 == it_05 );

   auto it_07( end );
   comp = comparer_idx.end();
   --it_07;
   --comp;
   BOOST_REQUIRE( *it_07 == *comp );

   auto end_02( end );
   auto it_08( end_02-- );
   it_08--;
   BOOST_REQUIRE( *it_08 == *comp );

   auto end_03( end );
   auto it_09( --end_03 );
   comp = comparer_idx.end();
   --comp;
   BOOST_REQUIRE( *it_09 == *comp );

   it_09++;
   BOOST_REQUIRE( it_09 == end );

   comp = comparer_idx.begin();
   it_09 = begin;
   BOOST_REQUIRE( it_09 == begin );
   BOOST_REQUIRE( *it_09 == *comp );

   it_09 = end;
   BOOST_REQUIRE( it_09 == end );

   it_08 = it_09;
   BOOST_REQUIRE( it_08 == end );

   it_08 = it_09 = begin;
   BOOST_REQUIRE( ( it_08 == it_09 ) && ( it_09 == begin ) );
   BOOST_REQUIRE( *it_09 == *comp );

   it_08 = ++it_09;
   BOOST_REQUIRE( it_08 == it_09 );
   ++comp;
   BOOST_REQUIRE( *it_09 == *comp );

   it_08 = it_09++;
   BOOST_REQUIRE( it_08 != it_09 );
   ++comp;
   BOOST_REQUIRE( *it_09 == *comp );

   it_08++;
   --it_08;
   ++it_08;
   it_08--;
   it_08++;
   BOOST_REQUIRE( it_08 == it_09 );
}

BOOST_AUTO_TEST_SUITE(concatenation_enumeration_tests)

BOOST_AUTO_TEST_CASE(benchmark_tests)
{
   using obj2 = ce_tests::test_object2;
   using bmic2 = ce_tests::test_object_index2;
   using oidx2 = ce_tests::OrderedIndex2;
   using c_oidx2 = ce_tests::CompositeOrderedIndex2;
   using cmp4 = ce_tests::cmp4;

   benchmark_test_2_sources
   <
      false/*Is_Another_Source*/,
      bmic2,
      oidx2,
      c_oidx2,
      obj2,
      cmp4
   >();

   benchmark_test_3_sources
   <
      false/*Is_Another_Source*/,
      bmic2,
      oidx2,
      c_oidx2,
      obj2,
      cmp4
   >();

   benchmark_test_2_sources
   <
      true/*Is_Another_Source*/,
      bmic2,
      oidx2,
      c_oidx2,
      obj2,
      cmp4
   >();

   benchmark_test_3_sources
   <
      true/*Is_Another_Source*/,
      bmic2,
      oidx2,
      c_oidx2,
      obj2,
      cmp4
   >();
/*
Checked on hardware:
	16 GB RAM
	Intel® Core™ i5-7400 CPU @ 3.00GHz × 4
	OS-type 64 bit

************************************************************************************************************************
'benchmark_test_2_sources - Is_Another_Source( false )'
`2 levels( persistent_state( 1mln objects ) + volatile state( 100 000 objects ) -> keys searching in collection with length: 100 000 objects`
iterator: bmic iterator: 33 ms
iterator: concatenation iterator: 143 ms
ratio: 4.33333
reverse_iterator: bmic iterator: 42 ms
reverse_iterator: concatenation iterator: 126 ms
ratio: 3

'benchmark_test_2_sources - Is_Another_Source( true )'
2 levels( persistent_state( 1mln objects ) + volatile state( 100 000 objects ) -> keys searching in collection with length: 0 objects
iterator: bmic iterator: 39 ms
iterator: concatenation iterator: 77 ms
ratio: 1.97436
reverse_iterator: bmic iterator: 44 ms
reverse_iterator: concatenation iterator: 62 ms
ratio: 1.40909

************************************************************************************************************************

'benchmark_test_3_sources - Is_Another_Source( false )'
`3 levels( persistent_state( 1mln objects ) + volatile state( 100 000 objects ) + volatile state( 100 000 objects ) -> keys searching in 2 collections - every has 100 000 objects`
iterator: bmic iterator: 48 ms
iterator: concatenation iterator: 354 ms
ratio: 7.375
reverse_iterator: bmic iterator: 44 ms
reverse_iterator: concatenation iterator: 260 ms
ratio: 5.90909

'benchmark_test_3_sources - Is_Another_Source( true )'
`3 levels( persistent_state( 1mln objects ) + volatile state( 100 000 objects ) + volatile state( 100 000 objects ) -> keys searching in 2 collections - every has 0 objects`
iterator: bmic iterator: 46 ms
iterator: concatenation iterator: 246 ms
ratio: 5.34783
reverse_iterator: bmic iterator: 45 ms
reverse_iterator: concatenation iterator: 153 ms
ratio: 3.4
*/
}

BOOST_AUTO_TEST_CASE(different_tests)
{
   using obj = ce_tests::test_object;
   using bmic = ce_tests::test_object_index;
   using oidx = ce_tests::OrderedIndex;

   {
      using cmp1 = ce_tests::cmp1;
      using iterator = ce::concatenation_iterator< obj, cmp1 >;
      using reverse_iterator = ce::concatenation_reverse_iterator< obj, cmp1 >;

      auto f1 = []( bmic& collection ){ ce_tests::fill6< obj >( collection ); };

      different_test< iterator, reverse_iterator, bmic, obj, oidx, cmp1 >( f1 );
   }

   {
      using cmp2 = ce_tests::cmp2;
      using oidx_a = ce_tests::CompositeOrderedIndexA;
      using iterator = ce::concatenation_iterator_ex< obj, cmp2 >;
      using reverse_iterator = ce::concatenation_reverse_iterator_ex< obj, cmp2 >;

      auto f1 = []( bmic& collection ){ ce_tests::fill9< obj >( collection ); };
      auto f2 = []( bmic& collection ){ ce_tests::fill9a< obj >( collection ); };
      auto f3 = []( bmic& collection ){ ce_tests::fill9b< obj >( collection ); };
      auto s = []( bmic& collection ){ ce_tests::sort9< obj >( collection ); };

      different_test_sub_index< iterator, reverse_iterator, bmic, obj, oidx, oidx_a, cmp2 >( f1, f2, f3, s );
   }
}

BOOST_AUTO_TEST_CASE(inc_dec_basic_reverse_tests)
{
   using obj = ce_tests::test_object;
   using bmic = ce_tests::test_object_index;
   using cmp1 = ce_tests::cmp1;
   using oidx = ce_tests::OrderedIndex;

   auto f1 = []( bmic& collection ){ ce_tests::fill6< obj >( collection ); };
   using reverse_iterator = ce::concatenation_reverse_iterator< obj, cmp1 >;

   inc_dec_basic_reverse_test< reverse_iterator, bmic, obj, oidx, cmp1 >( f1 );
}

BOOST_AUTO_TEST_CASE(misc_operations_sub_index_tests)
{
   using obj = ce_tests::test_object;
   using bmic = ce_tests::test_object_index;
   using cmp2 = ce_tests::cmp2;
   using oidx = ce_tests::OrderedIndex;
   using oidx_a = ce_tests::CompositeOrderedIndexA;

   auto f1 = []( bmic& collection ){ ce_tests::fill9< obj >( collection ); };
   auto f2 = []( bmic& collection ){ ce_tests::fill9a< obj >( collection ); };
   auto f3 = []( bmic& collection ){ ce_tests::fill9b< obj >( collection ); };
   auto s = []( bmic& collection ){ ce_tests::sort9< obj >( collection ); };

   misc_operations_sub_index_test< ce::concatenation_iterator_ex< obj, cmp2 >, bmic, obj, oidx, oidx_a, cmp2 >( f1, f2, f3, s );
}

BOOST_AUTO_TEST_CASE(comparision_assignment_sub_index_tests)
{
   using obj = ce_tests::test_object;
   using bmic = ce_tests::test_object_index;
   using cmp2 = ce_tests::cmp2;
   using oidx = ce_tests::OrderedIndex;
   using oidx_a = ce_tests::CompositeOrderedIndexA;

   auto f1 = []( bmic& collection ){ ce_tests::fill9< obj >( collection ); };
   auto f2 = []( bmic& collection ){ ce_tests::fill9a< obj >( collection ); };
   auto f3 = []( bmic& collection ){ ce_tests::fill9b< obj >( collection ); };
   auto s = []( bmic& collection ){ ce_tests::sort9< obj >( collection ); };

   comparision_assignment_sub_index_test< ce::concatenation_iterator_ex< obj, cmp2 >, bmic, obj, oidx, oidx_a, cmp2 >( f1, f2, f3, s );
}

BOOST_AUTO_TEST_CASE(comparision_assignment_tests)
{
   using obj = ce_tests::test_object;
   using bmic = ce_tests::test_object_index;
   using cmp1 = ce_tests::cmp1;
   using oidx = ce_tests::OrderedIndex;

   auto f1 = []( bmic& collection ){ ce_tests::fill7< obj >( collection ); };
   auto f2 = []( bmic& collection ){ ce_tests::fill7a< obj >( collection ); };
   auto f3 = []( bmic& collection ){ ce_tests::fill7b< obj >( collection ); };
   auto s = []( bmic& collection ){ ce_tests::sort7< obj >( collection ); };

   comparision_assignment_test< ce::concatenation_iterator< obj, cmp1 >, bmic, obj, oidx, cmp1 >( f1, f2, f3, s );
}

BOOST_AUTO_TEST_CASE(inc_dec_basic_tests3)
{
   using obj = ce_tests::test_object;
   using bmic = ce_tests::test_object_index;
   using cmp2 = ce_tests::cmp2;
   using oidx = ce_tests::OrderedIndex;
   using oidx_a = ce_tests::CompositeOrderedIndexA;

   auto f1 = []( bmic& collection ){ ce_tests::fill9< obj >( collection ); };
   auto f2 = []( bmic& collection ){ ce_tests::fill9a< obj >( collection ); };
   auto f3 = []( bmic& collection ){ ce_tests::fill9b< obj >( collection ); };
   auto s = []( bmic& collection ){ ce_tests::sort9< obj >( collection ); };

   using iterator = ce::concatenation_iterator_ex< obj, cmp2 >;
   using reverse_iterator = ce::concatenation_reverse_iterator_ex< obj, cmp2 >;

   inc_dec_basic_3_sources_sub_index_test< iterator, reverse_iterator, bmic, obj, oidx, oidx_a, cmp2 >( f1, f2, f3, s );
}

BOOST_AUTO_TEST_CASE(inc_dec_basic_tests2)
{
   using obj = ce_tests::test_object;
   using bmic = ce_tests::test_object_index;
   using cmp1 = ce_tests::cmp1;
   using oidx = ce_tests::OrderedIndex;

   auto f1 = []( bmic& collection ){ ce_tests::fill8< obj >( collection ); };
   auto f2 = []( bmic& collection ){ ce_tests::fill8a< obj >( collection ); };
   auto f3 = []( bmic& collection ){ ce_tests::fill8b< obj >( collection ); };
   auto f4 = []( bmic& collection ){ ce_tests::fill8c< obj >( collection ); };
   auto f5 = []( bmic& collection ){ ce_tests::fill8d< obj >( collection ); };
   auto f6 = []( bmic& collection ){ ce_tests::fill8e< obj >( collection ); };
   auto s = []( bmic& collection ){ ce_tests::sort8< obj >( collection ); };

   inc_dec_basic_6_sources_test< bmic, obj, oidx, cmp1 >( f1, f2, f3, f4, f5, f6, s );
}

BOOST_AUTO_TEST_CASE(inc_dec_basic_tests1)
{
   using obj = ce_tests::test_object;
   using bmic = ce_tests::test_object_index;
   using cmp1 = ce_tests::cmp1;
   using oidx = ce_tests::OrderedIndex;

   auto f1 = []( bmic& collection ){ ce_tests::fill7< obj >( collection ); };
   auto f2 = []( bmic& collection ){ ce_tests::fill7a< obj >( collection ); };
   auto f3 = []( bmic& collection ){ ce_tests::fill7b< obj >( collection ); };
   auto s = []( bmic& collection ){ ce_tests::sort7< obj >( collection ); };

   inc_dec_basic_3_sources_test< bmic, obj, oidx, cmp1 >( f1, f2, f3, s );
}

BOOST_AUTO_TEST_CASE(inc_dec_basic_tests)
{
   using obj = ce_tests::test_object;
   using bmic = ce_tests::test_object_index;
   using cmp1 = ce_tests::cmp1;
   using oidx = ce_tests::OrderedIndex;

   auto f1 = []( bmic& collection ){ ce_tests::fill6< obj >( collection ); };
   using iterator = ce::concatenation_iterator< obj, cmp1 >;
   using reverse_iterator = ce::concatenation_reverse_iterator< obj, cmp1 >;

   inc_dec_basic_1_source_test< iterator, reverse_iterator, bmic, obj, oidx, cmp1 >( f1 );
}

BOOST_AUTO_TEST_CASE(basic_tests)
{
   using obj = ce_tests::test_object;
   using bmic = ce_tests::test_object_index;
   using cmp1 = ce_tests::cmp1;
   using cmp2 = ce_tests::cmp2;
   using cmp3 = ce_tests::cmp3;
   using oidx = ce_tests::OrderedIndex;
   using c_oidx_a = ce_tests::CompositeOrderedIndexA;
   using c_oidx_b = ce_tests::CompositeOrderedIndexB;

   std::vector< obj > sorted;

   basic_test< std::set< obj, cmp1 >, obj >();

   ce_tests::sort1< obj >( sorted );
   test_different_sources< bmic, std::set< obj, cmp1 >, oidx, obj, cmp1 >( sorted );

   ce_tests::sort2< obj >( sorted );
   auto c1 = []( bmic& collection ){ ce_tests::fill2< obj >( collection ); };
   auto c2 = []( bmic& collection ){ ce_tests::fill2b< obj >( collection ); };
   test_with_sub_index<
                  decltype( c1 ),
                  decltype( c2 ),
                  bmic,
                  oidx,
                  c_oidx_a,
                  obj,
                  cmp2
                  >( c1, c2, sorted );

   ce_tests::sort4< obj >( sorted );
   test_with_sub_index_3_sources<
                  bmic,
                  std::set< obj, cmp2 >,
                  oidx,
                  c_oidx_a,
                  obj,
                  cmp2
                  >( sorted );

   ce_tests::sort4a< obj >( sorted );
   test_with_sub_index_3_sources<
                  bmic,
                  std::set< obj, cmp3 >,
                  oidx,
                  c_oidx_b,
                  obj,
                  cmp3
                  >( sorted );

   ce_tests::sort5< obj >( sorted );
   auto c1a = []( bmic& collection ){ ce_tests::fill5< obj >( collection ); };
   auto c2a = []( bmic& collection ){ ce_tests::fill5b< obj >( collection ); };
   test_with_sub_index<
                  decltype( c1a ),
                  decltype( c2a ),
                  bmic,
                  oidx,
                  c_oidx_a,
                  obj,
                  cmp2
                  >( c1a, c2a, sorted );
}

BOOST_AUTO_TEST_CASE(advanced_tests)
{
   using obj2 = ce_tests::test_object2;
   using bmic2 = ce_tests::test_object_index2;
   using oidx2 = ce_tests::OrderedIndex2;
   using c_oidx2 = ce_tests::CompositeOrderedIndex2;
   using cmp4 = ce_tests::cmp4;

   test_with_sub_index_2_sources_many_objects_without_id_repeat
   <
      bmic2,
      oidx2,
      c_oidx2,
      obj2,
      cmp4
   >();

   test_with_sub_index_2_sources_many_objects_with_id_repeat
   <
      bmic2,
      oidx2,
      c_oidx2,
      obj2,
      cmp4
   >();

   test_with_sub_index_3_sources_many_objects_with_id_repeat
   <
      bmic2,
      oidx2,
      c_oidx2,
      obj2,
      cmp4
   >();
}

BOOST_AUTO_TEST_SUITE_END()
