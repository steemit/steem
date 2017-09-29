#include <steem/plugins/chain_api/chain_api_plugin.hpp>
#include <steem/plugins/chain_api/chain_api.hpp>

namespace steem { namespace plugins { namespace chain {

namespace detail {

class chain_api_impl
{
   public:
      chain_api_impl() : _chain( appbase::app().get_plugin<chain_plugin>() ) {}

      DECLARE_API(
         (push_block)
         (push_transaction) )

   private:
      chain_plugin& _chain;
};

DEFINE_API( chain_api_impl, push_block )
{
   push_block_return result;

   result.success = false;

   try
   {
      _chain.accept_block(args.block, args.currently_syncing, chain::database::skip_nothing);
      result.success = true;
   }
   catch (const fc::exception& e)
   {
      result.error = e.to_detail_string();
   }
   catch (const std::exception& e)
   {
      result.error = e.what();
   }
   catch (...)
   {
      result.error = "uknown error";
   }

   return result;
}

DEFINE_API( chain_api_impl, push_transaction )
{
   push_transaction_return result;

   result.success = false;

   try
   {
      _chain.accept_transaction(args);
      result.success = true;
   }
   catch (const fc::exception& e)
   {
      result.error = e.to_detail_string();
   }
   catch (const std::exception& e)
   {
      result.error = e.what();
   }
   catch (...)
   {
      result.error = "uknown error";
   }
   
   return result;
}

} // detail

chain_api::chain_api(): my( new detail::chain_api_impl() )
{
   JSON_RPC_REGISTER_API( STEEM_CHAIN_API_PLUGIN_NAME );
}

chain_api::~chain_api() {}

DEFINE_API( chain_api, push_block )
{
   return my->push_block( args );
}

DEFINE_API( chain_api, push_transaction )
{
   return my->push_transaction( args );
}

} } } //steem::plugins::chain
