#pragma once
#include <steemit/chain/protocol/base.hpp>
#include <steemit/chain/protocol/block_header.hpp>
#include <steemit/chain/protocol/asset.hpp>

#include <fc/utf8.hpp>

namespace steemit { namespace chain {
   struct account_create_operation : public base_operation
   {
      asset             fee;
      string            creator;
      string            new_account_name;
      authority         owner;
      authority         active;
      authority         posting;
      public_key_type   memo_key;
      string            json_metadata;

      void validate()const;
      void get_required_active_authorities( flat_set<string>& a )const{ a.insert(creator); }
   };



   struct account_update_operation : public base_operation
   {
      string                        account;
      optional< authority >         owner;
      optional< authority >         active;
      optional< authority >         posting;
      public_key_type               memo_key;
      string                        json_metadata;

      void validate()const;

      void get_required_owner_authorities( flat_set<string>& a )const
      { if( owner ) a.insert( account ); }

      void get_required_active_authorities( flat_set<string>& a )const
      { if( !owner /*&& active*/) a.insert( account ); }

      //void get_required_posting_authorities( flat_set<string>& a )const
      //{ if( !active && !owner && posting ) a.insert( account ); }
   };

   struct comment_operation : public base_operation
   {
      string            parent_author;
      string            parent_permlink;

      string            author;
      string            permlink;

      string            title;
      string            body;
      string            json_metadata;

      void validate()const;
      void get_required_posting_authorities( flat_set<string>& a )const{ a.insert(author); }
   };

   struct delete_comment_operation : public base_operation {
      string author;
      string permlink;

      void validate()const;
      void get_required_posting_authorities( flat_set<string>& a )const{ a.insert(author); }
   };

   struct vote_operation : public base_operation
   {
      string    voter;
      string    author;
      string    permlink;
      int16_t   weight = 0;

      void validate()const;
      void get_required_posting_authorities( flat_set<string>& a )const{ a.insert(voter); }
   };

   struct comment_reward_operation : public base_operation {
      comment_reward_operation(){}
      comment_reward_operation( const string& a, const string& l, const string& o_a, const string& o_l, const asset& p, const asset& v )
         :author(a),permlink(l),originating_author(o_a),originating_permlink(o_l),payout(p),vesting_payout(v){}
      string author;
      string permlink;
      string originating_author;
      string originating_permlink;
      asset  payout;
      asset  vesting_payout;
      void  validate()const { FC_ASSERT( false, "this is a virtual operation" ); }
   };

   struct curate_reward_operation : public base_operation
   {
      curate_reward_operation(){}
      curate_reward_operation( const string& c, const asset& r, const string& a, const string& p )
         :curator(c),reward(r),comment_author(a),comment_permlink(p){}

      string curator;
      asset  reward;
      string comment_author;
      string comment_permlink;
      void   validate()const { FC_ASSERT( false, "this is a virtual operation" ); }
   };

   struct liquidity_reward_operation : public base_operation {
      liquidity_reward_operation( string o = string(), asset p = asset() )
      :owner(o),payout(p){}

      string owner;
      asset  payout;
      void  validate()const { FC_ASSERT( false, "this is a virtual operation" ); }
   };

   struct interest_operation : public base_operation {
      interest_operation( const string& o = "", const asset& i = asset(0,SBD_SYMBOL) )
         :owner(o),interest(i){}

      string owner;
      asset  interest;

      void  validate()const { FC_ASSERT( false, "this is a virtual operation" ); }
   };

   struct fill_convert_request_operation : public base_operation {
      fill_convert_request_operation(){}
      fill_convert_request_operation( const string& o, const uint32_t id, const asset& in, const asset& out )
         :owner(o), requestid(id), amount_in(in), amount_out(out){}
      string   owner;
      uint32_t requestid = 0;
      asset    amount_in;
      asset    amount_out;
      void  validate()const { FC_ASSERT( false, "this is a virtual operation" ); }
   };

   struct fill_vesting_withdraw_operation : public base_operation
   {
      fill_vesting_withdraw_operation(){}
      fill_vesting_withdraw_operation( const string& a, const asset& v, const asset& s )
         :account(a), vesting_shares(v), steem(s){}
      string account;
      asset  vesting_shares;
      asset  steem;

      void  validate()const { FC_ASSERT( false, "this is a virtual operation" ); }
   };

