#include <steemit/chain2/chain_database.hpp>
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

   } catch ( const fc::exception& e ) {
      edump( (e.to_detail_string()) );
   }

   return 0;
}
