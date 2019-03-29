#include <steem/plugins/sps_api/sps_api_plugin.hpp>
#include <steem/plugins/sps_api/sps_api.hpp>
#include <steem/chain/sps_objects.hpp>
#include <appbase/application.hpp>
#include <steem/utilities/iterate_results.hpp>

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

template<typename RESULT_TYPE, typename FIELD_TYPE>
void sort_results_helper(RESULT_TYPE& result, order_direction_type order_direction, FIELD_TYPE api_proposal_object::*field)
{
  switch (order_direction)
  {
    case direction_ascending:
    {
      std::sort(result.begin(), result.end(), [&](const api_proposal_object& a, const api_proposal_object& b)
      {
        return a.*field < b.*field;
      });
    }
    break;
    case direction_descending:
    {
      std::sort(result.begin(), result.end(), [&](const api_proposal_object& a, const api_proposal_object& b)
      {
        return a.*field > b.*field;
      });
    }
    break;
    default:
      FC_ASSERT(false, "Unknown or unsupported sort order");
  }
}

template<typename RESULT_TYPE>
void sort_results(RESULT_TYPE& result, order_by_type order_by, order_direction_type order_direction)
{
  switch (order_by)
  {
    case by_creator:
    {
      sort_results_helper<RESULT_TYPE, account_name_type>(result, order_direction, &api_proposal_object::creator);
      return;
    }

    case by_start_date:
    {
      sort_results_helper<RESULT_TYPE, time_point_sec>(result, order_direction, &api_proposal_object::start_date);
      return;
    }

    case by_end_date:
    {
      sort_results_helper<RESULT_TYPE, time_point_sec>(result, order_direction, &api_proposal_object::end_date);
      return;
    }

    case by_total_votes:
    {
      sort_results_helper<RESULT_TYPE, uint64_t>(result, order_direction, &api_proposal_object::total_votes);
      return;
    }
    default:
      FC_ASSERT(false, "Unknown or unsupported field name");
  }
}

template <typename CONTAINER, typename PREDICATE>
CONTAINER filter(const CONTAINER &container, PREDICATE predicate) {
    CONTAINER result;
    std::copy_if(container.begin(), container.end(), std::back_inserter(result), predicate);
    return result;
}

template<typename OrderType, typename ValueType, typename ResultType, typename OnPush>
void iterate_ordered_results(ValueType start, std::vector<ResultType>& result, uint32_t limit, chain::database& db, bool ordered_ascending, bool start_from_end, fc::optional<uint64_t> last_id, OnPush&& on_push)
{
  const auto& idx = db.get_index< proposal_index, OrderType >();

  if (ordered_ascending)
  {
    // we are introducing last_id parameter to fix situations where one wants to paginathe trough results with the same values that
    // exceed given limit
    auto itr = last_id.valid() ? 
      (std::find_if(idx.begin(), idx.end(), [=](auto &proposal) {return static_cast<uint64_t>(proposal.id) == *last_id;})) : 
      idx.lower_bound(start);
    auto end = idx.end();

    while (result.size() < limit && itr != end)
    {
      result.push_back(on_push(*itr));
      ++itr;
    }
  }
  else
  {
    // we are introducing last_id parameter to fix situations where one wants to paginathe trough results with the same values that
    // exceed given limit
    auto itr = last_id.valid() ? 
      (std::find_if(idx.rbegin(), idx.rend(), [=](auto &proposal) {return static_cast<uint64_t>(proposal.id) == *last_id;})) :
      (start_from_end ? idx.rbegin() : boost::reverse_iterator<decltype(idx.upper_bound(start))>(idx.upper_bound(start)));
    auto end = idx.rend();

    while (result.size() < limit && itr != end)
    {
      result.push_back(on_push(*itr));
      ++itr;
    }
  }
}

DEFINE_API_IMPL(sps_api_impl, find_proposals) {
  ilog("find_proposal called");
  // cannot query for more than SPS_API_SINGLE_QUERY_LIMIT ids
  FC_ASSERT(args.id_set.size() <= SPS_API_SINGLE_QUERY_LIMIT);

  find_proposals_return result;
  result.reserve(args.id_set.size());

  auto currentTime = _db.head_block_time();

  std::for_each(args.id_set.begin(), args.id_set.end(), [&](auto& id) {
    auto po = _db.find<steem::chain::proposal_object, steem::chain::by_id>(id);
    if (po != nullptr)
    {
      result.emplace_back(api_proposal_object(*po, currentTime));
    }
  });

  return result;
}

