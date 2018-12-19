#pragma once

#include <vector>
#include <boost/test/unit_test.hpp>

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
   //c.clear();
   auto i = c.begin();

   while (i != c.end())
   {
      c.erase(i);
      ++i;
   }

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
