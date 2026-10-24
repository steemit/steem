#pragma once
#include <steem/chain/steem_fwd.hpp>
#include <appbase/application.hpp>

#include <steem/plugins/chain/chain_plugin.hpp>

namespace steem { namespace plugins { namespace ingest {

namespace detail { class ingest_plugin_impl; }

using namespace appbase;

#define STEEM_INGEST_PLUGIN_NAME "ingest"

class ingest_plugin : public appbase::plugin< ingest_plugin >
{
   public:
      ingest_plugin();
      virtual ~ingest_plugin();

      APPBASE_PLUGIN_REQUIRES( (steem::plugins::chain::chain_plugin) )

      static const std::string& name() { static std::string name = STEEM_INGEST_PLUGIN_NAME; return name; }

      virtual void set_program_options( boost::program_options::options_description& cli,
                                       boost::program_options::options_description& cfg ) override;
      virtual void plugin_initialize( const boost::program_options::variables_map& options ) override;
      virtual void plugin_startup() override;
      virtual void plugin_shutdown() override;

   private:
      std::unique_ptr< detail::ingest_plugin_impl > my;
};

} } } // steem::plugins::ingest
