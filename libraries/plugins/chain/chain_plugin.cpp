#include <steem/chain/database_exceptions.hpp>
#include <steem/chain/statefile/statefile.hpp>

#include <steem/plugins/chain/abstract_block_producer.hpp>
#include <steem/plugins/chain/chain_plugin.hpp>
#include <steem/plugins/statsd/utility.hpp>

#include <steem/utilities/benchmark_dumper.hpp>

#include <fc/string.hpp>

#include <boost/asio.hpp>
#include <boost/optional.hpp>
#include <boost/bind.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/thread/future.hpp>
#include <boost/lockfree/queue.hpp>

#include <thread>
#include <memory>
#include <iostream>

namespace steem { namespace plugins { namespace chain {

using namespace steem;
using fc::flat_map;
using steem::chain::block_id_type;
namespace asio = boost::asio;

#define NUM_THREADS 1

struct generate_block_request
{
   generate_block_request( const fc::time_point_sec w, const account_name_type& wo, const fc::ecc::private_key& priv_key, uint32_t s ) :
      when( w ),
      witness_owner( wo ),
      block_signing_private_key( priv_key ),
      skip( s ) {}

   const fc::time_point_sec when;
   const account_name_type& witness_owner;
   const fc::ecc::private_key& block_signing_private_key;
   uint32_t skip;
   signed_block block;
};

typedef fc::static_variant< const signed_block*, const signed_transaction*, generate_block_request* > write_request_ptr;
typedef fc::static_variant< boost::promise< void >*, fc::future< void >* > promise_ptr;

struct write_context
{
   write_request_ptr             req_ptr;
   uint32_t                      skip = 0;
   bool                          success = true;
   fc::optional< fc::exception > except;
   promise_ptr                   prom_ptr;
};

namespace detail {

class chain_plugin_impl
{
   public:
      chain_plugin_impl() : write_queue( 64 ) {}
      ~chain_plugin_impl() { stop_write_processing(); }

      void start_write_processing();
      void stop_write_processing();

      uint64_t                         shared_memory_size = 0;
      uint16_t                         shared_file_full_threshold = 0;
      uint16_t                         shared_file_scale_rate = 0;
      bfs::path                        shared_memory_dir;
      bool                             replay = false;
      bool                             resync   = false;
      bool                             readonly = false;
      bool                             check_locks = false;
      bool                             validate_invariants = false;
      bool                             dump_memory_details = false;
      bool                             benchmark_is_enabled =false;
      bool                             statsd_on_replay = false;
      uint32_t                         stop_replay_at = 0;
      uint32_t                         benchmark_interval = 0;
      uint32_t                         flush_interval = 0;
      flat_map<uint32_t,block_id_type> loaded_checkpoints;
      std::string                      from_state = "";
      std::string                      to_state = "";

      uint32_t allow_future_time = 5;

      bool                             running = true;
      std::shared_ptr< std::thread >   write_processor_thread;
      boost::lockfree::queue< write_context* > write_queue;
      int16_t                          write_lock_hold_time = 500;

      vector< string >                 loaded_plugins;
      fc::mutable_variant_object       plugin_state_opts;

      database  db;
      std::string block_generator_registrant;
      std::shared_ptr< abstract_block_producer > block_generator;
};

struct write_request_visitor
{
   write_request_visitor() {}

   database* db;
   uint32_t  skip = 0;
   fc::optional< fc::exception >* except;
   std::shared_ptr< abstract_block_producer > block_generator;

   typedef bool result_type;

   bool operator()( const signed_block* block )
   {
      bool result = false;

      try
      {
         STATSD_START_TIMER( "chain", "write_time", "push_block", 1.0f )
         result = db->push_block( *block, skip );
         STATSD_STOP_TIMER( "chain", "write_time", "push_block" )
      }
      catch( fc::exception& e )
      {
         *except = e;
      }
      catch( ... )
      {
         *except = fc::unhandled_exception( FC_LOG_MESSAGE( warn, "Unexpected exception while pushing block." ),
                                           std::current_exception() );
      }

      return result;
   }

