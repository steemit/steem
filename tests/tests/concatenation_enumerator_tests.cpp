#include <boost/test/unit_test.hpp>

#include "../concatenation_enumerator_tests/concatenation_enumerator_tests_mgr.hpp"
#include <chainbase/util/concatenation_enumerator.hpp>

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
   ce::concatenation_enumerator< Object, ce_tests::cmp1 > it01( ce_tests::cmp1(), p1 );
   while( !it01.end() )
   {
      ++it01;
      ++cnt;
   }
   BOOST_REQUIRE( cnt == 0 );

   BOOST_TEST_MESSAGE( "2 empty sources" );
   cnt = 0;
   ce::concatenation_enumerator< Object, ce_tests::cmp1 > it02( ce_tests::cmp1(), p1, p2 );
   while( !it02.end() )
   {
      ++it02;
      ++cnt;
   }
   BOOST_REQUIRE( cnt == 0 );

   BOOST_TEST_MESSAGE( "1 filled source" );
   ce_tests::fill1< Object >( s1 );
   ce::concatenation_enumerator< Object, ce_tests::cmp1 > it03( ce_tests::cmp1(), std::make_tuple( s1.begin(), s1.end() ) );
   auto it_src03 = s1.begin();
   while( !it03.end() )
   {
      BOOST_REQUIRE( ( *it03 ) == ( *it_src03 ) );

      ++it03;
      ++it_src03;
   }

   BOOST_TEST_MESSAGE( "2 filled sources: one has content, second is empty" );
   ce_tests::fill1< Object >( s1 );
   ce::concatenation_enumerator< Object, ce_tests::cmp1 > it04 (  ce_tests::cmp1(),
                                                                           std::make_tuple( s1.begin(), s1.end() ),
                                                                           std::make_tuple( s2.begin(), s2.end() )
                                                                        );
   auto it_src04_1 = s1.begin();
   while( !it04.end() )
   {
      BOOST_REQUIRE( ( *it04 ) == ( *it_src04_1 ) );

      ++it04;
      ++it_src04_1;
   }

   BOOST_TEST_MESSAGE( "2 filled sources with the same content" );
   ce_tests::fill1< Object >( s1 );
   ce_tests::fill1< Object >( s2 );
   ce::concatenation_enumerator< Object, ce_tests::cmp1 > it05 (  ce_tests::cmp1(),
                                                                           std::make_tuple( s1.begin(), s1.end() ),
                                                                           std::make_tuple( s2.begin(), s2.end() )
                                                                        );
   auto it_src05_1 = s1.begin();
   auto it_src05_2 = s2.begin();
   while( !it05.end() )
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
   ce::concatenation_enumerator< Object, ce_tests::cmp1 > it06 (  ce_tests::cmp1(),
                                                                           std::make_tuple( s1.begin(), s1.end() ),
                                                                           std::make_tuple( s2.begin(), s2.end() )
                                                                        );
   cnt = 0;
   while( !it06.end() )
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
   ce::concatenation_enumerator< Object, Cmp > it( Cmp(), p1, p2, p3 );

   auto sorted_it = sorted.begin();
   while( !it.end() )
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

   ce::concatenation_enumerator_ex< Object, Cmp > it( Cmp(), p1, p2 );

   auto sorted_it = sorted.begin();
   while( !it.end() )
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
   ce::concatenation_enumerator_ex< Object, Cmp > it( Cmp(), p1, p2, p3 );

   auto sorted_it = sorted.begin();
   while( !it.end() )
   {
      BOOST_REQUIRE( ( *it ) == ( *sorted_it ) );

      ++it;
      ++sorted_it;
   }

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
   ce::concatenation_enumerator_ex< Object, Cmp > it( Cmp(), p1, p2 );

   auto sorted_it = sorted.begin();
   while( !it.end() )
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
   ce::concatenation_enumerator_ex< Object, Cmp > it( Cmp(), p1, p2 );

   auto sorted_it = sorted.begin();
   while( !it.end() )
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
   ce::concatenation_enumerator_ex< Object, Cmp > it( Cmp(), p1, p2, p3 );

   auto sorted_it = sorted.begin();
   while( !it.end() )
   {
      BOOST_REQUIRE( ( *it ) == ( *sorted_it ) );

      ++it;
      ++sorted_it;
   }
}

