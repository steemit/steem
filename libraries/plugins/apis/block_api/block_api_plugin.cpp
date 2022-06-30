#include <steem/plugins/block_api/block_api.hpp>
#include <steem/plugins/block_api/block_api_plugin.hpp>

namespace steem { namespace plugins { namespace block_api {

#define OPTION_QUERY_LIMIT_GET_BLOCKS_IN_RANGE "query-limit-get-blocks-in-range"

block_api_plugin::block_api_plugin() {}
block_api_plugin::~block_api_plugin() {}

void block_api_plugin::set_program_options( options_description& cli, options_description& cfg )
{
   cfg.add_options()
      (
         OPTION_QUERY_LIMIT_GET_BLOCKS_IN_RANGE,
         bpo::value< uint32_t >()->default_value( BLOCK_API_DEFAULT_QUERY_LIMIT ),
         "Defines the maximum single query limit for the 'get_blocks_in_range' method."
      );
}

void block_api_plugin::plugin_initialize( const variables_map& options )
{
   uint32_t query_limit = (
      options.count( OPTION_QUERY_LIMIT_GET_BLOCKS_IN_RANGE )
         ? options.at( OPTION_QUERY_LIMIT_GET_BLOCKS_IN_RANGE ).as< uint32_t >()
         : BLOCK_API_DEFAULT_QUERY_LIMIT
   );
   api = std::make_shared< block_api >( query_limit );
}

void block_api_plugin::plugin_startup() {}

void block_api_plugin::plugin_shutdown() {}

} } } // steem::plugins::block_api
