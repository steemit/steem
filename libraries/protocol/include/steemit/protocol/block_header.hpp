#pragma once
#include <steemit/protocol/base.hpp>

namespace steemit { namespace protocol {

   struct block_header
   {
      digest_type                   digest()const;
      block_id_type                 previous;
      uint32_t                      block_num()const { return num_from_id(previous) + 1; }
      fc::time_point_sec            timestamp;
      string                        witness;
      checksum_type                 transaction_merkle_root;
      block_header_extensions_type  extensions;

      static uint32_t num_from_id(const block_id_type& id);
   };

   struct signed_block_header : public block_header
   {
      block_id_type              id()const;
      fc::ecc::public_key        signee()const;
      void                       sign( const fc::ecc::private_key& signer );
      bool                       validate_signee( const fc::ecc::public_key& expected_signee )const;

      signature_type             witness_signature;
   };


} } // steemit::protocol

FC_REFLECT( steemit::protocol::block_header, (previous)(timestamp)(witness)(transaction_merkle_root)(extensions) )
FC_REFLECT_DERIVED( steemit::protocol::signed_block_header, (steemit::protocol::block_header), (witness_signature) )
