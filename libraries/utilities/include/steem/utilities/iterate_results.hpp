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
}}