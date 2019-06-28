#pragma once

#include <steem/protocol/base.hpp>
#include <steem/protocol/asset.hpp>
#include <steem/protocol/misc_utilities.hpp>

#ifdef STEEM_ENABLE_SMT

#define SMT_MAX_UNIT_ROUTES            10
#define SMT_MAX_UNIT_COUNT             20
#define SMT_MAX_DECIMAL_PLACES         8
#define SMT_MIN_HARD_CAP_STEEM_UNITS   10000
#define SMT_MIN_SATURATION_STEEM_UNITS 1000
#define SMT_MIN_SOFT_CAP_STEEM_UNITS   1000

namespace steem { namespace protocol {

/**
 * Base of all smt operations issued by token creator, holding what's needed by all of them.
 */
struct smt_base_operation : public base_operation
{
   /// Account that controls the SMT.
   account_name_type control_account;
   /// The token's Numerical Asset Identifier (NAI) coupled with token's precision.
   asset_symbol_type symbol;

   void validate()const;
   void get_required_active_authorities( flat_set<account_name_type>& a )const
   { a.insert( control_account ); }
};

/**
 * Base of all smt operations issued any user (aka executor).
 */
struct smt_executor_base_operation : public base_operation
{
   /// Account that executes the operation.
   account_name_type executor;
   /// The token's Numerical Asset Identifier (NAI) coupled with token's precision.
   asset_symbol_type symbol;

   void validate()const;
   void get_required_active_authorities( flat_set< account_name_type >& a )const
   { a.insert( executor ); }
};

/**
 * This operation introduces new SMT into blockchain as identified by
 * Numerical Asset Identifier (NAI). Also the SMT precision (decimal points)
 * is explicitly provided.
 */
struct smt_create_operation : public smt_base_operation
{
   /// The amount to be transfered from @account to null account as elevation fee.
   asset             smt_creation_fee;
   /// Separately provided precision for clarity and redundancy.
   uint8_t           precision;

   extensions_type   extensions;

   void validate()const;
};

struct smt_generation_unit
{
   flat_map< account_name_type, uint16_t >        steem_unit;
   flat_map< account_name_type, uint16_t >        token_unit;

   uint32_t steem_unit_sum()const;
   uint32_t token_unit_sum()const;

   void validate()const;
};

struct smt_capped_generation_policy
{
   smt_generation_unit pre_soft_cap_unit;
   smt_generation_unit post_soft_cap_unit;

   uint16_t            soft_cap_percent = 0;

   uint32_t            min_unit_ratio = 0;
   uint32_t            max_unit_ratio = 0;

   extensions_type     extensions;

   void validate()const;
};

typedef static_variant<
   smt_capped_generation_policy
   > smt_generation_policy;

struct smt_setup_operation : public smt_base_operation
{
   uint8_t                 decimal_places = 0;
   int64_t                 max_supply = STEEM_MAX_SHARE_SUPPLY;

   smt_generation_policy   initial_generation_policy;

   time_point_sec          contribution_begin_time;
   time_point_sec          contribution_end_time;
   time_point_sec          launch_time;

   share_type              steem_units_min_cap;
   share_type              steem_units_hard_cap;

   extensions_type         extensions;

   void validate()const;
};

struct smt_emissions_unit
{
   flat_map< account_name_type, uint16_t >        token_unit;
};

struct smt_setup_emissions_operation : public smt_base_operation
{
   time_point_sec      schedule_time;
   smt_emissions_unit  emissions_unit;

   uint32_t            interval_seconds = 0;
   uint32_t            interval_count = 0;

   time_point_sec      lep_time;
   time_point_sec      rep_time;

   asset               lep_abs_amount;
   asset               rep_abs_amount;
   uint32_t            lep_rel_amount_numerator = 0;
   uint32_t            rep_rel_amount_numerator = 0;

   uint8_t             rel_amount_denom_bits = 0;
   bool                remove = false;

   extensions_type     extensions;

