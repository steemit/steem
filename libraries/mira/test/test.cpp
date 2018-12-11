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

struct book : public chainbase::object<0, book> {

   template<typename Constructor, typename Allocator>
    book(  Constructor&& c, Allocator&& a ) {
       c(*this);
    }

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
  chainbase::allocator<book>
> book_index;

CHAINBASE_SET_INDEX_TYPE( book, book_index )

/*

Create call stack

ord_index_impl::emplace_impl
index_base::final_emplace_
multi_index_container::emplace_
ord_index_impl::insert_
...
index_base::insert_

*/

/*

Inheritance:

index_base
ord_index_impl
... (For each index)
multi_index_container
*/

BOOST_AUTO_TEST_CASE( open_and_create )
{
   boost::filesystem::path temp = boost::filesystem::unique_path();
   try
   {
      std::cerr << temp.native() << " \n";

      chainbase::database db;
      db.open( temp, 0, 1024*1024*8 );

      chainbase::database db2; /// open an already created db

      db.add_index< book_index >();

      BOOST_TEST_MESSAGE( "Creating book" );
      //const auto& new_book =
      db.create<book>( []( book& b )
      {
          b.a = 3;
          b.b = 4;
      });

      std::cout << db.get_index< book_index >().indices().get_column_size() << std::endl;
      const auto& idx = db.get_index< book_index >().indices();

      mira::multi_index::column_definitions defs;

      idx.populate_column_families_( defs );
   }
   catch( ... )
   {
      chainbase::bfs::remove_all( temp );
      throw;
   }
}
