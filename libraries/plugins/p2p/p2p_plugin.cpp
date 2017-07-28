/*
 * Copyright (c) 2017, Respective Authors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <steemit/plugins/p2p/p2p_plugin.hpp>

#include <graphene/net/node.hpp>
#include <graphene/net/exceptions.hpp>

#include <steemit/chain/database_exceptions.hpp>

#include <fc/network/ip.hpp>
#include <fc/network/resolve.hpp>
#include <fc/thread/thread.hpp>

#include <boost/range/algorithm/reverse.hpp>
#include <boost/range/adaptor/reversed.hpp>

#include <boost/any.hpp>

using std::string;
using std::vector;

namespace steemit { namespace plugins { namespace p2p {

using appbase::app;

using graphene::net::item_hash_t;
using graphene::net::item_id;
using graphene::net::message;
using graphene::net::block_message;
using graphene::net::trx_message;

using steemit::protocol::block_header;
using steemit::protocol::signed_block_header;
using steemit::protocol::signed_block;
using steemit::protocol::block_id_type;

// This exists in p2p_plugin and http_plugin. It should be added to fc.
std::vector<fc::ip::endpoint> resolve_string_to_ip_endpoints( const std::string& endpoint_string )
{
   try
   {
      string::size_type colon_pos = endpoint_string.find( ':' );
      if( colon_pos == std::string::npos )
         FC_THROW( "Missing required port number in endpoint string \"${endpoint_string}\"",
                  ("endpoint_string", endpoint_string) );

      std::string port_string = endpoint_string.substr( colon_pos + 1 );

      try
      {
         uint16_t port = boost::lexical_cast< uint16_t >( port_string );
         std::string hostname = endpoint_string.substr( 0, colon_pos );
         std::vector< fc::ip::endpoint > endpoints = fc::resolve( hostname, port );

         if( endpoints.empty() )
            FC_THROW_EXCEPTION( fc::unknown_host_exception, "The host name can not be resolved: ${hostname}", ("hostname", hostname) );

         return endpoints;
      }
      catch( const boost::bad_lexical_cast& )
      {
         FC_THROW("Bad port: ${port}", ("port", port_string) );
      }
   }
   FC_CAPTURE_AND_RETHROW( (endpoint_string) )
}

class p2p_plugin_impl : public graphene::net::node_delegate
{
public:

   p2p_plugin_impl( plugins::chain::chain_plugin& c )
      : chain( c ) {}
   virtual ~p2p_plugin_impl() {}

   bool is_included_block(const block_id_type& block_id);
   steemit::chain::chain_id_type get_chain_id()const;

   // node_delegate interface
   virtual bool has_item( const graphene::net::item_id& ) override;
   virtual bool handle_block( const graphene::net::block_message&, bool, std::vector<fc::uint160_t>& ) override;
   virtual void handle_transaction( const graphene::net::trx_message& ) override;
   virtual void handle_message( const graphene::net::message& ) override;
   virtual std::vector< graphene::net::item_hash_t > get_block_ids( const std::vector< graphene::net::item_hash_t >&, uint32_t&, uint32_t ) override;
   virtual graphene::net::message get_item( const graphene::net::item_id& ) override;
   virtual std::vector< graphene::net::item_hash_t > get_blockchain_synopsis( const graphene::net::item_hash_t&, uint32_t ) override;
   virtual void sync_status( uint32_t, uint32_t ) override;
   virtual void connection_count_changed( uint32_t ) override;
   virtual uint32_t get_block_number( const graphene::net::item_hash_t& ) override;
   virtual fc::time_point_sec get_block_time(const graphene::net::item_hash_t& ) override;
   virtual fc::time_point_sec get_blockchain_now() override;
   virtual graphene::net::item_hash_t get_head_block_id() const override;
   virtual uint32_t estimate_last_known_fork_from_git_revision_timestamp( uint32_t ) const override;
   virtual void error_encountered( const std::string& message, const fc::oexception& error ) override;

   fc::optional<fc::ip::endpoint> endpoint;
   vector<fc::ip::endpoint> seeds;
   string user_agent;
   uint32_t max_connections = 0;

   std::unique_ptr<graphene::net::node> node;

   plugins::chain::chain_plugin& chain;
   uint32_t allow_future_time = 5;

   fc::thread p2p_thread;
};

p2p_plugin::p2p_plugin()
   : appbase::plugin< p2p_plugin >( P2P_PLUGIN_NAME )
{
   my.reset( new p2p_plugin_impl( appbase::app().get_plugin< plugins::chain::chain_plugin >() ) );
}

p2p_plugin::~p2p_plugin() {}

void p2p_plugin::set_program_options(bpo::options_description&, bpo::options_description& config_file_options) {
   config_file_options.add_options()
      ("p2p-endpoint", bpo::value<string>()->implicit_value("127.0.0.1:9876"), "The local IP address and port to listen for incoming connections.")
      ("p2p-max-connections", bpo::value<uint32_t>(), "Maxmimum number of incoming connections on P2P endpoint")
      ("seed-node", bpo::value<vector<string>>()->composing(), "The IP address and port of a remote peer to sync with. Deprecated in favor of p2p-seed-node.")
      ("p2p-seed-node", bpo::value<vector<string>>()->composing(), "The IP address and port of a remote peer to sync with.")
      ("p2p-user-agent", bpo::value<string>()->implicit_value("Graphene Reference Implementation"),"User agent to advertise to peers")
      ;
}

void p2p_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
   if( options.count( "p2p-endpoint" ) )
      my->endpoint = fc::ip::endpoint::from_string( options.at( "p2p-endpoint" ).as< string >() );

   if( options.count( "p2p-user-agent" ) )
      my->user_agent = options.at( "p2p-user-agent" ).as< string >();

   if( options.count( "p2p-max-connections" ) )
      my->max_connections = options.at( "p2p-max-connections" ).as< uint32_t >();

   if( options.count( "seed-node" ) || options.count( "p2p-seed-node" ) )
   {
      vector< string > seeds;
      if( options.count( "seed-node" ) )
      {
         wlog( "Option seed-node is deprecated in favor of p2p-seed-node" );
         auto s = options.at("seed-node").as<vector<string>>();
         seeds.insert( seeds.end(), s.begin(), s.end() );
      }

      if( options.count( "p2p-seed-node" ) )
      {
         auto s = options.at("p2p-seed-node").as<vector<string>>();
         seeds.insert( seeds.end(), s.begin(), s.end() );
      }

      for( const string& endpoint_string : seeds )
      {
         try
         {
            std::vector<fc::ip::endpoint> endpoints = resolve_string_to_ip_endpoints(endpoint_string);
            my->seeds.insert( my->seeds.end(), endpoints.begin(), endpoints.end() );
         }
         catch( const fc::exception& e )
         {
            wlog( "caught exception ${e} while adding seed node ${endpoint}",
               ("e", e.to_detail_string())("endpoint", endpoint_string) );
         }
      }
   }
}

void p2p_plugin::plugin_startup()
{
   my->p2p_thread.async( [this]
   {
      my->node.reset(new graphene::net::node(my->user_agent));
      my->node->load_configuration(app().data_dir() / "p2p");
      my->node->set_node_delegate( &(*my) );

      if( my->endpoint )
      {
         ilog("Configuring P2P to listen at ${ep}", ("ep", my->endpoint));
         my->node->listen_on_endpoint(*my->endpoint, true);
      }

      for( const auto& seed : my->seeds )
      {
         ilog("P2P adding seed node ${s}", ("s", seed));
         my->node->add_node(seed);
         my->node->connect_to_endpoint(seed);
      }

      if( my->max_connections )
      {
         ilog( "Setting p2p max connections to ${n}", ("n", my->max_connections) );
         fc::variant_object node_param = fc::variant_object(
            "maximum_number_of_connections",
            fc::variant( my->max_connections ) );
         my->node->set_advanced_node_parameters( node_param );
      }

      my->node->listen_to_p2p_network();
      my->node->connect_to_p2p_network();
      my->node->sync_from(graphene::net::item_id(graphene::net::block_message_type, my->chain.db().head_block_id()), std::vector<uint32_t>());
      ilog("P2P node listening at ${ep}", ("ep", my->node->get_actual_listening_endpoint()));
   }).wait();
   idump( (my->p2p_thread.is_running()) );
}

void p2p_plugin::plugin_shutdown() {
   ilog("Shutting down P2P Plugin");
   my->node->close();
   my->p2p_thread.quit();
   my->node.reset();
}

void p2p_plugin::broadcast_block( const steemit::protocol::signed_block& block )
{
   ulog("Broadcasting block #${n}", ("n", block.block_num()));
   my->node->broadcast( graphene::net::block_message( block ) );
}

void p2p_plugin::broadcast_transaction( const steemit::protocol::signed_transaction& tx )
{
   ulog("Broadcasting tx #${n}", ("id", tx.id()));
   my->node->broadcast( graphene::net::trx_message( tx ) );
}

////////////////////////////// Begin node_delegate Implementation //////////////////////////////
bool p2p_plugin_impl::has_item( const graphene::net::item_id& id )
{
   try
   {
      if( id.item_type == graphene::net::block_message_type )
         return chain.db().is_known_block(id.item_hash);
      else
         return chain.db().is_known_transaction(id.item_hash);
   }
   FC_CAPTURE_AND_RETHROW( (id) )
}

bool p2p_plugin_impl::handle_block( const graphene::net::block_message& blk_msg, bool sync_mode, std::vector<fc::uint160_t>& )
{
   uint32_t head_block_num;

   head_block_num = chain.db().head_block_num();

   if (sync_mode)
      fc_ilog(fc::logger::get("sync"),
            "chain pushing sync block #${block_num} ${block_hash}, head is ${head}",
            ("block_num", blk_msg.block.block_num())
            ("block_hash", blk_msg.block_id)
            ("head", head_block_num));
   else
      fc_ilog(fc::logger::get("sync"),
            "chain pushing block #${block_num} ${block_hash}, head is ${head}",
            ("block_num", blk_msg.block.block_num())
            ("block_hash", blk_msg.block_id)
            ("head", head_block_num));
   if (sync_mode && blk_msg.block.block_num() % 10000 == 0)
   {
      ilog("Syncing Blockchain --- Got block: #${n} time: ${t}",
         ("t",blk_msg.block.timestamp)
         ("n", blk_msg.block.block_num()) );
   }

   time_point_sec now = fc::time_point::now();

   uint64_t max_accept_time = now.sec_since_epoch();
   max_accept_time += allow_future_time;
   FC_ASSERT( blk_msg.block.timestamp.sec_since_epoch() <= max_accept_time );

   try {
      // TODO: in the case where this block is valid but on a fork that's too old for us to switch to,
      // you can help the network code out by throwing a block_older_than_undo_history exception.
      // when the net code sees that, it will stop trying to push blocks from that chain, but
      // leave that peer connected so that they can get sync blocks from us
      bool result = chain.db().push_block( blk_msg.block );

      if( !sync_mode )
      {
         fc::microseconds latency = fc::time_point::now() - blk_msg.block.timestamp;
         ilog( "Got ${t} transactions on block ${b} by ${w} -- latency: ${l} ms",
            ("t", blk_msg.block.transactions.size())
            ("b", blk_msg.block.block_num())
            ("w", blk_msg.block.witness)
            ("l", latency.count() / 1000) );
      }

      return result;
   } catch ( const steemit::chain::unlinkable_block_exception& e ) {
      // translate to a graphene::net exception
      fc_elog(fc::logger::get("sync"),
            "Error when pushing block, current head block is ${head}:\n${e}",
            ("e", e.to_detail_string())
            ("head", head_block_num));
      elog("Error when pushing block:\n${e}", ("e", e.to_detail_string()));
      FC_THROW_EXCEPTION(graphene::net::unlinkable_block_exception, "Error when pushing block:\n${e}", ("e", e.to_detail_string()));
   } catch( const fc::exception& e ) {
      fc_elog(fc::logger::get("sync"),
            "Error when pushing block, current head block is ${head}:\n${e}",
            ("e", e.to_detail_string())
            ("head", head_block_num));
      elog("Error when pushing block:\n${e}", ("e", e.to_detail_string()));
      throw;
   }

   return false;
}

void p2p_plugin_impl::handle_transaction( const graphene::net::trx_message& trx_msg )
{
   try
   {
      chain.db().push_transaction( trx_msg.trx );
   } FC_CAPTURE_AND_RETHROW( (trx_msg) )
}

void p2p_plugin_impl::handle_message( const graphene::net::message& message_to_process )
{
   // not a transaction, not a block
   FC_THROW( "Invalid Message Type" );
}

std::vector< graphene::net::item_hash_t > p2p_plugin_impl::get_block_ids( const std::vector< graphene::net::item_hash_t >& blockchain_synopsis, uint32_t& remaining_item_count, uint32_t limit )
{
   vector<block_id_type> result;
   remaining_item_count = 0;
   if( chain.db().head_block_num() == 0 )
      return result;

   result.reserve( limit );
   block_id_type last_known_block_id;

   if( blockchain_synopsis.empty()
         || ( blockchain_synopsis.size() == 1 && blockchain_synopsis[0] == block_id_type() ) )
   {
      // peer has sent us an empty synopsis meaning they have no blocks.
      // A bug in old versions would cause them to send a synopsis containing block 000000000
      // when they had an empty blockchain, so pretend they sent the right thing here.
      // do nothing, leave last_known_block_id set to zero
   }
   else
   {
      bool found_a_block_in_synopsis = false;

      for( const item_hash_t& block_id_in_synopsis : boost::adaptors::reverse(blockchain_synopsis) )
      {
         if (block_id_in_synopsis == block_id_type() ||
            (chain.db().is_known_block(block_id_in_synopsis) && is_included_block(block_id_in_synopsis)))
         {
            last_known_block_id = block_id_in_synopsis;
            found_a_block_in_synopsis = true;
            break;
         }
      }

      if (!found_a_block_in_synopsis)
         FC_THROW_EXCEPTION(graphene::net::peer_is_on_an_unreachable_fork, "Unable to provide a list of blocks starting at any of the blocks in peer's synopsis");
   }

   for( uint32_t num = block_header::num_from_id(last_known_block_id);
         num <= chain.db().head_block_num() && result.size() < limit;
         ++num )
   {
      if( num > 0 )
         result.push_back(chain.db().get_block_id_for_num(num));
   }

   if( !result.empty() && block_header::num_from_id(result.back()) < chain.db().head_block_num() )
      remaining_item_count = chain.db().head_block_num() - block_header::num_from_id(result.back());

   return result;
}

graphene::net::message p2p_plugin_impl::get_item( const graphene::net::item_id& id )
{
   if( id.item_type == graphene::net::block_message_type )
   {
      return chain.db().with_read_lock( [&]()
      {
         auto opt_block = chain.db().fetch_block_by_id(id.item_hash);
         if( !opt_block )
            elog("Couldn't find block ${id} -- corresponding ID in our chain is ${id2}",
               ("id", id.item_hash)("id2", chain.db().get_block_id_for_num(block_header::num_from_id(id.item_hash))));
         FC_ASSERT( opt_block.valid() );
         // ilog("Serving up block #${num}", ("num", opt_block->block_num()));
         return block_message(std::move(*opt_block));
      });
   }

   return trx_message( chain.db().get_recent_transaction( id.item_hash ) );
}

steemit::chain::chain_id_type p2p_plugin_impl::get_chain_id() const
{
   return STEEMIT_CHAIN_ID;
}

std::vector< graphene::net::item_hash_t > p2p_plugin_impl::get_blockchain_synopsis( const graphene::net::item_hash_t& reference_point, uint32_t number_of_blocks_after_reference_point )
{
   try {
   std::vector<item_hash_t> synopsis;
   chain.db().with_read_lock( [&]()
   {
      synopsis.reserve(30);
      uint32_t high_block_num;
      uint32_t non_fork_high_block_num;
      uint32_t low_block_num = chain.db().last_non_undoable_block_num();
      std::vector<block_id_type> fork_history;

      if (reference_point != item_hash_t())
      {
         // the node is asking for a summary of the block chain up to a specified
         // block, which may or may not be on a fork
         // for now, assume it's not on a fork
         if (is_included_block(reference_point))
         {
            // reference_point is a block we know about and is on the main chain
            uint32_t reference_point_block_num = block_header::num_from_id(reference_point);
            assert(reference_point_block_num > 0);
            high_block_num = reference_point_block_num;
            non_fork_high_block_num = high_block_num;

            if (reference_point_block_num < low_block_num)
            {
               // we're on the same fork (at least as far as reference_point) but we've passed
               // reference point and could no longer undo that far if we diverged after that
               // block.  This should probably only happen due to a race condition where
               // the network thread calls this function, and then immediately pushes a bunch of blocks,
               // then the main thread finally processes this function.
               // with the current framework, there's not much we can do to tell the network
               // thread what our current head block is, so we'll just pretend that
               // our head is actually the reference point.
               // this *may* enable us to fetch blocks that we're unable to push, but that should
               // be a rare case (and correctly handled)
               low_block_num = reference_point_block_num;
            }
         }
         else
         {
            // block is a block we know about, but it is on a fork
            try
            {
               fork_history = chain.db().get_block_ids_on_fork(reference_point);
               // returns a vector where the last element is the common ancestor with the preferred chain,
               // and the first element is the reference point you passed in
               assert(fork_history.size() >= 2);

               if( fork_history.front() != reference_point )
               {
                  edump( (fork_history)(reference_point) );
                  assert(fork_history.front() == reference_point);
               }
               block_id_type last_non_fork_block = fork_history.back();
               fork_history.pop_back();  // remove the common ancestor
               boost::reverse(fork_history);

               if (last_non_fork_block == block_id_type()) // if the fork goes all the way back to genesis (does graphene's fork db allow this?)
                  non_fork_high_block_num = 0;
               else
                  non_fork_high_block_num = block_header::num_from_id(last_non_fork_block);

               high_block_num = non_fork_high_block_num + fork_history.size();
               assert(high_block_num == block_header::num_from_id(fork_history.back()));
            }
            catch (const fc::exception& e)
            {
               // unable to get fork history for some reason.  maybe not linked?
               // we can't return a synopsis of its chain
               elog("Unable to construct a blockchain synopsis for reference hash ${hash}: ${exception}", ("hash", reference_point)("exception", e));
               throw;
            }
            if (non_fork_high_block_num < low_block_num)
            {
               wlog("Unable to generate a usable synopsis because the peer we're generating it for forked too long ago "
                  "(our chains diverge after block #${non_fork_high_block_num} but only undoable to block #${low_block_num})",
                  ("low_block_num", low_block_num)
                  ("non_fork_high_block_num", non_fork_high_block_num));
               FC_THROW_EXCEPTION(graphene::net::block_older_than_undo_history, "Peer is are on a fork I'm unable to switch to");
            }
         }
      }
      else
      {
         // no reference point specified, summarize the whole block chain
         high_block_num = chain.db().head_block_num();
         non_fork_high_block_num = high_block_num;
         if (high_block_num == 0)
            return; // we have no blocks
      }

      if( low_block_num == 0)
         low_block_num = 1;

      // at this point:
      // low_block_num is the block before the first block we can undo,
      // non_fork_high_block_num is the block before the fork (if the peer is on a fork, or otherwise it is the same as high_block_num)
      // high_block_num is the block number of the reference block, or the end of the chain if no reference provided

      // true_high_block_num is the ending block number after the network code appends any item ids it
      // knows about that we don't
      uint32_t true_high_block_num = high_block_num + number_of_blocks_after_reference_point;
      do
      {
         // for each block in the synopsis, figure out where to pull the block id from.
         // if it's <= non_fork_high_block_num, we grab it from the main blockchain;
         // if it's not, we pull it from the fork history
         if( low_block_num <= non_fork_high_block_num )
            synopsis.push_back(chain.db().get_block_id_for_num(low_block_num));
         else
            synopsis.push_back(fork_history[low_block_num - non_fork_high_block_num - 1]);
         low_block_num += (true_high_block_num - low_block_num + 2) / 2;
      }
      while( low_block_num <= high_block_num );

      //idump((synopsis));
      return;
   });

   return synopsis;
} FC_CAPTURE_AND_RETHROW() }

void p2p_plugin_impl::sync_status( uint32_t item_type, uint32_t item_count )
{
   // any status reports to GUI go here
}

void p2p_plugin_impl::connection_count_changed( uint32_t c )
{
   // any status reports to GUI go here
}

uint32_t p2p_plugin_impl::get_block_number( const graphene::net::item_hash_t& block_id )
{
   try {
   return block_header::num_from_id(block_id);
} FC_CAPTURE_AND_RETHROW( (block_id) ) }

fc::time_point_sec p2p_plugin_impl::get_block_time( const graphene::net::item_hash_t& block_id )
{
   try
   {
      return chain.db().with_read_lock( [&]()
      {
         auto opt_block = chain.db().fetch_block_by_id( block_id );
         if( opt_block.valid() ) return opt_block->timestamp;
         return fc::time_point_sec::min();
      });
   } FC_CAPTURE_AND_RETHROW( (block_id) )
}

graphene::net::item_hash_t p2p_plugin_impl::get_head_block_id() const
{
   return chain.db().with_read_lock( [&]()
   {
      return chain.db().head_block_id();
   });
}

uint32_t p2p_plugin_impl::estimate_last_known_fork_from_git_revision_timestamp(uint32_t) const
{
   return 0; // there are no forks in graphene
}

void p2p_plugin_impl::error_encountered( const string& message, const fc::oexception& error )
{
   // notify GUI or something cool
}

fc::time_point_sec p2p_plugin_impl::get_blockchain_now()
{
   return fc::time_point::now();
}

bool p2p_plugin_impl::is_included_block(const block_id_type& block_id)
{
   return chain.db().with_read_lock( [&]()
   {
      uint32_t block_num = block_header::num_from_id(block_id);
      block_id_type block_id_in_preferred_chain = chain.db().get_block_id_for_num(block_num);
      return block_id == block_id_in_preferred_chain;
   });
}

////////////////////////////// End node_delegate Implementation //////////////////////////////

} } } // namespace steemit::plugins::p2p
