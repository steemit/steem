#pragma once
#include <steemit/app/plugin.hpp>
#include <steemit/chain/steem_objects.hpp>

namespace steemit { namespace fork_info {
using steemit::app::application;

#define FORK_INFO_PLUGIN_NAME "fork_info"

namespace detail { struct fork_info_plugin_impl; }

class fork_info_plugin : public steemit::app::plugin
{
   public:
      fork_info_plugin( application* app );
      virtual ~fork_info_plugin();

      std::string plugin_name()const override { return FORK_INFO_PLUGIN_NAME; }
      virtual void plugin_set_program_options(
         boost::program_options::options_description& cli,
         boost::program_options::options_description& cfg ) override;
      virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
      virtual void plugin_startup() override;

      std::unique_ptr<steemit::fork_info::detail::fork_info_plugin_impl> _my;
};

} } //steemit::fork_info
