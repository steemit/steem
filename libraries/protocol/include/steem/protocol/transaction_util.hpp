#pragma once
#include <steem/protocol/sign_state.hpp>
#include <steem/protocol/exceptions.hpp>

namespace steem { namespace protocol {

template< typename AuthContainerType >
void verify_authority( const vector<AuthContainerType>& auth_containers, const flat_set<public_key_type>& sigs,
                       const authority_getter& get_active,
                       const authority_getter& get_owner,
                       const authority_getter& get_posting,
                       uint32_t max_recursion_depth = STEEM_MAX_SIG_CHECK_DEPTH,
                       uint32_t max_membership = STEEM_MAX_AUTHORITY_MEMBERSHIP,
                       uint32_t max_account_auths = STEEM_MAX_SIG_CHECK_ACCOUNTS,
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

   get_required_auth_visitor auth_visitor( required_active, required_owner, required_posting, other );

   for( const auto& a : auth_containers )
      auth_visitor( a );

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
      s.max_membership = max_membership;
      s.max_account_auths = max_account_auths;
      for( auto& id : posting_approvals )
         s.approved_by.insert( id );
      for( const auto& id : required_posting )
      {
         STEEM_ASSERT( s.check_authority(id) ||
                          s.check_authority(get_active(id)) ||
                          s.check_authority(get_owner(id)),
                          tx_missing_posting_auth, "Missing Posting Authority ${id}",
                          ("id",id)
                          ("posting",get_posting(id))
                          ("active",get_active(id))
                          ("owner",get_owner(id)) );
      }
      STEEM_ASSERT(
         !s.remove_unused_signatures(),
         tx_irrelevant_sig,
         "Unnecessary signature(s) detected"
         );
      return;
   }

   flat_set< public_key_type > avail;
   sign_state s(sigs,get_active,avail);
   s.max_recursion = max_recursion_depth;
   s.max_membership = max_membership;
   s.max_account_auths = max_account_auths;
   for( auto& id : active_approvals )
      s.approved_by.insert( id );
   for( auto& id : owner_approvals )
      s.approved_by.insert( id );

   for( const auto& auth : other )
   {
      STEEM_ASSERT( s.check_authority(auth), tx_missing_other_auth, "Missing Authority", ("auth",auth)("sigs",sigs) );
   }

   // fetch all of the top level authorities
   for( const auto& id : required_active )
   {
      STEEM_ASSERT( s.check_authority(id) ||
                       s.check_authority(get_owner(id)),
                       tx_missing_active_auth, "Missing Active Authority ${id}", ("id",id)("auth",get_active(id))("owner",get_owner(id)) );
   }

   for( const auto& id : required_owner )
   {
      STEEM_ASSERT( owner_approvals.find(id) != owner_approvals.end() ||
                       s.check_authority(get_owner(id)),
                       tx_missing_owner_auth, "Missing Owner Authority ${id}", ("id",id)("auth",get_owner(id)) );
   }

   STEEM_ASSERT(
      !s.remove_unused_signatures(),
      tx_irrelevant_sig,
      "Unnecessary signature(s) detected"
      );
} FC_CAPTURE_AND_RETHROW( (auth_containers)(sigs) ) }

} } // steem::protocol