template< typename Collection, typename Object, typename Index, typename Cmp, typename Filler >
void inc_dec_basic_test( Filler&& filler )
{
   Collection bmic1;

   BOOST_TEST_MESSAGE( "1 sources - '++' and '--'" );
   filler( bmic1 );

   const auto& idx1 = bmic1.template get< Index >();

   auto p1 = std::make_tuple( idx1.begin(), idx1.end() );
   ce::concatenation_enumerator< Object, Cmp > it( Cmp(), p1 );

   auto it_comparer = idx1.begin();
   size_t size = idx1.size();
   if( size < 2 )
      return;

   size_t i = 0;

   {
      std::cout<<"[0]"<<std::endl;
      std::cout<<*it<<std::endl;
      std::cout<<*it_comparer<<std::endl;

      ++it;
      ++it_comparer;

      --it;
      --it_comparer;

      std::cout<<"[0] after operator--"<<std::endl;
      std::cout<<*it<<std::endl;
      std::cout<<*it_comparer<<std::endl;
   
      if( ++i == size ) return;
   }

   {
      ++it;
      ++it_comparer;

      std::cout<<"[1]"<<std::endl;
      std::cout<<*it<<std::endl;
      std::cout<<*it_comparer<<std::endl;

      ++it;
      ++it_comparer;

      --it;
      --it_comparer;

      std::cout<<"[1] after operator--"<<std::endl;
      std::cout<<*it<<std::endl;
      std::cout<<*it_comparer<<std::endl;
   
      if( ++i == size ) return;
   }

   {
      ++it;
      ++it_comparer;

      std::cout<<"[2]"<<std::endl;
      std::cout<<*it<<std::endl;
      std::cout<<*it_comparer<<std::endl;

      ++it;
      ++it_comparer;

      --it;
      --it_comparer;

      std::cout<<"[2] after operator--"<<std::endl;
      std::cout<<*it<<std::endl;
      std::cout<<*it_comparer<<std::endl;
   
      if( ++i == size ) return;
   }

   {
      ++it;
      ++it_comparer;

      std::cout<<"[3]"<<std::endl;
      std::cout<<*it<<std::endl;
      std::cout<<*it_comparer<<std::endl;

      ++it;
      ++it_comparer;

      --it;
      --it_comparer;

      std::cout<<"[3] after operator--"<<std::endl;
      std::cout<<*it<<std::endl;
      std::cout<<*it_comparer<<std::endl;
   
      --it;
      --it_comparer;

      std::cout<<"[2] after operator--"<<std::endl;
      std::cout<<*it<<std::endl;
      std::cout<<*it_comparer<<std::endl;

      --it;
      --it_comparer;

      std::cout<<"[1] after operator--"<<std::endl;
      std::cout<<*it<<std::endl;
      std::cout<<*it_comparer<<std::endl;

      --it;
      --it_comparer;

      std::cout<<"[0] after operator--"<<std::endl;
      std::cout<<*it<<std::endl;
      std::cout<<*it_comparer<<std::endl;
   }
}

BOOST_AUTO_TEST_SUITE(concatenation_enumeration_tests)

BOOST_AUTO_TEST_CASE(inc_dec_basic_tests)
{
   using obj = ce_tests::test_object;
   using bmic = ce_tests::test_object_index;
   using cmp1 = ce_tests::cmp1;
   using oidx = ce_tests::OrderedIndex;

   auto c1 = []( bmic& collection ){ ce_tests::fill6< obj >( collection ); };

   inc_dec_basic_test< bmic, obj, oidx, cmp1 >( c1 );
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
