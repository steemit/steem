#pragma once
#include <steem/protocol/operations.hpp>
#include <steem/protocol/sign_state.hpp>
#include <steem/protocol/types.hpp>

#include <numeric>

namespace steem { namespace protocol {

using fc::ecc::canonical_signature_type;

   struct transaction
   {
      uint16_t           ref_block_num    = 0;
      uint32_t           ref_block_prefix = 0;

      fc::time_point_sec expiration;

      vector<operation>  operations;
      extensions_type    extensions;

      digest_type         digest()const;
      transaction_id_type id()const;
      void                validate() const;
      digest_type         sig_digest( const chain_id_type& chain_id )const;

      void set_expiration( fc::time_point_sec expiration_time );
      void set_reference_block( const block_id_type& reference_block );

      template<typename Visitor>
      vector<typename Visitor::result_type> visit( Visitor&& visitor )
      {
         vector<typename Visitor::result_type> results;
         for( auto& op : operations )
            results.push_back(op.visit( std::forward<Visitor>( visitor ) ));
         return results;
      }
      template<typename Visitor>
      vector<typename Visitor::result_type> visit( Visitor&& visitor )const
      {
         vector<typename Visitor::result_type> results;
         for( auto& op : operations )
            results.push_back(op.visit( std::forward<Visitor>( visitor ) ));
         return results;
      }

      void get_required_authorities( flat_set< account_name_type >& active,
                                     flat_set< account_name_type >& owner,
                                     flat_set< account_name_type >& posting,
                                     vector< authority >& other )const;
   };

   struct signed_transaction : public transaction
   {
      signed_transaction( const transaction& trx = transaction() )
         : transaction(trx){}

      const signature_type& sign( const private_key_type& key, const chain_id_type& chain_id, canonical_signature_type canon_type/* = fc::ecc::fc_canonical*/ );

      signature_type sign( const private_key_type& key, const chain_id_type& chain_id, canonical_signature_type canon_type/* = fc::ecc::fc_canonical*/ )const;

      set<public_key_type> get_required_signatures(
         const chain_id_type& chain_id,
         const flat_set<public_key_type>& available_keys,
         const authority_getter& get_active,
         const authority_getter& get_owner,
         const authority_getter& get_posting,
         uint32_t max_recursion = STEEM_MAX_SIG_CHECK_DEPTH,
         uint32_t max_membership = STEEM_MAX_AUTHORITY_MEMBERSHIP,
         uint32_t max_account_auths = STEEM_MAX_SIG_CHECK_ACCOUNTS,
         canonical_signature_type canon_type = fc::ecc::fc_canonical
         )const;

      void verify_authority(
         const chain_id_type& chain_id,
         const authority_getter& get_active,
         const authority_getter& get_owner,
         const authority_getter& get_posting,
         uint32_t max_recursion/* = STEEM_MAX_SIG_CHECK_DEPTH*/,
         uint32_t max_membership = STEEM_MAX_AUTHORITY_MEMBERSHIP,
         uint32_t max_account_auths = STEEM_MAX_SIG_CHECK_ACCOUNTS,
         canonical_signature_type canon_type = fc::ecc::fc_canonical
         )const;

      set<public_key_type> minimize_required_signatures(
         const chain_id_type& chain_id,
         const flat_set<public_key_type>& available_keys,
         const authority_getter& get_active,
         const authority_getter& get_owner,
         const authority_getter& get_posting,
         uint32_t max_recursion = STEEM_MAX_SIG_CHECK_DEPTH,
         uint32_t max_membership = STEEM_MAX_AUTHORITY_MEMBERSHIP,
         uint32_t max_account_auths = STEEM_MAX_SIG_CHECK_ACCOUNTS,
         canonical_signature_type canon_type = fc::ecc::fc_canonical
         ) const;

      flat_set<public_key_type> get_signature_keys( const chain_id_type& chain_id, canonical_signature_type/* = fc::ecc::fc_canonical*/ )const;

      vector<signature_type> signatures;

      digest_type merkle_digest()const;

      void clear() { operations.clear(); signatures.clear(); }
   };

   struct annotated_signed_transaction : public signed_transaction {
      annotated_signed_transaction(){}
      annotated_signed_transaction( const signed_transaction& trx )
      :signed_transaction(trx),transaction_id(trx.id()){}

      transaction_id_type transaction_id;
      uint32_t            block_num = 0;
      uint32_t            transaction_num = 0;
   };


   /// @} transactions group

} } // steem::protocol

FC_REFLECT( steem::protocol::transaction, (ref_block_num)(ref_block_prefix)(expiration)(operations)(extensions) )
FC_REFLECT_DERIVED( steem::protocol::signed_transaction, (steem::protocol::transaction), (signatures) )
FC_REFLECT_DERIVED( steem::protocol::annotated_signed_transaction, (steem::protocol::signed_transaction), (transaction_id)(block_num)(transaction_num) );
