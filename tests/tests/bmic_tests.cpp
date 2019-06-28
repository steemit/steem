#include <boost/test/unit_test.hpp>

#include <steem/chain/steem_fwd.hpp>

#include "../bmic_objects/bmic_manager_tests.hpp"

template < typename Container, typename Object, typename Call >
void basic_test( const std::vector< uint64_t >& v, Call&& call )
{
   Container c;

   BOOST_TEST_MESSAGE( "Creating `v.size()` objects" );
   for( const auto& item: v )
   {
      auto constructor = [ &item, &call ]( Object &obj )
      {
         obj.id = item;
         call( obj );
         obj.val = 100 - item;
      };
      c.emplace( std::move( Object( constructor, std::allocator< Object >() ) ) );
   }
   BOOST_REQUIRE( v.size() == c.size() );

   BOOST_TEST_MESSAGE( "Removing all objects" );
   c.clear();
   BOOST_REQUIRE( c.size() == 0 );

   BOOST_TEST_MESSAGE( "Creating 1 object" );
   auto constructor2 = [ &call ]( Object &obj )
   {
      obj.id = 0;
      call( obj );
      obj.val = 888;
   };
   c.emplace( std::move( Object( constructor2, std::allocator< Object >() ) ) );

   BOOST_TEST_MESSAGE( "Modyfing 1 object" );
   BOOST_REQUIRE( c.modify( c.begin(), []( Object& obj ){ obj.val = obj.val + 1; } ) == true );
   BOOST_REQUIRE( c.size() == 1 );

   BOOST_TEST_MESSAGE( "Removing 1 object" );
   c.erase( c.begin() );
   BOOST_REQUIRE( c.size() == 0 );

   BOOST_TEST_MESSAGE( "Creating `v.size()` objects" );
   for( const auto& item: v )
   {
      auto constructor = [ &item, &call ]( Object &obj )
      {
         obj.id = item;
         call( obj );
         obj.val = 100 - item;
      };
      c.emplace( std::move( Object( constructor, std::allocator< Object >() ) ) );
   }
   BOOST_REQUIRE( v.size() == c.size() );

   BOOST_TEST_MESSAGE( "Removing all objects one by one" );
   auto it = c.begin();
   while( it != c.end() )
   {
      auto tmp = it;
      ++it;
      c.erase( tmp );
   }
   BOOST_REQUIRE( c.size() == 0 );
}

template < typename Container, typename Object, typename Call >
void insert_remove_test( const std::vector< uint64_t >& v, Call&& call )
{
   Container c;

   int64_t cnt = 0;

   BOOST_TEST_MESSAGE( "Creating `v.size()` objects" );
   for( const auto& item: v )
   {
      auto constructor = [ &item, &call ]( Object &obj )
      {
         obj.id = item;
         call( obj );
         obj.val = item;
      };
      c.emplace( std::move( Object( constructor, std::allocator< Object >() ) ) );
      ++cnt;
   }
   BOOST_REQUIRE( v.size() == c.size() );

   BOOST_TEST_MESSAGE( "Removing objects one by one. Every object is removed by erase method" );
   for( int64_t i = 0; i < cnt; ++i )
   {
      auto found = c.find( i );
      BOOST_REQUIRE( found != c.end() );
      c.erase( found );
      BOOST_REQUIRE( v.size() - i - 1 == c.size() );
   }
   BOOST_REQUIRE( c.size() == 0 );

   BOOST_TEST_MESSAGE( "Creating 2 objects" );
   cnt = 0;
   for( const auto& item: v )
   {
      auto constructor = [ &item, &cnt, &call ]( Object &obj )
      {
         obj.id = item;
         call( obj );
         obj.val = cnt;
      };
      c.emplace( std::move( Object( constructor, std::allocator< Object >() ) ) );
      if( ++cnt == 2 )
         break;
   }
   BOOST_REQUIRE( c.size() == 2 );

   BOOST_TEST_MESSAGE( "Removing first object" );
   const Object& object = *( c.begin() );
   auto found = c.iterator_to( object );
   BOOST_REQUIRE( found != c.end() );
   c.erase( found );
   BOOST_REQUIRE( c.size() == 1 );

   BOOST_TEST_MESSAGE( "Adding object with id_key = 0" );
   auto constructor1 = [ &v, &call ]( Object &obj ) { obj.id = v[0]; call( obj ); obj.val = 100; };
   c.emplace( std::move( Object( constructor1, std::allocator< Object >() ) ) );
   BOOST_REQUIRE( c.size() == 2 );

   BOOST_TEST_MESSAGE( "Remove second object" );
   found = c.end();
   --found;
   auto first = std::prev( found, 1 );
   c.erase( found );
   BOOST_REQUIRE( c.size() == 1 );
   c.erase( first );
   BOOST_REQUIRE( c.size() == 0 );
}

