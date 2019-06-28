#pragma once
#include <dpn/chain/dpn_fwd.hpp>
#include <dpn/plugins/json_rpc/json_rpc_plugin.hpp>
#include <dpn/plugins/transaction_status/transaction_status_plugin.hpp>

#include <appbase/application.hpp>

namespace dpn { namespace plugins { namespace transaction_status_api {

#define DPN_TRANSACTION_STATUS_API_PLUGIN_NAME "transaction_status_api"

class transaction_status_api_plugin : public appbase::plugin< transaction_status_api_plugin >
{
   public:
      transaction_status_api_plugin();
      virtual ~transaction_status_api_plugin();

      APPBASE_PLUGIN_REQUIRES(
         (dpn::plugins::json_rpc::json_rpc_plugin)
         (dpn::plugins::transaction_status::transaction_status_plugin)
      )

      static const std::string& name() { static std::string name = DPN_TRANSACTION_STATUS_API_PLUGIN_NAME; return name; }

      virtual void set_program_options(
         boost::program_options::options_description& cli,
         boost::program_options::options_description& cfg ) override;
      virtual void plugin_initialize( const boost::program_options::variables_map& options ) override;
      virtual void plugin_startup() override;
      virtual void plugin_shutdown() override;

      std::unique_ptr< class transaction_status_api > api;
};

} } } // dpn::plugins::transaction_status_api
