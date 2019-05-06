/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 */
#pragma once
#include <steem/chain/block_log.hpp>
#include <steem/chain/fork_database.hpp>
#include <steem/chain/global_property_object.hpp>
#include <steem/chain/hardfork_property_object.hpp>
#include <steem/chain/node_property_object.hpp>
#include <steem/chain/notifications.hpp>

#include <steem/chain/util/advanced_benchmark_dumper.hpp>
#include <steem/chain/util/signal.hpp>

#include <steem/protocol/protocol.hpp>
#include <steem/protocol/hardfork.hpp>

#include <appbase/plugin.hpp>

#include <fc/signals.hpp>

#include <fc/log/logger.hpp>

#include <map>

namespace steem { namespace chain {

   using steem::protocol::signed_transaction;
   using steem::protocol::operation;
   using steem::protocol::authority;
   using steem::protocol::asset;
   using steem::protocol::asset_symbol_type;
   using steem::protocol::price;
   using abstract_plugin = appbase::abstract_plugin;

   class database_impl;
   class custom_operation_interpreter;

   namespace util {
      struct comment_reward_context;
   }

   namespace util {
      class advanced_benchmark_dumper;
   }

   struct reindex_notification
   {
      bool reindex_success = false;
      uint32_t last_block_number = 0;
   };

   struct generate_optional_actions_notification {};

   /**
    *   @class database
    *   @brief tracks the blockchain state in an extensible manner
    */
   class database : public chainbase::database
   {
      public:
         database();
         ~database();

         bool is_producing()const { return _is_producing; }
         void set_producing( bool p ) { _is_producing = p;  }

         bool is_pending_tx()const { return _is_pending_tx; }
         void set_pending_tx( bool p ) { _is_pending_tx = p; }

         bool is_processing_block()const { return _currently_processing_block_id.valid(); }

         bool _is_producing = false;
         bool _is_pending_tx = false;

         bool _log_hardforks = true;

         enum validation_steps
         {
            skip_nothing                = 0,
            skip_witness_signature      = 1 << 0,  ///< used while reindexing
            skip_transaction_signatures = 1 << 1,  ///< used by non-witness nodes
            skip_transaction_dupe_check = 1 << 2,  ///< used while reindexing
            skip_fork_db                = 1 << 3,  ///< used while reindexing
            skip_block_size_check       = 1 << 4,  ///< used when applying locally generated transactions
            skip_tapos_check            = 1 << 5,  ///< used while reindexing -- note this skips expiration check as well
            skip_authority_check        = 1 << 6,  ///< used while reindexing -- disables any checking of authority on transactions
            skip_merkle_check           = 1 << 7,  ///< used while reindexing
            skip_undo_history_check     = 1 << 8,  ///< used while reindexing
            skip_witness_schedule_check = 1 << 9,  ///< used while reindexing
            skip_validate               = 1 << 10, ///< used prior to checkpoint, skips validate() call on transaction
            skip_validate_invariants    = 1 << 11, ///< used to skip database invariant check on block application
            skip_undo_block             = 1 << 12, ///< used to skip undo db on reindex
            skip_block_log              = 1 << 13  ///< used to skip block logging on reindex
         };

         typedef std::function<void(uint32_t, const abstract_index_cntr_t&)> TBenchmarkMidReport;
         typedef std::pair<uint32_t, TBenchmarkMidReport> TBenchmark;

         struct open_args
         {
            fc::path data_dir;
            fc::path shared_mem_dir;
            uint64_t initial_supply = STEEM_INIT_SUPPLY;
            uint64_t shared_file_size = 0;
            uint16_t shared_file_full_threshold = 0;
            uint16_t shared_file_scale_rate = 0;
            uint32_t chainbase_flags = 0;
            bool do_validate_invariants = false;
            bool benchmark_is_enabled = false;
            fc::variant database_cfg;

            // The following fields are only used on reindexing
            uint32_t stop_replay_at = 0;
            TBenchmark benchmark = TBenchmark(0, []( uint32_t, const abstract_index_cntr_t& ){});
         };

