#pragma once
#include <steem/plugins/statsd/statsd_plugin.hpp>

#include <fc/optional.hpp>
#include <fc/time.hpp>

namespace steem { namespace plugins { namespace statsd { namespace util {

using steem::plugins::statsd::statsd_plugin;

bool statsd_enabled()
{
   static bool enabled = appbase::app().find_plugin< statsd_plugin >() != nullptr;
   return enabled;
}

const statsd_plugin& get_statsd()
{
   static const statsd_plugin& statsd = appbase::app().get_plugin< statsd_plugin >();
   return statsd;
}

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

} } } } // steem::plugins::statsd::util

#define STATSD_INCREMENT( NAMESPACE, STAT, KEY, FREQ )   \
if( steem::plugins::statsd::util::statsd_enabled() )     \
{                                                        \
   steem::plugins::statsd::util::get_statsd().increment( \
      #NAMESPACE, #STAT, #KEY, FREQ                      \
   );                                                    \
}

#define STATSD_DECREMENT( NAMESPACE, STAT, KEY, FREQ )   \
if( steem::plugins::statsd::util::statsd_enabled() )     \
{                                                        \
   steem::plugins::statsd::util::get_statsd().decrement( \
      #NAMESPACE, #STAT, #KEY, FREQ                      \
   );                                                    \
}

#define STATSD_COUNT( NAMESPACE, STAT, KEY, VAL, FREQ )  \
if( steem::plugins::statsd::util::statsd_enabled() )     \
{                                                        \
   steem::plugins::statsd::util::get_statsd().count(     \
      #NAMESPACE, #STAT, #KEY, VAL, FREQ                 \
   );                                                    \
}

#define STATSD_GAUGE( NAMESPACE, STAT, KEY, VAL, FREQ )  \
if( steem::plugins::statsd::util::statsd_enabled() )     \
{                                                        \
   steem::plugins::statsd::util::get_statsd().gauge(     \
      #NAMESPACE, #STAT, #KEY, VAL, FREQ                 \
   );                                                    \
}

#define STATSD_START_TIMER( NAMESPACE, STAT, KEY, FREQ )                                              \
fc::optional< steem::plugins::statsd::util::statsd_timer_helper > NAMESPACE ## STAT ## KEY ## _timer; \
if( steem::plugins::statsd::util::statsd_enabled() )                                                  \
{                                                                                                     \
   NAMESPACE ## STAT ## KEY ## _timer = steem::plugins::statsd::util::statsd_timer_helper(            \
      #NAMESPACE, #STAT, #KEY, FREQ, steem::plugins::statsd::util::get_statsd()                       \
   );                                                                                                 \
}

#define STATSD_STOP_TIMER( NAMESPACE, STAT, KEY )        \
   NAMESPACE ## STAT ## KEY ## _timer.reset();           \
