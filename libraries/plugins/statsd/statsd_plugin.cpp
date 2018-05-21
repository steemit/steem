#include <steem/plugins/statsd/statsd_plugin.hpp>

#include <fc/network/resolve.hpp>

#include <boost/algorithm/string.hpp>

#include <sstream>

#include "StatsdClient.hpp"

namespace steem { namespace plugins { namespace statsd {

using namespace Statsd;

namespace detail
{
   inline std::string compose_key( const std::string& ns, const std::string& stat, const std::string& key )
   {
      std::stringstream ss;
      ss << ns << '.' << stat << '.' << key;
      return ss.str();
   }

   class statsd_plugin_impl
   {
      public:
         statsd_plugin_impl() {}
         ~statsd_plugin_impl() {}

         void start();
         void shutdown();

         bool filter_by_namespace( const std::string& ns, const std::string& stat ) const;

         void increment( const std::string& ns, const std::string& stat, const std::string& key,                       const float frequency ) const noexcept;
         void decrement( const std::string& ns, const std::string& stat, const std::string& key,                       const float frequency ) const noexcept;
         void count(     const std::string& ns, const std::string& stat, const std::string& key, const int64_t delta,  const float frequency ) const noexcept;
         void gauge(     const std::string& ns, const std::string& stat, const std::string& key, const uint64_t value, const float frequency ) const noexcept;
         void timing(    const std::string& ns, const std::string& stat, const std::string& key, const uint32_t ms,    const float frequency ) const noexcept;


         bool                                               _filter_stats = false;
         bool                                               _blacklist    = false;
         bool                                               _started      = false;

         std::set< std::string >                            _stat_namespaces;
         std::map< std::string, std::set< std::string > >   _stat_list;

         fc::optional< fc::ip::endpoint >                   _statsd_endpoint;
         uint32_t                                           _statsd_batchsize = 1;

         std::unique_ptr< StatsdClient >                    _statsd;
   };

   void statsd_plugin_impl::start()
   {
      if( _started )
      {
         return;
      }

      std::string host;
      uint32_t port = 0;

      if( _statsd_endpoint.valid() )
      {
         host = std::string( _statsd_endpoint->get_address() );
         port = _statsd_endpoint->port();
      }

      _statsd.reset( new StatsdClient( host, port, "steemd.", _statsd_batchsize ) );
      _started = true;
   }

   void statsd_plugin_impl::shutdown()
   {
      _statsd.reset();
   }

   bool statsd_plugin_impl::filter_by_namespace( const std::string& ns, const std::string& stat ) const
   {
      if( !_filter_stats )
      {
         return true;
      }

      bool found = false;

      if( _stat_namespaces.find( ns ) != _stat_namespaces.end() )
      {
         found = true;
      }

      if( !found )
      {
         auto itr = _stat_list.find( ns );
         if( itr != _stat_list.end() )
         {
            if( itr->second.find( stat ) != itr->second.end() )
            {
               found = true;
            }
         }
      }

      return _blacklist != found;
   }

   void statsd_plugin_impl::increment( const std::string& ns, const std::string& stat, const std::string& key, const float frequency ) const noexcept
   {
      if( !filter_by_namespace( ns, stat ) ) return;
      _statsd->increment( compose_key( ns, stat, key ), frequency );
   }

   void statsd_plugin_impl::decrement( const std::string& ns, const std::string& stat, const std::string& key, const float frequency ) const noexcept
   {
      if( !filter_by_namespace( ns, stat ) ) return;
      _statsd->decrement( compose_key( ns, stat, key ), frequency );
   }

   void statsd_plugin_impl::count( const std::string& ns, const std::string& stat, const std::string& key, const int64_t delta, const float frequency ) const noexcept
   {
      if( !filter_by_namespace( ns, stat ) ) return;
      _statsd->count( compose_key( ns, stat, key ), delta, frequency );
   }

   void statsd_plugin_impl::gauge( const std::string& ns, const std::string& stat, const std::string& key, const uint64_t value, const float frequency ) const noexcept
   {
      if( !filter_by_namespace( ns, stat ) ) return;
      _statsd->gauge( compose_key( ns, stat, key ), value, frequency );
   }