         /**
          * @brief Open a database, creating a new one if necessary
          *
          * Opens a database in the specified directory. If no initialized database is found the database
          * will be initialized with the default state.
          *
          * @param data_dir Path to open or create database in
          */
         void open( const open_args& args );

         /**
          * @brief Rebuild object graph from block history and open detabase
          *
          * This method may be called after or instead of @ref database::open, and will rebuild the object graph by
          * replaying blockchain history. When this method exits successfully, the database will be open.
          *
          * @return the last replayed block number.
          */
         uint32_t reindex( const open_args& args );

         /**
          * @brief wipe Delete database from disk, and potentially the raw chain as well.
          * @param include_blocks If true, delete the raw chain as well as the database.
          *
          * Will close the database before wiping. Database will be closed when this function returns.
          */
         void wipe(const fc::path& data_dir, const fc::path& shared_mem_dir, bool include_blocks);
         void close(bool rewind = true);

         //////////////////// db_block.cpp ////////////////////

         /**
          *  @return true if the block is in our fork DB or saved to disk as
          *  part of the official chain, otherwise return false
          */
         bool                       is_known_block( const block_id_type& id )const;
         bool                       is_known_transaction( const transaction_id_type& id )const;
         fc::sha256                 get_pow_target()const;
         uint32_t                   get_pow_summary_target()const;
         block_id_type              find_block_id_for_num( uint32_t block_num )const;
         block_id_type              get_block_id_for_num( uint32_t block_num )const;
         optional<signed_block>     fetch_block_by_id( const block_id_type& id )const;
         optional<signed_block>     fetch_block_by_number( uint32_t num )const;
         const signed_transaction   get_recent_transaction( const transaction_id_type& trx_id )const;
         std::vector<block_id_type> get_block_ids_on_fork(block_id_type head_of_fork) const;

         chain_id_type steem_chain_id = STEEM_CHAIN_ID;
         chain_id_type get_chain_id() const;
         void set_chain_id( const chain_id_type& chain_id );

         /** Allows to visit all stored blocks until processor returns true. Caller is responsible for block disasembling
          * const signed_block_header& - header of previous block
          * const signed_block& - block to be processed currently
         */
         void foreach_block(std::function<bool(const signed_block_header&, const signed_block&)> processor) const;

         /// Allows to process all blocks visit all transactions held there until processor returns true.
         void foreach_tx(std::function<bool(const signed_block_header&, const signed_block&,
            const signed_transaction&, uint32_t)> processor) const;
         /// Allows to process all operations held in blocks and transactions until processor returns true.
         void foreach_operation(std::function<bool(const signed_block_header&, const signed_block&,
            const signed_transaction&, uint32_t, const operation&, uint16_t)> processor) const;

         const witness_object&  get_witness(  const account_name_type& name )const;
         const witness_object*  find_witness( const account_name_type& name )const;

         const account_object&  get_account(  const account_name_type& name )const;
         const account_object*  find_account( const account_name_type& name )const;

         const comment_object&  get_comment(  const account_name_type& author, const shared_string& permlink )const;
         const comment_object*  find_comment( const account_name_type& author, const shared_string& permlink )const;

#ifndef ENABLE_STD_ALLOCATOR
         const comment_object&  get_comment(  const account_name_type& author, const string& permlink )const;
         const comment_object*  find_comment( const account_name_type& author, const string& permlink )const;
#endif

         const escrow_object&   get_escrow(  const account_name_type& name, uint32_t escrow_id )const;
         const escrow_object*   find_escrow( const account_name_type& name, uint32_t escrow_id )const;

         const limit_order_object& get_limit_order(  const account_name_type& owner, uint32_t id )const;
         const limit_order_object* find_limit_order( const account_name_type& owner, uint32_t id )const;

         const savings_withdraw_object& get_savings_withdraw(  const account_name_type& owner, uint32_t request_id )const;
         const savings_withdraw_object* find_savings_withdraw( const account_name_type& owner, uint32_t request_id )const;

