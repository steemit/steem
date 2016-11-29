#include <steemit/chain/database.hpp>
#include <steemit/protocol/block.hpp>
#include <fc/io/raw.hpp>

int main( int argc, char** argv, char** envp )
{
   try
   {
      //steemit::chain::database db;
      steemit::chain::block_log log;

      fc::temp_directory temp_dir( "." );

      //db.open( temp_dir );
      log.open( temp_dir.path() / "log" );

      idump( (log.head() ) );

      steemit::protocol::signed_block b1;
      b1.witness = "alice";
      b1.previous = steemit::protocol::block_id_type();

      log.append( b1 );
      log.flush();
      idump( (b1) );
      idump( ( log.head() ) );
      idump( (fc::raw::pack_size(b1)) );

      steemit::protocol::signed_block b2;
      b2.witness = "bob";
      b2.previous = b1.id();

      log.append( b2 );
      log.flush();
      idump( (b2) );
      idump( (log.head() ) );
      idump( (fc::raw::pack_size(b2)) );

      auto r1 = log.read_block( 0 );
      idump( (r1) );
      idump( (fc::raw::pack_size(r1.first)) );

      auto r2 = log.read_block( r1.second );
      idump( (r2) );
      idump( (fc::raw::pack_size(r2.first)) );

      idump( (log.read_head()) );
      idump( (fc::raw::pack_size(log.read_head())));

      auto r3 = log.read_block( r2.second );
      idump( (r3) );
   }
   catch ( const std::exception& e )
   {
      edump( ( std::string( e.what() ) ) );
   }

   return 0;
}