   /**
    * @ingroup operations
    *
    * @brief Transfers STEEM from one account to another.
    */
   struct transfer_operation : public base_operation
   {
      string            from;
      /// Account to transfer asset to
      string            to;
      /// The amount of asset to transfer from @ref from to @ref to
      asset             amount;

      /// The memo is plain-text, any encryption on the memo is up to
      /// a higher level protocol.
      string            memo;

      void              validate()const;
      void get_required_active_authorities( flat_set<string>& a )const{ if(amount.symbol != VESTS_SYMBOL) a.insert(from); }
      void get_required_owner_authorities( flat_set<string>& a )const { if(amount.symbol == VESTS_SYMBOL) a.insert(from); }
   };

   /**
    *  This operation converts STEEM into VFS (Vesting Fund Shares) at
    *  the current exchange rate. With this operation it is possible to
    *  give another account vesting shares so that faucets can
    *  pre-fund new accounts with vesting shares.
    */
   struct transfer_to_vesting_operation : public base_operation
   {
      string   from;
      string   to; ///< if null, then same as from
      asset    amount; ///< must be STEEM


      void validate()const;
      void get_required_active_authorities( flat_set<string>& a )const{ a.insert(from); }
   };

   /**
    * At any given point in time an account can be withdrawing from their
    * vesting shares. A user may change the number of shares they wish to
    * cash out at any time between 0 and their total vesting stake.
    *
    * After applying this operation, vesting_shares will be withdrawn
    * at a rate of vesting_shares/104 per week for two years starting
    * one week after this operation is included in the blockchain.
    *
    * This operation is not valid if the user has no vesting shares.
    */
   struct withdraw_vesting_operation : public base_operation
   {
      string      account;
      asset       vesting_shares;

      void validate()const;
      void get_required_active_authorities( flat_set<string>& a )const{ a.insert(account); }
   };

   /**
    * Witnesses must vote on how to set certain chain properties to ensure a smooth
    * and well functioning network.  Any time @owner is in the active set of witnesses these
    * properties will be used to control the blockchain configuration.
    */
   struct chain_properties {
      /**
       *  This fee, paid in STEEM, is converted into VESTING SHARES for the new account. Accounts
       *  without vesting shares cannot earn usage rations and therefore are powerless. This minimum
       *  fee requires all accounts to have some kind of commitment to the network that includes the
       *  ability to vote and make transactions.
       */
      asset             account_creation_fee =
         asset( STEEMIT_MIN_ACCOUNT_CREATION_FEE, STEEM_SYMBOL );

      /**
       *  This witnesses vote for the maximum_block_size which is used by the network
       *  to tune rate limiting and capacity
       */
      uint32_t          maximum_block_size = STEEMIT_MIN_BLOCK_SIZE_LIMIT;
      uint16_t          sbd_interest_rate  = STEEMIT_DEFAULT_SBD_INTEREST_RATE;

      void validate()const {
         FC_ASSERT( account_creation_fee.amount >= STEEMIT_MIN_ACCOUNT_CREATION_FEE);
         FC_ASSERT( maximum_block_size >= STEEMIT_MIN_BLOCK_SIZE_LIMIT);
         FC_ASSERT( sbd_interest_rate >= 0 );
         FC_ASSERT( sbd_interest_rate <= STEEMIT_100_PERCENT );
      }
   };

   /**
    *  Users who wish to become a witness must pay a fee acceptable to
    *  the current witnesses to apply for the position and allow voting
    *  to begin.
    *
    *  If the owner isn't a witness they will become a witness.  Witnesses
    *  are charged a fee equal to 1 weeks worth of witness pay which in
    *  turn is derived from the current share supply.  The fee is
    *  only applied if the owner is not already a witness.
    *
    *  If the block_signing_key is null then the witness is removed from
    *  contention.  The network will pick the top 21 witnesses for
    *  producing blocks.
    */
   struct witness_update_operation : public base_operation
   {
      string            owner;
      string            url;
      public_key_type   block_signing_key;
      chain_properties  props;
      asset             fee; ///< the fee paid to register a new witness, should be 10x current block production pay

      void validate()const;
      void get_required_active_authorities( flat_set<string>& a )const{ a.insert(owner); }
   };