   void validate()const;
};

struct smt_param_allow_voting
{
   bool value = true;
};

typedef static_variant<
   smt_param_allow_voting
   > smt_setup_parameter;

struct smt_param_windows_v1
{
   uint32_t cashout_window_seconds = 0;                // STEEM_CASHOUT_WINDOW_SECONDS
   uint32_t reverse_auction_window_seconds = 0;        // STEEM_REVERSE_AUCTION_WINDOW_SECONDS
};

struct smt_param_vote_regeneration_period_seconds_v1
{
   uint32_t vote_regeneration_period_seconds = 0;      // STEEM_VOTING_MANA_REGENERATION_SECONDS
   uint32_t votes_per_regeneration_period = 0;
};

struct smt_param_rewards_v1
{
   uint128_t               content_constant = 0;
   uint16_t                percent_curation_rewards = 0;
   protocol::curve_id      author_reward_curve;
   protocol::curve_id      curation_reward_curve;
};

struct smt_param_allow_downvotes
{
   bool value = true;
};

typedef static_variant<
   smt_param_windows_v1,
   smt_param_vote_regeneration_period_seconds_v1,
   smt_param_rewards_v1,
   smt_param_allow_downvotes
   > smt_runtime_parameter;

struct smt_set_setup_parameters_operation : public smt_base_operation
{
   flat_set< smt_setup_parameter >  setup_parameters;
   extensions_type                  extensions;

   void validate()const;
};

struct smt_set_runtime_parameters_operation : public smt_base_operation
{
   flat_set< smt_runtime_parameter >   runtime_parameters;
   extensions_type                     extensions;

   void validate()const;
};

struct smt_contribute_operation : public base_operation
{
   account_name_type  contributor;
   asset_symbol_type  symbol;
   uint32_t           contribution_id;
   asset              contribution;
   extensions_type    extensions;

   void validate() const;
   void get_required_active_authorities( flat_set<account_name_type>& a )const
   { a.insert( contributor ); }
};

} }

FC_REFLECT(
   steem::protocol::smt_base_operation,
   (control_account)
   (symbol)
)

FC_REFLECT(
   steem::protocol::smt_executor_base_operation,
   (executor)
   (symbol)
)

FC_REFLECT_DERIVED(
   steem::protocol::smt_create_operation,
   (steem::protocol::smt_base_operation),
   (smt_creation_fee)
   (extensions)
)

FC_REFLECT_DERIVED(
   steem::protocol::smt_setup_operation,
   (steem::protocol::smt_base_operation),
   (decimal_places)
   (max_supply)
   (initial_generation_policy)
   (contribution_begin_time)
   (contribution_end_time)
   (launch_time)
   (steem_units_min_cap)
   (steem_units_hard_cap)
   (extensions)
   )

FC_REFLECT(
   steem::protocol::smt_generation_unit,
   (steem_unit)
   (token_unit)
   )


FC_REFLECT(
   steem::protocol::smt_capped_generation_policy,
   (pre_soft_cap_unit)
   (post_soft_cap_unit)
   (soft_cap_percent)
   (min_unit_ratio)
   (max_unit_ratio)
   (extensions)
   )

FC_REFLECT(
   steem::protocol::smt_emissions_unit,
   (token_unit)
   )

FC_REFLECT_DERIVED(
   steem::protocol::smt_setup_emissions_operation,
   (steem::protocol::smt_base_operation),
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
   (remove)
   (extensions)
   )

FC_REFLECT(
   steem::protocol::smt_param_allow_voting,
   (value)
   )

FC_REFLECT_TYPENAME( steem::protocol::smt_setup_parameter )

FC_REFLECT(
   steem::protocol::smt_param_windows_v1,
   (cashout_window_seconds)
   (reverse_auction_window_seconds)
   )

FC_REFLECT(
   steem::protocol::smt_param_vote_regeneration_period_seconds_v1,
   (vote_regeneration_period_seconds)
   (votes_per_regeneration_period)
   )

FC_REFLECT(
   steem::protocol::smt_param_rewards_v1,
   (content_constant)
   (percent_curation_rewards)
   (author_reward_curve)
   (curation_reward_curve)
   )

FC_REFLECT(
   steem::protocol::smt_param_allow_downvotes,
   (value)
)

FC_REFLECT_TYPENAME(
   steem::protocol::smt_runtime_parameter
   )

FC_REFLECT_DERIVED(
   steem::protocol::smt_set_setup_parameters_operation,
   (steem::protocol::smt_base_operation),
   (setup_parameters)
   (extensions)
   )

FC_REFLECT_DERIVED(
   steem::protocol::smt_set_runtime_parameters_operation,
   (steem::protocol::smt_base_operation),
   (runtime_parameters)
   (extensions)
   )

FC_REFLECT(
   steem::protocol::smt_contribute_operation,
   (contributor)
   (symbol)
   (contribution_id)
   (contribution)
   (extensions)
   )

#endif
