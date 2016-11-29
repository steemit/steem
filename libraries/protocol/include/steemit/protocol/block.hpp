#pragma once
#include <steemit/protocol/block_header.hpp>
#include <steemit/protocol/transaction.hpp>

namespace steemit { namespace protocol {

   struct signed_block : public signed_block_header
   {
      checksum_type calculate_merkle_root()const;
      vector<signed_transaction> transactions;
   };

} } // steemit::protocol

FC_REFLECT_DERIVED( steemit::protocol::signed_block, (steemit::protocol::signed_block_header), (transactions) )
