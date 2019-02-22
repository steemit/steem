#include <steem/plugins/sps_api/sps_api_plugin.hpp>
#include <steem/plugins/sps_api/sps_api.hpp>
#include <steem/chain/sps_objects.hpp>
#include <appbase/application.hpp>

namespace steem { namespace plugins { namespace sps {

namespace detail {

using namespace steem::chain;

class sps_api_impl
{
  public:
    sps_api_impl();
    ~sps_api_impl();

    DECLARE_API_IMPL(
        (find_proposals)
        (list_proposals)
        (list_voter_proposals)
        )
    chain::database& _db;
};

sps_api_impl::sps_api_impl() : _db(appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db()) {}

sps_api_impl::~sps_api_impl() {}

api_proposal_object to_api_proposal_object(const proposal_object &po)
{
  api_proposal_object result;
  result.id = po.id;
  result.creator = po.creator;
  result.receiver = po.receiver;
  result.start_date = po.start_date;
  result.end_date = po.end_date;
  result.daily_pay = po.daily_pay;
  result.subject = to_string(po.subject);
  result.url = to_string(po.url);
  result.total_votes = po.total_votes;

  return result;
}

template<typename RESULT_TYPE, typename FIELD_TYPE>
void sort_results_helper(RESULT_TYPE& result, order_direction_type order_direction, FIELD_TYPE api_proposal_object::*field)
{
  switch (order_direction)
  {
    case direction_ascending:
    {
      std::sort(result.begin(), result.end(), [&](const api_proposal_object& a, const api_proposal_object& b)
      {
        return a.*field > b.*field;
      });
    }
    break;
    case direction_descending:
    {
      std::sort(result.begin(), result.end(), [&](const api_proposal_object& a, const api_proposal_object& b)
      {
        return a.*field < b.*field;
      });
    }
    break;
    default:
      FC_ASSERT(false, "Unknown or unsupported sort order");
  }
}

template<typename RESULT_TYPE>
void sort_results(RESULT_TYPE& result, const string& field_name, order_direction_type order_direction)
{
  // sorting operations
  if (field_name == "id")
  {
    sort_results_helper<RESULT_TYPE, api_id_type>(result, order_direction, &api_proposal_object::id);
    return;
  }

  if (field_name == "creator")
  {
    sort_results_helper<RESULT_TYPE, account_name_type>(result, order_direction, &api_proposal_object::creator);
    return;
  }

  if (field_name == "receiver")
  {
    sort_results_helper<RESULT_TYPE, account_name_type>(result, order_direction, &api_proposal_object::receiver);
    return;
  }

  if (field_name == "start_date")
  {
    sort_results_helper<RESULT_TYPE, time_point_sec>(result, order_direction, &api_proposal_object::start_date);
    return;
  }

  if (field_name == "end_date")
  {
    sort_results_helper<RESULT_TYPE, time_point_sec>(result, order_direction, &api_proposal_object::end_date);
    return;
  }

  if (field_name == "daily_pay")
  {
    sort_results_helper<RESULT_TYPE, asset>(result, order_direction, &api_proposal_object::daily_pay);
    return;
  }

  if (field_name == "total_votes")
  {
    sort_results_helper<RESULT_TYPE, uint64_t>(result, order_direction, &api_proposal_object::total_votes);
    return;
  }

  FC_ASSERT(false, "Unknown or unsupported field name");
}

DEFINE_API_IMPL(sps_api_impl, find_proposals) {
  ilog("find_proposal called");
  find_proposals_return result;
  const auto& pidx = _db.get_index<proposal_index>().indices().get<by_id>();
  std::for_each(args.id_set.begin(), args.id_set.end(), [&](auto& id) {
    auto found = pidx.find(id);
    if (found != pidx.end())
    {
      result.emplace_back(to_api_proposal_object(*found));
    }
  });

  if (!result.empty())
  {
    // sorting operations
    sort_results<find_proposals_return>(result, args.order_by, args.order_direction);
  }

  return result;
}

DEFINE_API_IMPL(sps_api_impl, list_proposals) {
  ilog("list_proposals called");
  list_proposals_return result;
  const auto& pidx = _db.get_index<proposal_index>().indices().get<by_id>();

  // populate result vector
  std::for_each(pidx.begin(), pidx.end(), [&](auto& proposal) {
    result.emplace_back(to_api_proposal_object(proposal));
  });

  if (!result.empty())
  {
    // sorting operations
    sort_results<list_proposals_return>(result, args.order_by, args.order_direction);
  }
  return result;
}

DEFINE_API_IMPL(sps_api_impl, list_voter_proposals) {
  ilog("list_voter_proposals called");
  list_voter_proposals_return result;

  const auto& pvidx = _db.get_index<proposal_vote_index>().indices().get<by_id>();

  std::vector<api_id_type> proposal_ids_of_voter;
  // filter out only proposal ids voted by our voter
  std::for_each(pvidx.begin(), pvidx.end(), [&](auto& vote_object) {
    if (vote_object.voter == args.voter)
    {
      proposal_ids_of_voter.emplace_back(vote_object.proposal_id);
    }
  });

  if (!proposal_ids_of_voter.empty())
  {
    // get all proposals
    const auto& pidx = _db.get_index<proposal_index>().indices().get<by_id>();
    // for each proposal id find proposal_obect and put it in result
    std::for_each(proposal_ids_of_voter.begin(), proposal_ids_of_voter.end(), [&](auto &proposal_id) {
      auto found = pidx.find(proposal_id);
      if (found != pidx.end())
      {
        result.emplace_back(to_api_proposal_object(*found));
      }
    });

    // sorting operations
    sort_results<list_voter_proposals_return>(result, args.order_by, args.order_direction);
  }
  return result;
}

} // detail

sps_api::sps_api(): my( new detail::sps_api_impl() )
{
   JSON_RPC_REGISTER_API( STEEM_SPS_API_PLUGIN_NAME );
}

sps_api::~sps_api() {}

DEFINE_READ_APIS(sps_api,
  (find_proposals)
  (list_proposals)
  (list_voter_proposals)
)

} } } //steem::plugins::sps_api