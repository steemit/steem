#define BOOST_TEST_MODULE mira test

#include "test_objects.hpp"
#include "test_templates.hpp"

#include <boost/test/unit_test.hpp>
#include <steem/utilities/database_configuration.hpp>
#include <iostream>

using namespace mira;

using mira::multi_index::multi_index_container;
using mira::multi_index::indexed_by;
using mira::multi_index::ordered_unique;
using mira::multi_index::tag;
using mira::multi_index::member;
using mira::multi_index::composite_key;
using mira::multi_index::composite_key_compare;
using mira::multi_index::const_mem_fun;

struct mira_fixture {
   boost::filesystem::path tmp;
   chainbase::database db;
   mira_fixture()
   {
      tmp = boost::filesystem::current_path() / boost::filesystem::unique_path();
      db.open( tmp, 0, 1024*1024*8, steem::utilities::default_database_configuration() );
   }

   ~mira_fixture()
   {
      chainbase::bfs::remove_all( tmp );
   }
};

BOOST_FIXTURE_TEST_SUITE( mira_tests, mira_fixture )

BOOST_AUTO_TEST_CASE( sanity_tests )
{
   try
   {
      db.add_index< book_index >();

      BOOST_TEST_MESSAGE( "Creating book" );
      const auto& new_book =
      db.create<book>( []( book& b )
      {
          b.a = 3;
          b.b = 4;
      });

      BOOST_REQUIRE( new_book.id._id == 0 );
      BOOST_REQUIRE( new_book.a == 3 );
      BOOST_REQUIRE( new_book.b == 4 );

      try
      {
         db.create<book>( []( book& b )
         {
            b.a = 3;
            b.b = 5;
         });

         BOOST_REQUIRE( false );
      } catch( ... ) {}

      db.create<book>( []( book& b )
      {
          b.a = 4;
          b.b = 5;
      });

      db.create<book>( []( book& b )
      {
          b.a = 2;
          b.b = 1;
      });

      const auto& book_idx = db.get_index< book_index, by_id >();
      auto itr = book_idx.begin();

      BOOST_REQUIRE( itr != book_idx.end() );

      {
         const auto& tmp_book = *itr;

         BOOST_REQUIRE( tmp_book.id._id == 0 );
         BOOST_REQUIRE( tmp_book.a == 3 );
         BOOST_REQUIRE( tmp_book.b == 4 );
      }

      ++itr;

      {
         const auto& tmp_book = *itr;

         BOOST_REQUIRE( tmp_book.id._id == 1 );
         BOOST_REQUIRE( tmp_book.a == 4 );
         BOOST_REQUIRE( tmp_book.b == 5 );
      }

      ++itr;

      {
         const auto& tmp_book = *itr;

         BOOST_REQUIRE( tmp_book.id._id == 2 );
         BOOST_REQUIRE( tmp_book.a == 2 );
         BOOST_REQUIRE( tmp_book.b == 1 );
      }

      ++itr;

      BOOST_REQUIRE( itr == book_idx.end() );

      itr = book_idx.end();

      BOOST_REQUIRE( itr == book_idx.end() );

      --itr;

      {
         const auto& tmp_book = *itr;

         BOOST_REQUIRE( tmp_book.id._id == 2 );
         BOOST_REQUIRE( tmp_book.a == 2 );
         BOOST_REQUIRE( tmp_book.b == 1 );
      }

      --itr;

      {
         const auto& tmp_book = *itr;

         BOOST_REQUIRE( tmp_book.id._id == 1 );
         BOOST_REQUIRE( tmp_book.a == 4 );
         BOOST_REQUIRE( tmp_book.b == 5 );
      }

      --itr;

      {
         const auto& tmp_book = *itr;

         BOOST_REQUIRE( tmp_book.id._id == 0 );
         BOOST_REQUIRE( tmp_book.a == 3 );
         BOOST_REQUIRE( tmp_book.b == 4 );
      }

      const auto& book_by_a_idx = db.get_index< book_index, by_a >();
      auto a_itr = book_by_a_idx.begin();

      {
         const auto& tmp_book = *a_itr;

         BOOST_REQUIRE( tmp_book.id._id == 2 );
         BOOST_REQUIRE( tmp_book.a == 2 );
         BOOST_REQUIRE( tmp_book.b == 1 );
      }

      ++a_itr;

      {
         const auto& tmp_book = *a_itr;

         BOOST_REQUIRE( tmp_book.id._id == 0 );
         BOOST_REQUIRE( tmp_book.a == 3 );
         BOOST_REQUIRE( tmp_book.b == 4 );
      }

      ++a_itr;

      {
         const auto& tmp_book = *a_itr;

         BOOST_REQUIRE( tmp_book.id._id == 1 );
         BOOST_REQUIRE( tmp_book.a == 4 );
         BOOST_REQUIRE( tmp_book.b == 5 );
      }

      {
         const auto& tmp_book = *(book_by_a_idx.lower_bound( 3 ));

         BOOST_REQUIRE( tmp_book.id._id == 0 );
         BOOST_REQUIRE( tmp_book.a == 3 );
         BOOST_REQUIRE( tmp_book.b == 4 );
      }

      BOOST_REQUIRE( book_by_a_idx.lower_bound( 5 ) == book_by_a_idx.end() );

      {
         const auto& tmp_book = *(book_by_a_idx.upper_bound( 3 ));

         BOOST_REQUIRE( tmp_book.id._id == 1 );
         BOOST_REQUIRE( tmp_book.a == 4 );
         BOOST_REQUIRE( tmp_book.b == 5 );
      }

      BOOST_REQUIRE( book_by_a_idx.upper_bound( 5 ) == book_by_a_idx.end() );

      {
         const auto& tmp_book = db.get< book, by_id >( 1 );

         BOOST_REQUIRE( tmp_book.id._id == 1 );
         BOOST_REQUIRE( tmp_book.a == 4 );
         BOOST_REQUIRE( tmp_book.b == 5 );
      }

      {
         const auto* book_ptr = db.find< book, by_a >( 4 );

         BOOST_REQUIRE( book_ptr->id._id == 1 );
         BOOST_REQUIRE( book_ptr->a == 4 );
         BOOST_REQUIRE( book_ptr->b == 5 );
      }

      bool is_found = db.find< book, by_a >( 10 ) != nullptr;

      BOOST_REQUIRE( !is_found );

      const auto& book_by_b_idx = db.get_index< book_index, by_b >();
      auto b_itr = book_by_b_idx.begin();

      BOOST_REQUIRE( b_itr != book_by_b_idx.end() );

      {
         const auto& tmp_book = *b_itr;

         BOOST_REQUIRE( tmp_book.id._id == 1 );
         BOOST_REQUIRE( tmp_book.a == 4 );
         BOOST_REQUIRE( tmp_book.b == 5 );
      }

      ++b_itr;

      {
         const auto& tmp_book = *b_itr;

         BOOST_REQUIRE( tmp_book.id._id == 0 );
         BOOST_REQUIRE( tmp_book.a == 3 );
         BOOST_REQUIRE( tmp_book.b == 4 );
      }

      ++b_itr;

      {
         const auto& tmp_book = *b_itr;

         BOOST_REQUIRE( tmp_book.id._id == 2 );
         BOOST_REQUIRE( tmp_book.a == 2 );
         BOOST_REQUIRE( tmp_book.b == 1 );
      }

      ++b_itr;

      BOOST_REQUIRE( b_itr == book_by_b_idx.end() );

      const auto book_by_b = db.get< book, by_b >( boost::make_tuple( 5, 4 ) );

      BOOST_REQUIRE( book_by_b.id._id == 1 );
      BOOST_REQUIRE( book_by_b.a == 4 );
      BOOST_REQUIRE( book_by_b.b == 5 );

      b_itr = book_by_b_idx.lower_bound( 10 );

      {
         const auto& tmp_book = *b_itr;

         BOOST_REQUIRE( tmp_book.id._id == 1 );
         BOOST_REQUIRE( tmp_book.a == 4 );
         BOOST_REQUIRE( tmp_book.b == 5 );
      }

      const auto& book_by_sum_idx = db.get_index< book_index, by_sum >();
      auto by_sum_itr = book_by_sum_idx.begin();

      {
         const auto& tmp_book = *by_sum_itr;

         BOOST_REQUIRE( tmp_book.id._id == 2 );
         BOOST_REQUIRE( tmp_book.a == 2 );
         BOOST_REQUIRE( tmp_book.b == 1 );
      }

      ++by_sum_itr;

      {
         const auto& tmp_book = *by_sum_itr;

         BOOST_REQUIRE( tmp_book.id._id == 0 );
         BOOST_REQUIRE( tmp_book.a == 3 );
         BOOST_REQUIRE( tmp_book.b == 4 );
      }

      ++by_sum_itr;

      {
         const auto& tmp_book = *by_sum_itr;

         BOOST_REQUIRE( tmp_book.id._id == 1 );
         BOOST_REQUIRE( tmp_book.a == 4 );
         BOOST_REQUIRE( tmp_book.b == 5 );
      }

      ++by_sum_itr;

      BOOST_REQUIRE( by_sum_itr == book_by_sum_idx.end() );

      const auto& book_by_id = db.get< book, by_id >( 0 );
      const auto& book_by_a = db.get< book, by_a >( 3 );

      BOOST_REQUIRE( &book_by_id == &book_by_a );

      db.modify( db.get< book, by_id >( 0 ), []( book& b )
      {
         b.a = 10;
         b.b = 5;
      });

      {
         const auto& tmp_book = db.get< book, by_id >( 0 );

         BOOST_REQUIRE( tmp_book.id._id == 0 );
         BOOST_REQUIRE( tmp_book.a == 10 );
         BOOST_REQUIRE( tmp_book.b == 5 );
      }

      try
      {
         // Failure due to collision on 'a'
         db.modify( db.get< book, by_id >( 0 ), []( book& b )
         {
            b.a = 4;
            b.b = 10;
         });
         BOOST_REQUIRE( false );
      }
      catch( ... )
      {
         const auto& tmp_book = db.get< book, by_id >( 0 );

         BOOST_REQUIRE( tmp_book.id._id == 0 );
         BOOST_REQUIRE( tmp_book.a == 10 );
         BOOST_REQUIRE( tmp_book.b == 5 );
      }

      try
      {
         // Failure due to collision on 'sum'
         db.modify( db.get< book, by_id >( 0 ), []( book& b )
         {
            b.a = 6;
            b.b = 3;
         });
         BOOST_REQUIRE( false );
      }
      catch( ... )
      {
         const auto& tmp_book = db.get< book, by_id >( 0 );

         BOOST_REQUIRE( tmp_book.id._id == 0 );
         BOOST_REQUIRE( tmp_book.a == 10 );
         BOOST_REQUIRE( tmp_book.b == 5 );
      }

      a_itr = book_by_a_idx.begin();

      BOOST_REQUIRE( a_itr != book_by_a_idx.end() );

      {
         const auto& tmp_book = *a_itr;

         BOOST_REQUIRE( tmp_book.id._id == 2 );
         BOOST_REQUIRE( tmp_book.a == 2 );
         BOOST_REQUIRE( tmp_book.b == 1 );
      }

      ++a_itr;

      {
         const auto& tmp_book = *a_itr;

         BOOST_REQUIRE( tmp_book.id._id == 1 );
         BOOST_REQUIRE( tmp_book.a == 4 );
         BOOST_REQUIRE( tmp_book.b == 5 );
      }

      ++a_itr;

      {
         const auto& tmp_book = *a_itr;

         BOOST_REQUIRE( tmp_book.id._id == 0 );
         BOOST_REQUIRE( tmp_book.a == 10 );
         BOOST_REQUIRE( tmp_book.b == 5 );
      }

      ++a_itr;

      BOOST_REQUIRE( a_itr == book_by_a_idx.end() );

      b_itr = book_by_b_idx.begin();

      BOOST_REQUIRE( b_itr != book_by_b_idx.end() );

      {
         const auto& tmp_book = *b_itr;

         BOOST_REQUIRE( tmp_book.id._id == 1 );
         BOOST_REQUIRE( tmp_book.a == 4 );
         BOOST_REQUIRE( tmp_book.b == 5 );

      }

      ++b_itr;

      {
         const auto& tmp_book = *b_itr;

         BOOST_REQUIRE( tmp_book.id._id == 0 );
         BOOST_REQUIRE( tmp_book.a == 10 );
         BOOST_REQUIRE( tmp_book.b == 5 );
      }

      ++b_itr;

      {
         const auto& tmp_book = *b_itr;

         BOOST_REQUIRE( tmp_book.id._id == 2 );
         BOOST_REQUIRE( tmp_book.a == 2 );
         BOOST_REQUIRE( tmp_book.b == 1 );
      }

      ++b_itr;

      BOOST_REQUIRE( b_itr == book_by_b_idx.end() );

      b_itr = book_by_b_idx.lower_bound( boost::make_tuple( 5, 5 ) );

      {
         const auto& tmp_book = *b_itr;

         BOOST_REQUIRE( tmp_book.id._id == 0 );
         BOOST_REQUIRE( tmp_book.a == 10 );
         BOOST_REQUIRE( tmp_book.b == 5 );
      }

      by_sum_itr = book_by_sum_idx.begin();

      {
         const auto& tmp_book = *by_sum_itr;

         BOOST_REQUIRE( tmp_book.id._id == 2 );
         BOOST_REQUIRE( tmp_book.a == 2 );
         BOOST_REQUIRE( tmp_book.b == 1 );
      }

      ++by_sum_itr;

      {
         const auto& tmp_book = *by_sum_itr;

         BOOST_REQUIRE( tmp_book.id._id == 1 );
         BOOST_REQUIRE( tmp_book.a == 4 );
         BOOST_REQUIRE( tmp_book.b == 5 );
      }

      ++by_sum_itr;

      {
         const auto& tmp_book = *by_sum_itr;

         BOOST_REQUIRE( tmp_book.id._id == 0 );
         BOOST_REQUIRE( tmp_book.a == 10 );
         BOOST_REQUIRE( tmp_book.b == 5 );
      }

      ++by_sum_itr;

      BOOST_REQUIRE( by_sum_itr == book_by_sum_idx.end() );

      db.remove( db.get< book, by_id >( 0 ) );

      is_found = db.find< book, by_id >( 0 ) != nullptr;

      BOOST_REQUIRE( !is_found );

      itr = book_idx.begin();

      BOOST_REQUIRE( itr != book_idx.end() );

      {
         const auto& tmp_book = *itr;

         BOOST_REQUIRE( tmp_book.id._id == 1 );
         BOOST_REQUIRE( tmp_book.a == 4 );
         BOOST_REQUIRE( tmp_book.b == 5 );
      }

      ++itr;

      {
         const auto& tmp_book = *itr;

         BOOST_REQUIRE( tmp_book.id._id == 2 );
         BOOST_REQUIRE( tmp_book.a == 2 );
         BOOST_REQUIRE( tmp_book.b == 1 );
      }

      ++itr;

      BOOST_REQUIRE( itr == book_idx.end() );

      a_itr = book_by_a_idx.begin();

      BOOST_REQUIRE( a_itr != book_by_a_idx.end() );

      {
         const auto& tmp_book = *a_itr;

         BOOST_REQUIRE( tmp_book.id._id == 2 );
         BOOST_REQUIRE( tmp_book.a == 2 );
         BOOST_REQUIRE( tmp_book.b == 1 );
      }

      ++a_itr;

      {
         const auto& tmp_book = *a_itr;

         BOOST_REQUIRE( tmp_book.id._id == 1 );
         BOOST_REQUIRE( tmp_book.a == 4 );
         BOOST_REQUIRE( tmp_book.b == 5 );
      }

      ++a_itr;

      BOOST_REQUIRE( a_itr == book_by_a_idx.end() );

   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( single_index_test )
{
   try
   {
      db.add_index< single_index_index >();

      db.create< single_index_object >( [&]( single_index_object& ){} );

      const auto& sio = db.get( single_index_object::id_type() );

      BOOST_REQUIRE( sio.id._id == 0 );
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( variable_length_key_test )
{
   try
   {
      db.add_index< account_index >();

      const auto& acc_by_name_idx = db.get_index< account_index, by_name >();
      auto itr = acc_by_name_idx.begin();

      BOOST_REQUIRE( itr == acc_by_name_idx.end() );

      db.create< account_object >( [&]( account_object& a )
      {
         a.name = "alice";
      });

      db.create< account_object >( [&]( account_object& a )
      {
         a.name = "bob";
      });

      db.create< account_object >( [&]( account_object& a )
      {
         a.name = "charlie";
      });

      itr = acc_by_name_idx.begin();

      BOOST_REQUIRE( itr->name == "alice" );

      ++itr;

      BOOST_REQUIRE( itr->name == "bob" );

      ++itr;

      BOOST_REQUIRE( itr->name == "charlie" );

      ++itr;

      BOOST_REQUIRE( itr == acc_by_name_idx.end() );

      itr = acc_by_name_idx.lower_bound( "archibald" );

      BOOST_REQUIRE( itr->name == "bob" );
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( sanity_modify_test )
{
   try
   {
      db.add_index< book_index >();

      const auto& b1 = db.create<book>( []( book& b )
      {
          b.a = 1;
          b.b = 2;
      } );

      const auto& b2 = db.create<book>( []( book& b )
      {
          b.a = 2;
          b.b = 3;
      } );

      const auto& b3 = db.create<book>( []( book& b )
      {
          b.a = 4;
          b.b = 5;
      } );

      BOOST_REQUIRE( b1.a == 1 );
      BOOST_REQUIRE( b1.b == 2 );
      BOOST_REQUIRE( b1.sum() == 3 );

      BOOST_REQUIRE( b2.a == 2 );
      BOOST_REQUIRE( b2.b == 3 );
      BOOST_REQUIRE( b2.sum() == 5 );

      BOOST_REQUIRE( b3.a == 4 );
      BOOST_REQUIRE( b3.b == 5 );
      BOOST_REQUIRE( b3.sum() == 9 );

      db.modify( b2, [] ( book& b )
      {
         b.a = 10;
         b.b = 20;
      } );

      BOOST_REQUIRE( b2.a == 10 );
      BOOST_REQUIRE( b2.b == 20 );
      BOOST_REQUIRE( b2.sum() == 30 );

      auto& idx_by_a = db.get_index< book_index, by_a >();

      auto bb = idx_by_a.end();
      --bb;

      BOOST_REQUIRE( bb->a == 10 );
      BOOST_REQUIRE( bb->b == 20 );
      BOOST_REQUIRE( bb->sum() == 30 );
      BOOST_REQUIRE( &*bb == &b2 );

      auto& idx_by_sum = db.get_index< book_index, by_sum >();
      auto bb2 = idx_by_sum.end();
      --bb2;

      BOOST_REQUIRE( bb2->a == 10 );
      BOOST_REQUIRE( bb2->b == 20 );
      BOOST_REQUIRE( bb2->sum() == 30 );
      BOOST_REQUIRE( &*bb2 == &b2 );
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( range_test )
{
   try
   {
      db.add_index< test_object3_index >();

      for ( uint32_t i = 0; i < 10; i++ )
      {
         for ( uint32_t j = 0; j < 10; j++ )
         {
            db.create< test_object3 >( [=] ( test_object3& o )
            {
               o.val = i;
               o.val2 = j;
               o.val3 = i + j;
            } );
         }
      }

      const auto& idx = db.get_index< test_object3_index, composite_ordered_idx3a >();

      auto er = idx.equal_range( 5 );

      BOOST_REQUIRE( er.first->val == 5 );
      BOOST_REQUIRE( er.first->val2 == 0 );

      BOOST_REQUIRE( er.second->val == 6 );
      BOOST_REQUIRE( er.second->val2 == 0 );

      auto er2 = idx.equal_range( 9 );

      BOOST_REQUIRE( er2.first->val == 9 );
      BOOST_REQUIRE( er2.first->val2 == 0 );

      BOOST_REQUIRE( er2.second == idx.end() );
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( bounds_test )
{
   try
   {
      db.add_index< test_object3_index >();

      for ( uint32_t i = 0; i < 10; i++ )
      {
         for ( uint32_t j = 0; j < 10; j++ )
         {
            db.create< test_object3 >( [=] ( test_object3& o )
            {
               o.val = i;
               o.val2 = j;
               o.val3 = i + j;
            } );
         }
      }

      const auto& idx = db.get_index< test_object3_index, composite_ordered_idx3a >();

      auto upper_bound_not_found = idx.upper_bound( 10 );
      BOOST_REQUIRE( upper_bound_not_found == idx.end() );

      auto lower_bound_not_found = idx.lower_bound( 10 );
      BOOST_REQUIRE( lower_bound_not_found == idx.end() );

      auto composite_lower_bound = idx.lower_bound( boost::make_tuple( 3, 1 ) );

      BOOST_REQUIRE( composite_lower_bound->val == 3 );
      BOOST_REQUIRE( composite_lower_bound->val2 == 1 );

      auto composite_upper_bound = idx.upper_bound( boost::make_tuple( 3, 5 ) );

      BOOST_REQUIRE( composite_upper_bound->val == 3 );
      BOOST_REQUIRE( composite_upper_bound->val2 == 6 );

      auto lower_iter = idx.lower_bound( 5 );

      BOOST_REQUIRE( lower_iter->val == 5 );
      BOOST_REQUIRE( lower_iter->val2 == 0 );

      auto upper_iter = idx.upper_bound( 5 );

      BOOST_REQUIRE( upper_iter->val == 6 );
      BOOST_REQUIRE( upper_iter->val2 == 0 );
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( basic_tests )
{
   db.add_index< test_object_index >();
   db.add_index< test_object2_index >();
   db.add_index< test_object3_index >();

   auto c1 = []( test_object& obj ) { obj.name = "_name"; };
   auto c1b = []( test_object2& obj ) {};
   auto c1c = []( test_object3& obj ) { obj.val2 = 5; obj.val3 = 5; };

   basic_test< test_object_index, test_object, ordered_idx >( { 0, 1, 2, 3, 4, 5 }, c1, db );
   basic_test< test_object2_index, test_object2, ordered_idx2 >( { 0, 1, 2 }, c1b, db );
   basic_test< test_object3_index, test_object3, ordered_idx3 >( { 0, 1, 2, 3, 4 }, c1c, db );
}

BOOST_AUTO_TEST_CASE( insert_remove_tests )
{
   db.add_index< test_object_index >();
   db.add_index< test_object2_index >();
   db.add_index< test_object3_index >();

   auto c1 = []( test_object& obj ) { obj.name = "_name"; };
   auto c1b = []( test_object2& obj ) {};
   auto c1c = []( test_object3& obj ) { obj.val2 = 7; obj.val3 = obj.val2 + 1; };

   insert_remove_test< test_object_index, test_object, ordered_idx >( { 0, 1, 2, 3, 4, 5, 6, 7 }, c1, db );
   insert_remove_test< test_object2_index, test_object2, ordered_idx2 >( { 0, 1, 2, 3, 4, 5, 6, 7 }, c1b, db );
   insert_remove_test< test_object3_index, test_object3, ordered_idx3 >( { 0, 1, 2, 3 }, c1c, db );
}

BOOST_AUTO_TEST_CASE( insert_remove_collision_tests )
{
   db.add_index< test_object_index >();
   db.add_index< test_object2_index >();
   db.add_index< test_object3_index >();

   auto c1 = []( test_object& obj ) { obj.id = 0; obj.name = "_name7"; obj.val = 7; };
   auto c2 = []( test_object& obj ) { obj.id = 0; obj.name = "_name8"; obj.val = 8; };
   auto c3 = []( test_object& obj ) { obj.id = 0; obj.name = "the_same_name"; obj.val = 7; };
   auto c4 = []( test_object& obj ) { obj.id = 1; obj.name = "the_same_name"; obj.val = 7; };

   auto c1b = []( test_object2& obj ) { obj.id = 0; obj.val = 7; };
   auto c2b = []( test_object2& obj ) { obj.id = 0; obj.val = 8; };
   auto c3b = []( test_object2& obj ) { obj.id = 6; obj.val = 7; };
   auto c4b = []( test_object2& obj ) { obj.id = 6; obj.val = 7; };

   auto c1c = []( test_object3& obj ) { obj.id = 0; obj.val = 20; obj.val2 = 20; };
   auto c2c = []( test_object3& obj ) { obj.id = 1; obj.val = 20; obj.val2 = 20; };
   auto c3c = []( test_object3& obj ) { obj.id = 2; obj.val = 30; obj.val3 = 30; };
   auto c4c = []( test_object3& obj ) { obj.id = 3; obj.val = 30; obj.val3 = 30; };

   insert_remove_collision_test< test_object_index, test_object, ordered_idx >( {}, c1, c2, c3, c4, db );
   insert_remove_collision_test< test_object2_index, test_object2, ordered_idx2 >( {}, c1b, c2b, c3b, c4b, db );
   insert_remove_collision_test< test_object3_index, test_object3, ordered_idx3 >( {}, c1c, c2c, c3c, c4c, db );
}

BOOST_AUTO_TEST_CASE( modify_tests )
{
   db.add_index< test_object_index >();
   db.add_index< test_object2_index >();
   db.add_index< test_object3_index >();

   auto c1 = []( test_object& obj ) { obj.name = "_name"; };
   auto c2 = []( test_object& obj ){ obj.name = "new_name"; };
   auto c3 = []( const test_object& obj ){ BOOST_REQUIRE( obj.name == "new_name" ); };
   auto c4 = []( const test_object& obj ){ BOOST_REQUIRE( obj.val == size_t( obj.id ) + 100 ); };
   auto c5 = []( bool result ){ BOOST_REQUIRE( result == false ); };

   auto c1b = []( test_object2& obj ) { obj.val = 889; };
   auto c2b = []( test_object2& obj ){ obj.val = 2889; };
   auto c3b = []( const test_object2& obj ){ BOOST_REQUIRE( obj.val == 2889 ); };
   auto c4b = []( const test_object2& obj ){ /*empty*/ };
   auto c5b = []( bool result ){ BOOST_REQUIRE( result == true ); };

   modify_test< test_object_index, test_object, ordered_idx >( { 0, 1, 2, 3 }, c1, c2, c3, c4, c5, db );
   modify_test< test_object2_index, test_object2, ordered_idx2 >( { 0, 1, 2, 3, 4, 5 }, c1b, c2b, c3b, c4b, c5b, db );
}

BOOST_AUTO_TEST_CASE( misc_tests )
{
   db.add_index< test_object_index >();

   misc_test< test_object_index, test_object, ordered_idx, composited_ordered_idx >( { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }, db );
}

BOOST_AUTO_TEST_CASE( misc_tests3 )
{
   db.add_index< test_object3_index >();

   misc_test3< test_object3_index, test_object3, ordered_idx3, composite_ordered_idx3a, composite_ordered_idx3b >( { 0, 1, 2 }, db );
}

BOOST_AUTO_TEST_SUITE_END()
