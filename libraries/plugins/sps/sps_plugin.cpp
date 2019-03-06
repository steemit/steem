#include <steem/plugins/sps/sps_plugin.hpp>

namespace steem { namespace plugins { namespace sps {

namespace detail {

class sps_plugin_impl
{
   private:

      chain::database& _db;
      sps_plugin& _self;

   public:

      sps_plugin_impl( sps_plugin& _plugin ) :
         _db( appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db() ),
         _self( _plugin ) {}

      ~sps_plugin_impl() {}
};


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
   }
   FC_CAPTURE_AND_RETHROW()
}

void sps_plugin::plugin_startup() {}

void sps_plugin::plugin_shutdown(){}

} } } // steem::plugins::sps
