#pragma once

#ifdef STEEM_ENABLE_SMT

#include <cstdint>
#include <utility>
#include <vector>

#include <fc/reflect/reflect.hpp>

namespace steem { namespace chain {

struct rational_u64
{
   rational_u64() {}
   rational_u64( const std::pair< uint64_t, uint64_t >& p )
      : numerator(p.first), denominator(p.second) {}

   uint64_t numerator = 0;
   uint64_t denominator = 0;
};

const std::vector< rational_u64 >& get_mm_ticks();

} }

FC_REFLECT( steem::chain::rational_u64,
   (numerator)
   (denominator)
   )
#endif
