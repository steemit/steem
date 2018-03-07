
#include <steem/chain/steem_evaluator.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/steem_objects.hpp>
#include <steem/chain/smt_objects.hpp>

#include <steem/chain/util/reward.hpp>

#include <steem/protocol/smt_operations.hpp>

#include <steem/protocol/smt_operations.hpp>
#ifdef STEEM_ENABLE_SMT
namespace steem { namespace chain {

namespace {

/// Return SMT token object controlled by this account identified by its symbol. Throws assert exception when not found!
inline const smt_token_object& get_controlled_smt( const database& db, const account_name_type& control_account, const asset_symbol_type& smt_symbol )
{
   const smt_token_object* smt = db.find< smt_token_object, by_symbol >( smt_symbol );
   // The SMT is supposed to be found.
   FC_ASSERT( smt != nullptr, "SMT numerical asset identifier ${smt} not found", ("smt", smt_symbol.to_nai()) );
   // Check against unotherized account trying to access (and alter) SMT. Unotherized means "other than the one that created the SMT".
   FC_ASSERT( smt->control_account == control_account, "The account ${account} does NOT control the SMT ${smt}",
      ("account", control_account)("smt", smt_symbol.to_nai()) );
   
   return *smt;
}
   
}

namespace {

class smt_setup_parameters_visitor : public fc::visitor<bool>
{
public:
   smt_setup_parameters_visitor(smt_token_object& smt_token) : _smt_token(smt_token) {}

   bool operator () (const smt_param_allow_voting& allow_voting)
   {
      _smt_token.allow_voting = allow_voting.value;
      return true;
   }

   bool operator () (const smt_param_allow_vesting& allow_vesting)
   {
      _smt_token.allow_vesting = allow_vesting.value;
      return true;
   }

private:
   smt_token_object& _smt_token;
};

const smt_token_object& common_pre_setup_evaluation(
   const database& _db, const asset_symbol_type& symbol, const account_name_type& control_account )
{
   const smt_token_object& smt = get_controlled_smt( _db, control_account, symbol );

   // Check whether it's not too late to setup emissions operation.
   FC_ASSERT( smt.phase < smt_token_object::smt_phase::setup_completed, "SMT pre-setup operation no longer allowed after setup phase is over" );

   return smt;
}

} // namespace

void smt_create_evaluator::do_apply( const smt_create_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEM_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEM_SMT_HARDFORK) );
   const dynamic_global_property_object& dgpo = _db.get_dynamic_global_properties();

   // Check that SMT with given nai has not been created already.
   // Note that symbols with the same nai and different precision (decimal places) are not allowed,
   // therefore we can't use 'by_symbol' index here, as we need to strip the symbol from precision info.
   FC_ASSERT( (_db.find< smt_token_object, by_nai >( o.symbol.to_nai() ) == nullptr),
      "SMT ${nai} has already been created.", ("nai", o.symbol.to_nai()));

   asset effective_elevation_fee;

   FC_ASSERT( dgpo.smt_creation_fee.symbol == STEEM_SYMBOL || dgpo.smt_creation_fee.symbol == SBD_SYMBOL,
      "Unexpected internal error - wrong symbol ${s} of SMT creation fee.", ("s", dgpo.smt_creation_fee.symbol) );
   FC_ASSERT( o.smt_creation_fee.symbol == STEEM_SYMBOL || o.smt_creation_fee.symbol == SBD_SYMBOL,
      "Asset fee must be STEEM or SBD, was ${s}", ("s", o.smt_creation_fee.symbol) );
   if( o.smt_creation_fee.symbol == dgpo.smt_creation_fee.symbol )
   {
      effective_elevation_fee = dgpo.smt_creation_fee;
   }
   else
   {
      const auto& fhistory = _db.get_feed_history();
      FC_ASSERT( !fhistory.current_median_history.is_null(), "Cannot pay the fee using SBD because there is no price feed." );
      if( o.smt_creation_fee.symbol == STEEM_SYMBOL )
      {
         effective_elevation_fee = _db.to_sbd( o.smt_creation_fee );
      }
      else
      {
         effective_elevation_fee = _db.to_steem( o.smt_creation_fee );         
      }
   }

   FC_ASSERT( o.smt_creation_fee >= effective_elevation_fee, 
      "Fee of ${of} is too small, must be at least ${ef}", ("of", o.smt_creation_fee)("ef", effective_elevation_fee) );
   FC_ASSERT( _db.get_balance( o.control_account, o.smt_creation_fee.symbol ) >= o.smt_creation_fee,
    "Account does not have sufficient funds for specified fee of ${of}", ("of", o.smt_creation_fee) );

   _db.adjust_balance( o.control_account , -o.smt_creation_fee );
   _db.adjust_balance( STEEM_NULL_ACCOUNT,  o.smt_creation_fee );

   // Create SMT object common to both liquid and vesting variants of SMT.
   _db.create< smt_token_object >( [&]( smt_token_object& token )
   {
      token.liquid_symbol = o.symbol;
      token.control_account = o.control_account;
      token.market_maker.token_balance = asset( 0, token.liquid_symbol );
   });
}

struct smt_setup_evaluator_visitor
{
   const smt_token_object& _token;
   database& _db;

   smt_setup_evaluator_visitor( const smt_token_object& token, database& db ): _token( token ), _db( db ){}

   typedef void result_type;

