#pragma once
#include <steemit/protocol/sign_state.hpp>
#include <steemit/protocol/exceptions.hpp>

namespace steemit { namespace protocol {

void verify_authority( const vector<operation>& ops, const flat_set<public_key_type>& sigs,
                       const authority_getter& get_active,
                       const authority_getter& get_owner,
                       const authority_getter& get_posting,
                       uint32_t max_recursion_depth = STEEMIT_MAX_SIG_CHECK_DEPTH,
                       bool allow_committe = false,
                       const flat_set< account_name_type >& active_approvals = flat_set< account_name_type >(),
                       const flat_set< account_name_type >& owner_approvals = flat_set< account_name_type >(),
                       const flat_set< account_name_type >& posting_approvals = flat_set< account_name_type >()
                       )
{ try {
   flat_set< account_name_type > required_active;
   flat_set< account_name_type > required_owner;
   flat_set< account_name_type > required_posting;
   vector< authority > other;

   for( const auto& op : ops )
      operation_get_required_authorities( op, required_active, required_owner, required_posting, other );

   /**
    *  Transactions with operations required posting authority cannot be combined
    *  with transactions requiring active or owner authority. This is for ease of
    *  implementation. Future versions of authority verification may be able to
    *  check for the merged authority of active and posting.
    */
   if( required_posting.size() ) {
      FC_ASSERT( required_active.size() == 0 );
      FC_ASSERT( required_owner.size() == 0 );
      FC_ASSERT( other.size() == 0 );

      flat_set< public_key_type > avail;
      sign_state s(sigs,get_posting,avail);
      s.max_recursion = max_recursion_depth;
      for( auto& id : posting_approvals )
         s.approved_by.insert( id );
      for( const auto& id : required_posting )
      {
         STEEMIT_ASSERT( s.check_authority(id) ||
                          s.check_authority(get_active(id)) ||
                          s.check_authority(get_owner(id)),
                          tx_missing_posting_auth, "Missing Posting Authority ${id}",
                          ("id",id)
                          ("posting",get_posting(id))
                          ("active",get_active(id))
                          ("owner",get_owner(id)) );
      }
      STEEMIT_ASSERT(
         !s.remove_unused_signatures(),
         tx_irrelevant_sig,
         "Unnecessary signature(s) detected"
         );
      return;
   }

   flat_set< public_key_type > avail;
   sign_state s(sigs,get_active,avail);
   s.max_recursion = max_recursion_depth;
   for( auto& id : active_approvals )
      s.approved_by.insert( id );
   for( auto& id : owner_approvals )
      s.approved_by.insert( id );

   for( const auto& auth : other )
   {
      STEEMIT_ASSERT( s.check_authority(auth), tx_missing_other_auth, "Missing Authority", ("auth",auth)("sigs",sigs) );
   }

   // fetch all of the top level authorities
   for( const auto& id : required_active )
   {
      STEEMIT_ASSERT( s.check_authority(id) ||
                       s.check_authority(get_owner(id)),
                       tx_missing_active_auth, "Missing Active Authority ${id}", ("id",id)("auth",get_active(id))("owner",get_owner(id)) );
   }

   for( const auto& id : required_owner )
   {
      STEEMIT_ASSERT( owner_approvals.find(id) != owner_approvals.end() ||
                       s.check_authority(get_owner(id)),
                       tx_missing_owner_auth, "Missing Owner Authority ${id}", ("id",id)("auth",get_owner(id)) );
   }

   STEEMIT_ASSERT(
      !s.remove_unused_signatures(),
      tx_irrelevant_sig,
      "Unnecessary signature(s) detected"
      );
} FC_CAPTURE_AND_RETHROW( (ops)(sigs) ) }

} } // steemit::protocol
