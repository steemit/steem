#include <steemit/plugins/witness_api/witness_api_plugin.hpp>
#include <steemit/plugins/witness_api/witness_api.hpp>

namespace steemit { namespace plugins { namespace witness {

namespace detail {

class witness_api_impl
{
   public:
      witness_api_impl() : _db( appbase::app().get_plugin< steemit::plugins::chain::chain_plugin >().db() ) {}

      get_account_bandwidth_return get_account_bandwidth( const get_account_bandwidth_args& args );

      steemit::chain::database& _db;
};

get_account_bandwidth_return witness_api_impl::get_account_bandwidth( const get_account_bandwidth_args& args )
{
   get_account_bandwidth_return result;

   auto band = _db.find< witness::account_bandwidth_object, witness::by_account_bandwidth_type >( boost::make_tuple( args.account, args.type ) );
   if( band != nullptr )
      result.bandwidth = *band;

   return result;
}

} // detail

witness_api::witness_api()
{
   my = std::make_shared< detail::witness_api_impl >();

   appbase::app().get_plugin< plugins::json_rpc::json_rpc_plugin >().add_api(
      STEEM_WITNESS_API_PLUGIN_NAME,
      {
         API_METHOD( get_account_bandwidth )
      });
}

get_account_bandwidth_return witness_api::get_account_bandwidth( const get_account_bandwidth_args& args )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_account_bandwidth( args );
   });
}

} } } // steemit::plugins::witness
