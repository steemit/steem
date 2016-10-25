#include <steemit/chain/block_log.hpp>
#include <fstream>
#include <fc/io/raw.hpp>


namespace steemit { namespace chain {

   namespace detail {
      class block_log_impl {
         public:
            optional< signed_block > head;
            block_id_type            head_id;
            std::fstream             out_blocks;
            std::fstream             in_blocks;
            std::fstream             out_index;
            std::fstream             in_index;
      };
   }

   block_log::block_log()
   :my( new detail::block_log_impl() ){}

   block_log::~block_log(){
   }

   void block_log::open( const fc::path& file )
   {
      const fc::path index_file( file.generic_string() + ".index" );
      my->out_blocks.close();
      my->in_blocks.close();
      my->out_blocks.open( file.generic_string().c_str(), std::ios::app | std::ios::binary );
      my->in_blocks.open( file.generic_string().c_str(), std::ios::in | std::ios::binary );
      my->out_index.open( index_file.generic_string().c_str(), std::ios::app | std::ios::binary );
      my->in_index.open( index_file.generic_string().c_str(), std::ios::in | std::ios::binary );

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
         my->head = read_head();
         my->head_id = my->head->id();

         if( index_size )
         {
            uint64_t block_pos;
            my->in_blocks.seekg( -sizeof( uint64_t), std::ios::end );
            my->in_blocks.read( (char*)&block_pos, sizeof( block_pos ) );

            uint64_t index_pos;
            my->in_index.seekg( -sizeof( uint64_t), std::ios::end );
            my->in_index.read( (char*)&index_pos, sizeof( index_pos ) );

            if( block_pos < index_pos )
            {
               my->out_index.close();
               my->in_index.close();
               fc::remove_all( index_file );
               my->out_index.open( index_file.generic_string().c_str(), std::ios::app | std::ios::binary );
               my->in_index.open( index_file.generic_string().c_str(), std::ios::in | std::ios::binary );
            }
            else if( block_pos > index_pos )
            {
               construct_index( index_pos );
            }
         }
         else
         {
            construct_index( 0 );
         }
      }
      else if( index_size )
      {
         my->out_index.close();
         my->in_index.close();
         fc::remove_all( index_file );
         my->out_index.open( index_file.generic_string().c_str(), std::ios::app | std::ios::binary );
         my->in_index.open( index_file.generic_string().c_str(), std::ios::in | std::ios::binary );
      }
   }

   void block_log::close()
   {
      my.reset( new detail::block_log_impl() );
   }

   bool block_log::is_open()const
   {
      return my->in_blocks.is_open();
   }

   uint64_t block_log::append( const signed_block& b )
   {
      uint64_t pos = my->out_blocks.tellp();
      fc::raw::pack( my->out_blocks, b );
      my->out_blocks.write( (char*)&pos, sizeof( pos ) );
      my->out_index.write( (char*)&pos, sizeof( pos ) );
      my->head = b;
      my->head_id = b.id();
      return pos;
   }

   void block_log::flush()
   {
      my->out_blocks.flush();
   }

   std::pair< signed_block, uint64_t > block_log::read_block( uint64_t pos )const
   {
      my->in_blocks.seekg( pos );
      std::pair<signed_block,uint64_t> result;
      fc::raw::unpack( my->in_blocks, result.first );
      result.second = uint64_t(my->in_blocks.tellg()) + 8;
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
      my->in_index.seekg( sizeof( uint64_t ) * ( block_num - 1 ) );
      uint64_t pos;
      my->in_index.read( (char*)&pos, sizeof( pos ) );
      return pos;
   }

   signed_block block_log::read_head()const
   {
      uint64_t pos;
      my->in_blocks.seekg( -sizeof(pos), std::ios::end );
      my->in_blocks.read( (char*)&pos, sizeof(pos) );
      return read_block( pos ).first;
   }

   const optional< signed_block >& block_log::head()const
   {
      return my->head;
   }

   void block_log::construct_index( uint64_t start_pos )
   {
      uint64_t end_pos;
      my->in_blocks.seekg( -sizeof( uint64_t), std::ios::end );
      my->in_blocks.read( (char*)&end_pos, sizeof( end_pos ) );
      signed_block tmp;

      while( start_pos < end_pos )
      {
         my->in_blocks.seekg( start_pos );
         fc::raw::unpack( my->in_blocks, tmp );
         start_pos = uint64_t(my->in_blocks.tellg()) + 8;
         my->out_index.write( (char*)&start_pos, sizeof( start_pos ) );
      }
   }

} }
