/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <steemit/app/database_api.hpp>
#include <steemit/chain/get_config.hpp>
#include <steemit/chain/steem_objects.hpp>
#include <fc/bloom_filter.hpp>
#include <fc/smart_ref_impl.hpp>

#include <fc/crypto/hex.hpp>

#include <boost/range/iterator_range.hpp>
#include <boost/algorithm/string.hpp>

#include <cctype>

#include <cfenv>
#include <iostream>

#define GET_REQUIRED_FEES_MAX_RECURSION 4

namespace steemit { namespace app {

class database_api_impl;


class database_api_impl : public std::enable_shared_from_this<database_api_impl>
{
   public:
      database_api_impl( steemit::chain::database& db );
      ~database_api_impl();

      // Objects
      fc::variants get_objects(const vector<object_id_type>& ids)const;

      // Subscriptions
      void set_subscribe_callback( std::function<void(const variant&)> cb, bool clear_filter );
      void set_pending_transaction_callback( std::function<void(const variant&)> cb );
      void set_block_applied_callback( std::function<void(const variant& block_id)> cb );
      void cancel_all_subscriptions();

      // Blocks and transactions
      optional<block_header> get_block_header(uint32_t block_num)const;
      optional<signed_block> get_block(uint32_t block_num)const;

      // Globals
      fc::variant_object get_config()const;
      dynamic_global_property_object get_dynamic_global_properties()const;

      // Keys
      vector<set<string>> get_key_references( vector<public_key_type> key )const;

      // Accounts
      vector< account_object > get_accounts( vector< string > names )const;
      vector<account_id_type> get_account_references( account_id_type account_id )const;
      vector<optional<account_object>> lookup_account_names(const vector<string>& account_names)const;
      set<string> lookup_accounts(const string& lower_bound_name, uint32_t limit)const;
      uint64_t get_account_count()const;

      // Witnesses
      vector<optional<witness_object>> get_witnesses(const vector<witness_id_type>& witness_ids)const;
      fc::optional<witness_object> get_witness_by_account( string account_name )const;
      set<string> lookup_witness_accounts(const string& lower_bound_name, uint32_t limit)const;
      uint64_t get_witness_count()const;

      // Market
      order_book get_order_book( uint32_t limit )const;

      // Authority / validation
      std::string get_transaction_hex(const signed_transaction& trx)const;
      set<public_key_type> get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const;
      set<public_key_type> get_potential_signatures( const signed_transaction& trx )const;
      bool verify_authority( const signed_transaction& trx )const;
      bool verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& signers )const;

      mutable fc::bloom_filter                _subscribe_filter;
      std::function<void(const fc::variant&)> _subscribe_callback;
      std::function<void(const fc::variant&)> _pending_trx_callback;
      std::function<void(const fc::variant&)> _block_applied_callback;

      steemit::chain::database&                _db;