   /**
    * All accounts with a VFS can vote for or against any witness.
    *
    * If a proxy is specified then all existing votes are removed.
    */
   struct account_witness_vote_operation : public base_operation
   {
      string  account;
      string  witness;
      bool    approve = true;

      void validate() const;
      void get_required_active_authorities( flat_set<string>& a )const{ a.insert(account); }
   };

   struct account_witness_proxy_operation : public base_operation
   {
      string  account;
      string  proxy;

      void validate()const;
      void get_required_active_authorities( flat_set<string>& a )const{ a.insert(account); }
   };

   /**
    * @brief provides a generic way to add higher level protocols on top of witness consensus
    * @ingroup operations
    *
    * There is no validation for this operation other than that required auths are valid
    */
   struct custom_operation : public base_operation
   {
      flat_set<string>  required_auths;
      uint16_t          id = 0;
      vector<char>      data;

      void validate()const;
      void get_required_active_authorities( flat_set<string>& a )const{ for( const auto& i : required_auths ) a.insert(i); }
   };

   /** serves the same purpose as custom_operation but also supports required posting authorities. Unlike custom_operation,
    * this operation is designed to be human readable/developer friendly. 
    **/
   struct custom_json_operation : public base_operation {
      flat_set<string>  required_auths;
      flat_set<string>  required_posting_auths;
      string            id; ///< must be less than 32 characters long
      string            json; ///< must be proper utf8 / JSON string.

      void validate()const;
      void get_required_active_authorities( flat_set<string>& a )const{ for( const auto& i : required_auths ) a.insert(i); }
      void get_required_posting_authorities( flat_set<string>& a )const{ for( const auto& i : required_posting_auths ) a.insert(i); }
   };

   /**
    *  Feeds can only be published by the top N witnesses which are included in every round and are
    *  used to define the exchange rate between steem and the dollar.
    */
   struct feed_publish_operation : public base_operation
   {
      string   publisher;
      price    exchange_rate;

      void  validate()const;
      void  get_required_active_authorities( flat_set<string>& a )const{ a.insert(publisher); }
   };

   /**
    *  This operation instructs the blockchain to start a conversion between STEEM and SBD,
    *  The funds are depositted after STEEMIT_CONVERSION_DELAY
    */
   struct convert_operation : public base_operation
   {
      string   owner;
      uint32_t requestid = 0;
      asset    amount;

      void  validate()const;
      void  get_required_active_authorities( flat_set<string>& a )const{ a.insert(owner); }
   };

   /**
    * This operation creates a limit order and matches it against existing open orders.
    */
   struct limit_order_create_operation : public base_operation
   {
      string           owner;
      uint32_t         orderid = 0; /// an ID assigned by owner, must be unique
      asset            amount_to_sell;
      asset            min_to_receive;
      bool             fill_or_kill = false;
      time_point_sec   expiration = time_point_sec::maximum();

      void  validate()const;
      void  get_required_active_authorities( flat_set<string>& a )const{ a.insert(owner); }

      price           get_price()const { return amount_to_sell / min_to_receive; }

      pair<asset_symbol_type,asset_symbol_type> get_market()const
      {
         return amount_to_sell.symbol < min_to_receive.symbol ?
                std::make_pair(amount_to_sell.symbol, min_to_receive.symbol) :
                std::make_pair(min_to_receive.symbol, amount_to_sell.symbol);
      }
   };

   struct fill_order_operation : public base_operation {
      fill_order_operation(){}
      fill_order_operation( const string& o, uint32_t id, const asset& p, const asset& r )
      :owner(o),orderid(id),pays(p),receives(r){}

      string   owner;
      uint32_t orderid;
      asset    pays;
      asset    receives;
      void  validate()const { FC_ASSERT( false, "this is a virtual operation" ); }
   };

   /**
    *  Cancels an order and returns the balance to owner.
    */
   struct limit_order_cancel_operation : public base_operation
   {
      string   owner;
      uint32_t orderid = 0;

      void  validate()const;
      void  get_required_active_authorities( flat_set<string>& a )const{ a.insert(owner); }
   };

   struct pow {
      public_key_type   worker;
      digest_type       input;
      signature_type    signature;
      digest_type       work;

      void create( const fc::ecc::private_key& w, const digest_type& i );
      void validate()const;
   };