   bool operator()( const signed_transaction* trx )
   {
      bool result = false;

      try
      {
         STATSD_START_TIMER( "chain", "write_time", "push_transaction", 1.0f )
         db->push_transaction( *trx );
         STATSD_STOP_TIMER( "chain", "write_time", "push_transaction" )

         result = true;
      }
      catch( fc::exception& e )
      {
         *except = e;
      }
      catch( ... )
      {
         *except = fc::unhandled_exception( FC_LOG_MESSAGE( warn, "Unexpected exception while pushing block." ),
                                           std::current_exception() );
      }

      return result;
   }

   bool operator()( generate_block_request* req )
   {
      bool result = false;

      try
      {
         if( !block_generator )
            FC_THROW_EXCEPTION( chain_exception, "Received a generate block request, but no block generator has been registered." );

         STATSD_START_TIMER( "chain", "write_time", "generate_block", 1.0f )
         req->block = block_generator->generate_block(
            req->when,
            req->witness_owner,
            req->block_signing_private_key,
            req->skip
            );
         STATSD_STOP_TIMER( "chain", "write_time", "generate_block" )

         result = true;
      }
      catch( fc::exception& e )
      {
         *except = e;
      }
      catch( ... )
      {
         *except = fc::unhandled_exception( FC_LOG_MESSAGE( warn, "Unexpected exception while pushing block." ),
                                           std::current_exception() );
      }

      return result;
   }
};

struct request_promise_visitor
{
   request_promise_visitor(){}

   typedef void result_type;

