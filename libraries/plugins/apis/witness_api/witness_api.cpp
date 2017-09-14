#include <steem/plugins/witness_api/witness_api_plugin.hpp>
#include <steem/plugins/witness_api/witness_api.hpp>

namespace steem { namespace plugins { namespace witness {

namespace detail {

class witness_api_impl
{
   public:
      witness_api_impl() : _db( appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db() ) {}

      get_account_bandwidth_return get_account_bandwidth( const get_account_bandwidth_args& args );

      chain::database& _db;
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

witness_api::witness_api(): my( new detail::witness_api_impl() )
{
   JSON_RPC_REGISTER_API( STEEM_WITNESS_API_PLUGIN_NAME );
}

witness_api::~witness_api() {}

get_account_bandwidth_return witness_api::get_account_bandwidth( const get_account_bandwidth_args& args )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_account_bandwidth( args );
   });
}

} } } // steem::plugins::witness
