
#define BOOST_THREAD_PROVIDES_EXECUTORS
#define BOOST_THREAD_PROVIDES_FUTURE

#include <steem/plugins/block_data_export/block_data_export_plugin.hpp>
#include <steem/plugins/block_data_export/exportable_block_data.hpp>

#include <steem/chain/account_object.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/global_property_object.hpp>
#include <steem/chain/index.hpp>

#include <boost/thread/future.hpp>
#include <boost/thread/sync_bounded_queue.hpp>

#include <fstream>
#include <iostream>
#include <queue>
#include <sstream>

namespace steem { namespace plugins { namespace block_data_export {

using steem::chain::block_notification;
using steem::chain::database;

using steem::protocol::block_id_type;

namespace detail {

struct api_export_data_object
{
   block_id_type                    block_id;
   block_id_type                    previous;
   flat_map< string, std::shared_ptr< exportable_block_data > >
                                    export_data;

   void clear()
   {
      block_id = block_id_type();
      previous = block_id_type();
      export_data.clear();
   }
};

} } } }

FC_REFLECT( steem::plugins::block_data_export::detail::api_export_data_object, (block_id)(previous)(export_data) )

namespace steem { namespace plugins { namespace block_data_export { namespace detail {

struct work_item
{
   std::shared_ptr< api_export_data_object >          edo;
   boost::promise< std::shared_ptr< std::string > >   edo_json_promise;
   boost::future< std::shared_ptr< std::string > >    edo_json_future = edo_json_promise.get_future();
};

class block_data_export_plugin_impl
{
   public:
      block_data_export_plugin_impl( block_data_export_plugin& _plugin ) :
         _db( appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db() ),
         _self( _plugin ),
         _data_queue( _max_queue_size ),
         _output_queue( _max_queue_size ) {}

      void on_pre_apply_block( const block_notification& note );
      void on_post_apply_block( const block_notification& note );

      void register_export_data_factory( const std::string& name, std::function< std::shared_ptr< exportable_block_data >() >& factory );
      void create_export_data( const block_id_type& previous, const block_id_type& block_id );
      void send_export_data();
      std::shared_ptr< exportable_block_data > find_abstract_export_data( const std::string& name );

      void start_threads();
      void stop_threads();
      void convert_to_json_thread_main();
      void output_thread_main();

      database&                     _db;
      block_data_export_plugin&     _self;
      boost::signals2::connection   _pre_apply_block_conn;
      boost::signals2::connection   _post_apply_block_conn;
      std::shared_ptr< api_export_data_object >
                                    _edo;
      std::vector< std::pair<
         string,
         std::function< std::shared_ptr< exportable_block_data >() >
         > >                        _factory_list;
      std::string                   _output_name;
      bool                          _enabled = false;

      size_t                        _max_queue_size = 100;
      boost::concurrent::sync_bounded_queue< std::shared_ptr< work_item > >    _data_queue;
      boost::concurrent::sync_bounded_queue< std::shared_ptr< work_item > >    _output_queue;

      size_t                        _thread_stack_size = 4096*1024;
      std::shared_ptr< boost::thread >                      _output_thread;

