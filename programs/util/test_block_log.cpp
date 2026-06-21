#include <steem/chain/database.hpp>
#include <steem/protocol/block.hpp>
#include <fc/io/raw.hpp>
#include <fc/exception/exception.hpp>

int main( int argc, char** argv, char** envp )
{
   try
   {
      //steem::chain::database db;
      steem::chain::block_log log;

      fc::temp_directory temp_dir( "." );

      //db.open( temp_dir );
      log.open( temp_dir.path() / "log" );

      idump( (log.head() ) );

      steem::protocol::signed_block b1;
      b1.witness = "alice";
      b1.previous = steem::protocol::block_id_type();

      log.append( b1 );
      log.flush();
      idump( (b1) );
      idump( ( log.head() ) );
      idump( (fc::raw::pack_size(b1)) );

      steem::protocol::signed_block b2;
      b2.witness = "bob";
      b2.previous = b1.id();

      log.append( b2 );
      log.flush();
      idump( (b2) );
      idump( (log.head() ) );
      idump( (fc::raw::pack_size(b2)) );

      auto r1 = log.read_block( 0 );
      FC_ASSERT( r1.first.id() == b1.id() );
      idump( (r1) );
      idump( (fc::raw::pack_size(r1.first)) );

      auto r2 = log.read_block( r1.second );
      FC_ASSERT( r2.first.id() == b2.id() );
      idump( (r2) );
      idump( (fc::raw::pack_size(r2.first)) );

      idump( (log.read_head()) );
      idump( (fc::raw::pack_size(log.read_head())));

      // the following is expected to fail
      // as EOF should be reached
      bool threw = false;
      try {
         auto r3 = log.read_block(r2.second);
         idump( (r3) );
      }
      catch (const fc::exception& e) {
         threw = true;
         edump((e.to_detail_string()));
      }
      FC_ASSERT(threw, "Expected EOF exception but none was thrown");
      log.close();
   }
   catch ( const std::exception& e )
   {
      edump( ( std::string( e.what() ) ) );
      return -1;
   }

   return 0;
}