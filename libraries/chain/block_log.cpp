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

   void block_log::open( const fc::path& file ) {
      my->out_blocks.close();
      my->in_blocks.close();
      my->out_blocks.open( file.generic_string().c_str(), std::ios::app | std::ios::binary );
      my->in_blocks.open( file.generic_string().c_str(), std::ios::in | std::ios::binary );
      my->out_blocks.open( ( file.generic_string() + ".index" ).c_str(), std::ios::app | std::ios::binary );
      my->in_blocks.open( ( file.generic_string() + ".index" ).c_str(), std::ios::in | std::ios::binary );
      if( fc::file_size( file ) > 8 )
         my->head = read_head();
   }

   void block_log::close() {
      my.reset( new detail::block_log_impl() );
   }

   bool block_log::is_open()const {
      return my->in_blocks.is_open();
   }

   uint64_t block_log::append( const signed_block& b ) {
      uint64_t pos = my->out_blocks.tellp();
      fc::raw::pack( my->out_blocks, b );
      my->out_blocks.write( (char*)&pos, sizeof(pos) );
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
      if( pos < ~0 )
         fc::raw::unpack( my->in_blocks, result.first );
         result.second = uint64_t(my->in_blocks.tellg()) + 8;
         if( my->head && my->head_id == result.first.id() )
            result.second = ~0;
      else
         result.second = ~0;
      return result;
   }

   uint64_t block_log::get_block_pos( uint32_t block_num ) const
   {
      if( !( my->head && block_num <= protocol::block_header::num_from_id( my->head_id ) ) )
         return ~0;
      my->in_index.seekg( sizeof( uint64_t ) * (block_num - 1) );
      uint64_t pos;
      my->in_index.read( (char*)&pos, sizeof( pos ) );
      return pos;
   }

   signed_block block_log::read_head()const {
      uint64_t pos;
      my->in_blocks.seekg( -sizeof(pos), std::ios::end );
      my->in_blocks.read( (char*)&pos, sizeof(pos) );
      return read_block( pos ).first;
   }

   const optional< signed_block >& block_log::head()const {
      return my->head;
   }

} }
