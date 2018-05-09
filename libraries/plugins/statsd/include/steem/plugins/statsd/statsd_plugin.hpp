#pragma once

#include <appbase/application.hpp>

#include <steem/plugins/chain/chain_plugin.hpp>

#include <boost/config.hpp>

#define STEEM_STATSD_PLUGIN_NAME "statsd"

namespace steem { namespace plugins { namespace statsd {

using namespace appbase;

namespace detail
{
   class statsd_plugin_impl;
}

class statsd_plugin : public appbase::plugin< statsd_plugin >
{
   public:
      statsd_plugin();
      virtual ~statsd_plugin();

      APPBASE_PLUGIN_REQUIRES( (steem::plugins::chain::chain_plugin) );
      virtual void set_program_options( options_description&, options_description& ) override;

      static const std::string& name() { static std::string name = STEEM_STATSD_PLUGIN_NAME; return name; }

      virtual void plugin_initialize( const variables_map& options ) override;
      virtual void plugin_startup() override;
      virtual void plugin_shutdown() override;

      void increment( const std::string& ns, const std::string& stat, const std::string& key,                       const float frequency = 1.0f ) const noexcept;
      void decrement( const std::string& ns, const std::string& stat, const std::string& key,                       const float frequency = 1.0f ) const noexcept;
      void count(     const std::string& ns, const std::string& stat, const std::string& key, const int64_t delta,  const float frequency = 1.0f ) const noexcept;
      void gauge(     const std::string& ns, const std::string& stat, const std::string& key, const uint64_t value, const float frequency = 1.0f ) const noexcept;
      void timing(    const std::string& ns, const std::string& stat, const std::string& key, const uint32_t ms,    const float frequency = 1.0f ) const noexcept;

   private:
      unique_ptr< detail::statsd_plugin_impl > my;
};

} } } // steem::plugins::statsd
