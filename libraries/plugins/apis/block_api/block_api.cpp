#include <appbase/application.hpp>

#include <steemit/plugins/block_api/block_api.hpp>
#include <steemit/plugins/block_api/block_api_plugin.hpp>

#include <steemit/protocol/get_config.hpp>

namespace steemit { namespace plugins { namespace block_api {

class block_api_impl
{
   public:
      block_api_impl();
      ~block_api_impl();

      DECLARE_API
      (
         (get_block_header)
         (get_block)
      )

      chain::database& _db;
};

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Constructors                                                     //
//                                                                  //
//////////////////////////////////////////////////////////////////////

block_api::block_api()
   : my( new block_api_impl() )
{
   JSON_RPC_REGISTER_API(
      STEEM_BLOCK_API_PLUGIN_NAME,
      (get_block_header)
      (get_block)
   );
}

block_api::~block_api() {}

block_api_impl::block_api_impl()
   : _db( appbase::app().get_plugin< steemit::plugins::chain::chain_plugin >().db() ) {}

block_api_impl::~block_api_impl() {}


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Blocks and transactions                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////
DEFINE_API( block_api, get_block_header )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_block_header( args );
   });
}

DEFINE_API( block_api_impl, get_block_header )
{
   get_block_header_return result;
   auto block = _db.fetch_block_by_number( args.block_num );

   if( block )
      result.header = *block;

   return result;
}

DEFINE_API( block_api, get_block )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_block( args );
   });
}

DEFINE_API( block_api_impl, get_block )
{
   get_block_return result;
   auto block = _db.fetch_block_by_number( args.block_num );

   if( block )
      result.block = *block;

   return result;
}

} } } // steemit::plugins::block_api
