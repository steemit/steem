#pragma once
#include <appbase/application.hpp>

#include <steem/plugins/chain/chain_plugin.hpp>

namespace steem { namespace plugins { namespace rc {

namespace detail { class rc_plugin_impl; }

using namespace appbase;

#define STEEM_RC_PLUGIN_NAME "rc"

class rc_plugin : public appbase::plugin< rc_plugin >
{
   public:
      rc_plugin();
      virtual ~rc_plugin();

      APPBASE_PLUGIN_REQUIRES( (steem::plugins::chain::chain_plugin) )

      static const std::string& name() { static std::string name = STEEM_RC_PLUGIN_NAME; return name; }

      virtual void set_program_options( options_description& cli, options_description& cfg ) override;
      virtual void plugin_initialize( const variables_map& options ) override;
      virtual void plugin_startup() override;
      virtual void plugin_shutdown() override;

   private:
      std::unique_ptr< detail::rc_plugin_impl > my;
};

} } } // steem::plugins::rc
