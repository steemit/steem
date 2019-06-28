#pragma once
#include <steem/plugins/statsd/statsd_plugin.hpp>

#include <fc/optional.hpp>
#include <fc/time.hpp>

namespace steem { namespace plugins { namespace statsd { namespace util {

using steem::plugins::statsd::statsd_plugin;

bool statsd_enabled();
const statsd_plugin& get_statsd();

class statsd_timer_helper
{
   public:
      statsd_timer_helper( const std::string& ns, const std::string& stat, const std::string& key, float freq, const statsd_plugin& statsd ) :
         _ns( ns ),
         _stat( stat ),
         _key( key ),
         _freq( freq ),
         _statsd( statsd )
      {
         _start = fc::time_point::now();
      }

      ~statsd_timer_helper()
      {
         record();
      }

      void record()
      {
         fc::time_point stop = fc::time_point::now();
         if( !_recorded )
         {
            _statsd.timing( _ns, _stat, _key, (stop - _start).count() / 1000 , _freq );
            _recorded = true;
         }
      }

   private:
      std::string          _ns;
      std::string          _stat;
      std::string          _key;
      float                _freq = 1.0f;
      fc::time_point       _start;
      const statsd_plugin& _statsd;
      bool                 _recorded = false;
};

inline uint32_t timing_helper( const fc::microseconds& time ) { return time.count() / 1000; }
inline uint32_t timing_helper( const fc::time_point& time ) { return time.time_since_epoch().count() / 1000; }
inline uint32_t timing_helper( const fc::time_point_sec& time ) { return time.sec_since_epoch() * 1000; }
inline uint32_t timing_helper( uint32_t time ) { return time; }

} } } } // steem::plugins::statsd::util

#define STATSD_INCREMENT( NAMESPACE, STAT, KEY, FREQ )   \
if( steem::plugins::statsd::util::statsd_enabled() )     \
{                                                        \
   steem::plugins::statsd::util::get_statsd().increment( \
      NAMESPACE, STAT, KEY, FREQ                         \
   );                                                    \
}

#define STATSD_DECREMENT( NAMESPACE, STAT, KEY, FREQ )   \
if( steem::plugins::statsd::util::statsd_enabled() )     \
{                                                        \
   steem::plugins::statsd::util::get_statsd().decrement( \
      NAMESPACE, STAT, KEY, FREQ                         \
   );                                                    \
}

#define STATSD_COUNT( NAMESPACE, STAT, KEY, VAL, FREQ )  \
if( steem::plugins::statsd::util::statsd_enabled() )     \
{                                                        \
   steem::plugins::statsd::util::get_statsd().count(     \
      NAMESPACE, STAT, KEY, VAL, FREQ                    \
   );                                                    \
}

#define STATSD_GAUGE( NAMESPACE, STAT, KEY, VAL, FREQ )  \
if( steem::plugins::statsd::util::statsd_enabled() )     \
{                                                        \
   steem::plugins::statsd::util::get_statsd().gauge(     \
      NAMESPACE, STAT, KEY, VAL, FREQ                    \
   );                                                    \
}

// You can only have one statsd timer in the current scope at a time
#define STATSD_START_TIMER( NAMESPACE, STAT, KEY, FREQ )                         \
fc::optional< steem::plugins::statsd::util::statsd_timer_helper > statsd_timer;  \
if( steem::plugins::statsd::util::statsd_enabled() )                             \
{                                                                                \
   statsd_timer = steem::plugins::statsd::util::statsd_timer_helper(             \
      NAMESPACE, STAT, KEY, FREQ, steem::plugins::statsd::util::get_statsd()     \
   );                                                                            \
}

#define STATSD_STOP_TIMER( NAMESPACE, STAT, KEY )        \
   statsd_timer.reset();

#define STATSD_TIMER( NAMESPACE, STAT, KEY, VAL, FREQ )  \
if( steem::plugins::statsd::util::statsd_enabled() )     \
{                                                        \
   steem::plugins::statsd::util::get_statsd().timing(    \
      NAMESPACE, STAT, KEY,                              \
      steem::plugins::statsd::util::timing_helper( VAL ),\
      FREQ                                               \
   );                                                    \
}
