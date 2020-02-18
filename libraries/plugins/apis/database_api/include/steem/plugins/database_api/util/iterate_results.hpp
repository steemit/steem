#pragma once

namespace steem { namespace plugins { namespace database_api { namespace util {

template< typename ResultType >
static ResultType on_push_default( const ResultType& r ) { return r; }

template< typename ValueType >
static bool filter_default( const ValueType& r ) { return true; }

template<typename IndexType,  typename StartType, typename ResultType, typename OnPushType, typename FilterType>
void iterate_results(
   const IndexType& idx,
   StartType start,
   std::vector<ResultType>& result,
   uint32_t limit,
   OnPushType&& on_push,
   FilterType&& filter,
   bool ascending = true )
{
   if( ascending )
   {
      auto itr = idx.lower_bound( start );
      auto end = idx.end();

      while( result.size() < limit && itr != end )
      {
         if( filter( *itr ) )
            result.push_back( on_push( *itr ) );

         ++itr;
      }
   }
   else
   {
      auto itr = boost::make_reverse_iterator( idx.lower_bound( start ) );
      auto end = idx.rend();

      while( result.size() < limit && itr != end )
      {
         if( filter( *itr ) )
            result.push_back( on_push( *itr ) );

         ++itr;
      }
   }
}

} } } } // steem::plugins::database_api::util
