#include <steem/plugins/witness/block_producer.hpp>

#include <steem/chain/database_exceptions.hpp>
#include <steem/chain/db_with.hpp>
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

   _db.adjust_witness_hardfork_version_vote( _db.get_witness( witness_owner ), pending_block );

   _db.apply_pending_transactions( witness_owner, when, pending_block );

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

} } } // steem::plugins::witness
