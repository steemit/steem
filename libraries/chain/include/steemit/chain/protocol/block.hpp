#pragma once
#include <steemit/chain/protocol/block_header.hpp>
#include <steemit/chain/protocol/transaction.hpp>

namespace steemit { namespace chain {

   struct signed_block : public signed_block_header
   {
      checksum_type calculate_merkle_root()const;
      vector<signed_transaction> transactions;
   };

} } // steemit::chain

FC_REFLECT_DERIVED( steemit::chain::signed_block, (steemit::chain::signed_block_header), (transactions) )
