#include <steem/plugins/witness/witness_operations.hpp>
#include <steem/plugins/witness/witness_objects.hpp>

#include <steem/chain/block_summary_object.hpp>
#include <steem/chain/database.hpp>

namespace steem { namespace plugins { namespace witness {

void enable_content_editing_evaluator::do_apply( const enable_content_editing_operation& o )
{
   try
   {
      auto edit_lock = _db.find< content_edit_lock_object, by_account >( o.account );

      if( edit_lock == nullptr )
      {
         _db.create< content_edit_lock_object >( [&]( content_edit_lock_object& lock )
         {
            lock.account = o.account;
            lock.lock_time = o.relock_time;
         });
      }
      else
      {
         _db.modify( *edit_lock, [&]( content_edit_lock_object& lock )
         {
            lock.lock_time = o.relock_time;
         });
      }
   }
   FC_CAPTURE_AND_RETHROW( (o) )
}

void create_egg_evaluator::do_apply( const create_egg_operation& op )
{
   try
   {
      block_summary_id_type sid( block_header::num_from_id( op.work.input.recent_block_id ) & 0xffff );
      const block_summary_object& bso = _db.get< block_summary_object >( sid );
      FC_ASSERT( op.work.input.recent_block_id == bso.block_id );
      // TODO check the block is not too old (how old is too old?)
      // TODO compute the expected hashes and verify that they are enough to buy the required RC
      // TODO update the MM inventory and the cost of the next RC

      _db.create< egg_object >( [&]( egg_object& egg )
      {
         egg.egg_auth = op.egg_auth;
         egg.h_egg_auth = op.work.input.h_egg_auth;
         egg.used = false;
      } );
   }
   FC_CAPTURE_AND_RETHROW( (op) )
}

} } } // steem::plugins::witness
