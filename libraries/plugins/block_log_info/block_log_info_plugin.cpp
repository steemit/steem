#include <steem/plugins/block_log_info/block_log_info_plugin.hpp>
#include <steem/plugins/block_log_info/block_log_info_objects.hpp>

#include <steem/chain/account_object.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/global_property_object.hpp>
#include <steem/chain/index.hpp>
#include <steem/chain/operation_notification.hpp>

#include <fstream>
#include <iostream>
#include <sstream>

namespace steem { namespace plugins { namespace block_log_info {

namespace detail {

class block_log_info_plugin_impl
{
   public:
      block_log_info_plugin_impl( block_log_info_plugin& _plugin ) :
         _db( appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db() ),
         _self( _plugin ) {}

      void on_post_apply_block( const block_notification& note );
      void print_message( const block_log_message_data& data );

      database&                     _db;
      block_log_info_plugin&        _self;
      boost::signals2::connection   _post_apply_block_conn;
      int32_t                       print_interval_seconds = 0;
      bool                          print_irreversible = true;
      std::string                   output_name;
};

void block_log_info_plugin_impl::on_post_apply_block( const block_notification& note )
{
   const signed_block& b = note.block;
   uint32_t block_num = b.block_num();
   bool is_genesis = (block_num == 1);

   if( is_genesis )
   {
      _db.create< block_log_hash_state_object >( []( block_log_hash_state_object& bso )
      {
      } );
   }

   const block_log_hash_state_object& state = _db.get< block_log_hash_state_object, by_id >( block_log_hash_state_id_type(0) );
   uint64_t current_interval = state.last_interval;

   if( (print_interval_seconds > 0) && !is_genesis )
   {
      current_interval = b.timestamp.sec_since_epoch() / print_interval_seconds;
      if( current_interval != state.last_interval )
      {
         block_log_message_data data;
         data.block_num = block_num;
         data.total_size = state.total_size;
         data.current_interval = current_interval;
         data.rsha256 = state.rsha256;

         if( !print_irreversible )
         {
            print_message( data );
         }
         else
         {
            _db.create< block_log_pending_message_object >( [&data]( block_log_pending_message_object& msg )
            {
               msg.data = data;
            } );
         }
      }

      const dynamic_global_property_object& dgpo = _db.get_dynamic_global_properties();

      const auto& idx = _db.get_index< block_log_pending_message_index, by_id >();
      while( true )
      {
         auto it = idx.begin();
         if( it == idx.end() )
            break;
         if( it->data.block_num > dgpo.last_irreversible_block_num )
            break;
         print_message( it->data );
         _db.remove( *it );
      }
   }


   uint64_t offset = state.total_size;
   std::vector< char > data = fc::raw::pack_to_vector( b );
   for( int i=0; i<8; i++ )
   {
      data.push_back( (char) (offset & 0xFF) );
      offset >>= 8;
   }

   _db.modify( state, [&]( block_log_hash_state_object& bso )
   {
      bso.total_size += data.size();
      bso.rsha256.update( data.data(), data.size() );
      bso.last_interval = current_interval;
   } );
}

void block_log_info_plugin_impl::print_message( const block_log_message_data& data )
{
   std::stringstream ss;
   ss << "block_num=" << data.block_num << "   size=" << data.total_size << "   hash=" << data.rsha256.hexdigest();

   std::string msg = ss.str();

   if( output_name == "" )
      return;
   else if( (output_name == "STDOUT") || (output_name == "-") )
      std::cout << msg << std::endl;
   else if( output_name == "STDERR" )
      std::cerr << msg << std::endl;
   else if( output_name == "ILOG" )
      ilog( "${msg}", ("msg", msg) );
   else
   {
      std::ofstream out( output_name, std::ofstream::app );
      out << msg << std::endl;
      out.close();
   }
}

} // detail

block_log_info_plugin::block_log_info_plugin() {}
block_log_info_plugin::~block_log_info_plugin() {}

void block_log_info_plugin::set_program_options( options_description& cli, options_description& cfg )
{
   cfg.add_options()
         ("block-log-info-print-interval-seconds", boost::program_options::value< int32_t >()->default_value(60*60*24), "How often to print out block_log_info (default 1 day)")
         ("block-log-info-print-irreversible", boost::program_options::value< bool >()->default_value(true), "Whether to defer printing until block is irreversible")
         ("block-log-info-print-file", boost::program_options::value< string >()->default_value("ILOG"), "Where to print (filename or special sink ILOG, STDOUT, STDERR)")
         ;
}

void block_log_info_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   my = std::make_unique< detail::block_log_info_plugin_impl >( *this );
   try
   {
      ilog( "Initializing block_log_info plugin" );
      chain::database& db = appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db();

      my->_post_apply_block_conn = db.add_post_apply_block_handler(
         [&]( const block_notification& note ){ my->on_post_apply_block( note ); }, *this );

      add_plugin_index< block_log_hash_state_index >(db);
      add_plugin_index< block_log_pending_message_index >(db);

      my->print_interval_seconds = options.at( "block-log-info-print-interval-seconds" ).as< int32_t >();
      my->print_irreversible = options.at( "block-log-info-print-irreversible" ).as< bool >();
      my->output_name = options.at( "block-log-info-print-file" ).as< string >();

      if( my->print_interval_seconds <= 0 )
      {
         wlog( "print_interval_seconds set to value <= 0, if you don't need printing, consider disabling block_log_info_plugin entirely to improve performance" );
      }
   }
   FC_CAPTURE_AND_RETHROW()
}

void block_log_info_plugin::plugin_startup() {}

void block_log_info_plugin::plugin_shutdown()
{
   chain::util::disconnect_signal( my->_post_apply_block_conn );
}

} } } // steem::plugins::block_log_info
