#pragma once
#include <appbase/application.hpp>
#include <steem/chain/steem_object_types.hpp>
#include <steem/plugins/json_rpc/json_rpc_plugin.hpp>

#define STEEM_SPS_API_PLUGIN_NAME "sps_api"

namespace steem { namespace sps_api_plugin {

  using namespace appbase;

  struct create_proposal_args {
    // creator;receiver;start_date;end_date;daily_pay;subject;url
    account_name_type creator;
    account_name_type receiver;
    time_point_sec start_date;
    time_point_sec end_date;
    asset daily_pay;
    string subject;
    string url;
  };

  // return type for create_proposal
  typedef json_rpc::void_type create_proposal_return;

  struct update_proposal_votes_args {
    // voter;[set of worker_proposal_id];approve
    account_name_type voter;
    id_type_set proposal_ids;
    bool approve;
  };

  typedef json_rpc::void_type update_proposal_votes_return;

  // args type for get_proposals
  typedef json_rpc::void_type get_proposals_args;

  // return type for get_proposals
  struct get_proposals_return {
    
  };

  struct get_voter_proposals_args {
    // voter
    account_name_type voter;
  };

  struct get_voter_proposals_return {

  };

  class sps_api_plugin : public appbase::plugin<sps_api_plugin>
  {
    public:
      sps_api_plugin();
      virtual ~sps_api_plugin();

      APPBASE_PLUGIN_REQUIRES((plugins::json_rpc::json_rpc_plugin));

      static const std::string& name() {
        static std::string name = STEEM_SPS_API_PLUGIN_NAME; 
        return name; 
      }

      virtual void set_program_options(options_description&, options_description&) override {
      }

      virtual void plugin_initialize(const variables_map& options) override;
      virtual void plugin_startup() override;
      virtual void plugin_shutdown() override;

      create_proposal_return create_proposal(const create_proposal_args& args);
      update_proposal_votes_return update_proposal_votes(const update_proposal_votes_args& args);
      get_proposals_return get_proposals(const get_proposals_args& args);
      get_voter_proposals_return get_voter_proposals(const get_voter_proposals_args& args);

  };

  sps_api_plugin::sps_api_plugin() {

  }

  sps_api_plugin::~sps_api_plugin() {

  }

  void sps_api_plugin::plugin_initialize( const variables_map& options )
  {
    // This registers the API with the json rpc plugin
    JSON_RPC_REGISTER_API( name(), (create_proposal)(update_proposal_votes)(get_proposals)(get_voter_proposals) );
  }

  void sps_api_plugin::plugin_startup() {

  }

  void sps_api_plugin::plugin_shutdown() {

  }

  create_proposal_return sps_api_plugin::create_proposal(const create_proposal_args& args) {
    create_proposal_return result;

    return result;
  }

  update_proposal_votes_return sps_api_plugin::update_proposal_votes(const update_proposal_votes_args& args) {
    update_proposal_votes_return result;

    return result;
  }

  get_proposals_return sps_api_plugin::get_proposals(const get_proposals_args& args) {
    get_proposals_return result;

    return result;
  }

  get_voter_proposals_return sps_api_plugin::get_voter_proposals(const get_voter_proposals_args& args) {
    get_voter_proposals_return result;

    return result;
  }

} } // namespace steem::sps_api_plugin