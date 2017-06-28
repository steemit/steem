#pragma once
#include <steemit/plugins/chain/chain_plugin.hpp>
#include <steemit/plugins/http/http_plugin.hpp>

#include <appbase/application.hpp>
#include <steemit/chain/database.hpp>

#define CHAIN_API_PLUGIN_NAME "chain_api"

namespace steemit { namespace plugins { namespace chain_api {

using steemit::chain::database;
using std::unique_ptr;
using namespace appbase;

class chain_api_plugin : public plugin< chain_api_plugin >
{
   public:
      APPBASE_PLUGIN_REQUIRES((plugins::chain::chain_plugin)(plugins::http::http_plugin))

      chain_api_plugin() {}
      virtual ~chain_api_plugin() {}

      std::string plugin_name()const { return CHAIN_API_PLUGIN_NAME; }

      virtual void set_program_options(options_description&, options_description&) override {}

      void plugin_initialize(const variables_map&) {}
      void plugin_startup();
      void plugin_shutdown();
};

} } } // steemit::plugins::chain_api_plugin
