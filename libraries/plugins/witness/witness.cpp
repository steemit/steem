/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
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
#include <steemit/witness/witness.hpp>

#include <steemit/chain/database.hpp>
#include <steemit/chain/steem_objects.hpp>
#include <graphene/time/time.hpp>

#include <graphene/utilities/key_conversion.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>

#include <iostream>

using namespace steemit::witness_plugin;
using std::string;
using std::vector;

namespace bpo = boost::program_options;

void new_chain_banner( const steemit::chain::database& db )
{
   std::cerr << "\n"
      "********************************\n"
      "*                              *\n"
      "*   ------- NEW CHAIN ------   *\n"
      "*   -   Welcome to Steem!  -   *\n"
      "*   ------------------------   *\n"
      "*                              *\n"
      "********************************\n"
      "\n";
   if( db.get_slot_at_time( graphene::time::now() ) > 200 )
   {
      std::cerr << "Your genesis seems to have an old timestamp\n"
         "Please consider using the --genesis-timestamp option to give your genesis a recent timestamp\n"
         "\n"
         ;
   }
   return;
}

void witness_plugin::plugin_set_program_options(
   boost::program_options::options_description& command_line_options,
   boost::program_options::options_description& config_file_options)
{
   string witness_id_example = "initwitness";
   command_line_options.add_options()
         ("enable-stale-production", bpo::bool_switch()->notifier([this](bool e){_production_enabled = e;}), "Enable block production, even if the chain is stale.")
         ("required-participation", bpo::bool_switch()->notifier([this](int e){_required_witness_participation = uint32_t(e*STEEMIT_1_PERCENT);}), "Percent of witnesses (0-99) that must be participating in order to produce blocks")
         ("witness,w", bpo::value<vector<string>>()->composing()->multitoken(),
          ("name of witness controlled by this node (e.g. " + witness_id_example+" )" ).c_str())
         ("miner,m", bpo::value<vector<string>>()->composing()->multitoken(), "name of miner and its private key (e.g. [\"account\",\"WIF PRIVATE KEY\"] )" )
         ("mining-threads,t", bpo::value<uint32_t>(),"Number of threads to use for proof of work mining" )
         ("private-key", bpo::value<vector<string>>()->composing()->multitoken(), "WIF PRIVATE KEY to be used by one or more witnesses or miners" )
         ("miner-account-creation-fee", bpo::value<uint64_t>()->implicit_value(100000),"Account creation fee to be voted on upon successful POW - Minimum fee is 100.000 STEEM (written as 100000)")
         ("miner-maximum-block-size", bpo::value<uint32_t>()->implicit_value(131072),"Maximum block size (in bytes) to be voted on upon successful POW - Max block size must be between 128 KB and 750 MB")
         ("miner-sbd-interest-rate", bpo::value<uint32_t>()->implicit_value(1000),"SBD interest rate to be vote on upon successful POW - Default interest rate is 10% (written as 1000)")
         ;
   config_file_options.add(command_line_options);
}

std::string witness_plugin::plugin_name()const
{
   return "witness";
}

using std::vector;
using std::pair;
using std::string;

void witness_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{ try {
   ilog("witness plugin:  plugin_initialize() begin");
   _options = &options;
   LOAD_VALUE_SET(options, "witness", _witnesses, string)
   edump((_witnesses));

   if( options.count("miner") ) {

      const vector<string> miner_to_wif_pair_strings = options["miner"].as<vector<string>>();
      for( auto p : miner_to_wif_pair_strings )
      {
         auto m = steemit::app::dejsonify<pair<string,string>>(p);
         idump((m));

         fc::optional<fc::ecc::private_key> private_key = graphene::utilities::wif_to_key(m.second);
         FC_ASSERT( private_key.valid(), "unable to parse private key" );
         _private_keys[private_key->get_public_key()] = *private_key;
         _miners[m.first] = private_key->get_public_key();
      }
   }

   if( options.count("mining-threads") )
   {
      _mining_threads = std::min( options["mining-threads"].as<uint32_t>(), uint32_t(64) );
      _thread_pool.resize( _mining_threads );
      for( uint32_t i = 0; i < _mining_threads; ++i )
         _thread_pool[i] = std::make_shared<fc::thread>();
   }

   if( options.count("private-key") )
   {
      const std::vector<std::string> keys = options["private-key"].as<std::vector<std::string>>();
      for (const std::string& wif_key : keys )
      {
         fc::optional<fc::ecc::private_key> private_key = graphene::utilities::wif_to_key(wif_key);
         FC_ASSERT( private_key.valid(), "unable to parse private key" );
         _private_keys[private_key->get_public_key()] = *private_key;
      }
   }

   if( options.count("miner-account-creation-fee") )
   {
      const uint64_t account_creation_fee = options["miner-account-creation-fee"].as<uint64_t>();

      if( account_creation_fee < STEEMIT_MIN_ACCOUNT_CREATION_FEE )
         wlog( "miner-account-creation-fee is below the minimum fee, using minimum instead" );
      else
         _miner_prop_vote.account_creation_fee.amount = account_creation_fee;
   }

   if( options.count( "miner-maximum-block-size" ) )
   {
      const uint32_t maximum_block_size = options["miner-maximum-block-size"].as<uint32_t>();

      if( maximum_block_size < STEEMIT_MIN_BLOCK_SIZE_LIMIT )
         wlog( "miner-maximum-block-size is below the minimum block size limit, using minimum of 128 KB instead" );
      else if ( maximum_block_size > STEEMIT_MAX_BLOCK_SIZE )
      {
         wlog( "miner-maximum-block-size is above the maximum block size limit, using maximum of 750 MB instead" );
         _miner_prop_vote.maximum_block_size = STEEMIT_MAX_BLOCK_SIZE;
      }
      else
         _miner_prop_vote.maximum_block_size = maximum_block_size;
   }

   if( options.count( "miner-sbd-interest-rate" ) )
   {
      _miner_prop_vote.sbd_interest_rate = options["miner-sbd-interest-rate"].as<uint32_t>();
   }

   ilog("witness plugin:  plugin_initialize() end");
} FC_LOG_AND_RETHROW() }

