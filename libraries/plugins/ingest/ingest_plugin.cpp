
#include <steem/chain/steem_fwd.hpp>

#include <steem/plugins/ingest/ingest_plugin.hpp>

#include <steem/chain/account_object.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/index.hpp>
#include <steem/chain/notifications.hpp>

#include <steem/protocol/operations.hpp>
#include <steem/protocol/operation_util_impl.hpp>

#include <fc/io/json.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <boost/filesystem.hpp>
#include <appbase/application.hpp>

namespace steem { namespace plugins { namespace ingest {

using namespace steem::chain;

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;
namespace bfs = boost::filesystem;

namespace detail {

class ingest_plugin_impl
{
   public:
      ingest_plugin_impl( ingest_plugin& _plugin );
      ~ingest_plugin_impl();

      void on_post_apply_operation( const operation_notification& note );

      // HTTP sending
      void send_operation_json( const fc::variant& json );
      void http_send_worker();
      void send_http_post( const std::string& json_body );

      // File writing (for dry run)
      void write_operation_json( const std::string& json_str );
      void initialize_output_file();

      // JSON building
      fc::variant build_operation_json( const operation_notification& note );

      ingest_plugin& _plugin;
      database& _db;
      
      // Signal connection
      boost::signals2::connection _post_apply_operation_conn;
      
      // HTTP configuration
      std::string _ingest_endpoint;
      uint32_t _http_timeout_ms;
      uint32_t _max_queue_size;
      
      // Dry run configuration
      bool _dry_run;
      bfs::path _output_file_path;
      std::ofstream _output_file;
      std::mutex _file_mutex;
      