DEFINE_API_IMPL(sps_api_impl, list_proposals) {
  ilog("list_proposals called");
  FC_ASSERT(args.limit <= SPS_API_SINGLE_QUERY_LIMIT);

  list_proposals_return result;
  result.reserve(args.limit);

  auto currentTime = _db.head_block_time();

  switch(args.order_by)
  {
    case by_creator:
    {
      iterate_ordered_results<steem::chain::by_creator>(
        args.start.as<account_name_type>(),
        result,
        args.limit,
        _db,
        args.order_direction == sps::order_direction_type::direction_ascending,
        (args.start.as<std::string>()).empty(),
        args.last_id, 
        [&currentTime](auto& proposal) { return api_proposal_object(proposal, currentTime); }
      );
    }
    break;
    case by_start_date:
    {
      const auto start_value = ((args.start.as<std::string>()).empty() && args.order_direction == sps::order_direction_type::direction_descending) ? time_point_sec() : args.start.as<time_point_sec>();
      iterate_ordered_results<steem::chain::by_start_date>(
        start_value,
        result,
        args.limit,
        _db,
        args.order_direction == sps::order_direction_type::direction_ascending,
        (args.start.as<std::string>()).empty(),
        args.last_id, 
        [&currentTime](auto& proposal) { return api_proposal_object(proposal, currentTime); }
      );
    }
    break;
    case by_end_date:
    {
      const auto start_value = ((args.start.as<std::string>()).empty() && args.order_direction == sps::order_direction_type::direction_descending) ? time_point_sec() : args.start.as<time_point_sec>();
      iterate_ordered_results<steem::chain::by_end_date>(
        start_value,
        result,
        args.limit,
        _db,
        args.order_direction == sps::order_direction_type::direction_ascending,
        (args.start.as<std::string>()).empty(),
        args.last_id, 
        [&currentTime](auto& proposal) { return api_proposal_object(proposal, currentTime); }
      );
    }
    break;
    case by_total_votes:
    {
      const auto start_value = ((args.start.as<std::string>()).empty() && args.order_direction == sps::order_direction_type::direction_descending) ? 0 : args.start.as<uint64_t>();
      iterate_ordered_results<steem::chain::by_total_votes>(
        start_value,
        result,
        args.limit,
        _db,
        args.order_direction == sps::order_direction_type::direction_ascending,
        (args.start.as<std::string>()).empty(),
        args.last_id, 
        [&currentTime](auto& proposal) { return api_proposal_object(proposal, currentTime); }
      );
    }
    break;
    default:
      FC_ASSERT( false, "Unknown or unsupported sort order" );
  }

  if (args.status != proposal_status::all) // avoid not needed rewrite in case of active set to all
  {
    // filter with active flag
    result = filter(result, [&](const auto& proposal) {
      return args.status == proposal.get_status(_db.head_block_time());
    });
  }

  return result;
}

DEFINE_API_IMPL(sps_api_impl, list_voter_proposals) {
  ilog("list_voter_proposals called");
  FC_ASSERT(args.limit <= SPS_API_SINGLE_QUERY_LIMIT);

  list_voter_proposals_return result;

  auto current_time = _db.head_block_time();

  const auto& idx = _db.get_index<proposal_vote_index, by_voter_proposal>();
  auto itr = idx.lower_bound(args.start.as<account_name_type>());
  auto end = idx.end();

  size_t proposals_count = 0;
  while( proposals_count < args.limit && itr != end )
  {
    auto po = _db.find<steem::chain::proposal_object, steem::chain::by_id>(itr->proposal_id);
    FC_ASSERT(po != nullptr, "Proposal with given id does not exist");
    auto apo = api_proposal_object(*po, current_time);
    if (args.status == proposal_status::all || apo.get_status(current_time) == args.status)
    {
      result[itr->voter].push_back(apo);
      ++proposals_count;
    }
    ++itr;
  }

  if (!result.empty())
  {
    // sorting operations
    for (auto it = result.begin(); it != result.end(); ++it)
    {
      sort_results<std::vector<api_proposal_object> >(it->second, args.order_by, args.order_direction);
    }
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