      boost::signals2::scoped_connection       _block_applied_connection;


};

void find_accounts( set<string>& accounts, const discussion& d ) {
   accounts.insert( d.author );
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Subscriptions                                                    //
//                                                                  //
//////////////////////////////////////////////////////////////////////

void database_api::set_subscribe_callback( std::function<void(const variant&)> cb, bool clear_filter )
{
   my->set_subscribe_callback( cb, clear_filter );
}

void database_api_impl::set_subscribe_callback( std::function<void(const variant&)> cb, bool clear_filter )
{
   edump((clear_filter));
   _subscribe_callback = cb;
   if( clear_filter || !cb )
   {
      static fc::bloom_parameters param;
      param.projected_element_count    = 10000;
      param.false_positive_probability = 1.0/10000;
      param.maximum_size = 1024*8*8*2;
      param.compute_optimal_parameters();
      _subscribe_filter = fc::bloom_filter(param);
   }
}

void database_api::set_pending_transaction_callback( std::function<void(const variant&)> cb )
{
   my->set_pending_transaction_callback( cb );
}

void database_api_impl::set_pending_transaction_callback( std::function<void(const variant&)> cb )
{
   _pending_trx_callback = cb;
}

void database_api::set_block_applied_callback( std::function<void(const variant& block_id)> cb )
{
   my->set_block_applied_callback( cb );
   my->_block_applied_connection = my->_db.applied_block.connect( [&]( const chain::signed_block& b ) {
    my->_block_applied_callback( fc::variant(signed_block_header(b) ) );
   });
}

void database_api_impl::set_block_applied_callback( std::function<void(const variant& block_header)> cb )
{
   _block_applied_callback = cb;
}

void database_api::cancel_all_subscriptions()
{
   my->cancel_all_subscriptions();
}

void database_api_impl::cancel_all_subscriptions()
{
   set_subscribe_callback( std::function<void(const fc::variant&)>(), true);
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Constructors                                                     //
//                                                                  //
//////////////////////////////////////////////////////////////////////

database_api::database_api( steemit::chain::database& db )
   : my( new database_api_impl( db ) ) {}

database_api::~database_api() {}

database_api_impl::database_api_impl( steemit::chain::database& db ):_db(db)
{
   wlog("creating database api ${x}", ("x",int64_t(this)) );
}

database_api_impl::~database_api_impl()
{
   elog("freeing database api ${x}", ("x",int64_t(this)) );
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Blocks and transactions                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////

optional<block_header> database_api::get_block_header(uint32_t block_num)const
{
   return my->get_block_header( block_num );
}

optional<block_header> database_api_impl::get_block_header(uint32_t block_num) const
{
   auto result = _db.fetch_block_by_number(block_num);
   if(result)
      return *result;
   return {};
}

optional<signed_block> database_api::get_block(uint32_t block_num)const
{
   return my->get_block( block_num );
}

optional<signed_block> database_api_impl::get_block(uint32_t block_num)const
{
   return _db.fetch_block_by_number(block_num);
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Globals                                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////=

fc::variant_object database_api::get_config()const
{
   return my->get_config();
}

fc::variant_object database_api_impl::get_config()const
{
   return steemit::chain::get_config();
}

dynamic_global_property_object database_api::get_dynamic_global_properties()const
{
   return my->get_dynamic_global_properties();
}

chain_properties database_api::get_chain_properties()const
{
   return my->_db.get_witness_schedule_object().median_props;
}

feed_history_object database_api::get_feed_history()const {
   return my->_db.get_feed_history();
}

price database_api::get_current_median_history_price()const {
   return my->_db.get_feed_history().current_median_history;
}

dynamic_global_property_object database_api_impl::get_dynamic_global_properties()const
{
   return _db.get(dynamic_global_property_id_type());
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Keys                                                             //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<set<string>> database_api::get_key_references( vector<public_key_type> key )const
{
   return my->get_key_references( key );
}

/**
 *  @return all accounts that referr to the key or account id in their owner or active authorities.
 */
vector<set<string>> database_api_impl::get_key_references( vector<public_key_type> keys )const
{
   vector< set<string> > final_result;
   final_result.reserve(keys.size());

   for( auto& key : keys )
   {
      const auto& idx = _db.get_index_type<account_index>();
      const auto& aidx = dynamic_cast<const primary_index<account_index>&>(idx);
      const auto& refs = aidx.get_secondary_index<steemit::chain::account_member_index>();
      set<string> result;

      auto itr = refs.account_to_key_memberships.find(key);
      if( itr != refs.account_to_key_memberships.end() )
      {
         for( auto item : itr->second )
            result.insert(item);
      }
      final_result.emplace_back( std::move(result) );
   }

   return final_result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Accounts                                                         //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector< account_object > database_api::get_accounts( vector< string > names )const
{
   return my->get_accounts( names );
}

vector< account_object > database_api_impl::get_accounts( vector< string > names )const
{
   const auto& idx = _db.get_index_type< account_index >().indices().get< by_name >();
   vector< account_object > results;

   for( auto name: names )
   {
      auto itr = idx.find( name );
      if ( itr != idx.end() )
         results.push_back( *itr );
   }

   return results;
}

vector<account_id_type> database_api::get_account_references( account_id_type account_id )const
{
   return my->get_account_references( account_id );
}

vector<account_id_type> database_api_impl::get_account_references( account_id_type account_id )const
{
   /*const auto& idx = _db.get_index_type<account_index>();
   const auto& aidx = dynamic_cast<const primary_index<account_index>&>(idx);
   const auto& refs = aidx.get_secondary_index<steemit::chain::account_member_index>();
   auto itr = refs.account_to_account_memberships.find(account_id);
   vector<account_id_type> result;

   if( itr != refs.account_to_account_memberships.end() )
   {
      result.reserve( itr->second.size() );
      for( auto item : itr->second ) result.push_back(item);
   }
   return result;*/
   FC_ASSERT( false, "database_api::get_account_references --- Needs to be refactored for steem." );
}

vector<optional<account_object>> database_api::lookup_account_names(const vector<string>& account_names)const
{
   return my->lookup_account_names( account_names );
}

vector<optional<account_object>> database_api_impl::lookup_account_names(const vector<string>& account_names)const
{
   const auto& accounts_by_name = _db.get_index_type<account_index>().indices().get<by_name>();
   vector<optional<account_object> > result;
   result.reserve(account_names.size());
   std::transform(account_names.begin(), account_names.end(), std::back_inserter(result),
                  [&accounts_by_name](const string& name) -> optional<account_object> {
      auto itr = accounts_by_name.find(name);
      return itr == accounts_by_name.end()? optional<account_object>() : *itr;
   });
   return result;
}

set<string> database_api::lookup_accounts(const string& lower_bound_name, uint32_t limit)const
{
   return my->lookup_accounts( lower_bound_name, limit );
}

set<string> database_api_impl::lookup_accounts(const string& lower_bound_name, uint32_t limit)const
{
   FC_ASSERT( limit <= 1000 );
   const auto& accounts_by_name = _db.get_index_type<account_index>().indices().get<by_name>();
   set<string> result;

   for( auto itr = accounts_by_name.lower_bound(lower_bound_name);
        limit-- && itr != accounts_by_name.end();
        ++itr )
   {
      result.insert(itr->name);
   }

   return result;
}

uint64_t database_api::get_account_count()const
{
   return my->get_account_count();
}

uint64_t database_api_impl::get_account_count()const
{
   return _db.get_index_type<account_index>().indices().size();
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Witnesses                                                        //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<optional<witness_object>> database_api::get_witnesses(const vector<witness_id_type>& witness_ids)const
{
   return my->get_witnesses( witness_ids );
}

vector<optional<witness_object>> database_api_impl::get_witnesses(const vector<witness_id_type>& witness_ids)const
{
   vector<optional<witness_object>> result; result.reserve(witness_ids.size());
   std::transform(witness_ids.begin(), witness_ids.end(), std::back_inserter(result),
                  [this](witness_id_type id) -> optional<witness_object> {
      if(auto o = _db.find(id))
         return *o;
      return {};
   });
   return result;
}

fc::optional<witness_object> database_api::get_witness_by_account( string account_name ) const
{
   return my->get_witness_by_account( account_name );
}

fc::optional<witness_object> database_api_impl::get_witness_by_account( string account_name ) const
{
   const auto& idx = _db.get_index_type< witness_index >().indices().get< by_name >();
   auto itr = idx.find( account_name );
   if( itr != idx.end() )
      return *itr;
   return {};
}

set< string > database_api::lookup_witness_accounts( const string& lower_bound_name, uint32_t limit ) const
{
   return my->lookup_witness_accounts( lower_bound_name, limit );
}

set< string > database_api_impl::lookup_witness_accounts( const string& lower_bound_name, uint32_t limit ) const
{
   FC_ASSERT( limit <= 1000 );
   const auto& witnesses_by_id = _db.get_index_type< witness_index >().indices().get< by_id >();

   // get all the names and look them all up, sort them, then figure out what
   // records to return.  This could be optimized, but we expect the
   // number of witnesses to be few and the frequency of calls to be rare
   set< std::string > witnesses_by_account_name;
   for ( const witness_object& witness : witnesses_by_id )
      if ( witness.owner >= lower_bound_name ) // we can ignore anything below lower_bound_name
         witnesses_by_account_name.insert( witness.owner );

   auto end_iter = witnesses_by_account_name.begin();
   while ( end_iter != witnesses_by_account_name.end() && limit-- )
       ++end_iter;
   witnesses_by_account_name.erase( end_iter, witnesses_by_account_name.end() );
   return witnesses_by_account_name;
}

uint64_t database_api::get_witness_count()const
{
   return my->get_witness_count();
}

uint64_t database_api_impl::get_witness_count()const
{
   return _db.get_index_type<witness_index>().indices().size();
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Market                                                           //
//                                                                  //
//////////////////////////////////////////////////////////////////////

order_book database_api::get_order_book( uint32_t limit )const
{
   return my->get_order_book( limit );
}

order_book database_api_impl::get_order_book( uint32_t limit )const
{
   FC_ASSERT( limit <= 1000 );
   order_book result;
   const auto& order_idx = _db.get_index_type< limit_order_index >().indices().get< by_price >();
   auto end = order_idx.lower_bound( std::make_tuple( price( asset( 0, SBD_SYMBOL ), asset( 1, STEEM_SYMBOL ) ) ) );
   auto itr = order_idx.upper_bound( std::make_tuple( price( asset( ~0, SBD_SYMBOL ), asset( 1, STEEM_SYMBOL ) ) ) );

   while( itr != end && itr->sell_price.base.symbol == SBD_SYMBOL && result.bids.size() < limit )
   {
      order cur;
      cur.order_price = itr->sell_price;
      cur.sbd = itr->for_sale;
      cur.steem = ( asset( itr->for_sale, SBD_SYMBOL ) * cur.order_price ).amount;
      cur.created = itr->created;
      result.bids.push_back( cur );

      itr--;
   }

   end = order_idx.lower_bound( std::make_tuple( price( asset( 0, STEEM_SYMBOL ), asset( 1, SBD_SYMBOL ) ) ) );
   itr = order_idx.upper_bound( std::make_tuple( price( asset( ~0, STEEM_SYMBOL ), asset( 1, SBD_SYMBOL ) ) ) );

   while( itr != end && itr->sell_price.base.symbol == STEEM_SYMBOL && result.asks.size() < limit )
   {
      order cur;
      cur.order_price = itr->sell_price;
      cur.steem = itr->for_sale;
      cur.sbd = ( asset( itr->for_sale, STEEM_SYMBOL ) * cur.order_price ).amount;
      cur.created = itr->created;
      result.asks.push_back( cur );

      itr--;
   }

   return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Authority / validation                                           //
//                                                                  //
//////////////////////////////////////////////////////////////////////

std::string database_api::get_transaction_hex(const signed_transaction& trx)const
{
   return my->get_transaction_hex( trx );
}

std::string database_api_impl::get_transaction_hex(const signed_transaction& trx)const
{
   return fc::to_hex(fc::raw::pack(trx));
}

set<public_key_type> database_api::get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const
{
   return my->get_required_signatures( trx, available_keys );
}

set<public_key_type> database_api_impl::get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const
{
   wdump((trx)(available_keys));
   auto result = trx.get_required_signatures( STEEMIT_CHAIN_ID,
                                              available_keys,
                                              [&]( string account_name ){ return &_db.get_account( account_name ).active; },
                                              [&]( string account_name ){ return &_db.get_account( account_name ).owner; },
                                              [&]( string account_name ){ return &_db.get_account( account_name ).posting; },
                                              STEEMIT_MAX_SIG_CHECK_DEPTH );
   wdump((result));
   return result;
}

set<public_key_type> database_api::get_potential_signatures( const signed_transaction& trx )const
{
   return my->get_potential_signatures( trx );
}

set<public_key_type> database_api_impl::get_potential_signatures( const signed_transaction& trx )const
{
   wdump((trx));
   set<public_key_type> result;
   trx.get_required_signatures(
      STEEMIT_CHAIN_ID,
      flat_set<public_key_type>(),
      [&]( string account_name )
      {
         const auto& auth = _db.get_account(account_name).active;
         for( const auto& k : auth.get_keys() )
            result.insert(k);
         return &auth;
      },
      [&]( string account_name )
      {
         const auto& auth = _db.get_account(account_name).owner;
         for( const auto& k : auth.get_keys() )
            result.insert(k);
         return &auth;
      },
      [&]( string account_name )
      {
         const auto& auth = _db.get_account(account_name).posting;
         for( const auto& k : auth.get_keys() )
            result.insert(k);
         return &auth;
      },
      STEEMIT_MAX_SIG_CHECK_DEPTH
   );

   wdump((result));
   return result;
}

bool database_api::verify_authority( const signed_transaction& trx ) const
{
   return my->verify_authority( trx );
}

bool database_api_impl::verify_authority( const signed_transaction& trx )const
{
   trx.verify_authority( STEEMIT_CHAIN_ID,
                         [&]( string account_name ){ return &_db.get_account( account_name ).active; },
                         [&]( string account_name ){ return &_db.get_account( account_name ).owner; },
                         [&]( string account_name ){ return &_db.get_account( account_name ).posting; },
                         STEEMIT_MAX_SIG_CHECK_DEPTH );
   return true;
}

bool database_api::verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& signers )const
{
   return my->verify_account_authority( name_or_id, signers );
}

bool database_api_impl::verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& keys )const
{
   FC_ASSERT( name_or_id.size() > 0);
   const account_object* account = nullptr;
   if (std::isdigit(name_or_id[0]))
      account = _db.find(fc::variant(name_or_id).as<account_id_type>());
   else
   {
      const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
      auto itr = idx.find(name_or_id);
      if (itr != idx.end())
         account = &*itr;
   }
   FC_ASSERT( account, "no such account" );


   /// reuse trx.verify_authority by creating a dummy transfer
   signed_transaction trx;
   transfer_operation op;
   op.from = account->id;
   trx.operations.emplace_back(op);

   return verify_authority( trx );
}

vector<convert_request_object> database_api::get_conversion_requests( const string& account )const {
  const auto& idx = my->_db.get_index_type<convert_index>().indices().get<by_owner>();
  vector<convert_request_object> result;
  auto itr = idx.lower_bound(account);
  while( itr != idx.end() && itr->owner == account ) {
     result.push_back(*itr);
     ++itr;
  }
  return result;
}

discussion database_api::get_content( string author, string permlink )const {
   const auto& by_permlink_idx = my->_db.get_index_type< comment_index >().indices().get< by_permlink >();
   auto itr = by_permlink_idx.find( boost::make_tuple( author, permlink ) );
   if( itr != by_permlink_idx.end() )
   {
      discussion result(*itr);
      set_pending_payout(result);
      return result;
   }
   return discussion();
}

vector<vote_state> database_api::get_active_votes( string author, string permlink )const
{
   vector<vote_state> result;
   const auto& comment = my->_db.get_comment( author, permlink );
   const auto& idx = my->_db.get_index_type<comment_vote_index>().indices().get< by_comment_voter >();
   comment_id_type cid(comment.id);
   auto itr = idx.lower_bound( cid );
   while( itr != idx.end() && itr->comment == cid )
   {
      const auto& vo = itr->voter(my->_db);
      result.push_back(vote_state{vo.name,itr->weight});
      ++itr;
   }
   return result;

}

void database_api::set_pending_payout( discussion& d )const
{
   const auto& props = my->_db.get_dynamic_global_properties();
   const auto& hist  = my->_db.get_feed_history();
   asset pot = props.total_reward_fund_steem;
   if( !hist.current_median_history.is_null() ) pot = pot * hist.current_median_history;

   idump((pot)(props.total_reward_shares2));
   if( props.total_reward_shares2 > 0 ){
      fc::uint128_t r2(d.net_rshares.value);
      r2 *= r2;
      r2 *= pot.amount.value;
      r2 /= props.total_reward_shares2;
      idump((props.total_reward_shares2)(r2));

      auto tpp = d.children_rshares2;
      tpp *= pot.amount.value;
      tpp /= props.total_reward_shares2;

      d.pending_payout_value = asset( r2.to_uint64(), pot.symbol );
      d.total_pending_payout_value = asset( tpp.to_uint64(), pot.symbol );
   }
}

vector<discussion> database_api::get_content_replies( string author, string permlink )const {
   const auto& by_permlink_idx = my->_db.get_index_type< comment_index >().indices().get< by_parent >();
   auto itr = by_permlink_idx.find( boost::make_tuple( author, permlink ) );
   vector<discussion> result;
   while( itr != by_permlink_idx.end() && itr->parent_author == author && itr->parent_permlink == permlink )
   {
      result.push_back(*itr);
      set_pending_payout( result.back() );
      ++itr;
   }
   return result;
}

vector<discussion> database_api::get_discussions_by_last_update( string start_auth, string start_permlink, uint32_t limit )const {
   const auto& last_update_idx = my->_db.get_index_type< comment_index >().indices().get< by_last_update >();

   auto itr = last_update_idx.begin();

   if( start_auth.size() )
      itr = last_update_idx.iterator_to( my->_db.get_comment( start_auth, start_permlink ) );

   vector<discussion> result;
   while( itr != last_update_idx.end() && result.size() < limit && !itr->parent_author.size() ) {
      result.push_back( *itr );
      set_pending_payout(result.back());
      ++itr;
   }
   return result;
}

vector<discussion> database_api::get_discussions_by_cashout_time( string start_auth, string start_permlink, uint32_t limit )const {
   const auto& cashout_time_idx = my->_db.get_index_type< comment_index >().indices().get< by_cashout_time >();

   auto itr = cashout_time_idx.begin();

   if( start_auth.size() )
      itr = cashout_time_idx.iterator_to( my->_db.get_comment( start_auth, start_permlink ) );

   vector<discussion> result;
   while( itr != cashout_time_idx.end() && result.size() < limit ) {
      idump((*itr));
      result.push_back( *itr );
      set_pending_payout(result.back());
      ++itr;
   }
   return result;
}
vector<discussion> database_api::get_discussions_in_category_by_cashout_time( string category, string start_auth, string start_permlink, uint32_t limit )const {
   vector<discussion> result;
   const auto& cashout_time_in_category = my->_db.get_index_type< comment_index >().indices().get< by_cashout_time_in_category >();

   auto itr = cashout_time_in_category.lower_bound( boost::make_tuple( category, fc::time_point_sec::maximum() ) );

   if( start_auth.size() )
      itr = cashout_time_in_category.iterator_to( my->_db.get_comment( start_auth, start_permlink ) );

   while( itr != cashout_time_in_category.end() &&
          itr->parent_permlink == category &&
          result.size() < limit )
   {
      result.push_back( *itr );
      set_pending_payout(result.back());
      ++itr;
   }
   return result;
}

vector<discussion> database_api::get_discussions_in_category_by_last_update( string category, string start_auth, string start_permlink, uint32_t limit )const {
   const auto& last_update_in_category = my->_db.get_index_type< comment_index >().indices().get< by_last_update_in_category >();

   auto itr = last_update_in_category.lower_bound( boost::make_tuple( "", category, fc::time_point_sec::maximum() ) );

   if( start_auth.size() )
      itr = last_update_in_category.iterator_to( my->_db.get_comment( start_auth, start_permlink ) );

   vector<discussion> result;
   while( itr != last_update_in_category.end() &&
          itr->parent_permlink == category &&
          !itr->parent_author.size()
          && result.size() < limit )
   {
      result.push_back( *itr );
      set_pending_payout(result.back());
      ++itr;
   }
   return result;
}

vector<discussion> database_api::get_discussions_by_total_pending_payout( string start_auth, string start_permlink, uint32_t limit )const {
   const auto& total_pending_payout_idx = my->_db.get_index_type< comment_index >().indices().get< by_total_pending_payout >();

   auto itr = total_pending_payout_idx.begin();

   if( start_auth.size() )
      itr = total_pending_payout_idx.iterator_to( my->_db.get_comment( start_auth, start_permlink ) );

   vector<discussion> result;
   while( itr != total_pending_payout_idx.end() && result.size() < limit && !itr->parent_author.size() ) {
      result.push_back( *itr );
      set_pending_payout(result.back());
      ++itr;
   }
   return result;
}

vector<discussion> database_api::get_discussions_in_category_by_total_pending_payout( string category, string start_auth, string start_permlink, uint32_t limit )const {
   const auto& total_pending_payout_in_category = my->_db.get_index_type< comment_index >().indices().get< by_total_pending_payout_in_category >();

   auto itr = total_pending_payout_in_category.lower_bound( boost::make_tuple( "", category, fc::uint128_t::max_value() ) );

   if( start_auth.size() )
      itr = total_pending_payout_in_category.iterator_to( my->_db.get_comment( start_auth, start_permlink ) );

   vector<discussion> result;
   while( itr != total_pending_payout_in_category.end() &&
          itr->parent_permlink == category &&
          !itr->parent_author.size()
          && result.size() < limit )
   {
      result.push_back( *itr );
      set_pending_payout(result.back());
      ++itr;
   }
   return result;
}

map<uint32_t,operation_object> database_api::get_account_history( string account, uint64_t from, uint32_t limit )const {
   FC_ASSERT( limit <= 2000, "Limit of ${l} is greater than maxmimum allowed", ("l",limit) );
   FC_ASSERT( from >= limit, "From must be greater than limit" );
   idump((account)(from)(limit));
   const auto& idx = my->_db.get_index_type<account_history_index>().indices().get<by_account>();
   auto itr = idx.lower_bound( boost::make_tuple( account, from ) );
   if( itr != idx.end() ) idump((*itr));
   auto end = idx.upper_bound( boost::make_tuple( account, std::max( int64_t(0), int64_t(itr->sequence)-limit ) ) );
   if( end != idx.end() ) idump((*end));

   map<uint32_t,operation_object> result;
   while( itr != end ) {
      result[itr->sequence] = itr->op(my->_db);
      ++itr;
   }
   return result;
}

vector<category_object> database_api::get_trending_categories( string after, uint32_t limit )const {
   limit = std::min( limit, uint32_t(50) );
   vector<category_object> result; result.reserve( limit );

   const auto& nidx = my->_db.get_index_type<chain::category_index>().indices().get<by_name>();

   const auto& ridx = my->_db.get_index_type<chain::category_index>().indices().get<by_rshares>();
   auto itr = ridx.begin();
   if( after != "" && nidx.size() )
   {
      auto nitr = nidx.lower_bound( after );
      if( nitr == nidx.end() ) itr = ridx.end();
      else itr = ridx.iterator_to( *nitr );
   }

   while( itr != ridx.end() && result.size() < limit ) {
      result.push_back( *itr );
      ++itr;
   }
   return result;
}

vector<category_object> database_api::get_best_categories( string after, uint32_t limit )const {
   limit = std::min( limit, uint32_t(50) );
   vector<category_object> result; result.reserve( limit );
   return result;
}

vector<category_object> database_api::get_active_categories( string after, uint32_t limit )const {
   limit = std::min( limit, uint32_t(50) );
   vector<category_object> result; result.reserve( limit );
   return result;
}

vector<category_object> database_api::get_recent_categories( string after, uint32_t limit )const {
   limit = std::min( limit, uint32_t(50) );
   vector<category_object> result; result.reserve( limit );
   return result;
}


/**
 *  This call assumes root already stored as part of state, it will
 *  modify root.replies to contain links to the reply posts and then
 *  add the reply discussions to the state. This method also fetches
 *  any accounts referenced by authors.
 *
 */
void database_api::recursively_fetch_content( state& _state, discussion& root, set<string>& referenced_accounts )const
{
   if( root.author.size() )
     referenced_accounts.insert(root.author);

  auto replies = get_content_replies( root.author, root.permlink );
  for( auto& r : replies ) {
    recursively_fetch_content( _state, r, referenced_accounts );
    root.replies.push_back( r.author + "/" + r.permlink  );
    _state.content[r.author+"/"+r.permlink] = std::move(r);
    if( r.author.size() )
       referenced_accounts.insert(r.author);
  }
}

vector<string> database_api::get_miner_queue()const {
   vector<string> result;
   const auto& pow_idx = my->_db.get_index_type<witness_index>().indices().get<by_pow>();

   auto itr = pow_idx.upper_bound(0);
   while( itr != pow_idx.end() ) {
      if( itr->pow_worker )
         result.push_back( itr->owner );
      ++itr;
   }
   return result;
}

vector<string> database_api::get_active_witnesses()const {
   const auto& wso = my->_db.get_witness_schedule_object();
   return wso.current_shuffled_witnesses;
}

state database_api::get_state( string path )const
{
   if( path.size() && path[0] == '/' )
      path = path.substr(1); /// remove '/' from front

   if( !path.size() )
      path = "trending";

   state _state;
   _state.props         = get_dynamic_global_properties();
   _state.current_route = path;

   /// FETCH CATEGORY STATE
   auto trending_cat = get_trending_categories( "", 20 );
   for( const auto& c : trending_cat )
   {
      _state.category_idx.trending.push_back(c.name);
      _state.categories[c.name] = c;
   }
   auto recent_cat   = get_recent_categories( "", 20 );
   for( const auto& c : recent_cat )
   {
      _state.category_idx.active.push_back(c.name);
      _state.categories[c.name] = c;
   }
   auto active_cat   = get_active_categories( "", 20 );
   for( const auto& c : active_cat )
   {
      _state.category_idx.active.push_back(c.name);
      _state.categories[c.name] = c;
   }
   auto best_cat     = get_best_categories( "", 20 );
   for( const auto& c : best_cat )
   {
      _state.category_idx.best.push_back(c.name);
      _state.categories[c.name] = c;
   }
   /// END FETCH CATEGORY STATE

   set<string> accounts;

   vector<string> part; part.reserve(4);
   boost::split( part, path, boost::is_any_of("/") );
   part.resize(std::max( part.size(), size_t(4) ) ); // at least 4
   idump((part));

   if( part[0].size() && part[0][0] == '@' ) {
      auto acnt = part[0].substr(1);
      _state.accounts[acnt] = my->_db.get_account(acnt);
      auto& eacnt = _state.accounts[acnt];
      auto history = get_account_history( acnt, uint64_t(-1), 1000 );
      for( auto& item : history ) {
         switch( item.second.op.which() ) {
            case operation::tag<transfer_to_vesting_operation>::value:
            case operation::tag<withdraw_vesting_operation>::value:
            case operation::tag<interest_operation>::value:
            case operation::tag<transfer_operation>::value:
            case operation::tag<liquidity_reward_operation>::value:
            case operation::tag<comment_reward_operation>::value:
            case operation::tag<curate_reward_operation>::value:
               eacnt.transfer_history[item.first] =  item.second;
               break;
            case operation::tag<comment_operation>::value:
               eacnt.post_history[item.first] =  item.second;
               break;
            case operation::tag<limit_order_create_operation>::value:
            case operation::tag<limit_order_cancel_operation>::value:
            case operation::tag<fill_convert_request_operation>::value:
            case operation::tag<fill_order_operation>::value:
               eacnt.market_history[item.first] =  item.second;
               break;
            case operation::tag<vote_operation>::value:
            case operation::tag<account_witness_vote_operation>::value:
            case operation::tag<account_witness_proxy_operation>::value:
               eacnt.vote_history[item.first] =  item.second;
               break;
            case operation::tag<account_create_operation>::value:
            case operation::tag<account_update_operation>::value:
            case operation::tag<witness_update_operation>::value:
            case operation::tag<pow_operation>::value:
            case operation::tag<custom_operation>::value:
            default:
               eacnt.other_history[item.first] =  item.second;
         }
      }
      if( part[1] == "posts" ) {
        int count = 0;
        const auto& pidx = my->_db.get_index_type<comment_index>().indices().get<by_author_date>();
        auto itr = pidx.lower_bound( boost::make_tuple(acnt, time_point_sec::maximum() ) );
        while( itr != pidx.end() && itr->author == acnt && count < 100 ) {
           eacnt.posts.push_back(itr->permlink);
           ++itr;
           ++count;
        }
        _state.content[acnt+"/"+itr->permlink] = *itr;
      }
   }
   /// pull a complete discussion
   else if( part[1].size() && part[1][0] == '@' ) {
      auto account  = part[1].substr( 1 );
      auto category = part[0];
      auto slug     = part[2];

      auto key = account +"/" + slug;
      idump((key));
      auto dis = get_content( account, slug );
      recursively_fetch_content( _state, dis, accounts );
      _state.content[key] = std::move(dis);
   }
   else if( part[0] == "trending" && part[1].size() ) {
      auto trending_disc = get_discussions_in_category_by_total_pending_payout( part[1], "", "", 20 );

      auto& didx = _state.discussion_idx[part[1]];
      for( const auto& d : trending_disc ) {
         auto key = d.author +"/" + d.permlink;
         didx.trending.push_back( key );
         if( d.author.size() ) accounts.insert(d.author);
         _state.content[key] = std::move(d);
      }
   }
   else if( part[0] == "trending" || part[0] == "") {
      auto trending_disc = get_discussions_by_total_pending_payout( "", "", 20 );
      auto& didx = _state.discussion_idx[""];
      for( const auto& d : trending_disc ) {
         auto key = d.author +"/" + d.permlink;
         didx.trending.push_back( key );
         if( d.author.size() ) accounts.insert(d.author);
         _state.content[key] = std::move(d);
      }
   }
   else if( part[0] == "best" && part[1] == "" ) {
   }
   else if( part[0] == "maturing" && part[1] == "" ) {
      auto trending_disc = get_discussions_by_cashout_time( "", "", 20 );
      auto& didx = _state.discussion_idx[""];
      for( const auto& d : trending_disc ) {
         auto key = d.author +"/" + d.permlink;
         didx.maturing.push_back( key );
         accounts.insert(d.author);
         _state.content[key] = std::move(d);
      }
   }
   else if( part[0] == "maturing" ) {
      auto trending_disc = get_discussions_in_category_by_cashout_time( part[1], "", "", 20 );
      auto& didx = _state.discussion_idx[part[1]];
      for( const auto& d : trending_disc ) {
         auto key = d.author +"/" + d.permlink;
         didx.maturing.push_back( key );
         accounts.insert(d.author);
         _state.content[key] = std::move(d);
      }
   }
   else if( part[0] == "recent" && part[1] == "") {
      auto trending_disc = get_discussions_by_last_update( "", "", 20 );
      auto& didx = _state.discussion_idx[""];
      for( const auto& d : trending_disc ) {
         auto key = d.author +"/" + d.permlink;
         didx.recent.push_back( key );
         accounts.insert(d.author);
         _state.content[key] = std::move(d);
      }
   } else if( part[0] == "recent" ) {
      auto trending_disc = get_discussions_in_category_by_last_update( part[1], "", "", 20 );
      auto& didx = _state.discussion_idx[part[1]];
      for( const auto& d : trending_disc ) {
         auto key = d.author +"/" + d.permlink;
         didx.recent.push_back( key );
         accounts.insert(d.author);
         _state.content[key] = std::move(d);
      }
   }

   for( const auto& a : accounts )
   {
      _state.accounts.erase("");
      _state.accounts[a] = my->_db.get_account( a );
   }
   for( auto& d : _state.content ) {
      d.second.active_votes = get_active_votes( d.second.author, d.second.permlink );
   }

   _state.witnesses = my->_db.get_witness_schedule_object();

   return _state;
}

annotated_signed_transaction database_api::get_transaction( transaction_id_type id )const {
   const auto& idx = my->_db.get_index_type<operation_index>().indices().get<by_transaction_id>();
   auto itr = idx.lower_bound( id );
   if( itr != idx.end() && itr->trx_id == id ) {
      auto blk = my->_db.fetch_block_by_number( itr->block );
      FC_ASSERT( blk.valid() );
      FC_ASSERT( blk->transactions.size() > itr->trx_in_block );
      annotated_signed_transaction result = blk->transactions[itr->trx_in_block];
      result.block_num       = itr->block;
      result.transaction_num = itr->trx_in_block;
      return result;
   }
   FC_ASSERT( false, "Unknown Transaction ${t}", ("t",id));
}

} } // steemit::app
