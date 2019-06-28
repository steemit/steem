#pragma once
#include <dpn/protocol/block_header.hpp>
#include <dpn/protocol/transaction.hpp>

namespace dpn { namespace protocol {

   struct signed_block : public signed_block_header
   {
      checksum_type calculate_merkle_root()const;
      vector<signed_transaction> transactions;
   };

} } // dpn::protocol

FC_REFLECT_DERIVED( dpn::protocol::signed_block, (dpn::protocol::signed_block_header), (transactions) )
