#pragma once

#include <steemit/chain/database.hpp>

/*
 * This file provides with() functions which modify the database
 * temporarily, then restore it.  These functions are mostly internal
 * implementation detail of the database.
 *
 * Essentially, we want to be able to use "finally" to restore the
 * database regardless of whether an exception is thrown or not, but there
 * is no "finally" in C++.  Instead, C++ requires us to create a struct
 * and put the finally block in a destructor.  Aagh!
 */

namespace steemit { namespace chain { namespace detail {
/**
 * Class used to help the with_skip_flags implementation.
 * It must be defined in this header because it must be
 * available to the with_skip_flags implementation,
 * which is a template and therefore must also be defined
 * in this header.
 */
struct skip_flags_restorer
{
   skip_flags_restorer( node_property_object& npo, uint32_t old_skip_flags )
      : _npo( npo ), _old_skip_flags( old_skip_flags )
   {}

   ~skip_flags_restorer()
   {
      _npo.skip_flags = _old_skip_flags;
   }

   node_property_object& _npo;
   uint32_t _old_skip_flags;      // initialized in ctor
};

/**
 * Class used to help the without_pending_transactions
 * implementation.
 *
 * TODO:  Change the name of this class to better reflect the fact
 * that it restores popped transactions as well as pending transactions.
 */
struct pending_transactions_restorer
{
   pending_transactions_restorer( database& db, std::vector<signed_transaction>&& pending_transactions )
      : _db(db), _pending_transactions( std::move(pending_transactions) )
   {
      _db.clear_pending();
   }

   ~pending_transactions_restorer()
   {
      for( const auto& tx : _db._popped_tx )
      {
         try {
            if( !_db.is_known_transaction( tx.id() ) ) {
               // since push_transaction() takes a signed_transaction,
               // the operation_results field will be ignored.
               _db._push_transaction( tx );
            }
         } catch ( const fc::exception&  ) {
         }
      }
      _db._popped_tx.clear();
      for( const signed_transaction& tx : _pending_transactions )
      {
         try
         {
            if( !_db.is_known_transaction( tx.id() ) ) {
               // since push_transaction() takes a signed_transaction,
               // the operation_results field will be ignored.
               _db._push_transaction( tx );
            }
         }
         catch( const transaction_exception& e )
         {
            dlog( "Pending transaction became invalid after switching to block ${b} ${n} ${t}",
               ("b", _db.head_block_id())("n", _db.head_block_num())("t", _db.head_block_time()) );
            dlog( "The invalid transaction caused exception ${e}", ("e", e.to_detail_string()) );
            dlog( "${t}", ("t", tx) );
         }
         catch( const fc::exception& e )
         {

            /*
            dlog( "Pending transaction became invalid after switching to block ${b} ${n} ${t}",
               ("b", _db.head_block_id())("n", _db.head_block_num())("t", _db.head_block_time()) );
            dlog( "The invalid pending transaction caused exception ${e}", ("e", e.to_detail_string() ) );
            dlog( "${t}", ("t", tx) );
            */
         }
      }
   }

   database& _db;
   std::vector< signed_transaction > _pending_transactions;
};

/**
 * Set the skip_flags to the given value, call callback,
 * then reset skip_flags to their previous value after
 * callback is done.
 */
template< typename Lambda >
void with_skip_flags(
   database& db,
   uint32_t skip_flags,
   Lambda callback )
{
   node_property_object& npo = db.node_properties();
   skip_flags_restorer restorer( npo, npo.skip_flags );
   npo.skip_flags = skip_flags;
   callback();
   return;
}

/**
 * Empty pending_transactions, call callback,
 * then reset pending_transactions after callback is done.
 *
 * Pending transactions which no longer validate will be culled.
 */
template< typename Lambda >
void without_pending_transactions(
   database& db,
   std::vector<signed_transaction>&& pending_transactions,
   Lambda callback )
{
    pending_transactions_restorer restorer( db, std::move(pending_transactions) );
    callback();
    return;
}

} } } // steemit::chain::detail
