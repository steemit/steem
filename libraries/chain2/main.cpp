#include <graphene/db2/database.hpp>
#include <steemit/chain2/chain_database.hpp>
#include <steemit/chain2/block_objects.hpp>


#include <boost/interprocess/containers/vector.hpp>
#include <fc/io/raw_fwd.hpp>
#include <fc/io/json.hpp>

namespace fc {
namespace raw {
    namespace bip = boost::interprocess;

    template<typename Stream, typename T, typename... A>
    inline void pack( Stream& s, const bip::vector<T,A...>& value ) {
      pack( s, unsigned_int((uint32_t)value.size()) );
      auto itr = value.begin();
      auto end = value.end();
      while( itr != end ) {
        fc::raw::pack( s, *itr );
        ++itr;
      }
    }
    template<typename Stream, typename T, typename... A>
    inline void unpack( Stream& s, bip::vector<T,A...>& value ) {
      unsigned_int size;
      unpack( s, size );
      value.clear(); value.resize(size);
      for( auto& item : value )
          fc::raw::unpack( s, item );
    }
}}

#include <fc/io/raw.hpp>
using namespace steemit::chain2;


int main( int argc, char** argv ) {

   try {
      steemit::chain2::database db;
      db.open( "." );

      idump((db.head_block_num()));

      signed_block genesis;
      genesis.witness = "dantheman";

      if( db.head_block_num() )  {
         genesis.previous = db.head_block().block_id;
         genesis.timestamp = db.head_block().timestamp + fc::seconds(3);
      }

      idump((genesis));
      apply( db, genesis ); 
      idump((db.revision()));

      const auto& head = db.head_block();
      auto packed_block = fc::raw::pack( genesis );
      auto packed_db_obj = fc::raw::pack( head );

      idump((packed_db_obj)(packed_db_obj.size()));

      db.modify( head, [&]( block_object& b ) {
          fc::datastream<const char*> ds( packed_db_obj.data(), packed_db_obj.size() );
          fc::raw::unpack( ds, b );
      });
      packed_db_obj = fc::raw::pack( head );
      idump((packed_db_obj));
      idump((head));
      auto js = fc::json::to_string( head );
      idump((js)(js.size()));
      db.modify( head, [&]( block_object& b ) {
         auto var = fc::json::from_string( js );
         var.as(b);
      });
      js = fc::json::to_string( head );
      idump((js)(js.size()));

   } catch ( const fc::exception& e ) {
      edump( (e.to_detail_string()) );
   }

   return 0;
}
