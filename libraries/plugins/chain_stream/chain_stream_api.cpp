
#include <steemit/app/api_context.hpp>
#include <steemit/app/application.hpp>

#include <steemit/plugins/chain_stream/chain_stream_api.hpp>
#include <steemit/plugins/chain_stream/chain_stream_plugin.hpp>

namespace steemit { namespace plugin { namespace chain_stream {

namespace detail {

class chain_stream_api_impl
{
   public:
      chain_stream_api_impl( steemit::app::application& _app );

      std::shared_ptr< steemit::plugin::chain_stream::chain_stream_plugin > get_plugin();

      steemit::app::application& app;
};

chain_stream_api_impl::chain_stream_api_impl( steemit::app::application& _app ) : app( _app )
{}

std::shared_ptr< steemit::plugin::chain_stream::chain_stream_plugin > chain_stream_api_impl::get_plugin()
{
   return app.get_plugin< chain_stream_plugin >( "chain_stream" );
}

} // detail

chain_stream_api::chain_stream_api( const steemit::app::api_context& ctx )
{
   my = std::make_shared< detail::chain_stream_api_impl >(ctx.app);
}

void chain_stream_api::on_api_startup() { }

} } } // steemit::plugin::chain_stream
