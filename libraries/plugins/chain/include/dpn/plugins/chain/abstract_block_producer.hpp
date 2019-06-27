#pragma once

#include <fc/time.hpp>

#include <dpn/chain/database.hpp>

namespace dpn { namespace plugins { namespace chain {
   
class abstract_block_producer {
public:
   virtual ~abstract_block_producer() = default;

   virtual dpn::chain::signed_block generate_block(
      fc::time_point_sec when,
      const dpn::chain::account_name_type& witness_owner,
      const fc::ecc::private_key& block_signing_private_key,
      uint32_t skip = dpn::chain::database::skip_nothing) = 0;
};

} } } // dpn::plugins::chain
