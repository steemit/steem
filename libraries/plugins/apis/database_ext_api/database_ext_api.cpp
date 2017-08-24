#include <appbase/application.hpp>

#include <steemit/plugins/database_ext_api/database_ext_api.hpp>
#include <steemit/plugins/database_ext_api/database_ext_api_plugin.hpp>

#include <steemit/protocol/get_config.hpp>

namespace steemit { namespace plugins { namespace database_ext_api {

class database_ext_api_impl
{
   public:
      database_ext_api_impl();
      ~database_ext_api_impl();

      DECLARE_API
      (
         (get_block)
      )

      steemit::chain::database& _db;
};

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Constructors                                                     //
//                                                                  //
//////////////////////////////////////////////////////////////////////

database_ext_api::database_ext_api()
   : my( new database_ext_api_impl() )
{
   JSON_RPC_REGISTER_API(
      STEEM_DATABASE_EXT_API_PLUGIN_NAME,
      (get_block)
   );
}

database_ext_api::~database_ext_api() {}

database_ext_api_impl::database_ext_api_impl()
   : _db( appbase::app().get_plugin< steemit::plugins::chain::chain_plugin >().db() ) {}

database_ext_api_impl::~database_ext_api_impl() {}


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Blocks and transactions                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////

DEFINE_API( database_ext_api, get_block )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_block( args );
   });
}

DEFINE_API( database_ext_api_impl, get_block )
{
   get_block_return result;
   auto block = _db.fetch_block_by_number( args.block_num );

   if( block )
      result.block = *block;

   return result;
}

} } } // steemit::plugins::database_ext_api
