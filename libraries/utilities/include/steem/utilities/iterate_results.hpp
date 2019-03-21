#include <vector>
#include <steem/chain/database.hpp>
#include <typeinfo>

namespace steem { namespace utilities {
  template<typename IndexType, typename OrderType, typename ValueType, typename ResultType, typename OnPush>
  void iterate_results(ValueType start, std::vector<ResultType>& result, uint32_t limit, chain::database& db, OnPush&& on_push)
  {
    const auto& idx = db.get_index< IndexType, OrderType >();
    auto itr = idx.lower_bound( start );
    auto end = idx.end();

    while( result.size() < limit && itr != end )
    {
      result.push_back( on_push( *itr ) );
      ++itr;
    }
  }

  template<typename IndexType, typename OrderType, typename ValueType, typename ResultType, typename OnPush>
  void iterate_ordered_results(ValueType start, std::vector<ResultType>& result, uint32_t limit, chain::database& db, bool ordered_ascending, bool start_from_end, OnPush&& on_push)
  {
    const auto& idx = db.get_index< IndexType, OrderType >();

    if (ordered_ascending)
    {
      auto itr = idx.lower_bound(start);
      auto end = idx.end();

      while (result.size() < limit && itr != end)
      {
        result.push_back(on_push(*itr));
        ++itr;
      }
    }
    else
    {
      auto itr = start_from_end ? idx.rbegin() : boost::reverse_iterator<decltype(idx.upper_bound(start))>(idx.upper_bound(start));
      auto end = idx.rend();

      while (result.size() < limit && itr != end)
      {
        result.push_back(on_push(*itr));
        ++itr;
      }
    }
  }
}}