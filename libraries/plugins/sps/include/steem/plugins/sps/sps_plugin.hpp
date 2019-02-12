#pragma once

#include <steem/plugins/chain/chain_plugin.hpp>

#define STEEM_SPS_PLUGIN_NAME "sps"


namespace steem { namespace plugins{ namespace sps {

namespace detail { class sps_plugin_impl; }

using namespace appbase;

class sps_plugin : public appbase::plugin< sps_plugin >
{
   public:
      sps_plugin();
      virtual ~sps_plugin();

      APPBASE_PLUGIN_REQUIRES( (steem::plugins::chain::chain_plugin) )

      static const std::string& name() { static std::string name = STEEM_SPS_PLUGIN_NAME; return name; }

      virtual void set_program_options(
         options_description& cli,
         options_description& cfg ) override;
      virtual void plugin_initialize( const variables_map& options ) override;
      virtual void plugin_startup() override;
      virtual void plugin_shutdown() override;
 
   private:
      std::unique_ptr< detail::sps_plugin_impl > my;
};

} } } //steem::sps
