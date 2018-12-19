#define BOOST_TEST_MODULE mira test

#include <mira/multi_index_container.hpp>
#include <mira/ordered_index.hpp>
#include <mira/tag.hpp>
#include <mira/member.hpp>
#include <mira/indexed_by.hpp>
#include <mira/composite_key.hpp>
#include <mira/mem_fun.hpp>

#include <boost/test/unit_test.hpp>

#include <chainbase/chainbase.hpp>

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

struct rocksdb_fixture {
   boost::filesystem::path tmp;
   chainbase::database db;
   rocksdb_fixture()
   {
      tmp = boost::filesystem::current_path() / boost::filesystem::unique_path();
      db.open( tmp, 0, 1024*1024*8 );
   }

   ~rocksdb_fixture()
   {
      db.wipe( tmp );
      chainbase::bfs::remove_all( tmp );
   }
};

struct book : public chainbase::object< 0, book > {

   template<typename Constructor, typename Allocator>
   book(  Constructor&& c, Allocator&& a )
   {
      c(*this);
   }

   book() = default;

   id_type id;
   int a = 0;
   int b = 1;

   int sum()const { return a + b; }
};

struct by_id;
struct by_a;
struct by_b;
struct by_sum;

typedef multi_index_container<
   book,
   indexed_by<
      ordered_unique< tag< by_id >, member< book, book::id_type, &book::id > >,
      ordered_unique< tag< by_a >,  member< book, int,           &book::a  > >
      ,
      ordered_unique< tag< by_b >,
         composite_key< book,
            member< book, int, &book::b >,
            member< book, int, &book::a >
         >,
         composite_key_compare< std::greater< int >, std::less< int > >
      >,
      ordered_unique< tag< by_sum >, const_mem_fun< book, int, &book::sum > >
  >,
  chainbase::allocator< book >
> book_index;

BOOST_FIXTURE_TEST_SUITE( mira_tests, rocksdb_fixture )

BOOST_AUTO_TEST_CASE( basic_tests )
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

BOOST_AUTO_TEST_CASE( modify_test )
{
   try
   {
      db.add_index< book_index >();

      const auto& b1 = db.create<book>( []( book& b )
      {
          b.a = 1;
          b.b = 2;
      });

      const auto& b2 = db.create<book>( []( book& b )
      {
          b.a = 2;
          b.b = 3;
      });

      const auto& b3 = db.create<book>( []( book& b )
      {
          b.a = 4;
          b.b = 5;
      });

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
      });

      BOOST_REQUIRE( b2.a == 10 );
      BOOST_REQUIRE( b2.b == 20 );
      BOOST_REQUIRE( b2.sum() == 30 );

      auto& idx_by_a = db.get_index< book_index, by_a >();

      auto bb = idx_by_a.end();
      --bb;

      /*
      BOOST_REQUIRE( bb->a == 10 );
      BOOST_REQUIRE( bb->b == 20 );
      BOOST_REQUIRE( bb->sum() == 30 );
      */
      BOOST_REQUIRE( &*bb == &b2 );

      auto& idx_by_sum = db.get_index< book_index, by_sum >();
      auto bb2 = idx_by_sum.end();
      --bb2;

      /*
      BOOST_REQUIRE( bb2->a == 10 );
      BOOST_REQUIRE( bb2->b == 20 );
      BOOST_REQUIRE( bb2->sum() == 30 );
      */
      BOOST_REQUIRE( &*bb2 == &b2 );
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_SUITE_END()

FC_REFLECT( book::id_type, (_id) )
FC_REFLECT( book, (id)(a)(b) )
CHAINBASE_SET_INDEX_TYPE( book, book_index )
