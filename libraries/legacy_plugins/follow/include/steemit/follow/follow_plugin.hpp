#pragma once
#include <steemit/app/plugin.hpp>
#include <steemit/chain/database.hpp>

#include <fc/thread/future.hpp>

#include <steemit/follow/follow_api.hpp>

namespace steemit { namespace follow {
using steemit::app::application;

#define FOLLOW_PLUGIN_NAME "follow"

namespace detail { class follow_plugin_impl; }

class follow_plugin : public steemit::app::plugin
{
   public:
      follow_plugin( application* app );

      std::string plugin_name()const override { return FOLLOW_PLUGIN_NAME; }
      virtual void plugin_set_program_options(
         boost::program_options::options_description& cli,
         boost::program_options::options_description& cfg ) override;
      virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
      virtual void plugin_startup() override;

      friend class detail::follow_plugin_impl;
      std::unique_ptr<detail::follow_plugin_impl> my;
      uint32_t max_feed_size = 500;
      fc::time_point_sec start_feeds;
};

} } //steemit::follow