   template< typename T >
   void operator()( T* t )
   {
      t->set_value();
   }
};

void chain_plugin_impl::start_write_processing()
{
   write_processor_thread = std::make_shared< std::thread >( [&]()
   {
      bool is_syncing = true;
      write_context* cxt;
      fc::time_point_sec start = fc::time_point::now();
      write_request_visitor req_visitor;
      req_visitor.db = &db;
      req_visitor.block_generator = block_generator;

      request_promise_visitor prom_visitor;

      /* This loop monitors the write request queue and performs writes to the database. These
       * can be blocks or pending transactions. Because the caller needs to know the success of
       * the write and any exceptions that are thrown, a write context is passed in the queue
       * to the processing thread which it will use to store the results of the write. It is the
       * caller's responsibility to ensure the pointer to the write context remains valid until
       * the contained promise is complete.
       *
       * The loop has two modes, sync mode and live mode. In sync mode we want to process writes
       * as quickly as possible with minimal overhead. The outer loop busy waits on the queue
       * and the inner loop drains the queue as quickly as possible. We exit sync mode when the
       * head block is within 1 minute of system time.
       *
       * Live mode needs to balance between processing pending writes and allowing readers access
       * to the database. It will batch writes together as much as possible to minimize lock
       * overhead but will willingly give up the write lock after 500ms. The thread then sleeps for
       * 10ms. This allows time for readers to access the database as well as more writes to come
       * in. When the node is live the rate at which writes come in is slower and busy waiting is
       * not an optimal use of system resources when we could give CPU time to read threads.
       */
      while( running )
      {
         if( !is_syncing )
            start = fc::time_point::now();

         if( write_queue.pop( cxt ) )
         {
            db.with_write_lock( [&]()
            {
               STATSD_START_TIMER( "chain", "lock_time", "write_lock", 1.0f )
               while( true )
               {
                  req_visitor.skip = cxt->skip;
                  req_visitor.except = &(cxt->except);
                  cxt->success = cxt->req_ptr.visit( req_visitor );
                  cxt->prom_ptr.visit( prom_visitor );

                  if( is_syncing && start - db.head_block_time() < fc::minutes(1) )
                  {
                     start = fc::time_point::now();
                     is_syncing = false;
                  }

                  if( !is_syncing && write_lock_hold_time >= 0 && fc::time_point::now() - start > fc::milliseconds( write_lock_hold_time ) )
                  {
                     break;
                  }

                  if( !write_queue.pop( cxt ) )
                  {
                     break;
                  }
               }
            });
         }

         if( !is_syncing )
            boost::this_thread::sleep_for( boost::chrono::milliseconds( 10 ) );
      }
   });
}

void chain_plugin_impl::stop_write_processing()
{
   running = false;

   if( write_processor_thread )
      write_processor_thread->join();

   write_processor_thread.reset();
}

} // detail


chain_plugin::chain_plugin() : my( new detail::chain_plugin_impl() ) {}
chain_plugin::~chain_plugin(){}

database& chain_plugin::db() { return my->db; }
const steem::chain::database& chain_plugin::db() const { return my->db; }

bfs::path chain_plugin::state_storage_dir() const
{
   return my->shared_memory_dir;
}

void chain_plugin::set_program_options(options_description& cli, options_description& cfg)
{
   cfg.add_options()
         ("shared-file-dir", bpo::value<bfs::path>()->default_value("blockchain"),
            "the location of the chain shared memory files (absolute path or relative to application data dir)")
         ("shared-file-size", bpo::value<string>()->default_value("54G"), "Size of the shared memory file. Default: 54G. If running a full node, increase this value to 200G.")
         ("shared-file-full-threshold", bpo::value<uint16_t>()->default_value(0),
            "A 2 precision percentage (0-10000) that defines the threshold for when to autoscale the shared memory file. Setting this to 0 disables autoscaling. Recommended value for consensus node is 9500 (95%). Full node is 9900 (99%)" )
         ("shared-file-scale-rate", bpo::value<uint16_t>()->default_value(0),
            "A 2 precision percentage (0-10000) that defines how quickly to scale the shared memory file. When autoscaling occurs the file's size will be increased by this percent. Setting this to 0 disables autoscaling. Recommended value is between 1000-2000 (10-20%)" )
         ("checkpoint,c", bpo::value<vector<string>>()->composing(), "Pairs of [BLOCK_NUM,BLOCK_ID] that should be enforced as checkpoints.")
         ("flush-state-interval", bpo::value<uint32_t>(),
            "flush shared memory changes to disk every N blocks")
         ("from-state", bpo::value<string>()->default_value(""), "Load from state, then replay subsequent blocks (EXPERIMENTAL)")
         ("to-state", bpo::value<string>()->default_value(""), "File to save state after --stop-replay-at-block" )
         ;
   cli.add_options()
         ("replay-blockchain", bpo::bool_switch()->default_value(false), "clear chain database and replay all blocks" )
         ("resync-blockchain", bpo::bool_switch()->default_value(false), "clear chain database and block log" )
         ("stop-replay-at-block", bpo::value<uint32_t>(), "Stop and exit after reaching given block number")
         ("advanced-benchmark", "Make profiling for every plugin.")
         ("set-benchmark-interval", bpo::value<uint32_t>(), "Print time and memory usage every given number of blocks")
         ("dump-memory-details", bpo::bool_switch()->default_value(false), "Dump database objects memory usage info. Use set-benchmark-interval to set dump interval.")
         ("check-locks", bpo::bool_switch()->default_value(false), "Check correctness of chainbase locking" )
         ("validate-database-invariants", bpo::bool_switch()->default_value(false), "Validate all supply invariants check out" )
#ifdef IS_TEST_NET
         ("chain-id", bpo::value< std::string >()->default_value( STEEM_CHAIN_ID ), "chain ID to connect to")
#endif
         ;
}

void chain_plugin::plugin_initialize(const variables_map& options)
{
   my->shared_memory_dir = app().data_dir() / "blockchain";

   if( options.count("shared-file-dir") )
   {
      auto sfd = options.at("shared-file-dir").as<bfs::path>();
      if(sfd.is_relative())
         my->shared_memory_dir = app().data_dir() / sfd;
      else
         my->shared_memory_dir = sfd;
   }

   my->shared_memory_size = fc::parse_size( options.at( "shared-file-size" ).as< string >() );

   if( options.count( "shared-file-full-threshold" ) )
      my->shared_file_full_threshold = options.at( "shared-file-full-threshold" ).as< uint16_t >();

   if( options.count( "shared-file-scale-rate" ) )
      my->shared_file_scale_rate = options.at( "shared-file-scale-rate" ).as< uint16_t >();

   my->from_state          = options.at( "from-state" ).as<string>();
   my->to_state            = options.at( "to-state" ).as<string>();
   my->replay              = options.at( "replay-blockchain" ).as<bool>();
   my->resync              = options.at( "resync-blockchain" ).as<bool>();
   my->stop_replay_at      =
      options.count( "stop-replay-at-block" ) ? options.at( "stop-replay-at-block" ).as<uint32_t>() : 0;
   my->benchmark_interval  =
      options.count( "set-benchmark-interval" ) ? options.at( "set-benchmark-interval" ).as<uint32_t>() : 0;
   my->check_locks         = options.at( "check-locks" ).as< bool >();
   my->validate_invariants = options.at( "validate-database-invariants" ).as<bool>();
   my->dump_memory_details = options.at( "dump-memory-details" ).as<bool>();
   if( options.count( "flush-state-interval" ) )
      my->flush_interval = options.at( "flush-state-interval" ).as<uint32_t>();
   else
      my->flush_interval = 10000;

   if(options.count("checkpoint"))
   {
      auto cps = options.at("checkpoint").as<vector<string>>();
      my->loaded_checkpoints.reserve(cps.size());
      for(const auto& cp : cps)
      {
         auto item = fc::json::from_string(cp).as<std::pair<uint32_t,block_id_type>>();
         my->loaded_checkpoints[item.first] = item.second;
      }
   }

   my->benchmark_is_enabled = (options.count( "advanced-benchmark" ) != 0);

   if( options.count( "statsd-record-on-replay" ) )
   {
      my->statsd_on_replay = options.at( "statsd-record-on-replay" ).as< bool >();
   }

#ifdef IS_TEST_NET
   if( options.count( "chain-id" ) )
   {
      auto chain_id_str = options.at("chain-id").as< std::string >();

      try
      {
         my->db.set_chain_id( chain_id_type( chain_id_str) );
      }
      catch( fc::exception& )
      {
         FC_ASSERT( false, "Could not parse chain_id as hex string. Chain ID String: ${s}", ("s", chain_id_str) );
      }
   }
#endif
}

#define BENCHMARK_FILE_NAME "replay_benchmark.json"

void chain_plugin::plugin_startup()
{
   if( my->statsd_on_replay )
   {
      auto statsd = appbase::app().find_plugin< steem::plugins::statsd::statsd_plugin >();
      if( statsd != nullptr )
      {
         statsd->start_logging();
      }
   }

   ilog( "Starting chain with shared_file_size: ${n} bytes", ("n", my->shared_memory_size) );

   if(my->resync)
   {
      wlog("resync requested: deleting block log and shared memory");
      my->db.wipe( app().data_dir() / "blockchain", my->shared_memory_dir, true );
   }

   my->db.set_flush_interval( my->flush_interval );
   my->db.add_checkpoints( my->loaded_checkpoints );
   my->db.set_require_locking( my->check_locks );

   bool dump_memory_details = my->dump_memory_details;
   steem::utilities::benchmark_dumper dumper;

   const auto& abstract_index_cntr = my->db.get_abstract_index_cntr();

   typedef steem::utilities::benchmark_dumper::index_memory_details_cntr_t index_memory_details_cntr_t;
   auto get_indexes_memory_details = [dump_memory_details, &abstract_index_cntr]
      (index_memory_details_cntr_t& index_memory_details_cntr, bool onlyStaticInfo)
   {
      if (dump_memory_details == false)
         return;

      for (auto idx : abstract_index_cntr)
      {
         auto info = idx->get_statistics(onlyStaticInfo);
         index_memory_details_cntr.emplace_back(std::move(info._value_type_name), info._item_count,
            info._item_sizeof, info._item_additional_allocation, info._additional_container_allocation);
      }
   };

   database::open_args db_open_args;
   db_open_args.data_dir = app().data_dir() / "blockchain";
   db_open_args.shared_mem_dir = my->shared_memory_dir;
   db_open_args.initial_supply = STEEM_INIT_SUPPLY;
   db_open_args.shared_file_size = my->shared_memory_size;
   db_open_args.shared_file_full_threshold = my->shared_file_full_threshold;
   db_open_args.shared_file_scale_rate = my->shared_file_scale_rate;
   db_open_args.do_validate_invariants = my->validate_invariants;
   db_open_args.stop_replay_at = my->stop_replay_at;
   db_open_args.benchmark_is_enabled = my->benchmark_is_enabled;

   auto benchmark_lambda = [&dumper, &get_indexes_memory_details, dump_memory_details] ( uint32_t current_block_number,
      const chainbase::database::abstract_index_cntr_t& abstract_index_cntr )
   {
      if( current_block_number == 0 ) // initial call
      {
         typedef steem::utilities::benchmark_dumper::database_object_sizeof_cntr_t database_object_sizeof_cntr_t;
         auto get_database_objects_sizeofs = [dump_memory_details, &abstract_index_cntr]
            (database_object_sizeof_cntr_t& database_object_sizeof_cntr)
         {
            if (dump_memory_details == false)
               return;

            for (auto idx : abstract_index_cntr)
            {
               auto info = idx->get_statistics(true);
               database_object_sizeof_cntr.emplace_back(std::move(info._value_type_name), info._item_sizeof);
            }
         };

         dumper.initialize(get_database_objects_sizeofs, BENCHMARK_FILE_NAME);
         return;
      }

      const steem::utilities::benchmark_dumper::measurement& measure =
         dumper.measure(current_block_number, get_indexes_memory_details);
      ilog( "Performance report at block ${n}. Elapsed time: ${rt} ms (real), ${ct} ms (cpu). Memory usage: ${cm} (current), ${pm} (peak) kilobytes.",
         ("n", current_block_number)
         ("rt", measure.real_ms)
         ("ct", measure.cpu_ms)
         ("cm", measure.current_mem)
         ("pm", measure.peak_mem) );
   };

   if(my->replay || (my->from_state != ""))
   {
      ilog("Replaying blockchain on user request.");
      db_open_args.benchmark = steem::chain::database::TBenchmark(my->benchmark_interval, benchmark_lambda);
      if( my->from_state != "" )
      {
         db_open_args.genesis_func = std::make_shared< std::function<void( database& )> >( [&]( database& db )
         {
            statefile::init_genesis_from_state( db, my->from_state );
         } );
      }
      uint32_t last_block_number = my->db.reindex( db_open_args );

      if( my->benchmark_interval > 0 )
      {
         const steem::utilities::benchmark_dumper::measurement& total_data = dumper.dump(true, get_indexes_memory_details);
         ilog( "Performance report (total). Blocks: ${b}. Elapsed time: ${rt} ms (real), ${ct} ms (cpu). Memory usage: ${cm} (current), ${pm} (peak) kilobytes.",
               ("b", total_data.block_number)
               ("rt", total_data.real_ms)
               ("ct", total_data.cpu_ms)
               ("cm", total_data.current_mem)
               ("pm", total_data.peak_mem) );
      }

      if( my->stop_replay_at > 0 && my->stop_replay_at == last_block_number )
      {
         ilog("Stopped blockchain replaying on user request. Last applied block number: ${n}.", ("n", last_block_number));
         if( my->to_state != "" )
         {
            ilog( "Saving blockchain state" );
            auto result = statefile::write_state( my->db, my->to_state );
            ilog( "Blockchain state successful, size=${n} hash=${h}", ("n", result.size)("h", result.hash) );
         }
         exit(EXIT_SUCCESS);
      }
   }
   else
   {
      db_open_args.benchmark = steem::chain::database::TBenchmark(dump_memory_details, benchmark_lambda);

      try
      {
         ilog("Opening shared memory from ${path}", ("path",my->shared_memory_dir.generic_string()));

         my->db.open( db_open_args );

         if( dump_memory_details )
            dumper.dump( true, get_indexes_memory_details );
      }
      catch( const fc::exception& e )
      {
         wlog("Error opening database. If the binary or configuration has changed, replay the blockchain explicitly. Error: ${e}", ("e", e));
         exit(EXIT_FAILURE);
      }
   }

   ilog( "Started on blockchain with ${n} blocks", ("n", my->db.head_block_num()) );
   on_sync();

   my->start_write_processing();
}

void chain_plugin::plugin_shutdown()
{
   ilog("closing chain database");
   my->stop_write_processing();
   my->db.close();
   ilog("database closed successfully");
}

void chain_plugin::report_state_options( const string& plugin_name, const fc::variant_object& opts )
{
   my->loaded_plugins.push_back( plugin_name );
   my->plugin_state_opts( opts );
}

bool chain_plugin::accept_block( const steem::chain::signed_block& block, bool currently_syncing, uint32_t skip )
{
   if (currently_syncing && block.block_num() % 10000 == 0) {
      ilog("Syncing Blockchain --- Got block: #${n} time: ${t} producer: ${p}",
           ("t", block.timestamp)
           ("n", block.block_num())
           ("p", block.witness) );
   }

   check_time_in_block( block );

   boost::promise< void > prom;
   write_context cxt;
   cxt.req_ptr = &block;
   cxt.skip = skip;
   cxt.prom_ptr = &prom;

   my->write_queue.push( &cxt );

   prom.get_future().get();

   if( cxt.except ) throw *(cxt.except);

   return cxt.success;
}

void chain_plugin::accept_transaction( const steem::chain::signed_transaction& trx )
{
   boost::promise< void > prom;
   write_context cxt;
   cxt.req_ptr = &trx;
   cxt.prom_ptr = &prom;

   my->write_queue.push( &cxt );

   prom.get_future().get();

   if( cxt.except ) throw *(cxt.except);

   return;
}

steem::chain::signed_block chain_plugin::generate_block(
   const fc::time_point_sec when,
   const account_name_type& witness_owner,
   const fc::ecc::private_key& block_signing_private_key,
   uint32_t skip )
{
   generate_block_request req( when, witness_owner, block_signing_private_key, skip );
   boost::promise< void > prom;
   write_context cxt;
   cxt.req_ptr = &req;
   cxt.prom_ptr = &prom;

   my->write_queue.push( &cxt );

   prom.get_future().get();

   if( cxt.except ) throw *(cxt.except);

   FC_ASSERT( cxt.success, "Block could not be generated" );

   return req.block;
}

int16_t chain_plugin::set_write_lock_hold_time( int16_t new_time )
{
   FC_ASSERT( get_state() == appbase::abstract_plugin::state::initialized,
      "Can only change write_lock_hold_time while chain_plugin is initialized." );

   int16_t old_time = my->write_lock_hold_time;
   my->write_lock_hold_time = new_time;
   return old_time;
}

bool chain_plugin::block_is_on_preferred_chain(const steem::chain::block_id_type& block_id )
{
   // If it's not known, it's not preferred.
   if( !db().is_known_block(block_id) ) return false;

   // Extract the block number from block_id, and fetch that block number's ID from the database.
   // If the database's block ID matches block_id, then block_id is on the preferred chain. Otherwise, it's on a fork.
   return db().get_block_id_for_num( steem::chain::block_header::num_from_id( block_id ) ) == block_id;
}

void chain_plugin::check_time_in_block( const steem::chain::signed_block& block )
{
   time_point_sec now = fc::time_point::now();

   uint64_t max_accept_time = now.sec_since_epoch();
   max_accept_time += my->allow_future_time;
   FC_ASSERT( block.timestamp.sec_since_epoch() <= max_accept_time );
}

void chain_plugin::register_block_generator( const std::string& plugin_name, std::shared_ptr< abstract_block_producer > block_producer )
{
   FC_ASSERT( get_state() == appbase::abstract_plugin::state::initialized, "Can only register a block generator when the chain_plugin is initialized." );

   if ( my->block_generator )
      wlog( "Overriding a previously registered block generator by: ${registrant}", ("registrant", my->block_generator_registrant) );

   my->block_generator_registrant = plugin_name;
   my->block_generator = block_producer;
}

} } } // namespace steem::plugis::chain::chain_apis
