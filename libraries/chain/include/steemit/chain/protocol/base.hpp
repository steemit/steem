#pragma once

#include <steemit/chain/protocol/types.hpp>
#include <steemit/chain/protocol/authority.hpp>

namespace steemit { namespace chain {

   struct base_operation
   {
      void get_required_authorities( vector<authority>& )const{}
      void get_required_active_authorities( flat_set<string>& )const{}
      void get_required_posting_authorities( flat_set<string>& )const{}
      void get_required_owner_authorities( flat_set<string>& )const{}
      void validate()const{}
   };

   typedef static_variant<void_t>      future_extensions;
   typedef flat_set<future_extensions> extensions_type;

} } // steemit::chain

FC_REFLECT_TYPENAME( steemit::chain::future_extensions )
