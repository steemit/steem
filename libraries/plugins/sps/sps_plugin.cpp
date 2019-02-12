#include <steem/plugins/sps/sps_plugin.hpp>
#include <steem/chain/notifications.hpp>

namespace steem { namespace plugins { namespace sps {

using namespace steem::protocol;
using steem::chain::block_notification;

namespace detail {

class sps_plugin_impl
{
   public:
      sps_plugin_impl( sps_plugin& _plugin ) :
         _db( appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db() ),
         _self( _plugin ) {}
      ~sps_plugin_impl() {}

      chain::database& _db;
      sps_plugin& _self;
      boost::signals2::connection _on_proposal_processing;

      void on_proposal_processing( const block_notification& note );
};

void sps_plugin_impl::on_proposal_processing( const block_notification& note )
{
   
}

}; // detail

sps_plugin::sps_plugin() {}

sps_plugin::~sps_plugin() {}

void sps_plugin::set_program_options(
   boost::program_options::options_description& cli,
   boost::program_options::options_description& cfg
   )
{
}

void sps_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   try
   {
      ilog("Intializing sps plugin" );

      my = std::make_unique< detail::sps_plugin_impl >( *this );

      my->_on_proposal_processing = my->_db.add_on_proposal_processing_handler(
         [&]( const block_notification& note ){ my->on_proposal_processing( note ); }, *this, 0 );
   }
   FC_CAPTURE_AND_RETHROW()
}

void sps_plugin::plugin_startup() {}

void sps_plugin::plugin_shutdown()
{
   chain::util::disconnect_signal( my->_on_proposal_processing );
}

} } } // steem::plugins::sps
