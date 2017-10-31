#pragma once

#include <boost/interprocess/containers/flat_map.hpp>
#include <fc/static_variant.hpp>

#include <chainbase/chainbase.hpp>
#include <steem/chain/database.hpp>
#include <steem/protocol/asset_symbol.hpp>

#include <steem/chain/util/scheduler_event.hpp>

namespace steem { namespace chain {

class database;

namespace util {

using steem::protocol::asset_symbol_type;
using fc::time_point_sec;

class timed_event_scheduler_impl;

class timed_event_scheduler
{
   private:

      std::unique_ptr< timed_event_scheduler_impl > impl;

   public:

      timed_event_scheduler( database& _db );
      ~timed_event_scheduler();

      void close();
      void add( const time_point_sec& key, const timed_event_object& value, bool buffered = false );
      void run( const time_point_sec& head_block_time );

#ifdef IS_TEST_NET
      //for test purposes?
      size_t size();
      size_t size( const time_point_sec& head_block_time );
#endif
};

} } }