template < typename Container, typename Object, typename Call1, typename Call2, typename Call3, typename Call4 >
void insert_remove_collision_test( const std::vector< uint64_t >& v, Call1&& call1, Call2&& call2, Call3&& call3, Call4&& call4 )
{
   Container c;

   BOOST_TEST_MESSAGE( "Adding 2 objects with collision - the same id" );
   c.emplace( std::move( Object( call1, std::allocator< Object >() ) ) );
   auto failed_emplace = c.emplace( Object( call2, std::allocator< Object >() ) );
   BOOST_REQUIRE( failed_emplace.second == false );
   BOOST_REQUIRE( c.size() == 1 );

   BOOST_TEST_MESSAGE( "Removing object and adding 2 objects with collision - the same name+val" );
   c.erase( c.begin() );
   c.emplace( std::move( Object( call3, std::allocator< Object >() ) ) );
   failed_emplace = c.emplace( Object( call4, std::allocator< Object >() ) );
   BOOST_REQUIRE( failed_emplace.second == false );
   BOOST_REQUIRE( c.size() == 1 );
}

template < typename Container, typename Object, typename Call1, typename Call2, typename Call3, typename Call4, typename Call5 >
void modify_test( const std::vector< uint64_t >& v, Call1&& call1, Call2&& call2, Call3&& call3, Call4&& call4, Call5&& call5 )
{
   Container c;

   BOOST_TEST_MESSAGE( "Creating `v.size()` objects" );
   for( const auto& item: v )
   {
      auto constructor = [ &item, &call1 ]( Object &obj )
      {
         obj.id = item;
         call1( obj );
         obj.val = item + 100;
      };
      c.emplace( std::move( Object( constructor, std::allocator< Object >() ) ) );
   }
   BOOST_REQUIRE( v.size() == c.size() );

   BOOST_TEST_MESSAGE( "Modifying key in all objects" );
   auto it = c.begin();
   auto end = c.end();
   for( ; it != end; ++it )
      c.modify( it, call2 );
   BOOST_REQUIRE( v.size() == c.size() );
   int32_t cnt = 0;
   for( const auto& item : c )
   {
      BOOST_REQUIRE( size_t( item.id ) == v[ cnt++ ] );
      call3( item );
      call4( item );
   }

   BOOST_TEST_MESSAGE( "Modifying 'val' key in order to create collisions for all objects" );
   uint32_t first_val = c.begin()->val;
   it = std::next( c.begin(), 1 );
   for( ; it != end; ++it )
   {
      bool result = c.modify( it, [&]( Object& obj ){ obj.val = first_val; } );
      call5( result );
      if( !result )
         it = c.begin();
   }
}

