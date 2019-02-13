#pragma once
#include <steem/plugins/json_rpc/utility.hpp>

#include <steem/protocol/types.hpp>
#include <steem/chain/steem_object_types.hpp>
#include <steem/protocol/asset.hpp>

#include <fc/optional.hpp>
#include <fc/variant.hpp>
#include <fc/vector.hpp>

namespace steem { namespace plugins { namespace sps {
  using plugins::json_rpc::void_type;
  using namespace std;
  using namespace steem::chain;
  using steem::protocol::asset;

  namespace detail
  {
    class sps_api_impl;
  }
 
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
  typedef void_type create_proposal_return;

  struct update_proposal_votes_args {
    // voter;[set of worker_proposal_id];approve
    account_name_type voter;
    std::vector<uint64_t> proposal_ids;
    bool approve;
  };

  typedef void_type update_proposal_votes_return;

  // args type for get_proposals
  typedef void_type get_proposals_args;

  // return type for get_proposals
  struct get_proposals_return {
    std::vector<create_proposal_args> result;
  };

  struct get_voter_proposals_args {
    // voter
    account_name_type voter;
  };

  struct get_voter_proposals_return {
    std::vector<create_proposal_args> result;
  };

  class sps_api
  {
    public:
      sps_api();
      ~sps_api();

      DECLARE_API(
        (create_proposal)
        (update_proposal_votes)
        (get_proposals)
        (get_voter_proposals)
        )
    private:
        std::unique_ptr< detail::sps_api_impl > my;
  };

  } } }

  // Args and return types need to be reflected. We do not reflect typedefs of already reflected types
FC_REFLECT(steem::plugins::sps::create_proposal_args, (creator)(receiver)(start_date)(end_date)(daily_pay)(subject)(url));
FC_REFLECT(steem::plugins::sps::update_proposal_votes_args, (voter)(proposal_ids)(approve));
FC_REFLECT(steem::plugins::sps::get_proposals_return, (result));
FC_REFLECT(steem::plugins::sps::get_voter_proposals_args, (voter));
FC_REFLECT(steem::plugins::sps::get_voter_proposals_return, (result));
