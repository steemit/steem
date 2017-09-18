#pragma once

#include <steem/chain/steem_object_types.hpp>
#include <steem/protocol/smt_operations.hpp>

#ifdef STEEM_ENABLE_SMT

namespace steem { namespace chain {

class smt_token_object : public object< smt_token_object_type, smt_token_object >
{
   smt_token_object() = delete;

   public:
   enum class smt_phase : unsigned char
   {
      account_elevated,
      setup_completed,
   };

   public:
   template< typename Constructor, typename Allocator >
   smt_token_object( Constructor&& c, allocator< Allocator > a )
   {
      c( *this );
   }

   // id_type is actually oid<smt_token_object>
   id_type           id;

   account_name_type control_account;
   smt_phase         phase = smt_phase::account_elevated;

   /// set_setup_parameters
   bool                 allow_voting = false;
   bool                 allow_vesting = false;
   /// smt_setup_emissions
   time_point_sec       schedule_time = STEEM_GENESIS_TIME;
   steem::protocol::
   smt_emissions_unit   emissions_unit;
   uint32_t             interval_seconds = 0;
   uint32_t             interval_count = 0;
   time_point_sec       lep_time = STEEM_GENESIS_TIME;
   time_point_sec       rep_time = STEEM_GENESIS_TIME;
   asset                lep_abs_amount = asset( 0, STEEM_SYMBOL );
   asset                rep_abs_amount = asset( 0, STEEM_SYMBOL );
   uint32_t             lep_rel_amount_numerator = 0;
   uint32_t             rep_rel_amount_numerator = 0;
   uint8_t              rel_amount_denom_bits = 0;
};

struct by_control_account;

typedef multi_index_container <
   smt_token_object,
   indexed_by <
      ordered_unique< tag< by_id >,
         member< smt_token_object, smt_token_id_type, &smt_token_object::id > >,
      ordered_unique< tag< by_control_account >,
         member< smt_token_object, account_name_type, &smt_token_object::control_account > >
   >,
   allocator< smt_token_object >
> smt_token_index;

} }

FC_REFLECT_ENUM( steem::chain::smt_token_object::smt_phase,
                 (account_elevated)
                 (setup_completed)
)

FC_REFLECT( steem::chain::smt_token_object,
   (id)
   (control_account)
   (phase)
   (allow_voting)
   (allow_vesting)
   (schedule_time)
   (emissions_unit)
   (interval_seconds)
   (interval_count)
   (lep_time)
   (rep_time)
   (lep_abs_amount)
   (rep_abs_amount)
   (lep_rel_amount_numerator)
   (rep_rel_amount_numerator)
   (rel_amount_denom_bits)
)
CHAINBASE_SET_INDEX_TYPE( steem::chain::smt_token_object, steem::chain::smt_token_index )

#endif
