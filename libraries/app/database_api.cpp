#include <steemit/app/api_context.hpp>
#include <steemit/app/application.hpp>
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
      vector< extended_account > get_accounts( vector< string > names )const;
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

      // signal handlers
      void on_applied_block( const chain::signed_block& b );

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
}

void database_api_impl::on_applied_block( const chain::signed_block& b )
{
   _block_applied_callback( fc::variant(signed_block_header(b) ) );
}

void database_api_impl::set_block_applied_callback( std::function<void(const variant& block_header)> cb )
{
   _block_applied_callback = cb;
   _block_applied_connection = connect_signal( _db.applied_block, *this, &database_api_impl::on_applied_block );
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

database_api::database_api( const steemit::app::api_context& ctx )
   : database_api( *ctx.app.chain_database() ) {}

database_api::~database_api() {}

database_api_impl::database_api_impl( steemit::chain::database& db ):_db(db)
{
   wlog("creating database api ${x}", ("x",int64_t(this)) );
}

database_api_impl::~database_api_impl()
{
   elog("freeing database api ${x}", ("x",int64_t(this)) );
}

void database_api::on_api_startup() {}

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

witness_schedule_object database_api::get_witness_schedule()const
{
   return witness_schedule_id_type()( my->_db );
}

hardfork_version database_api::get_hardfork_version()const
{
   return hardfork_property_id_type()( my->_db ).current_hardfork_version;
}

scheduled_hardfork database_api::get_next_scheduled_hardfork() const
{
   scheduled_hardfork shf;
   const auto& hpo = hardfork_property_id_type()( my->_db );
   shf.hf_version = hpo.next_hardfork;
   shf.live_time = hpo.next_hardfork_time;
   return shf;
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

vector< extended_account > database_api::get_accounts( vector< string > names )const
{
   return my->get_accounts( names );
}

vector< extended_account > database_api_impl::get_accounts( vector< string > names )const
{
   const auto& idx  = _db.get_index_type< account_index >().indices().get< by_name >();
   const auto& vidx = _db.get_index_type< witness_vote_index >().indices().get< by_account_witness >();
   vector< extended_account > results;

   for( auto name: names )
   {
      auto itr = idx.find( name );
      if ( itr != idx.end() )
      {
         results.push_back( *itr );
         auto vitr = vidx.lower_bound( boost::make_tuple( itr->get_id(), witness_id_type() ) );
         while( vitr != vidx.end() && vitr->account == itr->get_id() ) {
            results.back().witness_votes.insert(vitr->witness(_db).owner);
            ++vitr;
         }
      }
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

vector< witness_object > database_api::get_witnesses_by_vote( string from, uint32_t limit )const
{
   //idump((from)(limit));
   FC_ASSERT( limit <= 100 );

   vector<witness_object> result;
   result.reserve(limit);

   const auto& name_idx = my->_db.get_index_type< witness_index >().indices().get< by_name >();
   const auto& vote_idx = my->_db.get_index_type< witness_index >().indices().get< by_vote_name >();

   auto itr = vote_idx.begin();
   if( from.size() ) {
      auto nameitr = name_idx.find( from );
      FC_ASSERT( nameitr != name_idx.end(), "invalid witness name ${n}", ("n",from) );
      itr = vote_idx.iterator_to( *nameitr );
   }

   while( itr != vote_idx.end()  &&
          result.size() < limit &&
          itr->votes > 0 )
   {
      result.push_back(*itr);
      ++itr;
   }
   return result;
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
      result.active_votes = get_active_votes( author, permlink );
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
      result.push_back(vote_state{vo.name,itr->weight,itr->rshares,itr->vote_percent,itr->last_update});
      ++itr;
   }
   return result;
}
vector<account_vote> database_api::get_account_votes( string voter )const {
   vector<account_vote> result;

   const auto& voter_acnt = my->_db.get_account(voter);
   const auto& idx = my->_db.get_index_type<comment_vote_index>().indices().get< by_voter_comment >();

   account_id_type aid(voter_acnt.id);
   auto itr = idx.lower_bound( aid );
   auto end = idx.upper_bound( aid );
   while( itr != end )
   {
      const auto& vo = itr->comment(my->_db);
      result.push_back(account_vote{(vo.author+"/"+vo.permlink),itr->weight,itr->rshares,itr->vote_percent, itr->last_update});
      ++itr;
   }
   return result;
}
u256 to256( const fc::uint128& t ) {
   u256 result( t.high_bits() );
   result <<= 65;
   result += t.low_bits();
   return result;
}

void database_api::set_pending_payout( discussion& d )const
{
   const auto& props = my->_db.get_dynamic_global_properties();
   const auto& hist  = my->_db.get_feed_history();
   asset pot = props.total_reward_fund_steem;
   if( !hist.current_median_history.is_null() ) pot = pot * hist.current_median_history;

   u256 total_r2 = to256( props.total_reward_shares2 );

   if( props.total_reward_shares2 > 0 ){
      int64_t abs_net_rshares = llabs(d.net_rshares.value);

      u256 r2 = to256(abs_net_rshares);
      r2 *= r2;
      r2 *= pot.amount.value;
      r2 /= total_r2;

      u256 tpp = to256(d.children_rshares2);
      tpp *= pot.amount.value;
      tpp /= total_r2;

      d.pending_payout_value = asset( static_cast<uint64_t>(r2), pot.symbol );
      d.total_pending_payout_value = asset( static_cast<uint64_t>(tpp), pot.symbol );

      if( d.net_rshares.value < 0 ) {
         d.pending_payout_value.amount.value *= -1;
      }
   }
   set_url(d);
}
void database_api::set_url( discussion& d )const {
   const comment_object* root = &d;
   while( root->parent_author.size() ) {
      root = &my->_db.get_comment( root->parent_author, root->parent_permlink );
   }
   d.url = "/" + root->category + "/@" + root->author + "/" + root->permlink;
   d.root_title = root->title;
   if( root != &d )
      d.url += "#@" + d.author + "/" + d.permlink;
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

/**
 *  This method can be used to fetch replies to start_auth
 */
vector<discussion> database_api::get_all_discussions_by_last_update( string start_parent_author, string start_permlink, uint32_t limit )const {

   idump((start_parent_author)(start_permlink)(limit) );
   const auto& last_update_idx = my->_db.get_index_type< comment_index >().indices().get< by_last_update >();

   auto itr = last_update_idx.begin();


   bool filter_by_parent_author = false;
   if( start_permlink.size() )
      itr = last_update_idx.iterator_to( my->_db.get_comment( start_parent_author, start_permlink ) );
   else if( start_parent_author.size() ) {
      itr = last_update_idx.lower_bound( boost::make_tuple( start_parent_author, time_point_sec::maximum(), object_id_type() ) );
      filter_by_parent_author = true;
   }

   vector<discussion> result;
   while( itr != last_update_idx.end() && result.size() < limit  ) {
      if( filter_by_parent_author && itr->parent_author != start_parent_author ) {
         return result;
      }

      result.push_back( *itr );
      set_pending_payout(result.back());
      result.back().active_votes = get_active_votes( itr->author, itr->permlink );
      ++itr;
   }
   return result;
}

vector<discussion> database_api::get_all_discussions_by_created( string start_parent_author, string start_permlink, uint32_t limit )const {

   idump((start_parent_author)(start_permlink)(limit) );
   const auto& last_update_idx = my->_db.get_index_type< comment_index >().indices().get< by_created >();

   auto itr = last_update_idx.begin();


   bool filter_by_parent_author = false;
   if( start_permlink.size() )
      itr = last_update_idx.iterator_to( my->_db.get_comment( start_parent_author, start_permlink ) );
   else if( start_parent_author.size() ) {
      itr = last_update_idx.lower_bound( boost::make_tuple( start_parent_author, time_point_sec::maximum(), object_id_type() ) );
      filter_by_parent_author = true;
   }

   vector<discussion> result;
   while( itr != last_update_idx.end() && result.size() < limit  ) {
      if( filter_by_parent_author && itr->parent_author != start_parent_author ) {
         return result;
      }

      result.push_back( *itr );
      set_pending_payout(result.back());
      result.back().active_votes = get_active_votes( itr->author, itr->permlink );
      ++itr;
   }
   return result;
}

vector<discussion> database_api::get_all_discussions_by_votes( string start_parent_author, string start_permlink, uint32_t limit )const {

   idump((start_parent_author)(start_permlink)(limit) );
   const auto& votes_idx = my->_db.get_index_type< comment_index >().indices().get< by_votes >();

   auto itr = votes_idx.begin();


   bool filter_by_parent_author = false;
   if( start_permlink.size() )
      itr = votes_idx.iterator_to( my->_db.get_comment( start_parent_author, start_permlink ) );
   else if( start_parent_author.size() ) {
      itr = votes_idx.lower_bound( boost::make_tuple( start_parent_author, std::numeric_limits<int32_t>::max(), object_id_type() ) );
      filter_by_parent_author = true;
   }

   vector<discussion> result;
   while( itr != votes_idx.end() && result.size() < limit  ) {
      if( filter_by_parent_author && itr->parent_author != start_parent_author ) {
         return result;
      }

      result.push_back( *itr );
      set_pending_payout(result.back());
      result.back().active_votes = get_active_votes( itr->author, itr->permlink );
      ++itr;
   }
   return result;
}
vector<discussion> database_api::get_all_discussions_by_responses( string start_parent_author, string start_permlink, uint32_t limit )const {

   idump((start_parent_author)(start_permlink)(limit) );
   const auto& votes_idx = my->_db.get_index_type< comment_index >().indices().get< by_responses >();

   auto itr = votes_idx.begin();


   bool filter_by_parent_author = false;
   if( start_permlink.size() )
      itr = votes_idx.iterator_to( my->_db.get_comment( start_parent_author, start_permlink ) );
   else if( start_parent_author.size() ) {
      itr = votes_idx.lower_bound( boost::make_tuple( start_parent_author, std::numeric_limits<int32_t>::max(), object_id_type() ) );
      filter_by_parent_author = true;
   }

   vector<discussion> result;
   while( itr != votes_idx.end() && result.size() < limit  ) {
      if( filter_by_parent_author && itr->parent_author != start_parent_author ) {
         return result;
      }

      result.push_back( *itr );
      set_pending_payout(result.back());
      result.back().active_votes = get_active_votes( itr->author, itr->permlink );
      ++itr;
   }
   return result;
}

vector<discussion> database_api::get_replies_by_last_update( string start_parent_author, string start_permlink, uint32_t limit )const {

   idump((start_parent_author)(start_permlink)(limit) );
   const auto& last_update_idx = my->_db.get_index_type< comment_index >().indices().get< by_last_update >();

   auto itr = last_update_idx.begin();


   bool filter_by_parent_author = true;
   if( start_permlink.size() )
      itr = last_update_idx.iterator_to( my->_db.get_comment( start_parent_author, start_permlink ) );
   else if( start_parent_author.size() ) {
      itr = last_update_idx.lower_bound( boost::make_tuple( start_parent_author, time_point_sec::maximum(), object_id_type() ) );
      filter_by_parent_author = true;
   }

   vector<discussion> result;
   while( itr != last_update_idx.end() && result.size() < limit  ) {
      if( filter_by_parent_author && itr->parent_author != start_parent_author ) {
         return result;
      }

      result.push_back( *itr );
      set_pending_payout(result.back());
      result.back().active_votes = get_active_votes( itr->author, itr->permlink );
      ++itr;
   }
   return result;
}




vector<discussion> database_api::get_all_discussions_by_last_active( string start_parent_author, string start_permlink, uint32_t limit )const {

   idump((start_parent_author)(start_permlink)(limit) );
   const auto& last_activity_idx = my->_db.get_index_type< comment_index >().indices().get< by_active >();

   auto itr = last_activity_idx.begin();


   if( start_permlink.size() )
      itr = last_activity_idx.iterator_to( my->_db.get_comment( start_parent_author, start_permlink ) );
   else if( start_parent_author.size() ) {
      itr = last_activity_idx.lower_bound( boost::make_tuple( start_parent_author, time_point_sec::maximum(), object_id_type() ) );
   }

   vector<discussion> result;
   while( itr != last_activity_idx.end() && result.size() < limit  ) {
      result.push_back( *itr );
      set_pending_payout(result.back());
      result.back().active_votes = get_active_votes( itr->author, itr->permlink );
      ++itr;
   }
   return result;
}



vector<discussion> database_api::get_all_discussions_by_cashout_time( string start_auth, string start_permlink, uint32_t limit )const {
   const auto& cashout_time_idx = my->_db.get_index_type< comment_index >().indices().get< by_cashout_time >();

   auto itr = cashout_time_idx.begin();

   if( start_auth.size() )
      itr = cashout_time_idx.iterator_to( my->_db.get_comment( start_auth, start_permlink ) );

   vector<discussion> result;
   while( itr != cashout_time_idx.end() && result.size() < limit ) {
      idump((*itr));
      result.push_back( *itr );
      set_pending_payout(result.back());
      result.back().active_votes = get_active_votes( itr->author, itr->permlink );
      ++itr;
   }
   return result;
}



vector<discussion> database_api::get_all_discussions_by_total_pending_payout( string start_auth, string start_permlink, uint32_t limit )const {
   const auto& total_pending_payout_idx = my->_db.get_index_type< comment_index >().indices().get< by_total_pending_payout >();

   auto itr = total_pending_payout_idx.begin();

   if( start_auth.size() )
      itr = total_pending_payout_idx.iterator_to( my->_db.get_comment( start_auth, start_permlink ) );

   vector<discussion> result;
   while( itr != total_pending_payout_idx.end() && result.size() < limit && !itr->parent_author.size() ) {
      result.push_back( *itr );
      set_pending_payout(result.back());
      result.back().active_votes = get_active_votes( itr->author, itr->permlink );
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

vector<tags::tag_stats_object> database_api::get_trending_tags( string after, uint32_t limit )const {
  vector<tags::tag_stats_object> result;
  const auto& tags_stats_idx = my->_db.get_index_type<tags::tag_stats_index>().indices().get<tags::by_tag>();
  auto itr = tags_stats_idx.lower_bound(after);

  while( itr != tags_stats_idx.end() && limit > 0 ) {
     result.push_back(*itr);
     --limit; ++itr;
  }

  return result;
}

discussion database_api::get_discussion( comment_id_type id )const {
   discussion d = id(my->_db);
   set_url( d );
   set_pending_payout( d );
   return d;
}


template<typename Index, typename StartItr>
vector<discussion> database_api::get_discussions( const discussion_query& query, 
                                                  const string& tag, 
                                                  comment_id_type parent,
                                                  const Index& tidx, StartItr tidx_itr )const
{
   idump((query));
   vector<discussion> result;

   const auto& cidx = my->_db.get_index_type<tags::tag_index>().indices().get<tags::by_comment>();
   comment_id_type start;

   if( query.start_author && query.start_permlink ) {
      start = my->_db.get_comment( *query.start_author, *query.start_permlink ).id;
      auto itr = cidx.find( start );
      while( itr != cidx.end() && itr->comment == start ) {
         if( itr->tag == tag ) {
            tidx_itr = tidx.iterator_to( *itr );
            break;
         }
         ++itr;
      }
   }

   uint32_t count = query.limit;
   while( count > 0 && tidx_itr != tidx.end() ) {
      if( tidx_itr->tag != tag || tidx_itr->parent != parent ) break;

      result.push_back( get_discussion( tidx_itr->comment ) );

      ++tidx_itr; --count;
   }
   return result;
}

comment_id_type database_api::get_parent( const discussion_query& query )const {
   comment_id_type parent;
   if( query.parent_author && query.parent_permlink ) {
      parent = my->_db.get_comment( *query.parent_author, *query.parent_permlink ).id;
   }
   return parent;
}

vector<discussion> database_api::get_discussions_by_trending( const discussion_query& query )const {
   query.validate();
   auto tag = fc::to_lower( query.tag );
   auto parent = get_parent( query );

   const auto& tidx = my->_db.get_index_type<tags::tag_index>().indices().get<tags::by_parent_children_rshares2>();
   auto tidx_itr = tidx.lower_bound( boost::make_tuple( tag, parent, fc::uint128_t::max_value() )  );

   return get_discussions( query, tag, parent, tidx, tidx_itr );
}



vector<discussion> database_api::get_discussions_by_created( const discussion_query& query )const {
   query.validate();
   auto tag = fc::to_lower( query.tag );
   auto parent = get_parent( query );

   const auto& tidx = my->_db.get_index_type<tags::tag_index>().indices().get<tags::by_parent_created>();
   auto tidx_itr = tidx.lower_bound( boost::make_tuple( tag, parent, fc::time_point_sec::maximum() )  );

   return get_discussions( query, tag, parent, tidx, tidx_itr );
}

vector<discussion> database_api::get_discussions_by_active( const discussion_query& query )const {
   query.validate();
   auto tag = fc::to_lower( query.tag );
   auto parent = get_parent( query );

   const auto& tidx = my->_db.get_index_type<tags::tag_index>().indices().get<tags::by_parent_active>();
   auto tidx_itr = tidx.lower_bound( boost::make_tuple( tag, parent, fc::time_point_sec::maximum() )  );

   return get_discussions( query, tag, parent, tidx, tidx_itr );
}

vector<discussion> database_api::get_discussions_by_cashout( const discussion_query& query )const {
   vector<discussion> result;
   return result;
}
vector<discussion> database_api::get_discussions_by_payout( const discussion_query& query )const {
   vector<discussion> result;
   return result;
}
vector<discussion> database_api::get_discussions_by_votes( const discussion_query& query )const {
   query.validate();
   auto tag = fc::to_lower( query.tag );
   auto parent = get_parent( query );

   const auto& tidx = my->_db.get_index_type<tags::tag_index>().indices().get<tags::by_parent_net_votes>();
   auto tidx_itr = tidx.lower_bound( boost::make_tuple( tag, parent, std::numeric_limits<int32_t>::max() )  );

   return get_discussions( query, tag, parent, tidx, tidx_itr );
}
vector<discussion> database_api::get_discussions_by_children( const discussion_query& query )const {
   query.validate();
   auto tag = fc::to_lower( query.tag );
   auto parent = get_parent( query );

   const auto& tidx = my->_db.get_index_type<tags::tag_index>().indices().get<tags::by_parent_children>();
   auto tidx_itr = tidx.lower_bound( boost::make_tuple( tag, parent, std::numeric_limits<int32_t>::max() )  );

   return get_discussions( query, tag, parent, tidx, tidx_itr );
}



vector<category_object> database_api::get_trending_categories( string after, uint32_t limit )const {
   limit = std::min( limit, uint32_t(100) );
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
   limit = std::min( limit, uint32_t(100) );
   vector<category_object> result; result.reserve( limit );
   return result;
}

vector<category_object> database_api::get_active_categories( string after, uint32_t limit )const {
   limit = std::min( limit, uint32_t(100) );
   vector<category_object> result; result.reserve( limit );
   return result;
}

vector<category_object> database_api::get_recent_categories( string after, uint32_t limit )const {
   limit = std::min( limit, uint32_t(100) );
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

vector<discussion>  database_api::get_discussions_by_author_before_date(
    string author, string start_permlink, time_point_sec before_date, uint32_t limit )const
{ try {
     vector<discussion> result;

     FC_ASSERT( limit <= 100 );
     int count = 0;
     const auto& didx = my->_db.get_index_type<comment_index>().indices().get<by_author_last_update>();

     if( before_date == time_point_sec() )
        before_date = time_point_sec::maximum();

     auto itr = didx.lower_bound( boost::make_tuple( author, time_point_sec::maximum() ) );
     if( start_permlink.size() ) {
        const auto& comment = my->_db.get_comment( author, start_permlink );
        if( comment.created < before_date )
           itr = didx.iterator_to(comment);
     }


     while( itr != didx.end() && itr->author ==  author && count < limit ) {
        result.push_back( *itr );
        set_pending_payout( result.back() );
        result.back().active_votes = get_active_votes( itr->author, itr->permlink );
        ++itr;
        ++count;
     }

     return result;
} FC_CAPTURE_AND_RETHROW( (author)(start_permlink)(before_date)(limit) ) }


state database_api::get_state( string path )const
{
   state _state;
   _state.props         = get_dynamic_global_properties();
   _state.current_route = path;
   _state.feed_price    = get_current_median_history_price();

   try {
   if( path.size() && path[0] == '/' )
      path = path.substr(1); /// remove '/' from front

   if( !path.size() )
      path = "trending";

   /// FETCH CATEGORY STATE
   auto trending_cat = get_trending_categories( "", 100 );
   for( const auto& c : trending_cat )
   {
      _state.category_idx.trending.push_back(c.name);
      _state.categories[c.name] = c;
   }
   auto best_cat     = get_best_categories( "", 50 );
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
      if( part[1] == "transfers" ) {
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
               //   eacnt.post_history[item.first] =  item.second;
                  break;
               case operation::tag<limit_order_create_operation>::value:
               case operation::tag<limit_order_cancel_operation>::value:
               case operation::tag<fill_convert_request_operation>::value:
               case operation::tag<fill_order_operation>::value:
               //   eacnt.market_history[item.first] =  item.second;
                  break;
               case operation::tag<vote_operation>::value:
               case operation::tag<account_witness_vote_operation>::value:
               case operation::tag<account_witness_proxy_operation>::value:
               //   eacnt.vote_history[item.first] =  item.second;
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
      } else if( part[1] == "recent-replies" ) {
        auto replies = get_replies_by_last_update( acnt, "", 50 );
        eacnt.recent_replies = vector<string>();
        for( const auto& reply : replies ) {
           auto reply_ref = reply.author+"/"+reply.permlink;
           _state.content[ reply_ref ] = reply;
           eacnt.recent_replies->push_back( reply_ref );
        }
      } else if( part[1] == "posts" ) {
        int count = 0;
        const auto& pidx = my->_db.get_index_type<comment_index>().indices().get<by_author_last_update>();
        auto itr = pidx.lower_bound( boost::make_tuple(acnt, time_point_sec::maximum() ) );
        eacnt.posts = vector<string>();
        while( itr != pidx.end() && itr->author == acnt && count < 100 ) {
           eacnt.posts->push_back(itr->permlink);
           _state.content[acnt+"/"+itr->permlink] = *itr;
           set_pending_payout( _state.content[acnt+"/"+itr->permlink] );
           ++itr;
           ++count;
        }
      } else if( part[1].size() == 0 || part[1] == "blog" ) {
           int count = 0;
           const auto& pidx = my->_db.get_index_type<comment_index>().indices().get<by_blog>();
           auto itr = pidx.lower_bound( boost::make_tuple(acnt, std::string(""), time_point_sec::maximum() ) );
           eacnt.blog = vector<string>();
           while( itr != pidx.end() && itr->author == acnt && count < 100 && !itr->parent_author.size() ) {
              eacnt.blog->push_back(itr->permlink);
              _state.content[acnt+"/"+itr->permlink] = *itr;
              set_pending_payout( _state.content[acnt+"/"+itr->permlink] );
              ++itr;
              ++count;
           }
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
   else if( part[0] == "witnesses" || part[0] == "~witnesses") {
      auto wits = get_witnesses_by_vote( "", 50 );
      for( const auto& w : wits ) {
         _state.witnesses[w.owner] = w;
      }
      _state.pow_queue = get_miner_queue();
   }
   else if( part[0] == "trending" && part[1].size() ) {
      auto trending_disc = get_discussions_by_trending( {part[1],20} );

      auto& didx = _state.discussion_idx[part[1]];
      for( const auto& d : trending_disc ) {
         auto key = d.author +"/" + d.permlink;
         didx.trending.push_back( key );
         if( d.author.size() ) accounts.insert(d.author);
         _state.content[key] = std::move(d);
      }
   }
   else if( part[0] == "responses" && part[1].size() ) {
      auto trending_disc = get_discussions_by_children( {part[1],20} );

      auto& didx = _state.discussion_idx[part[1]];
      for( const auto& d : trending_disc ) {
         auto key = d.author +"/" + d.permlink;
         didx.responses.push_back( key );
         if( d.author.size() ) accounts.insert(d.author);
         _state.content[key] = std::move(d);
      }
   }
   else if( part[0] == "votes" && part[1].size() ) {
      auto trending_disc = get_discussions_by_votes( {part[1],20} );

      auto& didx = _state.discussion_idx[part[1]];
      for( const auto& d : trending_disc ) {
         auto key = d.author +"/" + d.permlink;
         didx.votes.push_back( key );
         if( d.author.size() ) accounts.insert(d.author);
         _state.content[key] = std::move(d);
      }
   }
   else if( part[0] == "active" && part[1].size() ) {
      auto trending_disc = get_discussions_by_active( {part[1],20} );

      auto& didx = _state.discussion_idx[part[1]];
      for( const auto& d : trending_disc ) {
         auto key = d.author +"/" + d.permlink;
         didx.active.push_back( key );
         if( d.author.size() ) accounts.insert(d.author);
         _state.content[key] = std::move(d);
      }
   }
   else if( part[0] == "created" && part[1].size() ) {
      auto trending_disc = get_discussions_by_created( {part[1],20} );

      auto& didx = _state.discussion_idx[part[1]];
      for( const auto& d : trending_disc ) {
         auto key = d.author +"/" + d.permlink;
         didx.created.push_back( key );
         if( d.author.size() ) accounts.insert(d.author);
         _state.content[key] = std::move(d);
      }
   }
   else if( part[0] == "created" && part[1].size() ) {
      auto trending_disc = get_discussions_by_created( {part[1],20} );

      auto& didx = _state.discussion_idx[part[1]];
      for( const auto& d : trending_disc ) {
         auto key = d.author +"/" + d.permlink;
         didx.created.push_back( key );
         if( d.author.size() ) accounts.insert(d.author);
         _state.content[key] = std::move(d);
      }
   }
   else if( part[0] == "trending" || part[0] == "") {
      auto trending_disc = get_all_discussions_by_total_pending_payout( "", "", 20 );
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
      auto trending_disc = get_all_discussions_by_cashout_time( "", "", 50 );
      auto& didx = _state.discussion_idx[""];
      for( const auto& d : trending_disc ) {
         auto key = d.author +"/" + d.permlink;
         didx.maturing.push_back( key );
         accounts.insert(d.author);
         _state.content[key] = std::move(d);
      }
   }
   else if( (part[0] == "recent" || part[0] == "created") && part[1] == "") {
      auto trending_disc = get_all_discussions_by_created( "", "", 20 );
      auto& didx = _state.discussion_idx[""];
      for( const auto& d : trending_disc ) {
         auto key = d.author +"/" + d.permlink;
         didx.created.push_back( key );
         accounts.insert(d.author);
         _state.content[key] = std::move(d);
      }
   } 
   else if( part[0] == "updated" && part[1] == "") {
      auto trending_disc = get_all_discussions_by_last_update( "", "", 20 );
      auto& didx = _state.discussion_idx[""];
      for( const auto& d : trending_disc ) {
         auto key = d.author +"/" + d.permlink;
         didx.updated.push_back( key );
         accounts.insert(d.author);
         _state.content[key] = std::move(d);
      }
   } 
   else if( part[0] == "active" && part[1] == "") {
      auto trending_disc = get_all_discussions_by_last_active( "", "", 20 );
      auto& didx = _state.discussion_idx[""];
      for( const auto& d : trending_disc ) {
         auto key = d.author +"/" + d.permlink;
         didx.active.push_back( key );
         accounts.insert(d.author);
         _state.content[key] = std::move(d);
      }
   } 
   else if( part[0] == "responses" && part[1] == "") {
      auto trending_disc = get_all_discussions_by_responses( "", "", 20 );
      auto& didx = _state.discussion_idx[""];
      for( const auto& d : trending_disc ) {
         auto key = d.author +"/" + d.permlink;
         didx.responses.push_back( key );
         accounts.insert(d.author);
         _state.content[key] = std::move(d);
      }
   } 
   else if( part[0] == "votes" && part[1] == "") {
      auto trending_disc = get_all_discussions_by_votes( "", "", 20 );
      auto& didx = _state.discussion_idx[""];
      for( const auto& d : trending_disc ) {
         auto key = d.author +"/" + d.permlink;
         didx.votes.push_back( key );
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

   _state.witness_schedule = my->_db.get_witness_schedule_object();

 } catch ( const fc::exception& e ) {
    _state.error = e.to_detail_string();
 }
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