void witness_plugin::plugin_startup()
{ try {
   ilog("witness plugin:  plugin_startup() begin");
   chain::database& d = database();
   //Start NTP time client
   graphene::time::now();

   if( !_witnesses.empty() )
   {
      ilog("Launching block production for ${n} witnesses.", ("n", _witnesses.size()));
      app().set_block_production(true);
      if( _production_enabled )
      {
         if( d.head_block_num() == 0 )
            new_chain_banner(d);
         _production_skip_flags |= steemit::chain::database::skip_undo_history_check;
      }
      schedule_production_loop();
   } else
      elog("No witnesses configured! Please add witness names and private keys to configuration.");
   if( !_miners.empty() )
   {
      ilog("Starting mining...");
      d.applied_block.connect( [this]( const chain::signed_block& b ){ this->on_applied_block(b); } );
   }
   else
   {
      elog("No miners configured! Please add miner names and private keys to configuration.");
   }
   ilog("witness plugin:  plugin_startup() end");
} FC_CAPTURE_AND_RETHROW() }

void witness_plugin::plugin_shutdown()
{
   graphene::time::shutdown_ntp_time();
   if( !_miners.empty() )
   {
      ilog( "shutting downing mining threads" );
      _thread_pool.clear();
   }
   return;
}

void witness_plugin::schedule_production_loop()
{
   //Schedule for the next second's tick regardless of chain state
   // If we would wait less than 50ms, wait for the whole second.
   fc::time_point ntp_now = graphene::time::now();
   fc::time_point fc_now = fc::time_point::now();
   int64_t time_to_next_second = 1000000 - (ntp_now.time_since_epoch().count() % 1000000);
   if( time_to_next_second < 50000 )      // we must sleep for at least 50ms
       time_to_next_second += 1000000;

   fc::time_point next_wakeup( fc_now + fc::microseconds( time_to_next_second ) );

   //wdump( (now.time_since_epoch().count())(next_wakeup.time_since_epoch().count()) );
   _block_production_task = fc::schedule([this]{block_production_loop();},
                                         next_wakeup, "Witness Block Production");
}

block_production_condition::block_production_condition_enum witness_plugin::block_production_loop()
{
   if( fc::time_point::now() < fc::time_point(STEEMIT_GENESIS_TIME) )
   {
      wlog( "waiting until genesis time to produce block: ${t}", ("t",STEEMIT_GENESIS_TIME) );
      schedule_production_loop();
      return block_production_condition::wait_for_genesis;
   }

   block_production_condition::block_production_condition_enum result;
   fc::mutable_variant_object capture;
   try
   {
      result = maybe_produce_block(capture);
   }
   catch( const fc::canceled_exception& )
   {
      //We're trying to exit. Go ahead and let this one out.
      throw;
   }
   catch( const steemit::chain::unknown_hardfork_exception& e )
   {
      // Hit a hardfork that the current node know nothing about, stop production and inform user
      elog( "${e}\nNode may be out of date...", ("e", e.to_detail_string()) );
      throw;
   }
   catch( const fc::exception& e )
   {
      elog("Got exception while generating block:\n${e}", ("e", e.to_detail_string()));
      result = block_production_condition::exception_producing_block;
   }

   switch( result )
   {
      case block_production_condition::produced:
         ilog("Generated block #${n} with timestamp ${t} at time ${c} by ${w}", (capture));
         break;
      case block_production_condition::not_synced:
         //ilog("Not producing block because production is disabled until we receive a recent block (see: --enable-stale-production)");
         break;
      case block_production_condition::not_my_turn:
         //ilog("Not producing block because it isn't my turn");
         break;
      case block_production_condition::not_time_yet:
         // ilog("Not producing block because slot has not yet arrived");
         break;
      case block_production_condition::no_private_key:
         ilog("Not producing block for ${w} because I don't have the private key for ${scheduled_key}", (capture) );
         break;
      case block_production_condition::low_participation:
         elog("Not producing block because node appears to be on a minority fork with only ${pct}% witness participation", (capture) );
         break;
      case block_production_condition::lag:
         elog("Not producing block because node didn't wake up within 500ms of the slot time.");
         break;
      case block_production_condition::consecutive:
         elog("Not producing block because the last block was generated by the same witness.\nThis node is probably disconnected from the network so block production has been disabled.\nDisable this check with --allow-consecutive option.");
         break;
      case block_production_condition::exception_producing_block:
         elog("Failure when producing block with no transactions");
         break;
      case block_production_condition::wait_for_genesis:
         break;
   }

   schedule_production_loop();
   return result;
}

