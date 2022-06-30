#include <steem/chain/steem_fwd.hpp>
#include <appbase/application.hpp>

#include <steem/plugins/block_api/block_api.hpp>
#include <steem/plugins/block_api/block_api_plugin.hpp>

#include <steem/protocol/get_config.hpp>

namespace steem { namespace plugins { namespace block_api {

class block_api_impl
{
   public:
      block_api_impl( uint32_t query_limit );
      ~block_api_impl();

      DECLARE_API_IMPL(
         (get_block_header)
         (get_block)
         (get_blocks_in_range)
      )

      chain::database& _db;

   private:
      uint32_t _query_limit;
};

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Constructors                                                     //
//                                                                  //
//////////////////////////////////////////////////////////////////////

block_api::block_api( uint32_t query_limit )
   : my( new block_api_impl( query_limit ) )
{
   JSON_RPC_REGISTER_API( STEEM_BLOCK_API_PLUGIN_NAME );
}

block_api::~block_api() {}

block_api_impl::block_api_impl( uint32_t query_limit ) :
   _db( appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db() ),
   _query_limit( query_limit ) {}

block_api_impl::~block_api_impl() {}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Blocks and transactions                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////
DEFINE_API_IMPL( block_api_impl, get_block_header )
{
   get_block_header_return result;
   auto block = _db.fetch_block_by_number( args.block_num );

   if( block )
      result.header = *block;

   return result;
}

DEFINE_API_IMPL( block_api_impl, get_block )
{
   get_block_return result;
   auto block = _db.fetch_block_by_number( args.block_num );

   if( block )
      result.block = *block;

   return result;
}

DEFINE_API_IMPL( block_api_impl, get_blocks_in_range )
{
   FC_ASSERT( args.from_num > 0 && args.from_num <= args.to_num,
      "Invalid block range ${from_num}-${to_num}", ( "from_num", args.from_num )( "to_num", args.to_num )
   );
   uint32_t count = args.to_num - args.from_num + 1;
   FC_ASSERT( count <= _query_limit,
      "Single query limit of ${limit} blocks exceeded", ( "limit", _query_limit )
   );

   get_blocks_in_range_return result;
   result.blocks.reserve( count );

   for ( uint32_t block_num = args.from_num; block_num <= args.to_num; ++block_num )
   {
      auto block = _db.fetch_block_by_number( block_num );
      FC_ASSERT( block.valid(),
         "Block ${block_num} could not be found", ( "block_num", block_num )
      );
      result.blocks.push_back( *block );
   }

   return result;
}

DEFINE_READ_APIS( block_api,
   (get_block_header)
   (get_block)
   (get_blocks_in_range)
)

} } } // steem::plugins::block_api
