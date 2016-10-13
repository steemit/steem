#include <boost/test/unit_test.hpp>
#include <graphene/db2/database.hpp>
#include <fc/filesystem.hpp>
#include <fc/exception/exception.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <fc/io/raw.hpp>
#include <fc/reflect/variant.hpp>

using namespace graphene::db2;
using namespace boost::multi_index;

struct test_object : public graphene::db2::object<1, test_object>
{
   template<typename Constructor, typename Allocator>
   test_object( Constructor&& c, allocator<Allocator> a ){
      c(*this);
   };

   id_type id;
   int a = 0;
   int b = 0;
};

typedef multi_index_container<
  test_object,
  indexed_by<
     ordered_unique< BOOST_MULTI_INDEX_MEMBER(test_object,test_object::id_type,id), std::less<test_object::id_type> >,
     ordered_non_unique< BOOST_MULTI_INDEX_MEMBER(test_object,int,a) >,
     ordered_non_unique< BOOST_MULTI_INDEX_MEMBER(test_object,int,b) >
  >,
  bip::allocator<test_object,bip::managed_mapped_file::segment_manager>
> test_index;

GRAPHENE_DB2_SET_INDEX_TYPE( test_object, test_index );
FC_REFLECT( test_object, (id)(a)(b) );


BOOST_AUTO_TEST_CASE( db2_test )
{ try {
   try {
   fc::temp_directory temp_dir( "." );

   database db;
   uint64_t shared_file_size = 64*1024;

   db.open(  temp_dir.path() / "database.db", shared_file_size );
   ilog( "adding text_index" );
   db.add_index<test_index>();

   idump((db.revision()));

   ilog( "done adding text_index" );
   const auto& first = db.create<test_object>( [&]( test_object& o ) {
                           o.a = 1;
                           o.b = 2;
                           });
   idump((first));
   db.modify( first, [&]( test_object& o ) {
        o.b = 3;
   });
   const auto& idx = db.get_index<test_index>();

   wlog( "........... UNDO TEST .............. " );
   {
      for( const auto& item : idx.indices() ) idump((item));

      wlog( ".......... MODIFY ........." );
      auto s = db.start_undo_session(true);
      db.modify( first, [&]( test_object& o )
      {
         o.b = 4;
      });
      db.create<test_object>( [&]( test_object& o )
      {
         o.a = 20;
         o.b = 20;
      });

      for( const auto& item : idx.indices() ) idump((item));

      db.remove( first );

      for( const auto& item : idx.indices() ) idump((item));
      s.push();
   }
   for( const auto& item : idx.indices() ) idump((item));
   wlog( "........... END UNDO TEST .............. " );
   idump((db.revision()));
   db.undo();
   idump((db.revision()));
   for( const auto& item : idx.indices() ) idump((item));
   db.undo();
   idump((db.revision()));
   for( const auto& item : idx.indices() ) idump((item));
   db.close();

   ilog( "opening again" );
   db.open(  temp_dir.path() / "database.db", shared_file_size );
   {
   const auto& idx = db.get_index<test_index>();
   for( const auto& item : idx.indices() ) idump((item));
   }
   db.close();

   ilog( "opening copy" );
   database db2;
   db2.open(  temp_dir.path() / "database.db", shared_file_size );
   db2.close();


   } FC_CAPTURE_AND_RETHROW()
 } catch ( const fc::exception& e ) {
    edump( (e.to_detail_string() ) );
 }
}

BOOST_AUTO_TEST_CASE( db2_next_id )
{
   try
   {
      fc::temp_directory temp_dir( "." );

      database db;
      uint64_t shared_file_size = 64*1024;
      db.open( temp_dir.path() / "database.db", shared_file_size );

      db.add_index<test_index>();
      const test_object& first = db.create<test_object>( [&]( test_object& o ) {
         o.a = 11;
         o.b = 111;
         });
      BOOST_CHECK_EQUAL( first.id._id, 0 );

      {
         auto s = db.start_undo_session(true);
         const test_object& second = db.create<test_object>( [&]( test_object& o )
         {
            o.a = 22;
            o.b = 222;
         });
         BOOST_CHECK_EQUAL( second.id._id, 1 );
         db.remove(second);
         const test_object& third = db.create<test_object>( [&]( test_object& o )
         {
            o.a = 33;
            o.b = 333;
         });
         BOOST_CHECK_EQUAL( third.id._id, 2 );
         db.remove(third);
         {
            auto s2 = db.start_undo_session(true);
            const test_object& fourth = db.create<test_object>( [&]( test_object& o )
            {
               o.a = 44;
               o.b = 444;
            });
            BOOST_CHECK_EQUAL( fourth.id._id, 3 );
            db.remove(fourth);
            s2.undo();
         }
         const test_object& fifth = db.create<test_object>( [&]( test_object& o )
         {
            o.a = 55;
            o.b = 555;
         });
         BOOST_CHECK_EQUAL( fifth.id._id, 3 );
      }
   }
   catch( const fc::exception& e )
   {
      elog( "caught fc exception: ${e}", ("e", e.to_detail_string()) );
   }
}