   void operator()( const smt_capped_generation_policy& capped_generation_policy ) const
   {
      _db.modify( _token, [&]( smt_token_object& token )
      {
         token.capped_generation_policy = capped_generation_policy;
      });
   }
};

void smt_setup_evaluator::do_apply( const smt_setup_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEM_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEM_SMT_HARDFORK) );

   const auto* _token = _db.find< smt_token_object, by_control_account >( o.control_account );
   FC_ASSERT( _token, "SMT ${ac} not elevated yet.",("ac", o.control_account) );

   _db.modify(  *_token, [&]( smt_token_object& token )
   {
      token.max_supply = o.max_supply;
      token.generation_begin_time = o.generation_begin_time;
      token.generation_end_time = o.generation_end_time;
      token.announced_launch_time = o.announced_launch_time;
      token.launch_expiration_time = o.launch_expiration_time;

      //We should override precisions in 'lep_abs_amount' and 'rep_abs_amount'
      //in case when precision was changed.
      asset_symbol_type as_type = _token->liquid_symbol.get_paired_symbol();
      uint8_t new_precision = as_type.decimals();

      if( token.lep_abs_amount.decimals() != new_precision )
         token.lep_abs_amount = asset( token.lep_abs_amount, as_type );

      if( token.rep_abs_amount.decimals() != new_precision )
         token.rep_abs_amount = asset( token.rep_abs_amount, as_type );
   });

   smt_setup_evaluator_visitor visitor( *_token, _db );
   o.initial_generation_policy.visit( visitor );
}

void smt_cap_reveal_evaluator::do_apply( const smt_cap_reveal_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEM_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEM_SMT_HARDFORK) );
   // TODO: Check whether some impostor tries to hijack SMT operation.
}

void smt_refund_evaluator::do_apply( const smt_refund_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEM_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEM_SMT_HARDFORK) );
   // TODO: Check whether some impostor tries to hijack SMT operation.
}

void smt_setup_emissions_evaluator::do_apply( const smt_setup_emissions_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEM_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEM_SMT_HARDFORK) );

   const smt_token_object& smt = common_pre_setup_evaluation(_db, o.symbol, o.control_account);

   FC_ASSERT( o.lep_abs_amount.symbol == smt.liquid_symbol );
   // ^ Note that rep_abs_amount.symbol has been matched to lep's in validate().

   _db.modify( smt, [&]( smt_token_object& token )
   {
      token.schedule_time = o.schedule_time;
      token.emissions_unit = o.emissions_unit;
      token.interval_seconds = o.interval_seconds;
      token.interval_count = o.interval_count;
      token.lep_time = o.lep_time;
      token.rep_time = o.rep_time;
      token.lep_abs_amount = o.lep_abs_amount;
      token.rep_abs_amount = o.rep_abs_amount;
      token.lep_rel_amount_numerator = o.lep_rel_amount_numerator;
      token.rep_rel_amount_numerator = o.rep_rel_amount_numerator;
      token.rel_amount_denom_bits = o.rel_amount_denom_bits;
   });
}

void smt_set_setup_parameters_evaluator::do_apply( const smt_set_setup_parameters_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEM_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEM_SMT_HARDFORK) );

   const smt_token_object& smt_token = common_pre_setup_evaluation(_db, o.symbol, o.control_account);
   
   _db.modify( smt_token, [&]( smt_token_object& token )
   {
      smt_setup_parameters_visitor visitor(token);

      for (auto& param : o.setup_parameters)
         param.visit(visitor);
   });
}

struct smt_set_runtime_parameters_evaluator_visitor
{
   const smt_token_object& _token;
   database& _db;

   smt_set_runtime_parameters_evaluator_visitor( const smt_token_object& token, database& db ): _token( token ), _db( db ){}

   typedef void result_type;

   void operator()( const smt_param_windows_v1& param_windows ) const
   {
      _db.modify( _token, [&]( smt_token_object& token )
      {
         token.cashout_window_seconds = param_windows.cashout_window_seconds;
         token.reverse_auction_window_seconds = param_windows.reverse_auction_window_seconds;
      });
   }

   void operator()( const smt_param_vote_regeneration_period_seconds_v1& vote_regeneration ) const
   {
      _db.modify( _token, [&]( smt_token_object& token )
      {
         token.vote_regeneration_period_seconds = vote_regeneration.vote_regeneration_period_seconds;
         token.votes_per_regeneration_period = vote_regeneration.votes_per_regeneration_period;
      });
   }

   void operator()( const smt_param_rewards_v1& param_rewards ) const
   {
      _db.modify( _token, [&]( smt_token_object& token )
      {
         token.content_constant = param_rewards.content_constant;
         token.percent_curation_rewards = param_rewards.percent_curation_rewards;
         token.percent_content_rewards = param_rewards.percent_content_rewards;
         token.author_reward_curve = param_rewards.author_reward_curve;
         token.curation_reward_curve = param_rewards.curation_reward_curve;
      });
   }
};

void smt_set_runtime_parameters_evaluator::do_apply( const smt_set_runtime_parameters_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEM_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEM_SMT_HARDFORK) );

   const smt_token_object& _token = common_pre_setup_evaluation(_db, o.symbol, o.control_account);

   smt_set_runtime_parameters_evaluator_visitor visitor( _token, _db );

   for( auto& param: o.runtime_parameters )
      param.visit( visitor );
}

} }
#endif