block_production_condition::block_production_condition_enum witness_plugin::maybe_produce_block( fc::mutable_variant_object& capture )
{
   chain::database& db = database();
   fc::time_point now_fine = graphene::time::now();
   fc::time_point_sec now = now_fine + fc::microseconds( 500000 );

   // If the next block production opportunity is in the present or future, we're synced.
   if( !_production_enabled )
   {
      if( db.get_slot_time(1) >= now )
         _production_enabled = true;
      else
         return block_production_condition::not_synced;
   }

   // is anyone scheduled to produce now or one second in the future?
   uint32_t slot = db.get_slot_at_time( now );
   if( slot == 0 )
   {
      capture("next_time", db.get_slot_time(1));
      return block_production_condition::not_time_yet;
   }

   //
   // this assert should not fail, because now <= db.head_block_time()
   // should have resulted in slot == 0.
   //
   // if this assert triggers, there is a serious bug in get_slot_at_time()
   // which would result in allowing a later block to have a timestamp
   // less than or equal to the previous block
   //
   assert( now > db.head_block_time() );

   string scheduled_witness = db.get_scheduled_witness( slot );
   // we must control the witness scheduled to produce the next block.
   if( _witnesses.find( scheduled_witness ) == _witnesses.end() )
   {
      capture("scheduled_witness", scheduled_witness);
      return block_production_condition::not_my_turn;
   }

   const auto& witness_by_name = db.get_index_type< chain::witness_index >().indices().get< chain::by_name >();
   auto itr = witness_by_name.find( scheduled_witness );

   fc::time_point_sec scheduled_time = db.get_slot_time( slot );
   steemit::chain::public_key_type scheduled_key = itr->signing_key;
   auto private_key_itr = _private_keys.find( scheduled_key );

   if( private_key_itr == _private_keys.end() )
   {
      capture("scheduled_key", scheduled_key);
      return block_production_condition::no_private_key;
   }

   uint32_t prate = db.witness_participation_rate();
   if( prate < _required_witness_participation )
   {
      capture("pct", uint32_t(100*uint64_t(prate) / STEEMIT_1_PERCENT));
      return block_production_condition::low_participation;
   }

   if( llabs((scheduled_time - now).count()) > fc::milliseconds( 500 ).count() )
   {
      capture("scheduled_time", scheduled_time)("now", now);
      return block_production_condition::lag;
   }

   int retry = 0;
   do
   {
      try
      {
      auto block = db.generate_block(
         scheduled_time,
         scheduled_witness,
         private_key_itr->second,
         _production_skip_flags
         );
         capture("n", block.block_num())("t", block.timestamp)("c", now)("w",scheduled_witness);
         fc::async( [this,block](){ p2p_node().broadcast(graphene::net::block_message(block)); } );

         return block_production_condition::produced;
      }
      catch( fc::exception& e )
      {
         elog( "${e}", ("e",e.to_detail_string()) );
         elog( "Clearing pending transactions and attempting again" );
         db.clear_pending();
         retry++;
      }
   } while( retry < 2 );

   return block_production_condition::exception_producing_block;
}

/**
 * Every time a block is produced, this method is called. This method will iterate through all
 * mining accounts specified by commandline and for which the private key is known. The first
 * account that isn't already scheduled in the mining queue is selected to mine for the
 * BLOCK_INTERVAL minus 1 second. If a POW is solved or a a new block comes in then the
 * worker will stop early.
 *
 * Work is farmed out to N threads in parallel based upon the value specified on the command line.
 *
 * The miner assumes that the next block will be produced on time and that network propagation
 * will take at least 1 second. This 1 second consists of the time it took to receive the block
 * and how long it will take to broadcast the work. In other words, we assume 0.5s broadcast times
 * and therefore do not even attempt work that cannot be delivered on time.
 */