      // Async send queue
      std::queue< std::string > _send_queue;
      std::mutex _queue_mutex;
      std::condition_variable _queue_cv;
      std::thread _http_thread;
      bool _shutdown;
};

ingest_plugin_impl::ingest_plugin_impl( ingest_plugin& _plugin ) :
   _plugin( _plugin ),
   _db( appbase::app().get_plugin< chain::chain_plugin >().db() ),
   _ingest_endpoint( "http://localhost:8080/ingest/applied_op" ),
   _http_timeout_ms( 5000 ),
   _max_queue_size( 100000 ),
   _dry_run( false ),
   _shutdown( false )
{
}

ingest_plugin_impl::~ingest_plugin_impl()
{
   if( _http_thread.joinable() )
   {
      {
         std::lock_guard< std::mutex > lock( _queue_mutex );
         _shutdown = true;
      }
      _queue_cv.notify_one();
      _http_thread.join();
   }
   
   // Close output file if open
   if( _output_file.is_open() )
   {
      _output_file.close();
   }
}

void ingest_plugin_impl::on_post_apply_operation( const operation_notification& note )
{
   try
   {
      // Build Operation JSON
      fc::variant json = build_operation_json( note );
      
      // Async send
      send_operation_json( json );
   }
   catch( const fc::exception& e )
   {
      elog( "Error processing operation: ${e}", ("e", e.to_string()) );
   }
   catch( const std::exception& e )
   {
      elog( "Error processing operation: ${e}", ("e", e.what()) );
   }
}

fc::variant ingest_plugin_impl::build_operation_json( const operation_notification& note )
{
   fc::mutable_variant_object result;
   
   // Get current block
   auto block = _db.fetch_block_by_number( note.block );
   if( !block.valid() )
   {
      FC_THROW( "Block ${b} not found", ("b", note.block) );
   }
   
   // 1. block object
   fc::mutable_variant_object block_obj;
   block_obj["num"] = note.block;
   block_obj["id"] = block->id().str();
   block_obj["timestamp"] = block->timestamp.to_iso_string();
   result["block"] = block_obj;
   
   // 2. transaction object
   fc::mutable_variant_object trx_obj;
   
   // Check if virtual operation
   bool is_virtual = steem::protocol::is_virtual_operation( note.op );
   
   if( is_virtual )
   {
      // virtual operation: transaction info is null
      trx_obj["id"] = fc::variant();
      trx_obj["index"] = -1;
   }
   else
   {
      // real operation: get transaction info from note
      trx_obj["id"] = note.trx_id.str();
      trx_obj["index"] = static_cast<int32_t>( note.trx_in_block );
   }
   result["transaction"] = trx_obj;
   
   // 3. operation object
   fc::mutable_variant_object op_obj;
   
   // operation index (in transaction)
   if( is_virtual )
   {
      op_obj["index"] = static_cast<int32_t>( note.virtual_op );
   }
   else
   {
      op_obj["index"] = static_cast<int32_t>( note.op_in_trx );
   }
   
   // operation type (use FC visitor to get name)
   std::string op_name;
   fc::get_operation_name get_name_visitor( op_name );
   note.op.visit( get_name_visitor );
   op_obj["type"] = op_name;
   
   // operation value (serialize operation to JSON)
   // Use from_operation visitor to get pair<name, value>, then extract value
   fc::variant op_variant;
   fc::from_operation from_op_visitor( op_variant );
   note.op.visit( from_op_visitor );
   
   // from_operation creates variant(std::make_pair(name, value))
   // In FC, a pair is serialized as an array [name, value]
   // We need to extract the value part (second element)
   if( op_variant.is_array() && op_variant.size() == 2 )
   {
      op_obj["value"] = op_variant[1];  // Get the value part (second element)
   }
   else if( op_variant.is_object() )
   {
      // If it's an object, try to extract value field
      auto op_obj_map = op_variant.get_object();
      if( op_obj_map.contains( "value" ) )
      {
         op_obj["value"] = op_obj_map["value"];
      }
      else
      {
         // Use the whole variant as fallback
         op_obj["value"] = op_variant;
      }
   }
   else
   {
      // Fallback: use the whole variant
      op_obj["value"] = op_variant;
   }
   
   result["operation"] = op_obj;
   
   // 4. virtual marker
   result["virtual"] = is_virtual;
   
   return fc::variant( result );
}

void ingest_plugin_impl::send_operation_json( const fc::variant& json )
{
   // Serialize JSON
   std::string json_str = fc::json::to_string( json );
   
   if( _dry_run )
   {
      // Dry run mode: write to file instead of sending HTTP
      write_operation_json( json_str );
   }
   else
   {
      // Normal mode: send via HTTP
      // Check queue size
      {
         std::lock_guard< std::mutex > lock( _queue_mutex );
         if( _send_queue.size() >= _max_queue_size )
         {
            wlog( "Ingest queue full, dropping operation" );
            return;  // Drop if queue is full (avoid blocking steemd)
         }
         _send_queue.push( json_str );
      }
      _queue_cv.notify_one();
   }
}

void ingest_plugin_impl::http_send_worker()
{
   while( !_shutdown )
   {
      std::string json_str;
      
      // Get data from queue
      {
         std::unique_lock< std::mutex > lock( _queue_mutex );
         _queue_cv.wait( lock, [this] { return !_send_queue.empty() || _shutdown; } );
         
         if( _shutdown && _send_queue.empty() ) break;
         
         json_str = _send_queue.front();
         _send_queue.pop();
      }
      
      // Send HTTP POST
      try
      {
         send_http_post( json_str );
      }
      catch( const std::exception& e )
      {
         elog( "HTTP send error: ${e}", ("e", e.what()) );
         // Don't retry, avoid blocking
      }
   }
}

void ingest_plugin_impl::send_http_post( const std::string& json_body )
{
   // Parse URL (simplified, assume format: http://host:port/path)
   std::string url = _ingest_endpoint;
   std::string protocol = "http";
   std::string host;
   std::string port = "8080";
   std::string target = "/ingest/applied_op";
   
   // Simple URL parsing
   if( url.find( "http://" ) == 0 )
   {
      url = url.substr( 7 );
   }
   else if( url.find( "https://" ) == 0 )
   {
      protocol = "https";
      url = url.substr( 8 );
   }
   
   size_t port_pos = url.find( ':' );
   size_t path_pos = url.find( '/' );
   
   if( path_pos != std::string::npos )
   {
      target = url.substr( path_pos );
      url = url.substr( 0, path_pos );
   }
   
   if( port_pos != std::string::npos )
   {
      host = url.substr( 0, port_pos );
      port = url.substr( port_pos + 1 );
   }
   else
   {
      host = url;
   }
   
   // Create I/O context
   net::io_context ioc;
   tcp::resolver resolver( ioc );
   tcp::socket socket( ioc );
   
   // Resolve address
   auto const results = resolver.resolve( host, port );
   net::connect( socket, results.begin(), results.end() );
   
   // Build HTTP request
   http::request< http::string_body > req;
   req.method( http::verb::post );
   req.target( target );
   req.set( http::field::host, host + ":" + port );
   req.set( http::field::content_type, "application/json" );
   req.set( http::field::user_agent, "steemd-ingest-plugin/1.0" );
   req.body() = json_body;
   req.prepare_payload();
   
   // Send request
   http::write( socket, req );
   
   // Read response (simplified handling)
   beast::flat_buffer buffer;
   http::response< http::string_body > res;
   http::read( socket, buffer, res );
   
   // Check status code
   if( res.result() != http::status::ok )
   {
      elog( "HTTP error: ${code}", ("code", res.result_int()) );
   }
   
   // Close connection
   beast::error_code ec;
   socket.shutdown( tcp::socket::shutdown_both, ec );
}

void ingest_plugin_impl::initialize_output_file()
{
   // Skip if already initialized
   if( _output_file.is_open() )
   {
      return;
   }
   
   try
   {
      // Get data directory
      bfs::path data_dir = appbase::app().data_dir();
      bfs::path ingest_dir = data_dir / "ingest";
      
      // Create ingest directory if it doesn't exist
      if( !bfs::exists( ingest_dir ) )
      {
         bfs::create_directories( ingest_dir );
         ilog( "Created ingest directory: ${dir}", ("dir", ingest_dir.string()) );
      }
      
      // Generate unique filename with timestamp
      auto now = std::chrono::system_clock::now();
      auto time_t = std::chrono::system_clock::to_time_t( now );
      auto ms = std::chrono::duration_cast< std::chrono::milliseconds >(
         now.time_since_epoch() ) % 1000;
      
      std::stringstream ss;
      ss << "ingest_" 
         << std::put_time( std::localtime( &time_t ), "%Y%m%d_%H%M%S" )
         << "_" << std::setfill( '0' ) << std::setw( 3 ) << ms.count()
         << ".jsonl";  // JSON Lines format (one JSON object per line)
      
      _output_file_path = ingest_dir / ss.str();
      
      // Open file for writing
      _output_file.open( _output_file_path.string(), std::ios::out | std::ios::app );
      if( !_output_file.is_open() )
      {
         FC_THROW( "Failed to open output file: ${file}", ("file", _output_file_path.string()) );
      }
      
      ilog( "Dry run mode: writing to file ${file}", ("file", _output_file_path.string()) );
   }
   FC_CAPTURE_AND_RETHROW()
}

void ingest_plugin_impl::write_operation_json( const std::string& json_str )
{
   std::lock_guard< std::mutex > lock( _file_mutex );
   
   // Lazy initialization: open file if not already open
   if( !_output_file.is_open() )
   {
      try
      {
         initialize_output_file();
      }
      catch( const fc::exception& e )
      {
         elog( "Failed to initialize output file: ${e}", ("e", e.to_string()) );
         return;
      }
      catch( const std::exception& e )
      {
         elog( "Failed to initialize output file: ${e}", ("e", e.what()) );
         return;
      }
   }
   
   // Write JSON line (JSON Lines format)
   _output_file << json_str << "\n";
   _output_file.flush();  // Ensure data is written immediately
}

} // detail

ingest_plugin::ingest_plugin() {}
ingest_plugin::~ingest_plugin() {}

void ingest_plugin::set_program_options(
   boost::program_options::options_description& cli,
   boost::program_options::options_description& cfg
)
{
   cfg.add_options()
      ( "ingest-endpoint",
        boost::program_options::value< std::string >()->default_value( "http://localhost:8080/ingest/applied_op" ),
        "Ingest service HTTP endpoint" )
      ( "ingest-http-timeout",
        boost::program_options::value< uint32_t >()->default_value( 5000 ),
        "HTTP request timeout in milliseconds" )
      ( "ingest-queue-size",
        boost::program_options::value< uint32_t >()->default_value( 100000 ),
        "Maximum queue size for pending operations" )
      ( "ingest-dry-run",
        boost::program_options::value< bool >()->default_value( false ),
        "Dry run mode: write operations to file instead of sending HTTP" )
      ;
}

void ingest_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   try
   {
      ilog( "Initializing ingest plugin" );
      
      my = std::make_unique< detail::ingest_plugin_impl >( *this );
      
      // Read configuration
      if( options.count( "ingest-endpoint" ) )
         my->_ingest_endpoint = options["ingest-endpoint"].as< std::string >();
      
      if( options.count( "ingest-http-timeout" ) )
         my->_http_timeout_ms = options["ingest-http-timeout"].as< uint32_t >();
      
      if( options.count( "ingest-queue-size" ) )
         my->_max_queue_size = options["ingest-queue-size"].as< uint32_t >();
      
      if( options.count( "ingest-dry-run" ) )
         my->_dry_run = options["ingest-dry-run"].as< bool >();
      
      // Register signal handler
      database& db = appbase::app().get_plugin< chain::chain_plugin >().db();
      my->_post_apply_operation_conn = db.add_post_apply_operation_handler(
         [&]( const operation_notification& note ) {
            try { my->on_post_apply_operation( note ); } FC_LOG_AND_RETHROW()
         },
         *this,
         0  // group
      );
      
      appbase::app().get_plugin< chain::chain_plugin >().report_state_options( name(), fc::variant_object() );
      
      if( my->_dry_run )
      {
         ilog( "Ingest plugin initialized in DRY RUN mode" );
      }
      else
      {
         ilog( "Ingest plugin initialized, endpoint: ${ep}", ("ep", my->_ingest_endpoint) );
      }
   }
   FC_CAPTURE_AND_RETHROW()
}

void ingest_plugin::plugin_startup()
{
   try
   {
      ilog( "Starting ingest plugin" );
      
      if( my->_dry_run )
      {
         // Dry run mode: initialize output file
         my->initialize_output_file();
         ilog( "Ingest plugin started in DRY RUN mode" );
      }
      else
      {
         // Normal mode: start HTTP worker thread
         my->_shutdown = false;
         my->_http_thread = std::thread( [this]() {
            my->http_send_worker();
         } );
         ilog( "Ingest plugin started" );
      }
   }
   FC_CAPTURE_AND_RETHROW()
}

void ingest_plugin::plugin_shutdown()
{
   try
   {
      ilog( "Shutting down ingest plugin" );
      
      if( !my->_dry_run )
      {
         // Normal mode: stop HTTP worker thread
         {
            std::lock_guard< std::mutex > lock( my->_queue_mutex );
            my->_shutdown = true;
         }
         my->_queue_cv.notify_one();
         
         if( my->_http_thread.joinable() )
         {
            my->_http_thread.join();
         }
      }
      else
      {
         // Dry run mode: close output file
         if( my->_output_file.is_open() )
         {
            my->_output_file.close();
            ilog( "Closed output file: ${file}", ("file", my->_output_file_path.string()) );
         }
      }
      
      // Disconnect signal
      util::disconnect_signal( my->_post_apply_operation_conn );
      
      ilog( "Ingest plugin shut down" );
   }
   FC_CAPTURE_AND_RETHROW()
}

} } } // steem::plugins::ingest
