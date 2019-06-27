#pragma once
#include <steem/protocol/block_header.hpp>
#include <steem/protocol/transaction.hpp>

namespace steem { namespace protocol {

   struct signed_block : public signed_block_header
   {
      checksum_type calculate_merkle_root()const;
      vector<signed_transaction> transactions;
   };

} } // steem::protocol

FC_REFLECT_DERIVED( steem::protocol::signed_block, (steem::protocol::signed_block_header), (transactions) )