template < typename Container, typename Object, typename SimpleIndex, typename ComplexIndex >
void misc_test( const std::vector< uint64_t >& v )
{
   Container c;

   BOOST_TEST_MESSAGE( "Creating `v.size()` objects" );
   for( const auto& item: v )
   {
      auto constructor = [ &item ]( Object &obj )
      {
         obj.id = item;
         obj.name = "any_name";
         obj.val = item + 200;
      };
      c.emplace( std::move( Object( constructor, std::allocator< Object >() ) ) );
   }
   BOOST_REQUIRE( v.size() == c.size() );

   BOOST_TEST_MESSAGE( "Reading all elements" );
   uint32_t cnt = 0;
   for( const auto& item : c )
   {
      BOOST_REQUIRE( size_t( item.id ) == v[ cnt++ ] );
      BOOST_REQUIRE( item.name == "any_name" );
      BOOST_REQUIRE( item.val == size_t( item.id ) + 200 );
   }

   BOOST_TEST_MESSAGE( "Removing 2 objects: first and last" );
   c.erase( c.iterator_to( *( c.begin() ) ) );
   c.erase( c.iterator_to( *( std::prev( c.end(), 1 ) ) ) );
   BOOST_REQUIRE( v.size() - 2 == c.size() );

   //Modyfing key `name` in one object.
   auto it1 = c.iterator_to( *c.begin() );
   c.modify( it1, []( Object& obj ){ obj.name = "completely_different_name"; } );
   it1 = c.iterator_to( *c.begin() );
   BOOST_REQUIRE( it1->name == "completely_different_name" );
   uint32_t val1 = it1->val;

   //Creating collision in two objects: [1] and [2]
   auto it2 = c.iterator_to( *std::next( c.begin(), 2 ) );
   BOOST_REQUIRE( c.modify( it2, [ val1 ]( Object& obj ){ obj.name = "completely_different_name"; obj.val = val1;} ) == false );

   //Removing object [1].
   it1 = c.iterator_to( *std::next( c.begin(), 1 ) );
   c.erase( it1 );
   BOOST_REQUIRE( v.size() - 4 == c.size() );

   //Removing all remaining objects
   auto it_erase = c.begin();
   while( it_erase != c.end() )
   {
      auto tmp = it_erase;
      ++it_erase;
      c.erase( tmp );
   }
   BOOST_REQUIRE( c.size() == 0 );

   cnt = 0;
   BOOST_TEST_MESSAGE( "Creating `v.size()` objects. There are 'v.size() - 1' collisions." );
   for( const auto& item: v )
   {
      auto constructor = [ &item ]( Object &obj )
      {
         obj.id = item;
         obj.name = "all_objects_have_the_same_name";
         obj.val = 667;
      };
      if( cnt == 0 )
         c.emplace( std::move( Object( constructor, std::allocator< Object >() ) ) );
      else
         BOOST_REQUIRE_THROW( c.emplace( std::move( Object( constructor, std::allocator< Object >() ) ) ), boost::exception );
   }
   BOOST_REQUIRE( c.size() == 1 );
   auto it_only_one = c.iterator_to( *c.begin() );
   BOOST_REQUIRE( size_t( it_only_one->id ) == v[0] );
   BOOST_REQUIRE( it_only_one->name == "all_objects_have_the_same_name" );
   BOOST_REQUIRE( it_only_one->val == 667 );

   BOOST_TEST_MESSAGE( "Erasing one objects." );
   c.erase( it_only_one );
   BOOST_REQUIRE( c.size() == 0 );

   cnt = 0;
   BOOST_TEST_MESSAGE( "Creating `v.size()` objects." );
   for( const auto& item: v )
   {
      auto constructor = [ &item, &cnt ]( Object &obj )
      {
         obj.id = item;
         obj.name = "object nr:" + std::to_string( cnt++ );
         obj.val = 5000;
      };
      c.emplace( std::move( Object( constructor, std::allocator< Object >() ) ) );
   }
   BOOST_REQUIRE( c.size() == v.size() );

   BOOST_TEST_MESSAGE( "Finding some objects according to given index." );
   const auto& ordered_idx = c.template get< SimpleIndex >();
   const auto& composite_ordered_idx = c.template get< ComplexIndex >();

   auto found = ordered_idx.find( v.size() - 1 );
   BOOST_REQUIRE( found != ordered_idx.end() );
   BOOST_REQUIRE( size_t( found->id ) == v.size() - 1 );

   found = ordered_idx.find( 987654 );
   BOOST_REQUIRE( found == ordered_idx.end() );

   found = ordered_idx.find( v[0] );
   BOOST_REQUIRE( found != ordered_idx.end() );

   auto cfound = composite_ordered_idx.find( boost::make_tuple( "stupid_name", 5000 ) );
   BOOST_REQUIRE( cfound == composite_ordered_idx.end() );

   cfound = composite_ordered_idx.find( boost::make_tuple( "object nr:" + std::to_string( 0 ), 5000 ) );
   auto copy_cfound = cfound;
   auto cfound2 = composite_ordered_idx.find( boost::make_tuple( "object nr:" + std::to_string( 9 ), 5000 ) );
   BOOST_REQUIRE( cfound == composite_ordered_idx.begin() );
   BOOST_REQUIRE( cfound2 == std::prev( composite_ordered_idx.end(), 1 ) );
   cnt = 0;
   while( cfound++ != composite_ordered_idx.end() )
      ++cnt;
   BOOST_REQUIRE( cnt == v.size() );

   BOOST_TEST_MESSAGE( "Removing all data using iterators from 'ComplexIndex' index");
   while( copy_cfound != composite_ordered_idx.end() )
   {
      auto tmp = copy_cfound;
      ++copy_cfound;
      c.erase( c.iterator_to( *tmp ) );
   }
   BOOST_REQUIRE( c.size() == 0 );
}

