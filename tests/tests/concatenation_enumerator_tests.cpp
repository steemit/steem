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
   ce::concatenation_enumerator< Object, ce_tests::cmp1 > it_end_01( false, ce_tests::cmp1(), p1 );
   while( it01 != it_end_01 )
   {
      ++it01;
      ++cnt;
   }
   BOOST_REQUIRE( cnt == 0 );

   BOOST_TEST_MESSAGE( "2 empty sources" );
   cnt = 0;
   ce::concatenation_enumerator< Object, ce_tests::cmp1 > it02( ce_tests::cmp1(), p1, p2 );
   ce::concatenation_enumerator< Object, ce_tests::cmp1 > it_end_02( false, ce_tests::cmp1(), p1, p2 );
   while( it02 != it_end_02 )
   {
      ++it02;
      ++cnt;
   }
   BOOST_REQUIRE( cnt == 0 );

   BOOST_TEST_MESSAGE( "1 filled source" );
   ce_tests::fill1< Object >( s1 );
   ce::concatenation_enumerator< Object, ce_tests::cmp1 > it03( ce_tests::cmp1(), std::make_tuple( s1.begin(), s1.end() ) );
   ce::concatenation_enumerator< Object, ce_tests::cmp1 > it_end_03( false, ce_tests::cmp1(), std::make_tuple( s1.begin(), s1.end() ) );
   auto it_src03 = s1.begin();
   while( it03 != it_end_03 )
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

   ce::concatenation_enumerator< Object, ce_tests::cmp1 > it_end_04 ( false, ce_tests::cmp1(),
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
   ce::concatenation_enumerator< Object, ce_tests::cmp1 > it05 (  ce_tests::cmp1(),
                                                                           std::make_tuple( s1.begin(), s1.end() ),
                                                                           std::make_tuple( s2.begin(), s2.end() )
                                                                        );
   ce::concatenation_enumerator< Object, ce_tests::cmp1 > it_end_05 ( false, ce_tests::cmp1(),
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
   ce::concatenation_enumerator< Object, ce_tests::cmp1 > it06 (  ce_tests::cmp1(),
                                                                           std::make_tuple( s1.begin(), s1.end() ),
                                                                           std::make_tuple( s2.begin(), s2.end() )
                                                                        );
   ce::concatenation_enumerator< Object, ce_tests::cmp1 > it_end_06 ( false, ce_tests::cmp1(),
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
   ce::concatenation_enumerator< Object, Cmp > it( Cmp(), p1, p2, p3 );
   ce::concatenation_enumerator< Object, Cmp > it_end( false, Cmp(), p1, p2, p3 );

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

   ce::concatenation_enumerator_ex< Object, Cmp > it( Cmp(), p1, p2 );
   ce::concatenation_enumerator_ex< Object, Cmp > it_end( false, Cmp(), p1, p2 );

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
   ce::concatenation_enumerator_ex< Object, Cmp > it( Cmp(), p1, p2, p3 );
   ce::concatenation_enumerator_ex< Object, Cmp > it_end( false, Cmp(), p1, p2, p3 );

   auto sorted_it = sorted.begin();
   while( it != it_end )
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
   ce::concatenation_enumerator_ex< Object, Cmp > it_end( false, Cmp(), p1, p2 );

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
   ce::concatenation_enumerator_ex< Object, Cmp > it( Cmp(), p1, p2 );
   ce::concatenation_enumerator_ex< Object, Cmp > it_end( false, Cmp(), p1, p2 );

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
   ce::concatenation_enumerator_ex< Object, Cmp > it( Cmp(), p1, p2, p3 );
   ce::concatenation_enumerator_ex< Object, Cmp > it_end( false, Cmp(), p1, p2, p3 );

   auto sorted_it = sorted.begin();
   while( it != it_end )
   {
      BOOST_REQUIRE( ( *it ) == ( *sorted_it ) );

      ++it;
      ++sorted_it;
   }
}

template< typename Collection, typename Object, typename Index, typename Cmp, typename Filler >
void inc_dec_basic_1_source_test( Filler&& filler )
{
   Collection bmic1;

   BOOST_TEST_MESSAGE( "1 source - operations: '++' and '--'" );
   filler( bmic1 );

   const auto& idx1 = bmic1.template get< Index >();

   auto p1 = std::make_tuple( idx1.begin(), idx1.end() );
   ce::concatenation_enumerator< Object, Cmp > it( Cmp(), p1 );

   auto it_comparer = idx1.begin();
   size_t size = idx1.size();

   size_t i = 0;

   {
      std::cout<<"[0]"<<std::endl;
      BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

      ++it;
      ++it_comparer;

      --it;
      --it_comparer;

      std::cout<<"[0] after operator--"<<std::endl;
      BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   
      if( ++i == size ) return;
   }

   {
      ++it;
      ++it_comparer;

      std::cout<<"[1]"<<std::endl;
      BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

      ++it;
      ++it_comparer;

      --it;
      --it_comparer;

      std::cout<<"[1] after operator--"<<std::endl;
      BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   
      if( ++i == size ) return;
   }

   {
      ++it;
      ++it_comparer;

      std::cout<<"[2]"<<std::endl;
      BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

      ++it;
      ++it_comparer;

      --it;
      --it_comparer;

      std::cout<<"[2] after operator--"<<std::endl;
      BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   
      if( ++i == size ) return;
   }

   {
      ++it;
      ++it_comparer;

      std::cout<<"[3]"<<std::endl;
      BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

      ++it;
      ++it_comparer;

      --it;
      --it_comparer;

      std::cout<<"[3] after operator--"<<std::endl;
      BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   }

   --it;
   --it_comparer;

   std::cout<<"[2] after operator--"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it;
   --it_comparer;

   std::cout<<"[1] after operator--"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it;
   --it_comparer;

   std::cout<<"[0] after operator--"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
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

   ce::concatenation_enumerator< Object, Cmp > it( Cmp(),
                                                   std::make_tuple( idx1.begin(), idx1.end() ),
                                                   std::make_tuple( idx2.begin(), idx2.end() ),
                                                   std::make_tuple( idx3.begin(), idx3.end() )
                                                );

   auto it_comparer = comparer.begin();
   size_t size = comparer.size();

   size_t i = 0;

   {
      std::cout<<"[0]"<<std::endl;
      BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

      ++it;
      ++it_comparer;

      --it;
      --it_comparer;

      std::cout<<"[0] after operator--"<<std::endl;
      BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   
      if( ++i == size ) return;
   }

   {
      ++it;
      ++it_comparer;

      std::cout<<"[1]"<<std::endl;
      BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

      ++it;
      ++it_comparer;

      --it;
      --it_comparer;

      std::cout<<"[1] after operator--"<<std::endl;
      BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   
      if( ++i == size ) return;
   }

   {
      ++it;
      ++it_comparer;

      std::cout<<"[2]"<<std::endl;
      BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

      ++it;
      ++it_comparer;

      --it;
      --it_comparer;

      std::cout<<"[2] after operator--"<<std::endl;
      BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   
      if( ++i == size ) return;
   }

   {
      ++it;
      ++it_comparer;

      std::cout<<"[3]"<<std::endl;
      BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

      ++it;
      ++it_comparer;

      --it;
      --it_comparer;

      std::cout<<"[3] after operator--"<<std::endl;
      BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   }

   {
      ++it;
      ++it_comparer;

      std::cout<<"[4]"<<std::endl;
      BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

      ++it;
      ++it_comparer;

      --it;
      --it_comparer;

      std::cout<<"[4] after operator--"<<std::endl;
      BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   }

   {
      ++it;
      ++it_comparer;

      std::cout<<"[5]"<<std::endl;
      BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

      ++it;
      ++it_comparer;

      --it;
      --it_comparer;

      std::cout<<"[5] after operator--"<<std::endl;
      BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   }

   --it;
   --it_comparer;

   std::cout<<"[4] after operator--"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it;
   --it_comparer;

   std::cout<<"[3] after operator--"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it;
   --it_comparer;

   std::cout<<"[2] after operator--"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it;
   --it_comparer;

   std::cout<<"[1] after operator--"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it;
   --it_comparer;

   std::cout<<"[0] after operator--"<<std::endl;
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

   ce::concatenation_enumerator< Object, Cmp > it( Cmp(),
                                                   std::make_tuple( idx1.begin(), idx1.end() ),
                                                   std::make_tuple( idx2.begin(), idx2.end() ),
                                                   std::make_tuple( idx3.begin(), idx3.end() ),
                                                   std::make_tuple( idx4.begin(), idx4.end() ),
                                                   std::make_tuple( idx5.begin(), idx5.end() ),
                                                   std::make_tuple( idx6.begin(), idx6.end() )
                                                );

   auto it_comparer = comparer.begin();
   size_t size = comparer.size();

   std::cout<<"forward"<<std::endl;
   for( size_t i = 0 ; i < size; ++i )
   {
      BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
      ++it;
      ++it_comparer;
   }

   --it;
   --it_comparer;

   std::cout<<"backward"<<std::endl;
   for( size_t i = 1 ; i < size; ++i )
   {
      BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
      --it;
      --it_comparer;
   }

   ++it; ++it_comparer;
   std::cout<<"[1]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   std::cout<<"[2]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   std::cout<<"[1]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   std::cout<<"[2]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   std::cout<<"[3]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   std::cout<<"[4]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   std::cout<<"[5]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   std::cout<<"[4]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   std::cout<<"[3]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   std::cout<<"[2]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   std::cout<<"[1]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   std::cout<<"[0]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   std::cout<<"[1]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   std::cout<<"[2]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   std::cout<<"[3]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   std::cout<<"[4]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   std::cout<<"[5]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   std::cout<<"[6]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   std::cout<<"[5]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   std::cout<<"[6]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   std::cout<<"[5]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   std::cout<<"[6]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   std::cout<<"[5]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   std::cout<<"[4]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   std::cout<<"[3]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   std::cout<<"[2]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   std::cout<<"[3]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   std::cout<<"[2]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   std::cout<<"[1]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   std::cout<<"[2]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   std::cout<<"[1]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   std::cout<<"[0]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   std::cout<<"[1]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   std::cout<<"[0]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
}

template< typename Collection, typename Object, typename ID_Index, typename Index, typename Cmp,
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

   ce::concatenation_enumerator_ex< Object, Cmp > it( Cmp(),
                                                   std::make_tuple( idx1.begin(), idx1.end(), &id_idx1 ),
                                                   std::make_tuple( idx2.begin(), idx2.end(), &id_idx2 ),
                                                   std::make_tuple( idx3.begin(), idx3.end(), &id_idx3 )
                                                );

   const auto& comparer_idx = comparer.template get< Index >();
   auto it_comparer = comparer_idx.begin();
   size_t size = comparer_idx.size();

   std::cout<<"forward"<<std::endl;
   for( size_t i = 0 ; i < size; ++i )
   {
      std::cout<<"["<<i<<"]"<<std::endl;
      BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
      ++it;
      ++it_comparer;
   }

   std::cout<<"backward"<<std::endl;
   for( size_t i = 0 ; i < size; ++i )
   {
      --it;
      --it_comparer;
      std::cout<<"["<<size - i - 1<<"]"<<std::endl;
      BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
   }

   ++it; ++it_comparer;
   std::cout<<"[1]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   std::cout<<"[2]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   std::cout<<"[1]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   std::cout<<"[2]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   std::cout<<"[3]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   std::cout<<"[4]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   std::cout<<"[5]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   std::cout<<"[4]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   std::cout<<"[3]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   std::cout<<"[2]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   std::cout<<"[1]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   std::cout<<"[0]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   std::cout<<"[1]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   std::cout<<"[2]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   std::cout<<"[3]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   std::cout<<"[4]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   std::cout<<"[5]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   std::cout<<"[6]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   std::cout<<"[5]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   std::cout<<"[6]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   std::cout<<"[5]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   std::cout<<"[6]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   std::cout<<"[5]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   std::cout<<"[4]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   std::cout<<"[3]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   std::cout<<"[2]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   std::cout<<"[3]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   std::cout<<"[2]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   std::cout<<"[1]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   std::cout<<"[2]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   std::cout<<"[1]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   std::cout<<"[0]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   ++it; ++it_comparer;
   std::cout<<"[1]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );

   --it; --it_comparer;
   std::cout<<"[0]"<<std::endl;
   BOOST_REQUIRE( ( *it ) == ( *it_comparer ) );
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

   _t it_end( false, Cmp(),
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

   _t it_end( false, Cmp(),
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

   _t end( false, Cmp(),
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

   misc_operations_sub_index_test< ce::concatenation_enumerator_ex< obj, cmp2 >, bmic, obj, oidx, oidx_a, cmp2 >( f1, f2, f3, s );
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

   comparision_assignment_sub_index_test< ce::concatenation_enumerator_ex< obj, cmp2 >, bmic, obj, oidx, oidx_a, cmp2 >( f1, f2, f3, s );
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

   comparision_assignment_test< ce::concatenation_enumerator< obj, cmp1 >, bmic, obj, oidx, cmp1 >( f1, f2, f3, s );
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

   inc_dec_basic_3_sources_sub_index_test< bmic, obj, oidx, oidx_a, cmp2 >( f1, f2, f3, s );
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

   inc_dec_basic_1_source_test< bmic, obj, oidx, cmp1 >( f1 );
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
