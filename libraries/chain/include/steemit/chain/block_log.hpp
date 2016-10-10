#pragma once
#include <fc/filesystem.hpp>
#include <steemit/protocol/block.hpp>

namespace steemit { namespace chain {

   using namespace steemit::protocol;

   namespace detail { class block_log_impl; }

   class block_log {
      public:
         block_log();
         ~block_log();

         void open( const fc::path& file );
         void close();
         bool is_open()const;

         uint64_t append( const signed_block& b );
         void flush();
         std::pair< signed_block, uint64_t > read_block( uint64_t file_pos )const;
         signed_block read_head()const;
         const optional< signed_block >& head()const;

      private:
         std::unique_ptr<detail::block_log_impl> my;
   };

} }
