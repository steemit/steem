#pragma once
#include <steem/plugins/condenser_api/condenser_api_legacy_operations.hpp>

#include <steem/plugins/block_api/block_api_objects.hpp>

namespace steem { namespace plugins { namespace condenser_api {

struct legacy_signed_transaction
{
   legacy_signed_transaction() {}
   legacy_signed_transaction( const annotated_signed_transaction& t ) :
      ref_block_num( t.ref_block_num ),
      ref_block_prefix( t.ref_block_prefix ),
      expiration( t.expiration ),
      transaction_id( t.transaction_id ),
      block_num( t.block_num ),
      transaction_num( t.transaction_num )
   {
      for( const auto& o : t.operations )
      {
         legacy_operation op;
         o.visit( legacy_operation_conversion_visitor( op ) );
         operations.push_back( op );
      }

      // Signed transaction extensions field exists, but must be empty
      // Don't worry about copying them.

      signatures.insert( t.signatures.begin(), t.signatures.end() );
   }

   operator signed_transaction()const
   {
      signed_transaction tx;
      tx.ref_block_num = ref_block_num;
      tx.ref_block_prefix = ref_block_prefix;
      tx.expiration = expiration;

      convert_from_legacy_operation_visitor v;
      for( const auto& o : operations )
      {
         tx.operations.push_back( o.visit( v ) );
      }

      tx.signatures.insert( signatures.begin(), signatures.end() );
   }

   uint16_t                   ref_block_num    = 0;
   uint16_t                   ref_block_prefix = 0;
   fc::time_point_sec         expiration;
   vector< legacy_operation > operations;
   extensions_type            extensions;
   vector< signature_type >   signatures;
   transaction_id_type        transaction_id;
   uint32_t                   block_num = 0;
   uint32_t                   transaction_num = 0;
};

} } } // steem::plugins::condenser_api

FC_REFLECT( steem::plugins::condenser_api::legacy_signed_transaction,
            (ref_block_num)(ref_block_prefix)(expiration)(operations)(extensions)(signatures)(transaction_id)(block_num)(transaction_num) )
