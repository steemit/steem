#pragma once
#include <steem/plugins/json_rpc/utility.hpp>
#include <steem/chain/sps_objects.hpp>

namespace steem { namespace plugins { namespace sps {
  using plugins::json_rpc::void_type;
  using steem::chain::account_name_type;
  using steem::chain::proposal_object;
  using steem::chain::proposal_id_type;

  namespace detail
  {
    class sps_api_impl;
  }

  enum order_direction_type 
  {
    direction_ascending,
    direction_descending
  };

  // Struct with arguments for find_proposal methd
  struct find_proposal_args 
  {
    // id of the proposal to find
    steem::chain::proposal_id_type id;
  };

  // Return type for find_proposal method
  typedef std::vector<proposal_object> find_proposal_return;
  
  // Struct with argumentse for list_proposals method
  struct list_proposals_args 
  {
    // name of the field by which results will be sored
    string order_by;
    // sorting order (ascending or descending) of the result vector
    order_direction_type order_direction;
    // result will contain only data with active flag set to this value
    int8_t active;
  };

  // Return type for list_proposals
  typedef std::vector<proposal_object> list_proposals_return;
  
  // Struct with arguments for list_voter_proposals methid
  struct list_voter_proposals_args 
  {
    // list only proposal voted by this voter
    account_name_type voter;
    // name of the field by which results will be sored
    string order_by;
    // sorting order (ascending or descending) of the result vector
    order_direction_type order_direction;
    // result will contain only data with active flag set to this value
    int8_t active;
  };

  // Return type for list_voter_proposals
  typedef std::vector<proposal_object> list_voter_proposals_return;
  
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

FC_REFLECT(steem::plugins::sps::find_proposal_args, 
  (id)
  );

FC_REFLECT(steem::plugins::sps::list_proposals_args, 
  (order_by)
  (order_direction)
  (active)
  );

FC_REFLECT(steem::plugins::sps::list_voter_proposals_args, 
  (voter)
  (order_by)
  (order_direction)
  (active)
  );

