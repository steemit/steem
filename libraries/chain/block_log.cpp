#include <steemit/chain/block_log.hpp>
#include <fstream>
#include <fc/io/raw.hpp>

namespace steemit { namespace chain {

   namespace detail {
      class block_log_impl {
         public:
            optional< signed_block > head;
            block_id_type            head_id;
            std::fstream             block_stream;
            std::fstream             index_stream;
      };
   }

   block_log::block_log()
   :my( new detail::block_log_impl() )
   {
      my->block_stream.exceptions( std::fstream::failbit | std::fstream::badbit );
      my->index_stream.exceptions( std::fstream::failbit | std::fstream::badbit );
   }

   block_log::~block_log()
   {
      flush();
   }

   void block_log::open( const fc::path& file )
   {
      const fc::path index_file( file.generic_string() + ".index" );

      if( my->block_stream.is_open() )
         my->block_stream.close();
      if( my->index_stream.is_open() )
         my->index_stream.close();

      if( !fc::exists( file ) )
      {
         my->block_stream.open(       file.generic_string().c_str(), std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc );
         my->index_stream.open( index_file.generic_string().c_str(), std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc );
      }
      else
      {
         my->block_stream.open(       file.generic_string().c_str(), std::ios::in | std::ios::out | std::ios::binary );
         my->index_stream.open( index_file.generic_string().c_str(), std::ios::in | std::ios::out | std::ios::binary );
      }
      my->block_stream.seekp( 0, std::ios::end );
      my->index_stream.seekp( 0, std::ios::end );

      /* On startup of the block log, there are several states the log file and the index file can be
       * in relation to eachother.
       *
       *                          Block Log
       *                     Exists       Is New
       *                 +------------+------------+
       *          Exists |    Check   |   Delete   |
       *   Index         |    Head    |    Index   |
       *    File         +------------+------------+
       *          Is New |   Replay   |     Do     |
       *                 |    Log     |   Nothing  |
       *                 +------------+------------+
       *
       * Checking the heads of the files has several conditions as well.
       *  - If they are the same, do nothing.
       *  - If the index file head is not in the log file, delete the index and replay.
       *  - If the index file head is in the log, but not up to date, replay from index head.
       */
      auto log_size = fc::file_size( file );
      auto index_size = fc::file_size( index_file );

      if( log_size )
      {
         ilog( "Log is nonempty" );
         my->head = read_head();
         my->head_id = my->head->id();

         if( index_size )
         {
            ilog( "Index is nonempty" );
            uint64_t block_pos;
            my->block_stream.seekg( -sizeof( uint64_t), std::ios::end );
            my->block_stream.read( (char*)&block_pos, sizeof( block_pos ) );

            uint64_t index_pos;
            my->index_stream.seekg( -sizeof( uint64_t), std::ios::end );
            my->index_stream.read( (char*)&index_pos, sizeof( index_pos ) );

            if( block_pos < index_pos )
            {
               ilog( "block_pos < index_pos, close and reopen index_stream" );
               my->index_stream.close();
               fc::remove_all( index_file );
               my->index_stream.open( index_file.generic_string().c_str(), std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc );
               my->index_stream.seekp( 0, std::ios::end );
            }
            else if( block_pos > index_pos )
            {
               construct_index( index_pos );
            }
         }
         else
         {
            ilog( "Index is empty, rebuild it" );
            construct_index( 0 );
         }
      }
      else if( index_size )
      {
         ilog( "Index is nonempty, remove and recreate it" );
         my->index_stream.close();
         fc::remove_all( index_file );
         my->index_stream.open( index_file.generic_string().c_str(), std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc );
         my->index_stream.seekp( 0, std::ios::end );
      }
   }

   void block_log::close()
   {
      my.reset( new detail::block_log_impl() );
   }

   bool block_log::is_open()const
   {
      return my->block_stream.is_open();
   }

   uint64_t block_log::append( const signed_block& b )
   {
      uint64_t pos = my->block_stream.tellp();
      fc::raw::pack( my->block_stream, b );
      my->block_stream.write( (char*)&pos, sizeof( pos ) );
      my->index_stream.write( (char*)&pos, sizeof( pos ) );
      my->head = b;
      my->head_id = b.id();
      return pos;
   }

   void block_log::flush()
   {
      ilog( "Flushing block_log" );
      my->block_stream.flush();
      my->index_stream.flush();
   }

   std::pair< signed_block, uint64_t > block_log::read_block( uint64_t pos )const
   {
      my->block_stream.seekg( pos );
      std::pair<signed_block,uint64_t> result;
      fc::raw::unpack( my->block_stream, result.first );
      result.second = uint64_t(my->block_stream.tellg()) + 8;
      return result;
   }

   optional< signed_block > block_log::read_block_by_num( uint32_t block_num )const
   {
      optional< signed_block > b;
      uint64_t pos = get_block_pos( block_num );
      if( ~pos )
         b = read_block( pos ).first;
      return b;
   }

   uint64_t block_log::get_block_pos( uint32_t block_num ) const
   {
      if( !( my->head && block_num <= protocol::block_header::num_from_id( my->head_id ) ) )
         return ~0;
      my->index_stream.seekg( sizeof( uint64_t ) * ( block_num - 1 ) );
      uint64_t pos;
      my->index_stream.read( (char*)&pos, sizeof( pos ) );
      return pos;
   }

   signed_block block_log::read_head()const
   {
      uint64_t pos;
      my->block_stream.seekg( -sizeof(pos), std::ios::end );
      my->block_stream.read( (char*)&pos, sizeof(pos) );
      return read_block( pos ).first;
   }

   const optional< signed_block >& block_log::head()const
   {
      return my->head;
   }

   void block_log::construct_index( uint64_t start_pos )
   {
      uint64_t end_pos;
      my->block_stream.seekg( -sizeof( uint64_t), std::ios::end );
      my->block_stream.read( (char*)&end_pos, sizeof( end_pos ) );
      signed_block tmp;

      while( start_pos < end_pos )
      {
         my->block_stream.seekg( start_pos );
         fc::raw::unpack( my->block_stream, tmp );
         start_pos = uint64_t(my->block_stream.tellg()) + 8;
         my->index_stream.write( (char*)&start_pos, sizeof( start_pos ) );
      }
   }

} }