         const dynamic_global_property_object&  get_dynamic_global_properties()const;
         const node_property_object&            get_node_properties()const;
         const feed_history_object&             get_feed_history()const;
         const witness_schedule_object&         get_witness_schedule_object()const;
         const hardfork_property_object&        get_hardfork_property_object()const;

         const time_point_sec                   calculate_discussion_payout_time( const comment_object& comment )const;
         const reward_fund_object&              get_reward_fund( const comment_object& c )const;

         asset get_effective_vesting_shares( const account_object& account, asset_symbol_type vested_symbol )const;

         void max_bandwidth_per_share()const;

         /**
          *  Calculate the percent of block production slots that were missed in the
          *  past 128 blocks, not including the current block.
          */
         uint32_t witness_participation_rate()const;

         void                                   add_checkpoints( const flat_map<uint32_t,block_id_type>& checkpts );
         const flat_map<uint32_t,block_id_type> get_checkpoints()const { return _checkpoints; }
         bool                                   before_last_checkpoint()const;

         bool push_block( const signed_block& b, uint32_t skip = skip_nothing );
         void push_transaction( const signed_transaction& trx, uint32_t skip = skip_nothing );
         void _maybe_warn_multiple_production( uint32_t height )const;
         bool _push_block( const signed_block& b );
         void _push_transaction( const signed_transaction& trx );

         void pop_block();
         void clear_pending();

         void push_virtual_operation( const operation& op );
         void pre_push_virtual_operation( const operation& op );
         void post_push_virtual_operation( const operation& op );

         /*
          * Pushing an action without specifying an execution time will execute at head block.
          * The execution time must be greater than or equal to head block.
          */
         void push_required_action( const required_automated_action& a, time_point_sec execution_time );
         void push_required_action( const required_automated_action& a );

         void push_optional_action( const optional_automated_action& a, time_point_sec execution_time );
         void push_optional_action( const optional_automated_action& a );

         void notify_pre_apply_required_action( const required_action_notification& note );
         void notify_post_apply_required_action( const required_action_notification& note );

         void notify_pre_apply_optional_action( const optional_action_notification& note );
         void notify_post_apply_optional_action( const optional_action_notification& note );
         /**
          *  This method is used to track applied operations during the evaluation of a block, these
          *  operations should include any operation actually included in a transaction as well
          *  as any implied/virtual operations that resulted, such as filling an order.
          *  The applied operations are cleared after post_apply_operation.
          */
         void notify_pre_apply_operation( const operation_notification& note );
         void notify_post_apply_operation( const operation_notification& note );
         void notify_pre_apply_block( const block_notification& note );
         void notify_post_apply_block( const block_notification& note );
         void notify_irreversible_block( uint32_t block_num );
         void notify_pre_apply_transaction( const transaction_notification& note );
         void notify_post_apply_transaction( const transaction_notification& note );

         using apply_required_action_handler_t = std::function< void(const required_action_notification&) >;
         using apply_optional_action_handler_t = std::function< void(const optional_action_notification&) >;
         using apply_operation_handler_t = std::function< void(const operation_notification&) >;
         using apply_transaction_handler_t = std::function< void(const transaction_notification&) >;
         using apply_block_handler_t = std::function< void(const block_notification&) >;
         using irreversible_block_handler_t = std::function< void(uint32_t) >;
         using reindex_handler_t = std::function< void(const reindex_notification&) >;
         using generate_optional_actions_handler_t = std::function< void(const generate_optional_actions_notification&) >;


      private:
         template <typename TSignal,
                   typename TNotification = std::function<typename TSignal::signature_type>>
         boost::signals2::connection connect_impl( TSignal& signal, const TNotification& func,
            const abstract_plugin& plugin, int32_t group, const std::string& item_name = "" );

         template< bool IS_PRE_OPERATION >
         boost::signals2::connection any_apply_operation_handler_impl( const apply_operation_handler_t& func,
            const abstract_plugin& plugin, int32_t group );

      public:

