#include <steemit/chain/database_exceptions.hpp>

#include <steemit/plugins/chain/chain_plugin.hpp>

#include <fc/io/json.hpp>
#include <fc/string.hpp>

#include <iostream>

namespace steemit { namespace plugins { namespace chain {

using namespace steemit;
using fc::flat_map;
using steemit::chain::block_id_type;

class chain_plugin_impl
{
   public:
      uint64_t                         shared_memory_size = 0;
      bfs::path                        shared_memory_dir;
      bool                             replay = false;
      bool                             resync   = false;
      bool                             readonly = false;
      bool                             check_locks = false;
      uint32_t                         flush_interval = 0;
      flat_map<uint32_t,block_id_type> loaded_checkpoints;

      uint32_t allow_future_time = 5;

      database  db;
};


chain_plugin::chain_plugin() : my( new chain_plugin_impl() ) {}
chain_plugin::~chain_plugin(){}

database& chain_plugin::db() { return my->db; }
const steemit::chain::database& chain_plugin::db() const { return my->db; }

void chain_plugin::set_program_options(options_description& cli, options_description& cfg)
{
   cfg.add_options()
         ("shared-file-dir", bpo::value<bfs::path>()->default_value("blockchain"),
            "the location of the chain shared memory files (absolute path or relative to application data dir)")
         ("shared-file-size", bpo::value<string>()->default_value("54G"), "Size of the shared memory file. Default: 54G")
         ("checkpoint,c", bpo::value<vector<string>>()->composing(), "Pairs of [BLOCK_NUM,BLOCK_ID] that should be enforced as checkpoints.")
         ("flush-state-interval", bpo::value<uint32_t>()->default_value(0),
            "flush shared memory changes to disk every N blocks")
         ;
   cli.add_options()
         ("replay-blockchain", bpo::bool_switch()->default_value(false), "clear chain database and replay all blocks" )
         ("resync-blockchain", bpo::bool_switch()->default_value(false), "clear chain database and block log" )
         ("check-locks", bpo::bool_switch()->default_value(false), "Check correctness of chainbase locking" )
         ;
}

void chain_plugin::plugin_initialize(const variables_map& options) {
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

   my->replay           = options.at( "replay-blockchain").as<bool>();
   my->resync            = options.at( "resync-blockchain").as<bool>();
   my->check_locks      = options.at( "check-locks" ).as< bool >();
   my->flush_interval   = options.at( "flush-state-interval" ).as<uint32_t>();

   if(options.count("checkpoint"))
   {
      auto cps = options.at("checkpoint").as<vector<string>>();
      my->loaded_checkpoints.reserve(cps.size());
      for(auto cp : cps)
      {
         auto item = fc::json::from_string(cp).as<std::pair<uint32_t,block_id_type>>();
         my->loaded_checkpoints[item.first] = item.second;
      }
   }
}

void chain_plugin::plugin_startup()
{
   ilog( "Starting chain with shared_file_size: ${n} bytes", ("n", my->shared_memory_size) );

   if(my->resync)
   {
      wlog("resync requested: deleting block log and shared memory");
      my->db.wipe( app().data_dir() / "blockchain", my->shared_memory_dir, true );
   }

   my->db.set_flush_interval( my->flush_interval );
   my->db.add_checkpoints( my->loaded_checkpoints );
   my->db.set_require_locking( my->check_locks );


   if(my->replay)
   {
      ilog("Replaying blockchain on user request.");
      my->db.reindex( app().data_dir() / "blockchain", my->shared_memory_dir, my->shared_memory_size );
   }
   else
   {
      try
      {
         ilog("Opening shared memory from ${path}", ("path",my->shared_memory_dir.generic_string()));
         my->db.open( app().data_dir() / "blockchain", my->shared_memory_dir, 0, my->shared_memory_size, chainbase::database::read_write );
      }
      catch( const fc::exception& e )
      {
         wlog("Error opening database, attempting to replay blockchain. Error: ${e}", ("e", e));

         try
         {
            my->db.reindex( app().data_dir() / "blockchain", my->shared_memory_dir, my->shared_memory_size );
         }
         catch( steemit::chain::block_log_exception& )
         {
            wlog( "Error opening block log. Having to resync from network..." );
            my->db.open( app().data_dir() / "blockchain", my->shared_memory_dir, 0, my->shared_memory_size, chainbase::database::read_write );
         }
      }
   }
}

void chain_plugin::plugin_shutdown()
{
   ilog("closing chain database");
   my->db.close();
   ilog("database closed successfully");
}

bool chain_plugin::accept_block( const steemit::chain::signed_block& block, bool currently_syncing, uint32_t skip )
{
   if (currently_syncing && block.block_num() % 10000 == 0) {
      ilog("Syncing Blockchain --- Got block: #${n} time: ${t} producer: ${p}",
           ("t", block.timestamp)
           ("n", block.block_num()) );
   }

   check_time_in_block( block );

   return db().push_block(block, skip);
}

void chain_plugin::accept_transaction( const steemit::chain::signed_transaction& trx )
{
   db().push_transaction(trx);
}

bool chain_plugin::block_is_on_preferred_chain(const steemit::chain::block_id_type& block_id )
{
   // If it's not known, it's not preferred.
   if( !db().is_known_block(block_id) ) return false;

   // Extract the block number from block_id, and fetch that block number's ID from the database.
   // If the database's block ID matches block_id, then block_id is on the preferred chain. Otherwise, it's on a fork.
   return db().get_block_id_for_num( steemit::chain::block_header::num_from_id( block_id ) ) == block_id;
}

void chain_plugin::check_time_in_block( const steemit::chain::signed_block& block )
{
   time_point_sec now = fc::time_point::now();

   uint64_t max_accept_time = now.sec_since_epoch();
   max_accept_time += my->allow_future_time;
   FC_ASSERT( block.timestamp.sec_since_epoch() <= max_accept_time );
}

} } } // namespace steemit::plugis::chain::chain_apis
