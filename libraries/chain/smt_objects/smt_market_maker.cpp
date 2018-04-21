
#ifdef STEEM_ENABLE_SMT

#include <steem/jsonball/jsonball.hpp>
#include <steem/chain/smt_objects/smt_market_maker.hpp>

#include <fc/variant.hpp>
#include <fc/io/json.hpp>

#include <fc/variant.hpp>
#include <fc/io/raw.hpp>
#include <fc/reflect/variant.hpp>

#include <string>
#include <vector>

namespace steem { namespace chain {

static std::vector< rational_u64 > load_mm_ticks()
{
   std::string mm_ticks_json = steem::jsonball::get_market_maker_ticks();
   fc::variant mm_ticks_var = fc::json::from_string( mm_ticks_json, fc::json::strict_parser );
   std::vector< std::pair< uint64_t, uint64_t > > mm_ticks_pairs;
   fc::from_variant( mm_ticks_var, mm_ticks_pairs );
   std::vector< rational_u64 > mm_ticks;
   for( const std::pair< uint64_t, uint64_t >& p : mm_ticks_pairs )
      mm_ticks.emplace_back( p );
   return mm_ticks;
}

const std::vector< rational_u64 >& get_mm_ticks()
{
   //
   // In C++11, static local variable initialization is guaranteed to happen exactly once,
   // in a thread-safe way.  So we use such an initialization to do the heavy work of
   // converting the jsonball saved during compilation to an object.
   //
   static std::vector< rational_u64 > mm_ticks = load_mm_ticks();
   return mm_ticks;
}

} }

#endif
