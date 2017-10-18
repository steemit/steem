#include <appbase/application.hpp>

#include <steem/plugins/token_api/token_api.hpp>
#include <steem/plugins/token_api/token_api_plugin.hpp>
#include <steem/plugins/token_api/dict.hpp>

#include <steem/protocol/get_config.hpp>

namespace steem { namespace plugins { namespace token_api {

class token_api_impl
{
   public:
      token_api_impl();
      ~token_api_impl();

      DECLARE_API
      (
         (files_check)
         (db_check)
      )

      chain::database& _db;
};

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Constructors                                                     //
//                                                                  //
//////////////////////////////////////////////////////////////////////

token_api::token_api()
   : my( new token_api_impl() )
{
   JSON_RPC_REGISTER_API( STEEM_TOKEN_API_PLUGIN_NAME );
}

token_api::~token_api() {}

token_api_impl::token_api_impl()
   : _db( appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db() ) {}

token_api_impl::~token_api_impl() {}

DEFINE_API( token_api, files_check )
{
   my->_db.with_read_lock( [&]()
   {
      my->files_check( args );
   });

   return void_type();
}

DEFINE_API( token_api_impl, files_check )
{
   const std::string path = "content";
   file_catcher files( path );

   tokenizer t( files );
   t.preprocess();
   t.save_content();
   t.summary();

   return void_type();
}

DEFINE_API( token_api, db_check )
{
   my->_db.with_read_lock( [&]()
   {
      my->db_check( args );
   });

   return void_type();
}

DEFINE_API( token_api_impl, db_check )
{
   db_catcher dbc( &_db );

   tokenizer t( dbc );
   t.preprocess();
   t.save_content();
   t.summary();

   return void_type();
}

} } } // steem::plugins::token_api
