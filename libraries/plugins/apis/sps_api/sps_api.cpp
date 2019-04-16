#include <steem/plugins/sps_api/sps_api_plugin.hpp>
#include <steem/plugins/sps_api/sps_api.hpp>
#include <steem/chain/sps_objects.hpp>
#include <appbase/application.hpp>

namespace steem { namespace plugins { namespace sps {

namespace detail {

using namespace steem::chain;

template<bool NullCheck, typename Proposal>
bool check_proposal(const Proposal* proposal)
{
  if(NullCheck)
  {
    return proposal != nullptr && !proposal->removed;
  }
  return !proposal->removed;
}

inline bool filter_proposal_status(const api_proposal_object& po, const proposal_status filter, const time_point_sec& current_time)
{
   if(filter == proposal_status::all)
      return true;

   auto prop_status = po.get_status(current_time);
   return filter == prop_status ||
      (filter == proposal_status::votable && (prop_status == proposal_status::active || prop_status == proposal_status::inactive));
}

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

    template<class Iterator>
    boost::reverse_iterator<Iterator> make_reverse_iterator(Iterator iterator)
    {
      return boost::reverse_iterator<Iterator>(iterator);
    }

    template<typename OrderType, typename ValueType>
    void iterate_ordered_results(fc::variant start, std::vector<api_proposal_object>& result, uint32_t limit, order_direction_type direction, proposal_status status, fc::optional<uint64_t> last_id)
    {
      const auto& idx = _db.get_index<proposal_index, OrderType>();
      const std::string& start_as_str = start.as<std::string>();
      const bool last_id_valid = last_id.valid();
      const auto currentTime = _db.head_block_time();

      switch (direction)
      {
        case direction_ascending:
        {
          // we are introducing last_id parameter to fix situations where one wants to paginathe trough results with the same values that
          // exceed given limit
          // if last_id is valid variable we will try to get reverse iterator to the element with id set to last_id
          auto get_last_id_itr = [&]()
          {
            if (last_id_valid)
            {
              return idx.iterator_to(*(_db.get_index<proposal_index, by_proposal_id>().find(*last_id)));
            }
            return idx.begin();
          };

          auto itr = last_id_valid ? get_last_id_itr() : (start_as_str.empty() ? idx.begin() : idx.lower_bound(start.as<ValueType>()));
          auto end = idx.end();

          while (result.size() < limit && itr != end)
          {
            if(check_proposal<false/*NullCheck*/>(&(*itr)))
            {
              auto apo = api_proposal_object(*itr, currentTime);
              if (filter_proposal_status(apo, status, currentTime))
              {
                result.push_back(apo);
              }
            }
            ++itr;
          }
        }
        break;

        case direction_descending:
        {
          // we are introducing last_id parameter to fix situations where one wants to paginathe trough results with the same values that
          // exceed given limit
          // if last_id is valid variable we will try to get reverse iterator to the element with id set to last_id
          // reverse iterator is shifted by one in relation to normal iterator, so we need to increment it by one
          auto get_last_id_itr = [&]()
          {
            if (last_id_valid)
            {
              auto itr = idx.iterator_to(*(_db.get_index<proposal_index, by_proposal_id>().find(*last_id)));
              ++itr;
              return make_reverse_iterator(itr);
            }
            return idx.rbegin();
          };
          
          auto itr = last_id_valid ?  get_last_id_itr() :
            (start_as_str.empty() ? idx.rbegin() : make_reverse_iterator(idx.upper_bound(start.as<ValueType>())));
          auto end = idx.rend();

          while (result.size() < limit && itr != end)
          {
            if (check_proposal<false/*NullCheck*/>( &(*itr)))
            {
              auto apo = api_proposal_object(*itr, currentTime);
              if (filter_proposal_status(apo, status, currentTime))
              {
                result.push_back(apo);
              }
            }
            ++itr;
          }
        }
      }
    }
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

DEFINE_API_IMPL(sps_api_impl, find_proposals) {
  ilog("find_proposal called");
  // cannot query for more than SPS_API_SINGLE_QUERY_LIMIT ids
  FC_ASSERT(args.id_set.size() <= SPS_API_SINGLE_QUERY_LIMIT);

  find_proposals_return result;
  result.reserve(args.id_set.size());

  auto currentTime = _db.head_block_time();

  std::for_each(args.id_set.begin(), args.id_set.end(), [&](auto& id) {
    auto po = _db.find<steem::chain::proposal_object, steem::chain::by_proposal_id>(id);
    if ( check_proposal< true/*NullCheck*/>( po ) )
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

  switch(args.order_by)
  {
    case by_creator:
    {
      iterate_ordered_results<steem::chain::by_creator, account_name_type>(
        args.start,
        result,
        args.limit,
        args.order_direction,
        args.status,
        args.last_id
      );
    }
    break;
    case by_start_date:
    {
      iterate_ordered_results<steem::chain::by_start_date, time_point_sec>(
        args.start,
        result,
        args.limit,
        args.order_direction,
        args.status,
        args.last_id
      );
    }
    break;
    case by_end_date:
    {
      iterate_ordered_results<steem::chain::by_end_date, time_point_sec>(
        args.start,
        result,
        args.limit,
        args.order_direction,
        args.status,
        args.last_id
      );
    }
    break;
    case by_total_votes:
    {
      iterate_ordered_results<steem::chain::by_total_votes, uint64_t>(
        args.start,
        result,
        args.limit,
        args.order_direction,
        args.status,
        args.last_id
      );
    }
    break;
    default:
      FC_ASSERT( false, "Unknown or unsupported sort order" );
  }

  return result;
}

DEFINE_API_IMPL(sps_api_impl, list_voter_proposals) {
  ilog("list_voter_proposals called");
  FC_ASSERT(args.limit <= SPS_API_SINGLE_QUERY_LIMIT);

  list_voter_proposals_return result;

  auto current_time = _db.head_block_time();

  const auto& idx = _db.get_index<proposal_vote_index, by_voter_proposal>();
  auto itr = args.last_id.valid() ? idx.find(boost::make_tuple(args.start.as<account_name_type>(), *(args.last_id))) : idx.lower_bound(args.start.as<account_name_type>());
  auto end = idx.end();

  size_t proposals_count = 0;
  while( proposals_count < args.limit && itr != end )
  {
    auto po = _db.find<steem::chain::proposal_object, steem::chain::by_proposal_id>(itr->proposal_id);
    FC_ASSERT(check_proposal< true/*NullCheck*/>( po ), "Proposal with given id does not exist");
    auto apo = api_proposal_object(*po, current_time);
    if (filter_proposal_status(apo, args.status, current_time))
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