         boost::signals2::connection add_pre_apply_required_action_handler ( const apply_required_action_handler_t&     func, const abstract_plugin& plugin, int32_t group = -1 );
         boost::signals2::connection add_post_apply_required_action_handler( const apply_required_action_handler_t&     func, const abstract_plugin& plugin, int32_t group = -1 );
         boost::signals2::connection add_pre_apply_optional_action_handler ( const apply_optional_action_handler_t&     func, const abstract_plugin& plugin, int32_t group = -1 );
         boost::signals2::connection add_post_apply_optional_action_handler( const apply_optional_action_handler_t&     func, const abstract_plugin& plugin, int32_t group = -1 );
         boost::signals2::connection add_pre_apply_operation_handler       ( const apply_operation_handler_t&           func, const abstract_plugin& plugin, int32_t group = -1 );
         boost::signals2::connection add_post_apply_operation_handler      ( const apply_operation_handler_t&           func, const abstract_plugin& plugin, int32_t group = -1 );
         boost::signals2::connection add_pre_apply_transaction_handler     ( const apply_transaction_handler_t&         func, const abstract_plugin& plugin, int32_t group = -1 );
         boost::signals2::connection add_post_apply_transaction_handler    ( const apply_transaction_handler_t&         func, const abstract_plugin& plugin, int32_t group = -1 );
         boost::signals2::connection add_pre_apply_block_handler           ( const apply_block_handler_t&               func, const abstract_plugin& plugin, int32_t group = -1 );
         boost::signals2::connection add_post_apply_block_handler          ( const apply_block_handler_t&               func, const abstract_plugin& plugin, int32_t group = -1 );
         boost::signals2::connection add_irreversible_block_handler        ( const irreversible_block_handler_t&        func, const abstract_plugin& plugin, int32_t group = -1 );
         boost::signals2::connection add_pre_reindex_handler               ( const reindex_handler_t&                   func, const abstract_plugin& plugin, int32_t group = -1 );
         boost::signals2::connection add_post_reindex_handler              ( const reindex_handler_t&                   func, const abstract_plugin& plugin, int32_t group = -1 );
         boost::signals2::connection add_generate_optional_actions_handler ( const generate_optional_actions_handler_t& func, const abstract_plugin& plugin, int32_t group = -1 );

         //////////////////// db_witness_schedule.cpp ////////////////////

         /**
          * @brief Get the witness scheduled for block production in a slot.
          *
          * slot_num always corresponds to a time in the future.
          *
          * If slot_num == 1, returns the next scheduled witness.
          * If slot_num == 2, returns the next scheduled witness after
          * 1 block gap.
          *
          * Use the get_slot_time() and get_slot_at_time() functions
          * to convert between slot_num and timestamp.
          *
          * Passing slot_num == 0 returns STEEM_NULL_WITNESS
          */
         account_name_type get_scheduled_witness(uint32_t slot_num)const;

         /**
          * Get the time at which the given slot occurs.
          *
          * If slot_num == 0, return time_point_sec().
          *
          * If slot_num == N for N > 0, return the Nth next
          * block-interval-aligned time greater than head_block_time().
          */
         fc::time_point_sec get_slot_time(uint32_t slot_num)const;

         /**
          * Get the last slot which occurs AT or BEFORE the given time.
          *
          * The return value is the greatest value N such that
          * get_slot_time( N ) <= when.
          *
          * If no such N exists, return 0.
          */
         uint32_t get_slot_at_time(fc::time_point_sec when)const;

         /** @return the sbd created and deposited to_account, may return STEEM if there is no median feed */
         std::pair< asset, asset > create_sbd( const account_object& to_account, asset steem, bool to_reward_balance=false );
         asset create_vesting( const account_object& to_account, asset steem, bool to_reward_balance=false );
         void adjust_total_payout( const comment_object& a, const asset& sbd, const asset& curator_sbd_value, const asset& beneficiary_value );