template < typename Container, typename Object, typename SimpleIndex, typename ComplexIndex, typename AnotherComplexIndex >
void misc_test3( const std::vector< uint64_t >& v )
{
   Container c;

   BOOST_TEST_MESSAGE( "Creating `v.size()` objects" );
   for( const auto& item: v )
   {
      auto constructor = [ &item ]( Object &obj )
      {
         obj.id = item;
         obj.val = item + 1;
         obj.val2 = item + 2;
         obj.val3 = item + 3;
      };
      c.emplace( std::move( Object( constructor, std::allocator< Object >() ) ) );
   }
   BOOST_REQUIRE( v.size() == c.size() );

   BOOST_TEST_MESSAGE( "Finding some objects according to given index." );
   const auto& ordered_idx = c.template get< SimpleIndex >();
   const auto& composite_ordered_idx = c.template get< ComplexIndex >();
   const auto& another_composite_ordered_idx = c.template get< AnotherComplexIndex >();

   auto found = ordered_idx.lower_bound( 5739854 );
   BOOST_REQUIRE( found == ordered_idx.end() );

   found = ordered_idx.upper_bound( 5739854 );
   BOOST_REQUIRE( found == ordered_idx.end() );

   found = ordered_idx.lower_bound(  ordered_idx.begin()->id );
   BOOST_REQUIRE( found == ordered_idx.begin() );

   auto found2 = ordered_idx.upper_bound( ordered_idx.begin()->id );
   BOOST_REQUIRE( found == std::prev( found2, 1 ) );

   auto cfound = composite_ordered_idx.find( boost::make_tuple( 667, 5000 ) );
   BOOST_REQUIRE( cfound == composite_ordered_idx.end() );

   cfound = composite_ordered_idx.find( boost::make_tuple( 0 + 1, 0 + 2 ) );
   BOOST_REQUIRE( cfound != composite_ordered_idx.end() );

   auto another_cfound = another_composite_ordered_idx.find( boost::make_tuple( 0 + 1, 0 + 3 ) );
   BOOST_REQUIRE( another_cfound != another_composite_ordered_idx.end() );

   BOOST_TEST_MESSAGE( "Removing 1 object using iterator from 'AnotherComplexIndex' index");
   c.erase( c.iterator_to( *another_cfound ) );
   BOOST_REQUIRE( c.size() == v.size() - 1 );

   another_cfound = another_composite_ordered_idx.find( boost::make_tuple( 0 + 1, 0 + 3 ) );
   BOOST_REQUIRE( another_cfound == another_composite_ordered_idx.end() );
}

BOOST_AUTO_TEST_SUITE(bmic_tests)

BOOST_AUTO_TEST_CASE(basic_tests)
{
   auto c1 = []( bmic::test_object& obj ) { obj.name = "_name"; };
   auto c1b = []( bmic::test_object2& obj ) {};
   auto c1c = []( bmic::test_object3& obj ) { obj.val2 = 5; obj.val3 = 5; };

   basic_test< bmic::test_object_index, bmic::test_object >( { 0, 1, 2, 3, 4, 5 }, c1 );
   basic_test< bmic::test_object_index2, bmic::test_object2 >( { 0, 1, 2 }, c1b );
   basic_test< bmic::test_object_index3, bmic::test_object3 >( { 0, 1, 2, 3, 4 }, c1c );
}

BOOST_AUTO_TEST_CASE(insert_remove_tests)
{
   auto c1 = []( bmic::test_object& obj ) { obj.name = "_name"; };
   auto c1b = []( bmic::test_object2& obj ) {};
   auto c1c = []( bmic::test_object3& obj ) { obj.val2 = 7; obj.val3 = obj.val2 + 1; };

   insert_remove_test< bmic::test_object_index, bmic::test_object >( { 0, 1, 2, 3, 4, 5, 6, 7 }, c1 );
   insert_remove_test< bmic::test_object_index2, bmic::test_object2 >( { 0, 1, 2, 3, 4, 5, 6, 7 }, c1b );
   insert_remove_test< bmic::test_object_index3, bmic::test_object3 >( { 0, 1, 2, 3 }, c1c );
}

