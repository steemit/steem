#include <steem/plugins/witness/block_producer.hpp>

#include <steem/protocol/base.hpp>
#include <steem/protocol/config.hpp>
#include <steem/protocol/version.hpp>

#include <steem/chain/database_exceptions.hpp>
#include <steem/chain/db_with.hpp>
#include <steem/chain/pending_required_action_object.hpp>
#include <steem/chain/pending_optional_action_object.hpp>
#include <steem/chain/witness_objects.hpp>

#include <fc/macros.hpp>

namespace steem { namespace plugins { namespace witness {

chain::signed_block block_producer::generate_block(fc::time_point_sec when, const chain::account_name_type& witness_owner, const fc::ecc::private_key& block_signing_private_key, uint32_t skip)
{
   chain::signed_block result;
   steem::chain::detail::with_skip_flags(
      _db,
      skip,
      [&]()
      {
         try
         {
            result = _generate_block( when, witness_owner, block_signing_private_key );
         }
         FC_CAPTURE_AND_RETHROW( (witness_owner) )
      });
   return result;
}

chain::signed_block block_producer::_generate_block(fc::time_point_sec when, const chain::account_name_type& witness_owner, const fc::ecc::private_key& block_signing_private_key)
{
   uint32_t skip = _db.get_node_properties().skip_flags;
   uint32_t slot_num = _db.get_slot_at_time( when );
   FC_ASSERT( slot_num > 0 );
   string scheduled_witness = _db.get_scheduled_witness( slot_num );
   FC_ASSERT( scheduled_witness == witness_owner );

   const auto& witness_obj = _db.get_witness( witness_owner );

   if( !(skip & chain::database::skip_witness_signature) )
      FC_ASSERT( witness_obj.signing_key == block_signing_private_key.get_public_key() );

   chain::signed_block pending_block;

   pending_block.previous = _db.head_block_id();
   pending_block.timestamp = when;
   pending_block.witness = witness_owner;

   adjust_hardfork_version_vote( _db.get_witness( witness_owner ), pending_block );

   apply_pending_transactions( witness_owner, when, pending_block );

   // We have temporarily broken the invariant that
   // _pending_tx_session is the result of applying _pending_tx, as
   // _pending_tx now consists of the set of postponed transactions.
   // However, the push_block() call below will re-create the
   // _pending_tx_session.

   if( !(skip & chain::database::skip_witness_signature) )
      pending_block.sign( block_signing_private_key, _db.has_hardfork( STEEM_HARDFORK_0_20__1944 ) ? fc::ecc::bip_0062 : fc::ecc::fc_canonical );

   // TODO:  Move this to _push_block() so session is restored.
   if( !(skip & chain::database::skip_block_size_check) )
   {
      FC_ASSERT( fc::raw::pack_size(pending_block) <= STEEM_MAX_BLOCK_SIZE );
   }

   _db.push_block( pending_block, skip );

   return pending_block;
}

void block_producer::adjust_hardfork_version_vote(const chain::witness_object& witness, chain::signed_block& pending_block)
{
   using namespace steem::protocol;

   if( witness.running_version != STEEM_BLOCKCHAIN_VERSION )
      pending_block.extensions.insert( block_header_extensions( STEEM_BLOCKCHAIN_VERSION ) );

   const auto& hfp = _db.get_hardfork_property_object();

   if( hfp.current_hardfork_version < STEEM_BLOCKCHAIN_VERSION // Binary is newer hardfork than has been applied
      && ( witness.hardfork_version_vote != hfp.next_hardfork || witness.hardfork_time_vote != hfp.next_hardfork_time ) ) // Witness vote does not match binary configuration
   {
      // Make vote match binary configuration
      pending_block.extensions.insert( block_header_extensions( hardfork_version_vote( hfp.next_hardfork, hfp.next_hardfork_time ) ) );
   }
   else if( hfp.current_hardfork_version == STEEM_BLOCKCHAIN_VERSION // Binary does not know of a new hardfork
            && witness.hardfork_version_vote > STEEM_BLOCKCHAIN_VERSION ) // Voting for hardfork in the future, that we do not know of...
   {
      // Make vote match binary configuration. This is vote to not apply the new hardfork.
      pending_block.extensions.insert( block_header_extensions( hardfork_version_vote( hfp.current_hardfork_version, hfp.processed_hardforks.back() ) ) );
   }
}

void block_producer::apply_pending_transactions(
        const chain::account_name_type& witness_owner,
        fc::time_point_sec when,
        chain::signed_block& pending_block)
{
   // The 4 is for the max size of the transaction vector length
   size_t total_block_size = fc::raw::pack_size( pending_block ) + 4;
   const auto& gpo = _db.get_dynamic_global_properties();
   uint64_t maximum_block_size = gpo.maximum_block_size; //STEEM_MAX_BLOCK_SIZE;
   uint64_t maximum_transaction_partition_size = maximum_block_size -  ( maximum_block_size * gpo.required_actions_partition_percent ) / STEEM_100_PERCENT;

   //
   // The following code throws away existing pending_tx_session and
   // rebuilds it by re-applying pending transactions.
   //
   // This rebuild is necessary because pending transactions' validity
   // and semantics may have changed since they were received, because
   // time-based semantics are evaluated based on the current block
   // time.  These changes can only be reflected in the database when
   // the value of the "when" variable is known, which means we need to
   // re-apply pending transactions in this method.
   //
   _db.pending_transaction_session().reset();
   _db.pending_transaction_session() = _db.start_undo_session();

   FC_TODO( "Safe to remove after HF20 occurs because no more pre HF20 blocks will be generated" );
   if( _db.has_hardfork( STEEM_HARDFORK_0_20 ) )
   {
      /// modify current witness so transaction evaluators can know who included the transaction
      _db.modify(
             _db.get_dynamic_global_properties(),
             [&]( chain::dynamic_global_property_object& dgp )
             {
                dgp.current_witness = witness_owner;
             });
   }

   uint64_t postponed_tx_count = 0;
   // pop pending state (reset to head block state)
   for( const chain::signed_transaction& tx : _db._pending_tx )
   {
      // Only include transactions that have not expired yet for currently generating block,
      // this should clear problem transactions and allow block production to continue

      if( tx.expiration < when )
         continue;

      uint64_t new_total_size = total_block_size + fc::raw::pack_size( tx );

      // postpone transaction if it would make block too big
      if( new_total_size >= maximum_transaction_partition_size )
      {
         postponed_tx_count++;
         continue;
      }

      try
      {
         auto temp_session = _db.start_undo_session();
         _db.apply_transaction( tx, _db.get_node_properties().skip_flags );
         temp_session.squash();

         total_block_size = new_total_size;
         pending_block.transactions.push_back( tx );
      }
      catch ( const fc::exception& e )
      {
         // Do nothing, transaction will not be re-applied
         //wlog( "Transaction was not processed while generating block due to ${e}", ("e", e) );
         //wlog( "The transaction was ${t}", ("t", tx) );
      }
   }
   if( postponed_tx_count > 0 )
   {
      wlog( "Postponed ${n} transactions due to block size limit", ("n", postponed_tx_count) );
   }

   const auto& pending_required_action_idx = _db.get_index< chain::pending_required_action_index, chain::by_execution >();
   auto pending_required_itr = pending_required_action_idx.begin();
   chain::required_automated_actions required_actions;

   while( pending_required_itr != pending_required_action_idx.end() && pending_required_itr->execution_time <= when )
   {
      uint64_t new_total_size = total_block_size + fc::raw::pack_size( pending_required_itr->action );

      // required_actions_partizion_size is a lower bound of requirement.
      // If we have extra space to include actions we should use it.
      if( new_total_size > maximum_block_size )
         break;

      try
      {
         auto temp_session = _db.start_undo_session();
         _db.apply_required_action( pending_required_itr->action );
         temp_session.squash();
         total_block_size = new_total_size;
         required_actions.push_back( pending_required_itr->action );
         ++pending_required_itr;
      }
      catch( fc::exception& e )
      {
         FC_RETHROW_EXCEPTION( e, warn, "A required automatic action was rejected. ${a} ${e}", ("a", pending_required_itr->action)("e", e.to_detail_string()) );
      }
   }

FC_TODO( "Remove ifdef when required actions are added" )
#ifdef IS_TEST_NET
   if( required_actions.size() )
   {
      pending_block.extensions.insert( required_actions );
   }
#endif

   const auto& pending_optional_action_idx = _db.get_index< chain::pending_optional_action_index, chain::by_execution >();
   auto pending_optional_itr = pending_optional_action_idx.begin();
   chain::optional_automated_actions optional_actions;

   while( pending_optional_itr != pending_optional_action_idx.end() && pending_optional_itr->execution_time <= when )
   {
      uint64_t new_total_size = total_block_size + fc::raw::pack_size( pending_optional_itr->action );

      if( new_total_size > maximum_block_size )
         break;

      try
      {
         auto temp_session = _db.start_undo_session();
         _db.apply_optional_action( pending_optional_itr->action );
         temp_session.squash();
         total_block_size = new_total_size;
         optional_actions.push_back( pending_optional_itr->action );
      }
      catch( fc::exception& ) {}
      ++pending_optional_itr;
   }

FC_TODO( "Remove ifdef when optional actions are added" )
#ifdef IS_TEST_NET
   if( optional_actions.size() )
   {
      pending_block.extensions.insert( optional_actions );
   }
#endif

   _db.pending_transaction_session().reset();

   pending_block.transaction_merkle_root = pending_block.calculate_merkle_root();
}

} } } // steem::plugins::witness