         void        adjust_liquidity_reward( const account_object& owner, const asset& volume, bool is_bid );
         void        adjust_balance( const account_object& a, const asset& delta );
         void        adjust_balance( const account_name_type& name, const asset& delta );
         void        adjust_savings_balance( const account_object& a, const asset& delta );
         void        adjust_reward_balance( const account_object& a, const asset& value_delta,
                                            const asset& share_delta = asset(0,VESTS_SYMBOL) );
         void        adjust_reward_balance( const account_name_type& name, const asset& value_delta,
                                            const asset& share_delta = asset(0,VESTS_SYMBOL) );
         void        adjust_supply( const asset& delta, bool adjust_vesting = false );
         void        adjust_rshares2( const comment_object& comment, fc::uint128_t old_rshares2, fc::uint128_t new_rshares2 );
         void        update_owner_authority( const account_object& account, const authority& owner_authority );

         asset       get_balance( const account_object& a, asset_symbol_type symbol )const;
         asset       get_savings_balance( const account_object& a, asset_symbol_type symbol )const;
         asset       get_balance( const string& aname, asset_symbol_type symbol )const { return get_balance( get_account(aname), symbol ); }

         /** this updates the votes for witnesses as a result of account voting proxy changing */
         void adjust_proxied_witness_votes( const account_object& a,
                                            const std::array< share_type, STEEM_MAX_PROXY_RECURSION_DEPTH+1 >& delta,
                                            int depth = 0 );

         /** this updates the votes for all witnesses as a result of account VESTS changing */
         void adjust_proxied_witness_votes( const account_object& a, share_type delta, int depth = 0 );

         /** this is called by `adjust_proxied_witness_votes` when account proxy to self */
         void adjust_witness_votes( const account_object& a, share_type delta );

         /** this updates the vote of a single witness as a result of a vote being added or removed*/
         void adjust_witness_vote( const witness_object& obj, share_type delta );

         /** clears all vote records for a particular account but does not update the
          * witness vote totals.  Vote totals should be updated first via a call to
          * adjust_proxied_witness_votes( a, -a.witness_vote_weight() )
          */
         void clear_witness_votes( const account_object& a );
         void process_vesting_withdrawals();
         share_type pay_curators( const comment_object& c, share_type& max_rewards );
         share_type cashout_comment_helper( util::comment_reward_context& ctx, const comment_object& comment, bool forward_curation_remainder = true );
         void process_comment_cashout();
         void process_funds();
         void process_conversions();
         void process_savings_withdraws();
         void process_subsidized_accounts();
#ifdef STEEM_ENABLE_SMT
         void process_smt_objects();
#endif
         void account_recovery_processing();
         void expire_escrow_ratification();
         void process_decline_voting_rights();
         void update_median_feed();

         asset get_liquidity_reward()const;
         asset get_content_reward()const;
         asset get_producer_reward();
         asset get_curation_reward()const;
         asset get_pow_reward()const;

         uint16_t get_curation_rewards_percent( const comment_object& c ) const;

         share_type pay_reward_funds( share_type reward );

         void  pay_liquidity_reward();

         /**
          * Helper method to return the current sbd value of a given amount of
          * STEEM.  Return 0 SBD if there isn't a current_median_history
          */
         asset to_sbd( const asset& steem )const;
         asset to_steem( const asset& sbd )const;

         time_point_sec   head_block_time()const;
         uint32_t         head_block_num()const;
         block_id_type    head_block_id()const;

         node_property_object& node_properties();

         uint32_t last_non_undoable_block_num() const;
         //////////////////// db_init.cpp ////////////////////

         void initialize_evaluators();
         void register_custom_operation_interpreter( std::shared_ptr< custom_operation_interpreter > interpreter );
         std::shared_ptr< custom_operation_interpreter > get_custom_json_evaluator( const custom_id_type& id );

         /// Reset the object graph in-memory
         void initialize_indexes();
         void init_schema();
         void init_genesis(uint64_t initial_supply = STEEM_INIT_SUPPLY );

         /**
          *  This method validates transactions without adding it to the pending state.
          *  @throw if an error occurs
          */
         void validate_transaction( const signed_transaction& trx );

