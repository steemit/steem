#pragma once

#include <fc/time.hpp>

#include <steem/chain/database.hpp>

namespace steem { namespace plugins { namespace chain {
   
class abstract_block_producer {
public:
   virtual ~abstract_block_producer() = default;

   virtual steem::chain::signed_block generate_block(
      fc::time_point_sec when,
      const steem::chain::account_name_type& witness_owner,
      const fc::ecc::private_key& block_signing_private_key,
      uint32_t skip = steem::chain::database::skip_nothing) = 0;
};

} } } // steem::plugins::chain
