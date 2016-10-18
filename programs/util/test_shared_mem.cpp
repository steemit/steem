
#include <iostream>
#include <string>

#include <boost/algorithm/string.hpp>

#include <fc/io/json.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/variant.hpp>

#include <graphene/utilities/key_conversion.hpp>

#include <steemit/protocol/types.hpp>
#include <steemit/protocol/authority.hpp>

#include <steemit/chain/shared_authority.hpp>

#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <algorithm>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/deque.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

#include <fc/fixed_string.hpp>


using boost::multi_index_container;
using namespace boost::multi_index;
namespace bip=boost::interprocess;

/* shared_string is a string type placeable in shared memory,
 *  * courtesy of Boost.Interprocess.
 *   */

typedef bip::basic_string<
  char,std::char_traits<char>,
  bip::allocator<char,bip::managed_mapped_file::segment_manager>
> shared_string;


typedef bip::allocator<shared_string,bip::managed_mapped_file::segment_manager> basic_string_allocator;

namespace fc {
    void to_variant( const shared_string& s, fc::variant& vo ) {
       vo = std::string(s.c_str());
    }
    void from_variant( const fc::variant& var,  shared_string& vo ) {
       vo = var.as_string().c_str();
    }

    /*
    template<typename... T >
    void to_variant( const bip::deque< T... >& t, fc::variant& v ) {
      std::vector<variant> vars(t.size());
      for( size_t i = 0; i < t.size(); ++i ) {
         vars[i] = t[i];
      }
      v = std::move(vars);
    }

    template<typename T, typename... A>
    void from_variant( const fc::variant& v, bip::deque< T, A... >& d ) {
      const variants& vars = v.get_array();
      d.clear();
      d.resize( vars.size() );
      for( uint32_t i = 0; i < vars.size(); ++i ) {
         from_variant( vars[i], d[i] );
      }
    }
    */
}


    /* Book record. All its members can be placed in shared memory,
     *  * hence the structure itself can too.
     *   */

struct book
{
     typedef bip::allocator<book,bip::managed_mapped_file::segment_manager> allocator_type;

     template<typename Constructor, typename Allocator>
     book( Constructor&& c, const Allocator& al )
     :name(al),author(al),pages(0),prize(0),
     auth( bip::allocator<steemit::chain::shared_authority, bip::managed_mapped_file::segment_manager>( al.get_segment_manager() )),
     deq( basic_string_allocator( al.get_segment_manager() ) )
     {
        c( *this );
     }

     shared_string name;
     shared_string author;
     int32_t                          pages;
     int32_t                          prize;
     steemit::chain::shared_authority auth;
     bip::deque<shared_string,basic_string_allocator> deq;

     book(const shared_string::allocator_type& al):
     name(al),author(al),pages(0),prize(0),
     auth( bip::allocator<steemit::chain::shared_authority, bip::managed_mapped_file::segment_manager>( al.get_segment_manager() )),
     deq( basic_string_allocator( al.get_segment_manager() ) )
     {}

};

typedef multi_index_container<
  book,
  indexed_by<
     ordered_non_unique< BOOST_MULTI_INDEX_MEMBER(book,shared_string,author) >,
     ordered_non_unique< BOOST_MULTI_INDEX_MEMBER(book,shared_string,name) >,
     ordered_non_unique< BOOST_MULTI_INDEX_MEMBER(book,int32_t,prize) >
  >,
  bip::allocator<book,bip::managed_mapped_file::segment_manager>
> book_container;


FC_REFLECT( book, (name)(author)(pages)(prize)(deq)(auth) )

struct astr {
   fc::fixed_string<> str;
   fc::fixed_string<> str1;
   fc::fixed_string<> str2;
};
FC_REFLECT( astr, (str)(str1)(str2) );
struct bstr {
   std::string str;
   std::string str1;
   std::string str2;
};
FC_REFLECT( bstr, (str)(str1)(str2) );

int main(int argc, char** argv, char** envp)
{
   try {

   bip::managed_mapped_file seg( bip::open_or_create,"./book_container.db", 1024*100);
   bip::named_mutex mutex( bip::open_or_create,"./book_container.db");

   /*
   book b( book::allocator_type( seg.get_segment_manager() ) );
   b.name = "test name";
   b.author = "test author";
   b.deq.push_back( shared_string( "hello world", basic_string_allocator( seg.get_segment_manager() )  ) );
   idump((b));
   */
   book_container* pbc = seg.find_or_construct<book_container>("book container")( book_container::ctor_args_list(),
                                                                                  book_container::allocator_type(seg.get_segment_manager()));

   for( const auto& item : *pbc ) {
      idump((item));
   }

   //b.pages = pbc->size();
   //b.auth = steemit::chain::authority( 1, "dan", pbc->size() );
   pbc->emplace( [&]( book& b ) {
                 b.name = "emplace name";
                 b.pages = pbc->size();
                }, book::allocator_type( seg.get_segment_manager() ) );

   bip::deque< book, book::allocator_type > * deq = seg.find_or_construct<bip::deque<book,book::allocator_type> >("book deque")( book_container::allocator_type(seg.get_segment_manager()));
   idump((deq->size()));

  // book c( b ); //book::allocator_type( seg.get_segment_manager() ) );
  // deq->push_back( b );


   } catch ( const std::exception& e ) {
      edump( (std::string(e.what())) );
   }
   return 0;
}