         /** when popping a block, the transactions that were removed get cached here so they
          * can be reapplied at the proper time */
         std::deque< signed_transaction >       _popped_tx;
         vector< signed_transaction >           _pending_tx;

         bool apply_order( const limit_order_object& new_order_object );
         bool fill_order( const limit_order_object& order, const asset& pays, const asset& receives );
         void cancel_order( const limit_order_object& obj );
         int  match( const limit_order_object& bid, const limit_order_object& ask, const price& trade_price );

         void perform_vesting_share_split( uint32_t magnitude );
         void retally_comment_children();
         void retally_witness_votes();
         void retally_witness_vote_counts( bool force = false );
         void retally_liquidity_weight();
         void update_virtual_supply();

         bool has_hardfork( uint32_t hardfork )const;

         uint32_t get_hardfork()const;

         /* For testing and debugging only. Given a hardfork
            with id N, applies all hardforks with id <= N */
         void set_hardfork( uint32_t hardfork, bool process_now = true );

         void validate_invariants()const;
         /**
          * @}
          */

         const std::string& get_json_schema() const;

         void set_flush_interval( uint32_t flush_blocks );
         void check_free_memory( bool force_print, uint32_t current_block_num );

         void apply_transaction( const signed_transaction& trx, uint32_t skip = skip_nothing );
         void apply_required_action( const required_automated_action& a );
         void apply_optional_action( const optional_automated_action& a );

         optional< chainbase::database::session >& pending_transaction_session();

#ifdef IS_TEST_NET
         bool liquidity_rewards_enabled = true;
         bool skip_price_feed_limit_check = true;
         bool skip_transaction_delta_check = true;
         bool disable_low_mem_warning = true;
#endif

#ifdef STEEM_ENABLE_SMT
         ///Smart Media Tokens related methods
         ///@{
         void validate_smt_invariants()const;
         ///@}
#endif

   protected:
         //Mark pop_undo() as protected -- we do not want outside calling pop_undo(); it should call pop_block() instead
         //void pop_undo() { object_database::pop_undo(); }
         void notify_changed_objects();

      private:
         optional< chainbase::database::session > _pending_tx_session;

         void apply_block( const signed_block& next_block, uint32_t skip = skip_nothing );
         void _apply_block( const signed_block& next_block );
         void _apply_transaction( const signed_transaction& trx );
         void apply_operation( const operation& op );

         void process_required_actions( const required_automated_actions& actions );
         void process_optional_actions( const optional_automated_actions& actions );

         ///Steps involved in applying a new block
         ///@{

         const witness_object& validate_block_header( uint32_t skip, const signed_block& next_block )const;
         void create_block_summary(const signed_block& next_block);

         void clear_null_account_balance();

         void update_global_dynamic_data( const signed_block& b );
         void update_signing_witness(const witness_object& signing_witness, const signed_block& new_block);
         void update_last_irreversible_block();
         void migrate_irreversible_state();
         void clear_expired_transactions();
         void clear_expired_orders();
         void clear_expired_delegations();
         void process_header_extensions( const signed_block& next_block, required_automated_actions& req_actions, optional_automated_actions& opt_actions );

         void generate_required_actions();
         void generate_optional_actions();

         void init_hardforks();
         void process_hardforks();
         void apply_hardfork( uint32_t hardfork );

         ///@}
#ifdef STEEM_ENABLE_SMT
         template< typename smt_balance_object_type, class balance_operator_type >
         void adjust_smt_balance( const account_name_type& name, const asset& delta, bool check_account,
                                  balance_operator_type balance_operator );
#endif
         void modify_balance( const account_object& a, const asset& delta, bool check_balance );
         void modify_reward_balance( const account_object& a, const asset& value_delta, const asset& share_delta, bool check_balance );

         operation_notification create_operation_notification( const operation& op )const
         {
            operation_notification note(op);
            note.trx_id       = _current_trx_id;
            note.block        = _current_block_num;
            note.trx_in_block = _current_trx_in_block;
            note.op_in_trx    = _current_op_in_trx;
            return note;
         }

