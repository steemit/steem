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


    template< typename ResultType >
    static ResultType on_push_default( const ResultType& r ) { return r; }

    template< typename IndexType, typename OrderType, typename ValueType, typename ResultType, typename OnPush >
    void iterate_results( ValueType start, vector< ResultType >& result, uint32_t limit, OnPush&& on_push = &sps_api_impl::on_push_default< ResultType > )
    {
        const auto& idx = _db.get_index< IndexType, OrderType >();
        auto itr = idx.lower_bound( start );
        auto end = idx.end();

        while( result.size() < limit && itr != end )
        {
          result.push_back( on_push( *itr ) );
          ++itr;
        }
    }

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

    case by_total_votes:
    {
      sort_results_helper<RESULT_TYPE, uint64_t>(result, order_direction, &api_proposal_object::total_votes);
      return;
    }
    default:
      FC_ASSERT(false, "Unknown or unsupported field name");
  }
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

  if (!result.empty())
  {
    // sorting operations
    sort_results<find_proposals_return>(result, args.order_by, args.order_direction);
  }

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
      iterate_results<proposal_index, steem::chain::by_creator>(
        args.start.as<account_name_type>(),
        result,
        args.limit,
        [&](auto& proposal) { return api_proposal_object(proposal); } 
      );
    }
    break;
    case by_start_date:
    {
      iterate_results<proposal_index, steem::chain::by_date>(
        args.start.as<time_point_sec>(),
        result,
        args.limit,
        [&](auto& proposal) { return api_proposal_object(proposal); } 
      );
    }
    break;
    case by_total_votes:
    {
      iterate_results<proposal_index, steem::chain::by_total_votes>(
        args.start.as<uint64_t>(),
        result,
        args.limit,
        [&](auto& proposal) { return api_proposal_object(proposal); } 
      );
    }
    break;
    default:
      FC_ASSERT( false, "Unknown or unsupported sort order" );
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
  result.reserve(args.limit);

  std::vector<api_id_type> proposal_ids_of_voter;
  iterate_results<proposal_vote_index, by_voter_proposal>(
    account_name_type(args.voter),
    proposal_ids_of_voter,
    args.limit,
    [&](auto& vote_object) { return vote_object.proposal_id; }
  );

  if (!proposal_ids_of_voter.empty())
  {
    // for each proposal id find proposal_obect and put it in result
    std::for_each(proposal_ids_of_voter.begin(), proposal_ids_of_voter.end(), [&](auto &proposal_id) {
      auto po = _db.find<steem::chain::proposal_object, steem::chain::by_id>(proposal_id);
      if (po != nullptr)
      {
        result.emplace_back(api_proposal_object(*po));
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