BOOST_AUTO_TEST_CASE(insert_remove_collision_tests)
{
   auto c1 = []( bmic::test_object& obj ) { obj.id = 0; obj.name = "_name7"; obj.val = 7; };
   auto c2 = []( bmic::test_object& obj ) { obj.id = 0; obj.name = "_name8"; obj.val = 8; };
   auto c3 = []( bmic::test_object& obj ) { obj.id = 0; obj.name = "the_same_name"; obj.val = 7; };
   auto c4 = []( bmic::test_object& obj ) { obj.id = 1; obj.name = "the_same_name"; obj.val = 7; };

   auto c1b = []( bmic::test_object2& obj ) { obj.id = 0; obj.val = 7; };
   auto c2b = []( bmic::test_object2& obj ) { obj.id = 0; obj.val = 8; };
   auto c3b = []( bmic::test_object2& obj ) { obj.id = 6; obj.val = 7; };
   auto c4b = []( bmic::test_object2& obj ) { obj.id = 6; obj.val = 7; };

   auto c1c = []( bmic::test_object3& obj ) { obj.id = 0; obj.val = 20; obj.val2 = 20; };
   auto c2c = []( bmic::test_object3& obj ) { obj.id = 1; obj.val = 20; obj.val2 = 20; };
   auto c3c = []( bmic::test_object3& obj ) { obj.id = 2; obj.val = 30; obj.val3 = 30; };
   auto c4c = []( bmic::test_object3& obj ) { obj.id = 3; obj.val = 30; obj.val3 = 30; };

   insert_remove_collision_test< bmic::test_object_index, bmic::test_object >( {}, c1, c2, c3, c4 );
   insert_remove_collision_test< bmic::test_object_index2, bmic::test_object2 >( {}, c1b, c2b, c3b, c4b );
   insert_remove_collision_test< bmic::test_object_index3, bmic::test_object3 >( {}, c1c, c2c, c3c, c4c );
}

BOOST_AUTO_TEST_CASE(modify_tests)
{
   auto c1 = []( bmic::test_object& obj ) { obj.name = "_name"; };
   auto c2 = []( bmic::test_object& obj ){ obj.name = "new_name"; };
   auto c3 = []( const bmic::test_object& obj ){ BOOST_REQUIRE( obj.name == "new_name" ); };
   auto c4 = []( const bmic::test_object& obj ){ BOOST_REQUIRE( obj.val == size_t( obj.id ) + 100 ); };
   auto c5 = []( bool result ){ BOOST_REQUIRE( result == false ); };

   auto c1b = []( bmic::test_object2& obj ) { obj.val = 889; };
   auto c2b = []( bmic::test_object2& obj ){ obj.val = 2889; };
   auto c3b = []( const bmic::test_object2& obj ){ BOOST_REQUIRE( obj.val == 2889 ); };
   auto c4b = []( const bmic::test_object2& obj ){ /*empty*/ };
   auto c5b = []( bool result ){ BOOST_REQUIRE( result == true ); };

   modify_test< bmic::test_object_index, bmic::test_object >( { 0, 1, 2, 3 }, c1, c2, c3, c4, c5 );
   modify_test< bmic::test_object_index2, bmic::test_object2 >( { 0, 1, 2, 3, 4, 5 }, c1b, c2b, c3b, c4b, c5b );
}

BOOST_AUTO_TEST_CASE(misc_tests)
{
   misc_test< bmic::test_object_index, bmic::test_object, bmic::OrderedIndex, bmic::CompositeOrderedIndex >( { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 } );
}

BOOST_AUTO_TEST_CASE(misc_tests3)
{
  misc_test3< bmic::test_object_index3, bmic::test_object3, bmic::OrderedIndex3, bmic::CompositeOrderedIndex3a, bmic::CompositeOrderedIndex3b >( { 0, 1, 2 } );
}

BOOST_AUTO_TEST_SUITE_END()
