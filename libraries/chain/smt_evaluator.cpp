
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

} // namespace

void smt_elevate_account_evaluator::do_apply( const smt_elevate_account_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEM_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEM_SMT_HARDFORK) );
   const dynamic_global_property_object& dgpo = _db.get_dynamic_global_properties();

   asset effective_elevation_fee;

   FC_ASSERT( dgpo.smt_creation_fee.symbol == STEEM_SYMBOL || dgpo.smt_creation_fee.symbol == SBD_SYMBOL,
      "Unexpected internal error - wrong symbol ${s} of account elevation fee.", ("s", dgpo.smt_creation_fee.symbol) );
   FC_ASSERT( o.fee.symbol == STEEM_SYMBOL || o.fee.symbol == SBD_SYMBOL,
      "Asset fee must be STEEM or SBD, was ${s}", ("s", o.fee.symbol) );
   if( o.fee.symbol == dgpo.smt_creation_fee.symbol )
   {
      effective_elevation_fee = dgpo.smt_creation_fee;
   }
   else
   {
      const auto& fhistory = _db.get_feed_history();
      FC_ASSERT( !fhistory.current_median_history.is_null(), "Cannot pay the fee using SBD because there is no price feed." );
      if( o.fee.symbol == STEEM_SYMBOL )
      {
         effective_elevation_fee = _db.to_sbd( o.fee );
      }
      else
      {
         effective_elevation_fee = _db.to_steem( o.fee );         
      }
   }

   const account_object& acct = _db.get_account( o.account );
   FC_ASSERT( o.fee >= effective_elevation_fee, "Fee of ${of} is too small, must be at least ${ef}", ("of", o.fee)("ef", effective_elevation_fee) );
   FC_ASSERT( _db.get_balance( acct, o.fee.symbol ) >= o.fee, "Account does not have sufficient funds for specified fee of ${of}", ("of", o.fee) );

   const account_object& null_account = _db.get_account( STEEM_NULL_ACCOUNT );

   _db.adjust_balance( acct        , -o.fee );
   _db.adjust_balance( null_account,  o.fee );

   _db.create< smt_token_object >( [&]( smt_token_object& token )
   {
      token.control_account = o.account;
   });
}

void smt_setup_evaluator::do_apply( const smt_setup_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEM_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEM_SMT_HARDFORK) );
}

void smt_cap_reveal_evaluator::do_apply( const smt_cap_reveal_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEM_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEM_SMT_HARDFORK) );
}

void smt_refund_evaluator::do_apply( const smt_refund_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEM_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEM_SMT_HARDFORK) );
}

void smt_setup_inflation_evaluator::do_apply( const smt_setup_inflation_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEM_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEM_SMT_HARDFORK) );
}

void smt_set_setup_parameters_evaluator::do_apply( const smt_set_setup_parameters_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEM_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEM_SMT_HARDFORK) );
   auto& smt_token_by_account = _db.get_index<smt_token_index, by_control_account>();
   auto it = smt_token_by_account.find(o.control_account);
   FC_ASSERT( it != smt_token_by_account.end(), "SMT ${ac} not elevated yet.", ("ac", o.control_account));
   const smt_token_object& smt_token = *it;
   FC_ASSERT( smt_token.phase < smt_token_object::smt_phase::setup_completed, "SMT ${ac} setup already completed.", ("ac", o.control_account));
   
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

   const auto* _token = _db.find< smt_token_object, by_control_account >( o.control_account );
   FC_ASSERT( _token, "SMT ${ac} not elevated yet.",("ac", o.control_account) );

   smt_set_runtime_parameters_evaluator_visitor visitor( *_token, _db );

   for( auto& param: o.runtime_parameters )
      param.visit( visitor );
}

} }
#endif