#include <steem/plugins/sps_api/sps_api_plugin.hpp>
#include <steem/plugins/sps_api/sps_api.hpp>

namespace steem { namespace plugins { namespace sps {

  sps_api_plugin::sps_api_plugin() {}

  sps_api_plugin::~sps_api_plugin() {}

  void sps_api_plugin::plugin_initialize( const boost::program_options::variables_map& options )
  {
    api = std::make_shared< sps_api >();
  }

  void sps_api_plugin::plugin_startup() {}

  void sps_api_plugin::plugin_shutdown() {}

} } } // namespace steem::plugins::sps
