#pragma once
#include <steem/plugins/json_rpc/utility.hpp>
#include <steem/chain/sps_objects.hpp>

#define SPS_API_SINGLE_QUERY_LIMIT 1000

namespace steem { namespace plugins { namespace sps {
  using plugins::json_rpc::void_type;
  using steem::chain::account_name_type;
  using steem::chain::proposal_object;
  using steem::chain::proposal_id_type;
  using steem::protocol::asset;
  using steem::chain::proposal_object;
  using steem::chain::to_string;

  namespace detail
  {
    class sps_api_impl;
  }

  enum order_direction_type 
  {
    direction_ascending, ///< sort with ascending order
    direction_descending ///< sort with descending order
  };

  enum order_by_type
  {
    by_creator, ///< order by proposal creator
    by_start_date, ///< order by proposal start date
    by_end_date, ///< order by proposal end date
    by_total_votes, ///< order by total votes
  };

  typedef uint64_t api_id_type;

  struct api_proposal_object
  {
    api_proposal_object() {}  
    api_proposal_object(const proposal_object& po) : 
      id(po.id),
      creator(po.creator),
      receiver(po.receiver),
      start_date(po.start_date),
      end_date(po.end_date),
      daily_pay(po.daily_pay),
      subject(to_string(po.subject)),
      url(to_string(po.url)),
      total_votes(po.total_votes)
    {}

    //internal key
    api_id_type id = 0;

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

    const bool is_active(const time_point_sec &head_time) const
    {
      if (head_time >= start_date && head_time <= end_date)
      {
        return true;
      }
      return false;
    }
  };

  // Struct with arguments for find_proposals methd
  struct find_proposals_args 
  {
    // set of ids of the proposals to find
    flat_set<api_id_type> id_set;
  };

  // Return type for find_proposal method
  typedef std::vector<api_proposal_object> find_proposals_return;
  
  // Struct with argumentse for list_proposals method
  struct list_proposals_args 
  {
    // starting value for querying results
    fc::variant start;
    // name of the field by which results will be sored
    order_by_type order_by;
    // sorting order (ascending or descending) of the result vector
    order_direction_type order_direction;
    // query limit
    uint16_t limit;
    // result will contain only data with active flag set to this value
    int8_t active;
  };

  // Return type for list_proposals
  typedef std::vector<api_proposal_object> list_proposals_return;
  
  // Struct with arguments for list_voter_proposals methid
  struct list_voter_proposals_args 
  {
    // list only proposal voted by this voter
    account_name_type voter;
    // name of the field by which results will be sored
    order_by_type order_by;
    // sorting order (ascending or descending) of the result vector
    order_direction_type order_direction;
    // query limit
    uint16_t limit;
    // result will contain only data with active flag set to this value
    int8_t active;
  };

  // Return type for list_voter_proposals
  typedef std::map<std::string, std::vector<api_proposal_object> > list_voter_proposals_return;
  
  class sps_api
  {
    public:
      sps_api();
      ~sps_api();

      DECLARE_API(
        (find_proposals)
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

FC_REFLECT_ENUM(steem::plugins::sps::order_by_type, 
  (by_creator)
  (by_start_date)
  (by_end_date)
  (by_total_votes)
  );

FC_REFLECT(steem::plugins::sps::api_proposal_object,
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

FC_REFLECT(steem::plugins::sps::find_proposals_args, 
  (id_set)
  );

FC_REFLECT(steem::plugins::sps::list_proposals_args, 
  (start)
  (order_by)
  (order_direction)
  (limit)
  (active)
  );

FC_REFLECT(steem::plugins::sps::list_voter_proposals_args, 
  (voter)
  (order_by)
  (order_direction)
  (limit)
  (active)
  );

