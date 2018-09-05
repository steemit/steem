#pragma once

#include <fc/time.hpp>

#include <steem/plugins/chain/chain_plugin.hpp>

namespace steem { namespace plugins { namespace witness {

class block_producer {
public:
   block_producer( chain::database& db ) : _db( db ) {}

   chain::signed_block generate_block(
      fc::time_point_sec when,
      const chain::account_name_type& witness_owner,
      const fc::ecc::private_key& block_signing_private_key,
      uint32_t skip = 0);

private:
   chain::database& _db;

   chain::signed_block _generate_block(
      fc::time_point_sec when,
      const chain::account_name_type& witness_owner,
      const fc::ecc::private_key& block_signing_private_key);

   void adjust_hardfork_version_vote( const chain::witness_object& witness, chain::signed_block& pending_block );

   void apply_pending_transactions(
      const chain::account_name_type& witness_owner,
      fc::time_point_sec when,
      chain::signed_block& pending_block);
};

} } } // steem::plugins::witness
