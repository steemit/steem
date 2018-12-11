#pragma once

#include <boost/multi_index/detail/value_compare.hpp>

#include <rocksdb/slice.h>

#include <fc/io/raw.hpp>

namespace mira { namespace multi_index { namespace detail {

template< typename Value, typename KeyFromValue, typename Compare >
struct slice_comparator : boost::multi_index::detail::value_comparison< Value, KeyFromValue, Compare >
{
   slice_comparator( const KeyFromValue& key_=KeyFromValue(), const Compare& comp_=Compare() )
      : boost::multi_index::detail::value_comparison< Value, KeyFromValue, Compare >( key_, comp_)
   {}

   bool operator()( ::rocksdb::Slice x, ::rocksdb::Slice y )const
   {
      return (*this)(
         std::move( fc::raw::unpack_from_char_array< Value >( x.data(), x.size() ) ),
         std::move( fc::raw::unpack_from_char_array< Value >( y.data(), y.size() ) )
      );
   }
};

// TODO: Primitive Types can implement custom comparators that understand the independent format

} } } // mira::multi_index::detail
