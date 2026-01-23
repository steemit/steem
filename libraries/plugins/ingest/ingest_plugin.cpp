
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
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <memory>
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
      void on_post_apply_block( const block_notification& note );

      // HTTP sending
      void send_operation_json( const fc::variant& json );
      void http_send_worker();
      void http_retry_worker();
      void send_http_batch( const std::vector< std::string >& batch );
      void parse_endpoint_url( std::string& host, std::string& port, std::string& target );
      
      // Connection pool for HTTP connections
      struct http_connection;
      http_connection* get_or_create_connection();
      
      // Service readiness check
      bool wait_for_service_ready( uint32_t timeout_seconds );

      // File writing (for dry run)
      void write_operation_json( const std::string& json_str );
      void initialize_output_file();

      // JSON building
      fc::variant build_operation_json( const operation_notification& note );
      fc::variant build_block_only_json( const block_notification& note );

      ingest_plugin& _plugin;
      database& _db;
      
      // Signal connections
      boost::signals2::connection _post_apply_operation_conn;
      boost::signals2::connection _post_apply_block_conn;
      
      // Track blocks that have operations
      std::set< uint32_t > _blocks_with_ops;
      std::mutex _blocks_mutex;
      
      // HTTP configuration
      std::string _ingest_endpoint;
      uint32_t _http_timeout_ms;
      uint32_t _max_queue_size;
      uint32_t _batch_size;        // Batch size for sending multiple operations
      uint32_t _batch_timeout_ms;  // Max wait time before sending a batch
      uint32_t _shutdown_drain_timeout_seconds;  // Timeout for draining queue during shutdown
      uint32_t _startup_wait_timeout_seconds;  // Timeout for waiting service to be ready during startup
      
      // Dry run configuration
      bool _dry_run;
      bfs::path _output_file_path;
      std::ofstream _output_file;
      std::mutex _file_mutex;
      
      // Async send queue
      std::queue< std::string > _send_queue;
      std::mutex _queue_mutex;
      std::condition_variable _queue_cv;      // Notify worker thread when items available
      std::condition_variable _space_cv;     // Notify replay thread when space available
      std::thread _http_thread;
      std::thread _retry_thread;
      bool _shutdown;
      
      // Retry mechanism
      struct retry_item
      {
         std::vector< std::string > batch;
         uint32_t retry_count;
         std::chrono::steady_clock::time_point retry_time;
      };
      std::queue< retry_item > _retry_queue;
      std::mutex _retry_mutex;
      std::condition_variable _retry_cv;
      static constexpr uint32_t MAX_RETRY_COUNT = 5;  // Maximum retry attempts
      static constexpr uint32_t RETRY_DELAY_MS = 3000; // 3 seconds delay between retries
      
      // Connection pool for HTTP connections
      struct http_connection
      {
         net::io_context ioc;
         tcp::resolver resolver;
         tcp::socket socket;
         std::string host;
         std::string port;
         std::string target;
         bool connected;
         
         http_connection( const std::string& h, const std::string& p, const std::string& t ) :
            resolver( ioc ), socket( ioc ), host( h ), port( p ), target( t ), connected( false )
         {}
         
         void connect()
         {
            if( !connected )
            {
               auto const results = resolver.resolve( host, port );
               net::connect( socket, results.begin(), results.end() );
               connected = true;
            }
         }
         
         void disconnect()
         {
            if( connected )
            {
               beast::error_code ec;
               socket.shutdown( tcp::socket::shutdown_both, ec );
               socket.close( ec );
               connected = false;
            }
         }
         
         ~http_connection()
         {
            disconnect();
         }
      };
      
      std::unique_ptr< http_connection > _http_conn;
      std::mutex _conn_mutex;
};

