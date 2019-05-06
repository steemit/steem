#include <steem/chain/steem_fwd.hpp>

#include <steem/chain/steem_evaluator.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/steem_objects.hpp>
#include <steem/chain/smt_objects.hpp>
#include <steem/chain/util/reward.hpp>
#include <steem/chain/util/smt_token.hpp>

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
   smt_setup_parameters_visitor( smt_token_object& smt_token ) : _smt_token( smt_token ) {}

   bool operator () ( const smt_param_allow_voting& allow_voting )
   {
      _smt_token.allow_voting = allow_voting.value;
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
   FC_ASSERT( smt.phase < smt_phase::setup_completed, "SMT pre-setup operation no longer allowed after setup phase is over" );

   return smt;
}

} // namespace

void smt_create_evaluator::do_apply( const smt_create_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEM_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEM_SMT_HARDFORK) );
   const dynamic_global_property_object& dgpo = _db.get_dynamic_global_properties();

   FC_ASSERT( util::smt::find_token( _db, o.symbol, true ) == nullptr, "SMT ${nai} has already been created.", ("nai", o.symbol.to_nai() ) );
   FC_ASSERT(  _db.get< nai_pool_object >().contains( o.symbol ), "Cannot create an SMT that didn't come from the NAI pool." );

   asset creation_fee;

   if( o.smt_creation_fee.symbol == dgpo.smt_creation_fee.symbol )
   {
      creation_fee = o.smt_creation_fee;
   }
   else
   {
      const auto& fhistory = _db.get_feed_history();
      FC_ASSERT( !fhistory.current_median_history.is_null(), "Cannot pay the fee using different asset symbol because there is no price feed." );

      if( dgpo.smt_creation_fee.symbol == STEEM_SYMBOL )
         creation_fee = _db.to_steem( o.smt_creation_fee );
      else
         creation_fee = _db.to_sbd( o.smt_creation_fee );
   }

   FC_ASSERT( creation_fee == dgpo.smt_creation_fee,
      "Fee of ${ef} does not match the creation fee of ${sf}", ("ef", creation_fee)("sf", dgpo.smt_creation_fee) );

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

   remove_from_nai_pool( _db, o.symbol );

   if ( !_db.is_pending_tx() )
      replenish_nai_pool( _db );
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
#pragma message ("TODO: Adjust assertion below and add/modify negative tests appropriately.")
   const auto* _token = _db.find< smt_token_object, by_symbol >( o.symbol );
   FC_ASSERT( _token, "SMT ${ac} not elevated yet.",("ac", o.control_account) );

   _db.modify(  *_token, [&]( smt_token_object& token )
   {
#pragma message ("TODO: Add/modify test to check the token phase correctly set.")
      token.phase = smt_phase::setup_completed;
      token.control_account = o.control_account;
      token.max_supply = o.max_supply;

      token.generation_begin_time = o.generation_begin_time;
      token.generation_end_time = o.generation_end_time;
      token.announced_launch_time = o.announced_launch_time;
      token.launch_expiration_time = o.launch_expiration_time;
   } );

   smt_setup_evaluator_visitor visitor( *_token, _db );
   o.initial_generation_policy.visit( visitor );

   _db.create< smt_event_token_object >( [&]( smt_event_token_object& event_token )
   {
      event_token.parent = _token->id;

      event_token.generation_begin_time = _token->generation_begin_time;
      event_token.generation_end_time = _token->generation_end_time;
      event_token.announced_launch_time = _token->announced_launch_time;
      event_token.launch_expiration_time = _token->launch_expiration_time;
   });
}

void smt_cap_reveal_evaluator::do_apply( const smt_cap_reveal_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEM_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEM_SMT_HARDFORK) );

   const smt_token_object& smt = get_controlled_smt( _db, o.control_account, o.symbol );
   // Check whether it's not too early to reveal a cap.
   FC_ASSERT( smt.phase >= smt_phase::setup_completed, "SMT setup operation must succeed before cap reveal operaton is allowed" );
   // Check whether it's not too late to reveal a cap.
   FC_ASSERT( smt.phase < smt_phase::launch_failed, "Cap reveal operaton is allowed only until SMT ICO is concluded" );

   // As there's no information in cap reveal operation about which cap it reveals,
   // we'll check both, unless they are already revealed.
   FC_ASSERT( smt.steem_units_min_cap < 0 || smt.steem_units_hard_cap < 0, "Both min cap and max hard cap have already been revealed" );

   if( smt.steem_units_min_cap < 0 )
      try
      {
         o.cap.validate( smt.capped_generation_policy.min_steem_units_commitment );
         _db.modify( smt, [&]( smt_token_object& smt_object )
         {
            smt_object.steem_units_min_cap = o.cap.amount;
         });
         return;
      }
      catch( const fc::exception& e )
      {
         if( smt.steem_units_hard_cap >= 0 )
            throw;
      }

   o.cap.validate( smt.capped_generation_policy.hard_cap_steem_units_commitment );
   _db.modify( smt, [&]( smt_token_object& smt_object )
   {
      smt_object.steem_units_hard_cap = o.cap.amount;
   });
}