void witness_plugin::on_applied_block( const chain::signed_block& b )
{ try {
  if( !_mining_threads || _miners.size() == 0 ) return;
  chain::database& db = database();

   const auto& dgp = db.get_dynamic_global_properties();
   double hps   = (_total_hashes*1000000)/(fc::time_point::now()-_hash_start_time).count();
   int64_t bits    = (dgp.num_pow_witnesses/4) + 4;
   fc::uint128 hashes  = fc::uint128(1) << bits;
   hashes *= 1000000;
   hps += 1;
   hashes /= int64_t(hps*1000000);
   auto seconds = hashes.to_uint64();
   //double seconds = hashes/hps;
   auto minutes = uint64_t(seconds / 60.0);


   if( uint64_t(hps) > 0 )
      ilog( "hash rate: ${x} hps  target: ${t} queue: ${l} estimated time to produce: ${m} minutes",
              ("x",uint64_t(hps)) ("t",bits) ("m", minutes ) ("l",dgp.num_pow_witnesses)
         );


  _head_block_num = b.block_num();
  /// save these variables to be captured by worker lambda

  for( const auto& miner : _miners ) {
    const auto* w = db.find_witness( miner.first );
    if( !w || w->pow_worker == 0 ) {
       auto miner_pub_key = miner.second; //a.active.key_auths.begin()->first;
       auto priv_key_itr = _private_keys.find(miner_pub_key);
       if( priv_key_itr == _private_keys.end() ) {
          continue; /// skipping miner for lack of private key
       }

       auto miner_priv_key = priv_key_itr->second;
       start_mining( miner_pub_key, priv_key_itr->second, miner.first, b );
       break;
    } else {
        // ilog( "Skipping miner ${m} because it is already scheduled to produce a block", ("m",miner) );
    }
  } // for miner in miners

} catch ( const fc::exception& e ) { ilog( "exception thrown while attempting to mine" ); }
}

void witness_plugin::start_mining( const fc::ecc::public_key& pub, const fc::ecc::private_key& pk,
                            const string& miner, const steemit::chain::signed_block& b ) {

    static uint64_t seed = fc::time_point::now().time_since_epoch().count();
    static uint64_t start = fc::city_hash64( (const char*)&seed, sizeof(seed) );
    chain::database& db = database();

    auto head_block_num  = b.block_num();
    auto head_block_time = b.timestamp;
    auto block_id = b.id();


    auto target = db.get_pow_target();
    fc::thread* mainthread = &fc::thread::current();

    _total_hashes = 0;
    _hash_start_time = fc::time_point::now();

    auto stop = head_block_time + fc::seconds( STEEMIT_BLOCK_INTERVAL * 2 );

    uint32_t thread_num = 0;
    uint32_t num_threads = _mining_threads;
    for( auto& t : _thread_pool ) {
       t->async( [=](){
          chain::pow_operation op;
          op.block_id = block_id;
          op.worker_account = miner;
          op.work.worker = pub;
          op.nonce = start + thread_num;
          op.props = _miner_prop_vote;
          while( true )
          {
          //  if( ((op.nonce/num_threads) % 1000) == 0 ) idump((op.nonce));
            if( graphene::time::nonblocking_now() > stop )
            {
           //     ilog( "stop mining due to time out, nonce: ${n}", ("n",op.nonce) );
                return;
            }
            if( this->_head_block_num != head_block_num )
            {
           //     wlog( "stop mining due new block arrival, nonce: ${n}", ("n",op.nonce));
                return;
            }
            ++this->_total_hashes;

            op.nonce += num_threads;
            op.work.create( pk, op.work_input() );
            if( op.work.work < target )
            {
               ++this->_head_block_num; /// signal other workers to stop

               chain::signed_transaction trx;
               trx.operations.push_back(op);
               trx.ref_block_num = head_block_num;
               trx.ref_block_prefix = op.block_id._hash[1];
               trx.set_expiration( head_block_time + STEEMIT_MAX_TIME_UNTIL_EXPIRATION );
               mainthread->async( [this,miner,trx](){
                  try {
                     database().push_transaction( trx );
                     ilog( "Broadcasting Proof of Work for ${miner}", ("miner",miner) );
                     p2p_node().broadcast( graphene::net::trx_message(trx) );
                  } catch ( const fc::exception& e ) {
                  //   wdump((e.to_detail_string()));
                  }
               });
               return;
            }
          }
       });
       ++thread_num;
    }
}

STEEMIT_DEFINE_PLUGIN( witness, steemit::witness_plugin::witness_plugin )