ingest_plugin_impl::ingest_plugin_impl( ingest_plugin& _plugin ) :
   _plugin( _plugin ),
   _db( appbase::app().get_plugin< chain::chain_plugin >().db() ),
   _ingest_endpoint( "http://localhost:8080/ingest/applied_ops" ),
   _http_timeout_ms( 5000 ),
   _max_queue_size( 100000 ),
   _batch_size( 100 ),           // Default: batch 100 operations
   _batch_timeout_ms( 100 ),     // Default: wait max 100ms before sending
   _shutdown_drain_timeout_seconds( 180 ),  // Default: 3 minutes timeout for draining queue during shutdown
   _startup_wait_timeout_seconds( 30 ),  // Default: 30 seconds timeout for waiting service to be ready during startup
   _dry_run( false ),
   _shutdown( false )
{
}

ingest_plugin_impl::~ingest_plugin_impl()
{
   // Stop retry thread
   if( _retry_thread.joinable() )
   {
      {
         std::lock_guard< std::mutex > lock( _retry_mutex );
         _shutdown = true;
      }
      _retry_cv.notify_one();
      _retry_thread.join();
   }
   
   // Stop HTTP thread
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
      // Mark this block as having operations
      {
         std::lock_guard< std::mutex > lock( _blocks_mutex );
         _blocks_with_ops.insert( note.block );
      }
      
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

void ingest_plugin_impl::on_post_apply_block( const block_notification& note )
{
   try
   {
      uint32_t block_num = note.block.block_num();
      
      // Check if this block has any operations
      bool has_ops = false;
      {
         std::lock_guard< std::mutex > lock( _blocks_mutex );
         has_ops = ( _blocks_with_ops.find( block_num ) != _blocks_with_ops.end() );
         // Remove from set to free memory (we only need to track until block is complete)
         _blocks_with_ops.erase( block_num );
      }
      
      // If block has no operations, send block-only record
      if( !has_ops )
      {
         fc::variant json = build_block_only_json( note );
         send_operation_json( json );
      }
   }
   catch( const fc::exception& e )
   {
      elog( "Error processing block: ${e}", ("e", e.to_string()) );
   }
   catch( const std::exception& e )
   {
      elog( "Error processing block: ${e}", ("e", e.what()) );
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

fc::variant ingest_plugin_impl::build_block_only_json( const block_notification& note )
{
   fc::mutable_variant_object result;
   
   const signed_block& block = note.block;
   uint32_t block_num = block.block_num();
   
   // 1. block object
   fc::mutable_variant_object block_obj;
   block_obj["num"] = block_num;
   block_obj["id"] = block.id().str();
   block_obj["timestamp"] = block.timestamp.to_iso_string();
   result["block"] = block_obj;
   
   // 2. transaction object (null for block-only)
   fc::mutable_variant_object trx_obj;
   trx_obj["id"] = fc::variant();
   trx_obj["index"] = -1;
   result["transaction"] = trx_obj;
   
   // 3. operation object (null for block-only)
   fc::mutable_variant_object op_obj;
   op_obj["index"] = -1;
   op_obj["type"] = "";
   op_obj["value"] = fc::variant();
   result["operation"] = op_obj;
   
   // 4. virtual marker (false for block-only)
   result["virtual"] = false;
   
   // 5. block_only marker (indicates this is a block-only record)
   result["block_only"] = true;
   
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
      // Block until queue has space (protect API endpoint from overload)
      {
         std::unique_lock< std::mutex > lock( _queue_mutex );
         bool waited = false;
         if( _send_queue.size() >= _max_queue_size )
         {
            waited = true;
            wlog( "Ingest queue full, waiting for space" );
         }
         
         // Wait until queue has space
         _space_cv.wait( lock, [this] {
            return _send_queue.size() < _max_queue_size || _shutdown;
         } );
         
         if( _shutdown )
         {
            return;  // Plugin shutting down, skip
         }
         
         if( waited )
         {
            ilog( "Ingest queue has space, resuming enqueue" );
         }
         
         _send_queue.push( json_str );
      }
      _queue_cv.notify_one();  // Notify worker thread that new item is available
   }
}

void ingest_plugin_impl::http_send_worker()
{
   while( !_shutdown )
   {
      std::vector< std::string > batch;
      
      // Collect batch of operations
      {
         std::unique_lock< std::mutex > lock( _queue_mutex );
         
         // Wait for at least one item or timeout
         if( _send_queue.empty() )
         {
            _queue_cv.wait_for( lock, std::chrono::milliseconds( _batch_timeout_ms ),
                              [this] { return !_send_queue.empty() || _shutdown; } );
         }
         
         if( _shutdown && _send_queue.empty() ) break;
         
         // Collect up to _batch_size items, or all available if less
         // If batch_size is 1, disable batching (send immediately)
         uint32_t effective_batch_size = ( _batch_size == 1 ) ? 1 : _batch_size;
         auto start_time = std::chrono::steady_clock::now();
         size_t queue_size_before = _send_queue.size();
         
         while( batch.size() < effective_batch_size && !_send_queue.empty() )
         {
            batch.push_back( _send_queue.front() );
            _send_queue.pop();
            
            // If we have some items but not a full batch, wait a bit more
            // Skip batching logic if batch_size is 1 (immediate send)
            if( effective_batch_size > 1 && batch.size() < effective_batch_size && !_send_queue.empty() )
            {
               auto elapsed = std::chrono::steady_clock::now() - start_time;
               auto elapsed_ms = std::chrono::duration_cast< std::chrono::milliseconds >( elapsed ).count();
               
               if( elapsed_ms < _batch_timeout_ms )
               {
                  // Wait for more items or timeout
                  _queue_cv.wait_for( lock, std::chrono::milliseconds( _batch_timeout_ms - elapsed_ms ),
                                    [this, effective_batch_size] { return _send_queue.size() >= effective_batch_size || _shutdown; } );
               }
               else
               {
                  // Timeout reached, send what we have
                  break;
               }
            }
            else if( effective_batch_size == 1 )
            {
               // Immediate send, no batching
               break;
            }
         }
         
         // Notify waiting threads if queue now has space
         // Only notify if we've freed up significant space (to avoid spurious wakeups)
         size_t queue_size_after = _send_queue.size();
         if( queue_size_before >= _max_queue_size && queue_size_after < _max_queue_size )
         {
            _space_cv.notify_all();  // Notify all waiting threads that space is available
         }
      }
      
      // Send batch (outside lock to avoid blocking queue operations)
      if( !batch.empty() )
      {
         try
         {
            ilog( "Ingest worker sending batch: ${count}", ("count", batch.size()) );
            // Always use batch endpoint, even for single item
            send_http_batch( batch );
            
            // After successful send, notify waiting threads if queue has space
            // Also notify if queue is empty (for shutdown waiting)
            {
               std::lock_guard< std::mutex > lock( _queue_mutex );
               if( _send_queue.size() < _max_queue_size )
               {
                  _space_cv.notify_all();  // Notify replay thread that space is available
               }
               // Also notify if queue is empty (helps with shutdown waiting)
               if( _send_queue.empty() )
               {
                  _space_cv.notify_all();
               }
            }
         }
         catch( const std::exception& e )
         {
            elog( "HTTP send error: ${e}, will retry", ("e", e.what()) );
            
            // Add to retry queue instead of dropping
            {
               std::lock_guard< std::mutex > retry_lock( _retry_mutex );
               retry_item item;
               item.batch = batch;
               item.retry_count = 0;
               item.retry_time = std::chrono::steady_clock::now() + std::chrono::milliseconds( RETRY_DELAY_MS );
               _retry_queue.push( item );
            }
            _retry_cv.notify_one();
            
            // Even on error, notify waiting threads to avoid deadlock
            {
               std::lock_guard< std::mutex > lock( _queue_mutex );
               _space_cv.notify_all();
            }
         }
      }
   }
}

void ingest_plugin_impl::http_retry_worker()
{
   while( !_shutdown )
   {
      std::vector< retry_item > items_to_retry;
      
      // Collect items ready for retry
      {
         std::unique_lock< std::mutex > lock( _retry_mutex );
         
         auto now = std::chrono::steady_clock::now();
         
         // Wait for items or timeout
         if( _retry_queue.empty() )
         {
            _retry_cv.wait_for( lock, std::chrono::milliseconds( 1000 ),
                              [this] { return !_retry_queue.empty() || _shutdown; } );
         }
         
         if( _shutdown && _retry_queue.empty() ) break;
         
         // Collect items that are ready for retry
         while( !_retry_queue.empty() )
         {
            auto& item = _retry_queue.front();
            if( now >= item.retry_time )
            {
               items_to_retry.push_back( item );
               _retry_queue.pop();
            }
            else
            {
               // Wait for the earliest retry time
               auto wait_time = std::chrono::duration_cast< std::chrono::milliseconds >(
                  item.retry_time - now ).count();
               if( wait_time > 0 )
               {
                  _retry_cv.wait_for( lock, std::chrono::milliseconds( wait_time ),
                                    [this] { return _shutdown; } );
               }
               break;
            }
         }
      }
      
      // Retry sending
      for( auto& item : items_to_retry )
      {
         if( _shutdown ) break;
         
         try
         {
            ilog( "Retrying batch (attempt ${attempt}/${max}): ${count} items",
                  ("attempt", item.retry_count + 1)("max", MAX_RETRY_COUNT)("count", item.batch.size()) );
            // Always use batch endpoint, even for single item
            send_http_batch( item.batch );
            
            // Success: batch sent successfully
            ilog( "Retry successful for batch of ${count} items", ("count", item.batch.size()) );
         }
         catch( const std::exception& e )
         {
            elog( "Retry failed: ${e}", ("e", e.what()) );
            
            // Increment retry count
            item.retry_count++;
            
            if( item.retry_count < MAX_RETRY_COUNT )
            {
               // Schedule next retry
               item.retry_time = std::chrono::steady_clock::now() + std::chrono::milliseconds( RETRY_DELAY_MS );
               
               {
                  std::lock_guard< std::mutex > lock( _retry_mutex );
                  _retry_queue.push( item );
               }
               _retry_cv.notify_one();
            }
            else
            {
               // Max retries reached, log error and drop
               elog( "Max retries (${max}) reached for batch of ${count} items, dropping",
                     ("max", MAX_RETRY_COUNT)("count", item.batch.size()) );
            }
         }
      }
   }
}

void ingest_plugin_impl::parse_endpoint_url( std::string& host, std::string& port, std::string& target )
{
   std::string url = _ingest_endpoint;
   
   // Simple URL parsing
   if( url.find( "http://" ) == 0 )
   {
      url = url.substr( 7 );
   }
   else if( url.find( "https://" ) == 0 )
   {
      // HTTPS not supported yet, but parse anyway
      url = url.substr( 8 );
   }
   
   size_t port_pos = url.find( ':' );
   size_t path_pos = url.find( '/' );
   
   if( path_pos != std::string::npos )
   {
      target = url.substr( path_pos );
      url = url.substr( 0, path_pos );
   }
   else
   {
      target = "/ingest/applied_ops";
   }
   
   if( port_pos != std::string::npos )
   {
      host = url.substr( 0, port_pos );
      port = url.substr( port_pos + 1 );
   }
   else
   {
      host = url;
      port = "8080";
   }
}

ingest_plugin_impl::http_connection* ingest_plugin_impl::get_or_create_connection()
{
   std::lock_guard< std::mutex > lock( _conn_mutex );
   
   if( !_http_conn )
   {
      std::string host, port, target;
      parse_endpoint_url( host, port, target );
      _http_conn = std::make_unique< http_connection >( host, port, target );
   }
   
   // Reconnect if needed
   if( !_http_conn->connected )
   {
      try
      {
         _http_conn->connect();
      }
      catch( const std::exception& e )
      {
         elog( "Failed to connect: ${e}", ("e", e.what()) );
         _http_conn.reset();
         return nullptr;
      }
   }
   
   return _http_conn.get();
}

void ingest_plugin_impl::send_http_batch( const std::vector< std::string >& batch )
{
   // Build batch JSON array
   std::string batch_json = "[";
   for( size_t i = 0; i < batch.size(); ++i )
   {
      if( i > 0 ) batch_json += ",";
      batch_json += batch[i];
   }
   batch_json += "]";
   
   auto* conn = get_or_create_connection();
   if( !conn )
   {
      // Fallback: create temporary connection
      std::string host, port, target;
      parse_endpoint_url( host, port, target );
      
      ilog( "Ingest HTTP BATCH (fallback) -> ${host}:${port}${target} (ops: ${count}, bytes: ${bytes})",
            ("host", host)("port", port)("target", target)("count", batch.size())("bytes", batch_json.size()) );
      
      net::io_context ioc;
      tcp::resolver resolver( ioc );
      tcp::socket socket( ioc );
      
      auto const results = resolver.resolve( host, port );
      net::connect( socket, results.begin(), results.end() );
      
      http::request< http::string_body > req;
      req.method( http::verb::post );
      req.target( target );
      req.set( http::field::host, host + ":" + port );
      req.set( http::field::content_type, "application/json" );
      req.set( http::field::user_agent, "steemd-ingest-plugin/1.0" );
      req.set( http::field::connection, "keep-alive" );
      req.body() = batch_json;
      req.prepare_payload();
      
      http::write( socket, req );
      
      beast::flat_buffer buffer;
      http::response< http::string_body > res;
      http::read( socket, buffer, res );
      
      if( res.result() != http::status::ok )
      {
         elog( "HTTP batch error (fallback): ${code}, will retry", ("code", res.result_int()) );
         // Throw exception to trigger retry mechanism
         throw std::runtime_error( "HTTP batch error (fallback): " + std::to_string( res.result_int() ) );
      }
      else
      {
         ilog( "Ingest HTTP BATCH (fallback) success: ${code}", ("code", res.result_int()) );
      }
      
      beast::error_code ec;
      socket.shutdown( tcp::socket::shutdown_both, ec );
      return;
   }
   
   // Use connection pool
   try
   {
      ilog( "Ingest HTTP BATCH -> ${host}:${port}${target} (ops: ${count}, bytes: ${bytes})",
            ("host", conn->host)("port", conn->port)("target", conn->target)("count", batch.size())("bytes", batch_json.size()) );
            ("host", conn->host)("port", conn->port)("target", target)("count", batch.size())("bytes", batch_json.size()) );
      
      http::request< http::string_body > req;
      req.method( http::verb::post );
      req.target( target );
      req.set( http::field::host, conn->host + ":" + conn->port );
      req.set( http::field::content_type, "application/json" );
      req.set( http::field::user_agent, "steemd-ingest-plugin/1.0" );
      req.set( http::field::connection, "keep-alive" );
      req.body() = batch_json;
      req.prepare_payload();
      
      http::write( conn->socket, req );
      
      beast::flat_buffer buffer;
      http::response< http::string_body > res;
      http::read( conn->socket, buffer, res );
      
      if( res.result() != http::status::ok )
      {
         elog( "HTTP batch error: ${code}, will retry", ("code", res.result_int()) );
         // Connection might be broken, reset it
         std::lock_guard< std::mutex > lock( _conn_mutex );
         _http_conn->disconnect();
         _http_conn->connected = false;
         // Throw exception to trigger retry mechanism
         throw std::runtime_error( "HTTP batch error: " + std::to_string( res.result_int() ) );
      }
      else
      {
         ilog( "Ingest HTTP BATCH success: ${code}", ("code", res.result_int()) );
      }
   }
   catch( const std::exception& e )
   {
      elog( "HTTP batch send error: ${e}", ("e", e.what()) );
      // Connection broken, reset it
      std::lock_guard< std::mutex > lock( _conn_mutex );
      _http_conn->disconnect();
      _http_conn->connected = false;
      throw;
   }
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

bool ingest_plugin_impl::wait_for_service_ready( uint32_t timeout_seconds )
{
   if( _dry_run )
   {
      return true;  // Dry run mode: no service to wait for
   }
   
   std::string host, port, target;
   parse_endpoint_url( host, port, target );
   
   auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds( timeout_seconds );
   uint32_t attempt = 0;
   
   while( std::chrono::steady_clock::now() < timeout )
   {
      attempt++;
      try
      {
         // Try to create a connection and send a minimal test request
         net::io_context ioc;
         tcp::resolver resolver( ioc );
         tcp::socket socket( ioc );
         
         auto const results = resolver.resolve( host, port );
         net::connect( socket, results.begin(), results.end() );
         
         // Send a minimal POST request (empty array for batch endpoint)
         http::request< http::string_body > req;
         req.method( http::verb::post );
         req.target( target );
         req.set( http::field::host, host + ":" + port );
         req.set( http::field::content_type, "application/json" );
         req.set( http::field::user_agent, "steemd-ingest-plugin/1.0" );
         req.body() = "[]";  // Empty array for batch endpoint
         req.prepare_payload();
         
         http::write( socket, req );
         
         beast::flat_buffer buffer;
         http::response< http::string_body > res;
         http::read( socket, buffer, res );
         
         beast::error_code ec;
         socket.shutdown( tcp::socket::shutdown_both, ec );
         
         // 200 OK or 400 Bad Request (empty array) both mean service is ready
         if( res.result() == http::status::ok || res.result() == http::status::bad_request )
         {
            if( attempt > 1 )
            {
               ilog( "Ingest service is ready (attempt ${attempt})", ("attempt", attempt) );
            }
            return true;
         }
      }
      catch( const std::exception& e )
      {
         // Connection failed, wait and retry
         if( attempt % 5 == 0 )  // Log every 5 attempts
         {
            ilog( "Waiting for ingest service (attempt ${attempt}): ${e}", ("attempt", attempt)("e", e.what()) );
         }
      }
      
      // Wait 500ms before next attempt
      std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
   }
   
   wlog( "Ingest service not ready after ${timeout}s (${attempt} attempts)", ("timeout", timeout_seconds)("attempt", attempt) );
   return false;
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
        boost::program_options::value< std::string >()->default_value( "http://localhost:8080/ingest/applied_ops" ),
        "Ingest service HTTP endpoint (batch endpoint)" )
      ( "ingest-http-timeout",
        boost::program_options::value< uint32_t >()->default_value( 5000 ),
        "HTTP request timeout in milliseconds" )
      ( "ingest-queue-size",
        boost::program_options::value< uint32_t >()->default_value( 100000 ),
        "Maximum queue size for pending operations" )
      ( "ingest-batch-size",
        boost::program_options::value< uint32_t >()->default_value( 100 ),
        "Number of operations to batch together before sending (1 = disable batching)" )
      ( "ingest-batch-timeout",
        boost::program_options::value< uint32_t >()->default_value( 100 ),
        "Maximum milliseconds to wait before sending a batch (even if not full)" )
      ( "ingest-shutdown-drain-timeout",
        boost::program_options::value< uint32_t >()->default_value( 180 ),
        "Timeout in seconds for draining queue during shutdown (default: 180 = 3 minutes)" )
      ( "ingest-startup-wait-timeout",
        boost::program_options::value< uint32_t >()->default_value( 30 ),
        "Timeout in seconds for waiting service to be ready during startup (default: 30 seconds)" )
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
      
      if( options.count( "ingest-batch-size" ) )
         my->_batch_size = options["ingest-batch-size"].as< uint32_t >();
      
      if( options.count( "ingest-batch-timeout" ) )
         my->_batch_timeout_ms = options["ingest-batch-timeout"].as< uint32_t >();
      
      if( options.count( "ingest-shutdown-drain-timeout" ) )
         my->_shutdown_drain_timeout_seconds = options["ingest-shutdown-drain-timeout"].as< uint32_t >();
      
      if( options.count( "ingest-startup-wait-timeout" ) )
         my->_startup_wait_timeout_seconds = options["ingest-startup-wait-timeout"].as< uint32_t >();
      
      // Validate batch size
      if( my->_batch_size == 0 )
      {
         my->_batch_size = 1;  // Disable batching by setting to 1
      }
      
      if( options.count( "ingest-dry-run" ) )
         my->_dry_run = options["ingest-dry-run"].as< bool >();
      
      // Register signal handlers
      database& db = appbase::app().get_plugin< chain::chain_plugin >().db();
      my->_post_apply_operation_conn = db.add_post_apply_operation_handler(
         [&]( const operation_notification& note ) {
            try { my->on_post_apply_operation( note ); } FC_LOG_AND_RETHROW()
         },
         *this,
         0  // group
      );
      my->_post_apply_block_conn = db.add_post_apply_block_handler(
         [&]( const block_notification& note ) {
            try { my->on_post_apply_block( note ); } FC_LOG_AND_RETHROW()
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
         
         // Wait for service to be ready BEFORE starting workers
         // This ensures service is ready before replay starts processing blocks
         ilog( "Waiting for ingest service to be ready (timeout: ${timeout}s)...", ("timeout", my->_startup_wait_timeout_seconds) );
         bool service_ready = my->wait_for_service_ready( my->_startup_wait_timeout_seconds );
         if( !service_ready )
         {
            wlog( "Ingest service not ready after timeout, but continuing anyway. Some early blocks may be lost." );
         }
         else
         {
            ilog( "Ingest service is ready" );
         }
         
         // Start HTTP worker early to avoid blocking replay before plugin_startup
         if( !my->_http_thread.joinable() )
         {
            my->_shutdown = false;
            my->_http_thread = std::thread( [this]() {
               my->http_send_worker();
            } );
            ilog( "Ingest plugin HTTP worker started (early)" );
         }
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
         // Normal mode: start retry thread (HTTP worker already started in plugin_initialize)
         if( !my->_retry_thread.joinable() )
         {
            my->_retry_thread = std::thread( [this]() {
               my->http_retry_worker();
            } );
            ilog( "Ingest plugin retry worker started" );
         }
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
         // Normal mode: wait for queue to drain before shutting down
         // This ensures all blocks (including block-only records) are sent
         // Note: We don't set _shutdown yet, so worker continues processing
         {
            std::unique_lock< std::mutex > lock( my->_queue_mutex );
            size_t queue_size = my->_send_queue.size();
            if( queue_size > 0 )
            {
               ilog( "Waiting for ingest queue to drain (${size} items remaining, timeout: ${timeout}s)...", 
                     ("size", queue_size)("timeout", my->_shutdown_drain_timeout_seconds) );
               
               // Wait up to configured timeout for queue to drain
               // Poll periodically to check if queue is empty
               // Worker thread will continue processing until queue is empty
               auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds( my->_shutdown_drain_timeout_seconds );
               bool drained = false;
               
               while( std::chrono::steady_clock::now() < timeout )
               {
                  if( my->_send_queue.empty() )
                  {
                     // Queue is empty, wait a bit more to ensure no new items are being added
                     my->_queue_cv.wait_for( lock, std::chrono::milliseconds( 500 ), [this] {
                        return !my->_send_queue.empty();
                     } );
                     if( my->_send_queue.empty() )
                     {
                        drained = true;
                        break;
                     }
                  }
                  else
                  {
                     // Wait a bit and check again
                     my->_queue_cv.wait_for( lock, std::chrono::milliseconds( 100 ), [this] {
                        return my->_send_queue.empty();
                     } );
                     if( my->_send_queue.empty() )
                     {
                        drained = true;
                        break;
                     }
                  }
               }
               
               if( !drained && !my->_send_queue.empty() )
               {
                  size_t remaining = my->_send_queue.size();
                  wlog( "Ingest queue not fully drained after timeout. ${size} items remaining (may be lost)", ("size", remaining) );
               }
               else
               {
                  ilog( "Ingest queue drained successfully" );
               }
            }
         }
         
         // Now set shutdown flag and stop threads
         // Stop retry thread first
         {
            std::lock_guard< std::mutex > lock( my->_retry_mutex );
            my->_shutdown = true;
         }
         my->_retry_cv.notify_one();
         
         if( my->_retry_thread.joinable() )
         {
            my->_retry_thread.join();
         }
         
         // Stop HTTP worker thread
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
      
      // Disconnect signals
      util::disconnect_signal( my->_post_apply_operation_conn );
      util::disconnect_signal( my->_post_apply_block_conn );
      
      ilog( "Ingest plugin shut down" );
   }
   FC_CAPTURE_AND_RETHROW()
}

} } } // steem::plugins::ingest