void smt_refund_evaluator::do_apply( const smt_refund_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEM_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEM_SMT_HARDFORK) );
   // TODO: Check whether some impostor tries to hijack SMT operation.
}

void smt_setup_emissions_evaluator::do_apply( const smt_setup_emissions_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEM_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEM_SMT_HARDFORK) );

   const smt_token_object& smt = common_pre_setup_evaluation( _db, o.symbol, o.control_account );

   FC_ASSERT( o.schedule_time > _db.head_block_time(), "Emissions schedule time must be in the future" );

   auto end_time = util::smt::last_emission_time( _db, smt.liquid_symbol );

   if ( end_time.valid() )
   {
      FC_ASSERT( o.schedule_time > *end_time,
         "SMT emissions cannot overlap with existing ranges and must be in chronological order, last emission time: ${end}",
         ("end", *end_time) );
   }

   _db.create< smt_token_emissions_object >( [&]( smt_token_emissions_object& eo )
   {
      eo.symbol = smt.liquid_symbol;
      eo.schedule_time = o.schedule_time;
      eo.emissions_unit = o.emissions_unit;
      eo.interval_seconds = o.interval_seconds;
      eo.interval_count = o.interval_count;
      eo.lep_time = o.lep_time;
      eo.rep_time = o.rep_time;
      eo.lep_abs_amount = o.lep_abs_amount;
      eo.rep_abs_amount = o.rep_abs_amount;
      eo.lep_rel_amount_numerator = o.lep_rel_amount_numerator;
      eo.rep_rel_amount_numerator = o.rep_rel_amount_numerator;
      eo.rel_amount_denom_bits = o.rel_amount_denom_bits;
   });
}

void smt_set_setup_parameters_evaluator::do_apply( const smt_set_setup_parameters_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEM_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEM_SMT_HARDFORK) );

   const smt_token_object& smt_token = common_pre_setup_evaluation( _db, o.symbol, o.control_account );

   _db.modify( smt_token, [&]( smt_token_object& token )
   {
      smt_setup_parameters_visitor visitor( token );

      for ( auto& param : o.setup_parameters )
         param.visit( visitor );
   });
}

struct smt_set_runtime_parameters_evaluator_visitor
{
   smt_token_object& _token;

   smt_set_runtime_parameters_evaluator_visitor( smt_token_object& token ) : _token( token ) {}

   typedef void result_type;

   void operator()( const smt_param_windows_v1& param_windows )const
   {
      _token.cashout_window_seconds = param_windows.cashout_window_seconds;
      _token.reverse_auction_window_seconds = param_windows.reverse_auction_window_seconds;
   }

   void operator()( const smt_param_vote_regeneration_period_seconds_v1& vote_regeneration )const
   {
      _token.vote_regeneration_period_seconds = vote_regeneration.vote_regeneration_period_seconds;
      _token.votes_per_regeneration_period = vote_regeneration.votes_per_regeneration_period;
   }

   void operator()( const smt_param_rewards_v1& param_rewards )const
   {
      _token.content_constant = param_rewards.content_constant;
      _token.percent_curation_rewards = param_rewards.percent_curation_rewards;
      _token.author_reward_curve = param_rewards.author_reward_curve;
      _token.curation_reward_curve = param_rewards.curation_reward_curve;
   }

   void operator()( const smt_param_allow_downvotes& param_allow_downvotes )const
   {
      _token.allow_downvotes = param_allow_downvotes.value;
   }
};

void smt_set_runtime_parameters_evaluator::do_apply( const smt_set_runtime_parameters_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEM_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEM_SMT_HARDFORK) );

   const smt_token_object& token = common_pre_setup_evaluation(_db, o.symbol, o.control_account);

   _db.modify( token, [&]( smt_token_object& t )
   {
      smt_set_runtime_parameters_evaluator_visitor visitor( t );

      for( auto& param: o.runtime_parameters )
         param.visit( visitor );
   });
}

} }
#endif