         std::unique_ptr< database_impl > _my;

         fork_database                 _fork_db;
         fc::time_point_sec            _hardfork_times[ STEEM_NUM_HARDFORKS + 1 ];
         protocol::hardfork_version    _hardfork_versions[ STEEM_NUM_HARDFORKS + 1 ];

         block_log                     _block_log;

         // this function needs access to _plugin_index_signal
         template< typename MultiIndexType >
         friend void add_plugin_index( database& db );

         transaction_id_type           _current_trx_id;
         uint32_t                      _current_block_num    = 0;
         int32_t                       _current_trx_in_block = 0;
         uint16_t                      _current_op_in_trx    = 0;
         uint16_t                      _current_virtual_op   = 0;

         optional< block_id_type >     _currently_processing_block_id;

         flat_map<uint32_t,block_id_type>  _checkpoints;

         node_property_object              _node_property_object;

         uint32_t                      _flush_blocks = 0;
         uint32_t                      _next_flush_block = 0;

         uint32_t                      _last_free_gb_printed = 0;

         uint16_t                      _shared_file_full_threshold = 0;
         uint16_t                      _shared_file_scale_rate = 0;

         flat_map< custom_id_type, std::shared_ptr< custom_operation_interpreter > >   _custom_operation_interpreters;
         std::string                   _json_schema;

         util::advanced_benchmark_dumper  _benchmark_dumper;

         fc::signal<void(const required_action_notification&)> _pre_apply_required_action_signal;
         fc::signal<void(const required_action_notification&)> _post_apply_required_action_signal;

         fc::signal<void(const optional_action_notification&)> _pre_apply_optional_action_signal;
         fc::signal<void(const optional_action_notification&)> _post_apply_optional_action_signal;

         fc::signal<void(const operation_notification&)>       _pre_apply_operation_signal;
         /**
          *  This signal is emitted for plugins to process every operation after it has been fully applied.
          */
         fc::signal<void(const operation_notification&)>       _post_apply_operation_signal;

         /**
          *  This signal is emitted when we start processing a block.
          *
          *  You may not yield from this callback because the blockchain is holding
          *  the write lock and may be in an "inconstant state" until after it is
          *  released.
          */
         fc::signal<void(const block_notification&)>           _pre_apply_block_signal;

         fc::signal<void(uint32_t)>                            _on_irreversible_block;

         /**
          *  This signal is emitted after all operations and virtual operation for a
          *  block have been applied but before the get_applied_operations() are cleared.
          *
          *  You may not yield from this callback because the blockchain is holding
          *  the write lock and may be in an "inconstant state" until after it is
          *  released.
          */
         fc::signal<void(const block_notification&)>           _post_apply_block_signal;

         /**
          * This signal is emitted any time a new transaction is about to be applied
          * to the chain state.
          */
         fc::signal<void(const transaction_notification&)>     _pre_apply_transaction_signal;

         /**
          * This signal is emitted any time a new transaction has been applied to the
          * chain state.
          */
         fc::signal<void(const transaction_notification&)>     _post_apply_transaction_signal;

         /**
          * Emitted when reindexing starts
          */
         fc::signal<void(const reindex_notification&)>         _pre_reindex_signal;

         /**
          * Emitted when reindexing finishes
          */
         fc::signal<void(const reindex_notification&)>         _post_reindex_signal;

         fc::signal<void(const generate_optional_actions_notification& )> _generate_optional_actions_signal;

         /**
          *  Emitted After a block has been applied and committed.  The callback
          *  should not yield and should execute quickly.
          */
         //fc::signal<void(const vector< object_id_type >&)> changed_objects;

         /** this signal is emitted any time an object is removed and contains a
          * pointer to the last value of every object that was removed.
          */
         //fc::signal<void(const vector<const object*>&)>  removed_objects;

         /**
          * Internal signal to execute deferred registration of plugin indexes.
          */
         fc::signal<void()>                                    _plugin_index_signal;
   };

} }