      std::vector< boost::thread >  _json_conversion_threads;
};

void block_data_export_plugin_impl::start_threads()
{
   boost::thread::attributes attrs;
   attrs.set_stack_size( _thread_stack_size );

   size_t num_threads = boost::thread::hardware_concurrency()+1;
   for( size_t i=0; i<num_threads; i++ )
   {
      _json_conversion_threads.emplace_back( attrs, [this]() { convert_to_json_thread_main(); } );
   }

   _output_thread = std::make_shared< boost::thread >( attrs, [this]() { output_thread_main(); } );
}

void block_data_export_plugin_impl::stop_threads()
{
   //
   // We must close the output queue first:  The output queue may be waiting on a future.
   // If the conversion threads are still alive, the future will complete,
   // the output thread will then wait on _output_queue and see close() has been called.
   //
   // (If we closed the conversion threads first, the future would never complete,
   // and the output thread would wait forever.)
   //
   _output_queue.close();
   _output_thread->join();
   _output_thread.reset();

   _data_queue.close();
   for( boost::thread& t : _json_conversion_threads )
      t.join();
   _json_conversion_threads.clear();
}

void block_data_export_plugin_impl::convert_to_json_thread_main()
{
   while( true )
   {
      std::shared_ptr< work_item > work;
      try
      {
         _data_queue.pull_front( work );
      }
      catch( const boost::concurrent::sync_queue_is_closed& e )
      {
         break;
      }

      // TODO exception handling
      std::shared_ptr< std::string > edo_json = std::make_shared< std::string >( fc::json::to_string( work->edo ) );
      work->edo_json_promise.set_value( edo_json );
   }
}

void block_data_export_plugin_impl::output_thread_main()
{
   std::ofstream output_file( _output_name, std::ios::binary );
   while( true )
   {
      std::shared_ptr< work_item > work;
      try
      {
         _output_queue.pull_front( work );
      }
      catch( const boost::concurrent::sync_queue_is_closed& e )
      {
         break;
      }

      std::shared_ptr< std::string > edo_json = work->edo_json_future.get();

      output_file.write( edo_json->c_str(), edo_json->length() );
      output_file.put( '\n' );
      output_file.flush();
   }
}

void block_data_export_plugin_impl::register_export_data_factory(
   const std::string& name,
   std::function< std::shared_ptr< exportable_block_data >() >& factory
   )
{
   _factory_list.emplace_back( name, factory );
}

void block_data_export_plugin_impl::create_export_data( const block_id_type& previous, const block_id_type& block_id )
{
   _edo.reset();
   if( !_enabled )
      return;
   _edo = std::make_shared< api_export_data_object >();

   _edo->block_id = block_id;
   _edo->previous = previous;
   for( const auto& fact : _factory_list )
   {
      _edo->export_data.emplace( fact.first, fact.second() );
   }
}

void block_data_export_plugin_impl::send_export_data()
{
   std::shared_ptr< work_item > work = std::make_shared< work_item >();
   work->edo = _edo;
   _edo.reset();

   try
   {
      _data_queue.push_back( work );
      _output_queue.push_back( work );
   }
   catch( const boost::concurrent::sync_queue_is_closed& e )
   {
      // We should never see this exception because we should be done handling blocks
      // by the time we're closing queues
      elog( "Caught unexpected sync_queue_is_closed in block_data_export_plugin_impl::push_work()" );
   }
   return;
}

std::shared_ptr< exportable_block_data > block_data_export_plugin_impl::find_abstract_export_data( const std::string& name )
{
   std::shared_ptr< exportable_block_data > result;
   if( !_edo )
      return result;
   auto it = _edo->export_data.find(name);
   if( it != _edo->export_data.end() )
      result = it->second;
   return result;
}

void block_data_export_plugin_impl::on_pre_apply_block( const block_notification& note )
{
   create_export_data( note.block.previous, note.block_id );
}

void block_data_export_plugin_impl::on_post_apply_block( const block_notification& note )
{
   send_export_data();
}

} // detail

block_data_export_plugin::block_data_export_plugin() {}
block_data_export_plugin::~block_data_export_plugin() {}

void block_data_export_plugin::register_export_data_factory(
   const std::string& name,
   std::function< std::shared_ptr< exportable_block_data >() >& factory )
{
   my->register_export_data_factory( name, factory );
}

std::shared_ptr< exportable_block_data > block_data_export_plugin::find_abstract_export_data( const std::string& name )
{
   return my->find_abstract_export_data( name );
}


void block_data_export_plugin::set_program_options( options_description& cli, options_description& cfg )
{
   cfg.add_options()
         ("block-data-export-file", boost::program_options::value< string >()->default_value("NONE"), "Where to export data (NONE to discard)")
         ;
}

void block_data_export_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   my = std::make_unique< detail::block_data_export_plugin_impl >( *this );
   try
   {
      ilog( "Initializing block_data_export plugin" );

      my->_output_name = options.at( "block-data-export-file" ).as< string >();
      my->_enabled = (my->_output_name != "NONE");
      if( !my->_enabled )
         return;

      my->_pre_apply_block_conn = my->_db.add_pre_apply_block_handler(
         [&]( const block_notification& note ){ my->on_pre_apply_block( note ); }, *this, -9300 );
      my->_post_apply_block_conn = my->_db.add_post_apply_block_handler(
         [&]( const block_notification& note ){ my->on_post_apply_block( note ); }, *this, 9300 );

      my->start_threads();
   }
   FC_CAPTURE_AND_RETHROW()
}

void block_data_export_plugin::plugin_startup() {}

void block_data_export_plugin::plugin_shutdown()
{
   if( !my->_enabled )
      return;

   chain::util::disconnect_signal( my->_pre_apply_block_conn );
   chain::util::disconnect_signal( my->_post_apply_block_conn );

   my->stop_threads();
}

exportable_block_data::exportable_block_data() {}
exportable_block_data::~exportable_block_data() {}

} } } // steem::plugins::block_data_export
