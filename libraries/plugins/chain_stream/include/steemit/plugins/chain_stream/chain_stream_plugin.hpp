
#pragma once

#include <steemit/app/plugin.hpp>

namespace steemit { namespace plugin { namespace chain_stream {

namespace detail {
class chain_stream_plugin_impl;
}

class chain_stream_plugin : public steemit::app::plugin
{
   public:
      chain_stream_plugin( steemit::app::application* app );
      virtual ~chain_stream_plugin();

      virtual std::string plugin_name()const override;
      virtual void plugin_set_program_options(
         boost::program_options::options_description& cli,
         boost::program_options::options_description& cfg) override;

      virtual void plugin_initialize( const boost::program_options::variables_map& options ) override;
      virtual void plugin_startup() override;
      virtual void plugin_shutdown() override;

   private:
      std::shared_ptr< detail::chain_stream_plugin_impl > my;
};

} } }
