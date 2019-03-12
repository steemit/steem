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

DEFINE_API_IMPL(sps_api_impl, find_proposals) {
  ilog("find_proposal called");
  // cannot query for more than SPS_API_SINGLE_QUERY_LIMIT ids
  FC_ASSERT(args.id_set.size() <= SPS_API_SINGLE_QUERY_LIMIT);

  find_proposals_return result;
  
  std::for_each(args.id_set.begin(), args.id_set.end(), [&](auto& id) {
    auto po = _db.find<steem::chain::proposal_object, steem::chain::by_id>(id);
    if (po != nullptr)
    {
      result.emplace_back(api_proposal_object(*po));
    }
  });

  return result;
}

DEFINE_API_IMPL(sps_api_impl, list_proposals) {
  ilog("list_proposals called");
  FC_ASSERT(args.limit <= SPS_API_SINGLE_QUERY_LIMIT);

  list_proposals_return result;
  result.reserve(args.limit);

  switch(args.order_by)
  {
    case by_creator:
    {
      steem::utilities::iterate_results<proposal_index, steem::chain::by_creator>(
        args.start.as<account_name_type>(),
        result,
        args.limit,
        _db,
        [&](auto& proposal) { return api_proposal_object(proposal); } 
      );
    }
    break;
    case by_start_date:
    {
      steem::utilities::iterate_results<proposal_index, steem::chain::by_start_date>(
        args.start.as<time_point_sec>(),
        result,
        args.limit,
        _db,
        [&](auto& proposal) { return api_proposal_object(proposal); } 
      );
    }
    break;
    case by_end_date:
    {
      steem::utilities::iterate_results<proposal_index, steem::chain::by_end_date>(
        args.start.as<time_point_sec>(),
        result,
        args.limit,
        _db,
        [&](auto& proposal) { return api_proposal_object(proposal); } 
      );
    }
    break;
    case by_total_votes:
    {
      steem::utilities::iterate_results<proposal_index, steem::chain::by_total_votes>(
        args.start.as<uint64_t>(),
        result,
        args.limit,
        _db,
        [&](auto& proposal) { return api_proposal_object(proposal); } 
      );
    }
    break;
    default:
      FC_ASSERT( false, "Unknown or unsupported sort order" );
  }

  if (args.active != -1) // avoid not needed rewrite in case of active set to all
  {
    // filter with active flag
    result = filter(result, [&](const auto& proposal) {
      const bool is_active = proposal.is_active(_db.head_block_time());
      switch (args.active)
      {
        case 0:
          return !is_active;
        break;

        case 1:
          return is_active;
        break;

        default:
          return true;
      }
    });
  }

  if (!result.empty())
  {
    // sorting operations
    sort_results<list_proposals_return>(result, args.order_by, args.order_direction);
  }
  return result;
}

DEFINE_API_IMPL(sps_api_impl, list_voter_proposals) {
  ilog("list_voter_proposals called");
  FC_ASSERT(args.limit <= SPS_API_SINGLE_QUERY_LIMIT);

  list_voter_proposals_return result;

  const auto& idx = _db.get_index<proposal_vote_index, by_voter_proposal>();
  auto itr = idx.lower_bound(args.voter);
  auto end = idx.end();

  while( result.size() < args.limit && itr != end )
  {
    auto po = _db.find<steem::chain::proposal_object, steem::chain::by_id>(itr->proposal_id);
    FC_ASSERT(po != nullptr, "Proposal with given id does not exists");
    auto apo = api_proposal_object(*po);
    if (args.active == -1 || apo.is_active(_db.head_block_time()) == args.active)
    {
      result[itr->voter].push_back(apo);
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
