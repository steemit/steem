#pragma once

#include <steemit/chain/protocol/types.hpp>
#include <steemit/chain/protocol/authority.hpp>
#include <steemit/chain/protocol/version.hpp>

#include <fc/time.hpp>

namespace steemit { namespace chain {

   struct base_operation
   {
      void get_required_authorities( vector<authority>& )const{}
      void get_required_active_authorities( flat_set<string>& )const{}
      void get_required_posting_authorities( flat_set<string>& )const{}
      void get_required_owner_authorities( flat_set<string>& )const{}
      void validate()const{}
   };

   typedef static_variant<
      void_t,
      version,             // Normal witness version reporting, for diagnostics and voting
      hardfork_version,    // Voting for the next hardfork to trigger
      fc::time_point_sec   // Voting for when that hardfork should happen
      >                                future_extensions;
   typedef flat_set<future_extensions> extensions_type;

} } // steemit::chain

FC_REFLECT_TYPENAME( steemit::chain::future_extensions )
