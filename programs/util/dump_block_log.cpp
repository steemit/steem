
#include <steem/chain/block_log.hpp>
#include <steem/protocol/block.hpp>

#include <fc/optional.hpp>
#include <fc/io/json.hpp>
#include <fc/log/logger.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace bfs = boost::filesystem;
namespace bpo = boost::program_options;
using bpo::options_description;
using bpo::variables_map;

int main( int argc, char** argv, char** envp )
{
   try
   {
      wlog( "The dump_block_log tool is experimental, it is not guaranteed to exist in future Steem versions" );

      bpo::options_description opts;
      opts.add_options()
         ("help,h", "Print this help message and exit.")
         ("data-dir,d", bpo::value<std::string>(), "Data directory" )
         ("block-range,r", bpo::value<std::string>()->default_value("1:"), "Range of blocks in the form FIRST:LAST, includes FIRST but not LAST" )
         ;

      bpo::variables_map options;
      bpo::store( bpo::parse_command_line(argc, argv, opts), options );

      if( options.count("help") )
      {
         std::cout << opts << "\n";
         return 0;
      }

      std::string block_range = options.at("block-range").as< std::string >();
      std::vector< std::string > tokens;
      boost::split( tokens, block_range, boost::is_any_of(":") );
      for( std::string& t : tokens )
         boost::trim(t);
      uint32_t first_block = 1;
      uint32_t last_block = std::numeric_limits< uint32_t >::max();
      if( tokens.size() == 1 )
      {
         // Assume a single number
         first_block = boost::lexical_cast< uint32_t >( tokens[0] );
         last_block = first_block;
      }
      else if ( tokens.size() == 2 )
      {
         if( tokens[0] == "" )
            first_block = 1;
         else
            first_block = boost::lexical_cast< uint32_t >( tokens[0] );
         if( tokens[1] == "" )
            last_block = std::numeric_limits< uint32_t >::max();
         else
            last_block = boost::lexical_cast< uint32_t >( tokens[1] )-1;
      }
      else
      {
         FC_ASSERT( false, "Couldn't parse argument" );
      }

      std::string data_dir = options.at("data-dir").as< std::string >();
      steem::chain::block_log bl;
      bl.open( data_dir + "/blockchain/block_log" );
      for( uint32_t block_num=first_block; block_num<=last_block; ++block_num )
      {
         fc::optional< steem::protocol::signed_block > b = bl.read_block_by_num( block_num );
         if( !b )
         {
            break;
         }
         std::cout << fc::json::to_string( *b ) << std::endl;
      }
      bl.close();

      return 0;
   } FC_LOG_AND_RETHROW()
}