   void statsd_plugin_impl::timing( const std::string& ns, const std::string& stat, const std::string& key, const uint32_t ms, const float frequency ) const noexcept
   {
      if( !filter_by_namespace( ns, stat ) ) return;
      _statsd->timing( compose_key( ns, stat, key ), ms, frequency );
   }
}

statsd_plugin::statsd_plugin() : my( new detail::statsd_plugin_impl() ) {}
statsd_plugin::~statsd_plugin() {}

void statsd_plugin::set_program_options( options_description& cli, options_description& cfg )
{
   cli.add_options()
      ("statsd-record-on-replay", bpo::bool_switch()->default_value(false), "Records statsd events during replay");

   cfg.add_options()
      ("statsd-endpoint", bpo::value< std::string >(), "Endpoint to send statsd messages to.")
      ("statsd-batchsize", bpo::value< uint32_t >()->default_value( 1 ), "Size to batch statsd messages." )
      ("statsd-whitelist", bpo::value< vector< std::string > >()->composing(), "Whitelist of statistics to capture.")
      ("statsd-blacklist", bpo::value< vector< std::string > >()->composing(), "Blacklist of statistics to capture.");
}

void statsd_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   if( options.count( "statsd-endpoint" ) )
   {
      auto statsd_endpoint = options.at( "statsd-endpoint" ).as< string >();
      auto endpoints = fc::resolve_string_to_ip_endpoints( statsd_endpoint );
      FC_ASSERT( endpoints.size(), "statsd-endpoint ${hostname} did not resolve.", ("hostname", statsd_endpoint) );
      my->_statsd_endpoint = endpoints[0];
      ilog( "Configured statsd to send to ${ep}", ("ep", endpoints[0]) );
   }

   if( options.count( "statsd-whitelist" ) )
   {
      my->_filter_stats = true;
      my->_blacklist = false;

      // Stats are in the format namespace.stat
      for( auto& arg : options.at( "statsd-whitelist" ).as< vector< string > >() )
      {
         vector< string > stats;
         boost::split( stats, arg, boost::is_any_of( " \t" ) );

         for( const string& stat : stats )
         {
            if( stat.size() )
            {
               auto dot_pos = stat.find_first_of( '.' );
               if( dot_pos == string::npos )
               {
                  my->_stat_namespaces.insert( stat );
               }
               else
               {
                  my->_stat_list[ stat.substr( 0, dot_pos ) ].insert( stat.substr( dot_pos + 1 ) );
               }
            }
         }
      }
   }
   else if( options.count( "statsd-blacklist" ) )
   {
      my->_filter_stats = true;
      my->_blacklist = true;

      for( auto& arg : options.at( "statsd-blacklist" ).as< vector< string > >() )
      {
         vector< string > stats;
         boost::split( stats, arg, boost::is_any_of( " \t" ) );

         for( const string& stat : stats )
         {
            if( stat.size() )
            {
               auto dot_pos = stat.find_first_of( '.' );
               if( dot_pos == string::npos )
               {
                  my->_stat_namespaces.insert( stat );
               }
               else
               {
                  my->_stat_list[ stat.substr( 0, dot_pos ) ].insert( stat.substr( dot_pos + 1 ) );
               }
            }
         }
      }
   }
}

void statsd_plugin::plugin_startup()
{
   start_logging();
}

void statsd_plugin::plugin_shutdown()
{
   my->shutdown();
}

void statsd_plugin::start_logging()
{
   my->start();
}

void statsd_plugin::increment( const std::string& ns, const std::string& stat, const std::string& key, const float frequency ) const noexcept
{
   my->increment( ns, stat, key, frequency );
}

void statsd_plugin::decrement( const std::string& ns, const std::string& stat, const std::string& key, const float frequency ) const noexcept
{
   my->decrement( ns, stat, key, frequency );
}

void statsd_plugin::count( const std::string& ns, const std::string& stat, const std::string& key, const int64_t delta, const float frequency ) const noexcept
{
   my->count( ns, stat, key, delta, frequency );
}

void statsd_plugin::gauge( const std::string& ns, const std::string& stat, const std::string& key, const uint64_t value, const float frequency ) const noexcept
{
   my->gauge( ns, stat, key, value, frequency );
}

void statsd_plugin::timing( const std::string& ns, const std::string& stat, const std::string& key, const uint32_t ms, const float frequency ) const noexcept
{
   my->timing( ns, stat, key, ms, frequency );
}

} } } // steem::plugins::statsd