   struct pow_operation : public base_operation
   {
      string           worker_account;
      block_id_type    block_id;
      uint64_t         nonce = 0;
      pow              work;
      chain_properties props;

      void validate()const;
      fc::sha256 work_input()const;

      /** there is no need to verify authority, the proof of work is sufficient */
      void get_required_active_authorities( flat_set<string>& a )const{  }
   };

   /**
    * This operation is used to report a miner who signs two blocks
    * at the same time. To be valid, the violation must be reported within
    * STEEMIT_MAX_WITNESSES blocks of the head block (1 round) and the
    * producer must be in the ACTIVE witness set.
    *
    * Users not in the ACTIVE witness set should not have to worry about their
    * key getting compromised and being used to produced multiple blocks so
    * the attacker can report it and steel their vesting steem.
    *
    * The result of the operation is to transfer the full VESTING STEEM balance
    * of the block producer to the reporter.
    */
   struct report_over_production_operation : public base_operation {
      string              reporter;
      signed_block_header first_block;
      signed_block_header second_block;

      void validate()const;
   };
} } // steemit::chain

FC_REFLECT( steemit::chain::report_over_production_operation, (reporter)(first_block)(second_block) )
FC_REFLECT( steemit::chain::convert_operation, (owner)(requestid)(amount) )
FC_REFLECT( steemit::chain::feed_publish_operation, (publisher)(exchange_rate) )
FC_REFLECT( steemit::chain::pow, (worker)(input)(signature)(work) )
FC_REFLECT( steemit::chain::chain_properties, (account_creation_fee)(maximum_block_size)(sbd_interest_rate) );

FC_REFLECT( steemit::chain::pow_operation, (worker_account)(block_id)(nonce)(work)(props) )

FC_REFLECT( steemit::chain::account_create_operation,
            (fee)
            (creator)
            (new_account_name)
            (owner)
            (active)
            (posting)
            (memo_key)
            (json_metadata) )

FC_REFLECT( steemit::chain::account_update_operation,
            (account)
            (owner)
            (active)
            (posting)
            (memo_key)
            (json_metadata) )

FC_REFLECT( steemit::chain::transfer_operation, (from)(to)(amount)(memo) )
FC_REFLECT( steemit::chain::transfer_to_vesting_operation, (from)(to)(amount) )
FC_REFLECT( steemit::chain::withdraw_vesting_operation, (account)(vesting_shares) )
FC_REFLECT( steemit::chain::witness_update_operation, (owner)(url)(block_signing_key)(props)(fee) )
FC_REFLECT( steemit::chain::account_witness_vote_operation, (account)(witness)(approve) )
FC_REFLECT( steemit::chain::account_witness_proxy_operation, (account)(proxy) )
FC_REFLECT( steemit::chain::comment_operation, (parent_author)(parent_permlink)(author)(permlink)(title)(body)(json_metadata) )
FC_REFLECT( steemit::chain::vote_operation, (voter)(author)(permlink)(weight) )
FC_REFLECT( steemit::chain::custom_operation, (required_auths)(id)(data) )
FC_REFLECT( steemit::chain::custom_json_operation, (required_auths)(required_posting_auths)(id)(json) )
FC_REFLECT( steemit::chain::limit_order_create_operation, (owner)(orderid)(amount_to_sell)(min_to_receive)(fill_or_kill)(expiration) )
FC_REFLECT( steemit::chain::fill_order_operation, (owner)(orderid)(pays)(receives) );
FC_REFLECT( steemit::chain::limit_order_cancel_operation, (owner)(orderid) )

FC_REFLECT( steemit::chain::comment_reward_operation, (author)(permlink)(originating_author)(originating_permlink)(payout)(vesting_payout) )
FC_REFLECT( steemit::chain::curate_reward_operation, (curator)(reward)(comment_author)(comment_permlink) )
FC_REFLECT( steemit::chain::fill_convert_request_operation, (owner)(requestid)(amount_in)(amount_out) )
FC_REFLECT( steemit::chain::liquidity_reward_operation, (owner)(payout) )
FC_REFLECT( steemit::chain::interest_operation, (owner)(interest) )
FC_REFLECT( steemit::chain::fill_vesting_withdraw_operation, (account)(vesting_shares)(steem) )
FC_REFLECT( steemit::chain::delete_comment_operation, (author)(permlink) );
