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
  chainbase::allocator<book>
> book_index;

namespace fc
{
class variant;

template<typename T>
void to_variant( const chainbase::oid<T>& var,  variant& vo )
{
   vo = var._id;
}

template<typename T>
void from_variant( const variant& vo, chainbase::oid<T>& var )
{
   var._id = vo.as_int64();
}

/*
template< typename T > struct get_typename< chainbase::oid<T> >
{
   static const char* name()
   {
      static std::string n = std::string("chainbase::oid<") + get_typename<T>::name() + ">";
      return n.c_str();
   }
};
*/

namespace raw
{

template<typename Stream, typename T>
void pack( Stream& s, const chainbase::oid<T>& id )
{
   s.write( (const char*)&id._id, sizeof(id._id) );
}

template<typename Stream, typename T>
void unpack( Stream& s, chainbase::oid<T>& id )
{
   s.read( (char*)&id._id, sizeof(id._id));
}

} // raw

} // fc

/*
namespace mira { namespace multi_index { namespace detail {

template< typename T, typename CompareType >
struct slice_comparator< chainbase::oid< T >, CompareType > final : abstract_slice_comparator< chainbase::oid< T >, CompareType >
{
   slice_comparator() : abstract_slice_comparator< chainbase::oid< T >, CompareType >()
   {}

   virtual int Compare( const ::rocksdb::Slice& x, const ::rocksdb::Slice& y ) const override
   {
      std::cout << x.size() << ' ' << y.size() << std::endl;
      //assert( x.size() == y.size() );

      auto x_key = fc::raw::unpack_from_char_array;

      int r = (*this)(
         std::move( fc::raw::unpack_from_char_array< int64_t >( x.data(), x.size() ) ),
         std::move( fc::raw::unpack_from_char_array< int64_t >( y.data(), y.size() ) )
      );

      if( r ) return -1;

      if( memcmp( x.data(), y.data(), x.size() ) == 0 ) return 0;

      return 1;
   }

   virtual bool Equal( const ::rocksdb::Slice& x, const ::rocksdb::Slice& y ) const override
   {
      assert( x.size() == y.size() );
      return memcmp( x.data(), y.data(), x.size() ) == 0;
   }
};

} } } // mira::multi_index::detail
//*/

FC_REFLECT( book::id_type, (_id) )

FC_REFLECT( book, (id)(a)(b) )
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
   boost::filesystem::path temp = boost::filesystem::current_path() / boost::filesystem::unique_path();
   try
   {
      chainbase::database db;
      db.open( temp, 0, 1024*1024*8 );

      chainbase::database db2; /// open an already created db

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
   catch( ... )
   {
      std::cout << "exception" << std::endl;
      chainbase::bfs::remove_all( temp );
      throw;
   }

   chainbase::bfs::remove_all( temp );
}
