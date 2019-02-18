#pragma once
#include <steem/plugins/json_rpc/utility.hpp>

#include <steem/protocol/types.hpp>
#include <steem/chain/steem_object_types.hpp>
#include <steem/protocol/asset.hpp>

namespace steem { namespace plugins { namespace sps {
  using plugins::json_rpc::void_type;
  using namespace steem::chain;
  using steem::protocol::asset;

  namespace detail
  {
    class sps_api_impl;
  }

  enum order_direction_type 
  {
    direction_ascending,
    direction_descending
  };

  typedef uint64_t id_type;

  struct proposal_object 
  {
    //internal key
    id_type id;

    // account that created the proposal
    account_name_type creator;

    // account_being_funded
    account_name_type receiver;

    // start_date (when the proposal will begin paying out if it gets enough vote weight)
    time_point_sec start_date;

    // end_date (when the proposal expires and can no longer pay out)
    time_point_sec end_date;

    //daily_pay (the amount of SBD that is being requested to be paid out daily)
    asset daily_pay;

    //subject (a very brief description or title for the proposal)
    string subject;

    //url (a link to a page describing the work proposal in depth, generally this will probably be to a Steem post).
    string url;

    //This will be calculate every maintenance period
    uint64_t total_votes = 0;
  };

  struct find_proposal_args 
  {
    id_type id;
  };

  struct find_proposal_return 
  {
    proposal_object result;
  };

  // args type for get_proposals
  struct list_proposals_args 
  {
    string order_by;
    order_direction_type order_direction;
    int8_t active;
  };

  // return type for get_proposals
  struct list_proposals_return 
  {
    std::vector<proposal_object> result;
  };

  struct list_voter_proposals_args 
  {
    // voter
    account_name_type voter;
    string order_by;
    order_direction_type order_direction;
    int8_t active;
  };

  struct list_voter_proposals_return 
  {
    std::vector<proposal_object> result;
  };

  class sps_api
  {
    public:
      sps_api();
      ~sps_api();

      DECLARE_API(
        (find_proposal)
        (list_proposals)
        (list_voter_proposals)
        )
    private:
        std::unique_ptr<detail::sps_api_impl> my;
  };

  } } }

// Args and return types need to be reflected. We do not reflect typedefs of already reflected types
FC_REFLECT_ENUM(steem::plugins::sps::order_direction_type, 
  (direction_ascending)
  (direction_descending)
  );

FC_REFLECT(steem::plugins::sps::proposal_object, 
  (id)
  (creator)
  (receiver)
  (start_date)
  (end_date)
  (daily_pay)
  (subject)
  (url)
  (total_votes) 
  );

FC_REFLECT(steem::plugins::sps::find_proposal_args, 
  (id)
  );

FC_REFLECT(steem::plugins::sps::find_proposal_return, 
  (result)
  );

FC_REFLECT(steem::plugins::sps::list_proposals_args, 
  (order_by)
  (order_direction)
  (active)
  );

FC_REFLECT(steem::plugins::sps::list_proposals_return, 
  (result)
  );

FC_REFLECT(steem::plugins::sps::list_voter_proposals_args, 
  (voter)
  (order_by)
  (order_direction)
  (active)
  );

FC_REFLECT(steem::plugins::sps::list_voter_proposals_return, 
  (result)
  );
