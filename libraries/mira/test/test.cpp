#define BOOST_TEST_MODULE mira test

#include <mira/multi_index_container.hpp>
#include <mira/ordered_index.hpp>
#include <mira/tag.hpp>
#include <mira/member.hpp>
#include <mira/indexed_by.hpp>

#include <boost/test/unit_test.hpp>

#include <chainbase/chainbase.hpp>

#include <iostream>

using namespace mira;

using mira::multi_index::multi_index_container;
using mira::multi_index::indexed_by;
using mira::multi_index::ordered_unique;
using mira::multi_index::tag;
using mira::multi_index::member;

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
};

struct by_id;
struct by_a;

typedef multi_index_container<
  book,
  indexed_by<
     ordered_unique< tag< by_id >, member< book, book::id_type, &book::id > >,
     ordered_unique< tag< by_a >,  member< book, int,           &book::a  > >
  >,
  chainbase::allocator< book >
> book_index;

BOOST_FIXTURE_TEST_SUITE( mira_tests, rocksdb_fixture )

BOOST_AUTO_TEST_CASE( open_and_create )
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
          b.b = 2;
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
         BOOST_REQUIRE( tmp_book.b == 2 );
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
         BOOST_REQUIRE( tmp_book.b == 2 );
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
         BOOST_REQUIRE( tmp_book.b == 2 );
      }

      BOOST_REQUIRE( book_by_a_idx.upper_bound( 5 ) == book_by_a_idx.end() );

      {
         const auto& tmp_book = db.get< book, by_id >( 1 );

         BOOST_REQUIRE( tmp_book.id._id == 1 );
         BOOST_REQUIRE( tmp_book.a == 4 );
         BOOST_REQUIRE( tmp_book.b == 2 );
      }

      {
         const auto* book_ptr = db.find< book, by_a >( 4 );

         BOOST_REQUIRE( book_ptr->id._id == 1 );
         BOOST_REQUIRE( book_ptr->a == 4 );
         BOOST_REQUIRE( book_ptr->b == 2 );
      }

      bool is_found = db.find< book, by_a >( 10 ) != nullptr;

      BOOST_REQUIRE( !is_found );

      const auto& book_by_id = db.get< book, by_id >( 0 );
      const auto& book_by_a = db.get< book, by_a >( 3 );

      BOOST_REQUIRE( &book_by_id == &book_by_a );

      db.modify( db.get< book, by_id >( 0 ), []( book& b )
      {
         b.a = 10;
         b.b = 10;
      });

      {
         const auto& tmp_book = db.get< book, by_id >( 0 );

         BOOST_REQUIRE( tmp_book.id._id == 0 );
         BOOST_REQUIRE( tmp_book.a == 10 );
         BOOST_REQUIRE( tmp_book.b == 10 );
      }

      try
      {
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
         BOOST_REQUIRE( tmp_book.b == 10 );
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
         BOOST_REQUIRE( tmp_book.b == 2 );
      }

      ++a_itr;

      {
         const auto& tmp_book = *a_itr;

         BOOST_REQUIRE( tmp_book.id._id == 0 );
         BOOST_REQUIRE( tmp_book.a == 10 );
         BOOST_REQUIRE( tmp_book.b == 10 );
      }

      ++a_itr;

      BOOST_REQUIRE( a_itr == book_by_a_idx.end() );

      db.remove( db.get< book, by_id >( 0 ) );

      is_found = db.find< book, by_id >( 0 ) != nullptr;

      BOOST_REQUIRE( !is_found );

      itr = book_idx.begin();

      BOOST_REQUIRE( itr != book_idx.end() );

      {
         const auto& tmp_book = *itr;

         BOOST_REQUIRE( tmp_book.id._id == 1 );
         BOOST_REQUIRE( tmp_book.a == 4 );
         BOOST_REQUIRE( tmp_book.b == 2 );
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
         BOOST_REQUIRE( tmp_book.b == 2 );
      }

      ++a_itr;

      BOOST_REQUIRE( a_itr == book_by_a_idx.end() );

   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_SUITE_END()

FC_REFLECT( book::id_type, (_id) )

FC_REFLECT( book, (id)(a)(b) )
CHAINBASE_SET_INDEX_TYPE( book, book_index )
