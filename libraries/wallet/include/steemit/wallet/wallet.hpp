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
#pragma once

#include <steemit/app/api.hpp>
#include <steemit/chain/steem_objects.hpp>

#include <graphene/utilities/key_conversion.hpp>

#include <fc/real128.hpp>

using namespace steemit::app;
using namespace steemit::chain;
using namespace graphene::utilities;
using namespace std;

namespace steemit { namespace wallet {

using steemit::app::discussion;

typedef uint16_t transaction_handle_type;

/**
 * This class takes a variant and turns it into an object
 * of the given type, with the new operator.
 */

object* create_object( const variant& v );


struct brain_key_info
{
   string               brain_priv_key;
   public_key_type      pub_key;
   string               wif_priv_key;
};

struct wallet_data
{
   vector<char>              cipher_keys; /** encrypted keys */

   string                    ws_server = "ws://localhost:8090";
   string                    ws_user;
   string                    ws_password;
};

struct signed_block_with_info : public signed_block
{
   signed_block_with_info( const signed_block& block );
   signed_block_with_info( const signed_block_with_info& block ) = default;

   block_id_type block_id;
   public_key_type signing_key;
   vector< transaction_id_type > transaction_ids;
};

namespace detail {
class wallet_api_impl;
}

struct operation_detail {
   string               memo;
   string               description;
   operation_object     op;
};

/**
 * This wallet assumes it is connected to the database server with a high-bandwidth, low-latency connection and
 * performs minimal caching. This API could be provided locally to be used by a web interface.
 */
class wallet_api
{
   public:
      wallet_api( const wallet_data& initial_data, fc::api<login_api> rapi );
      virtual ~wallet_api();

      bool copy_wallet_file( string destination_filename );


      /** Returns a list of all commands supported by the wallet API.
       *
       * This lists each command, along with its arguments and return types.
       * For more detailed help on a single command, use \c get_help()
       *
       * @returns a multi-line string suitable for displaying on a terminal
       */
      string                              help()const;

      variant                             info();
      /** Returns info such as client version, git version of graphene/fc, version of boost, openssl.
       * @returns compile time info and client and dependencies versions
       */
      variant_object                      about() const;
      optional<signed_block_with_info>    get_block( uint32_t num );

      feed_history_object                 get_feed_history()const;

      vector<string>                      get_active_witnesses()const;
      vector<string>                      get_miner_queue()const;

      /**
       * Returns the state info associated with the URL
       */
      app::state                          get_state( string url );

      /**
       *  Gets the account information for all accounts for which this wallet has a private key
       */
      vector<account_object>              list_my_accounts();

      /** Lists all accounts registered in the blockchain.
       * This returns a list of all account names and their account ids, sorted by account name.
       *
       * Use the \c lowerbound and limit parameters to page through the list.  To retrieve all accounts,
       * start by setting \c lowerbound to the empty string \c "", and then each iteration, pass
       * the last account name returned as the \c lowerbound for the next \c list_accounts() call.
       *
       * @param lowerbound the name of the first account to return.  If the named account does not exist,
       *                   the list will start at the account that comes after \c lowerbound
       * @param limit the maximum number of accounts to return (max: 1000)
       * @returns a list of accounts mapping account names to account ids
       */
      set<string>  list_accounts(const string& lowerbound, uint32_t limit);

      /** Returns the block chain's rapidly-changing properties.
       * The returned object contains information that changes every block interval
       * such as the head block number, the next witness, etc.
       * @see \c get_global_properties() for less-frequently changing properties
       * @returns the dynamic global properties
       */
      dynamic_global_property_object    get_dynamic_global_properties() const;

      /** Returns information about the given account.
       *
       * @param account_name_or_id the name or id of the account to provide information about
       * @returns the public account data stored in the blockchain
       */
      account_object                    get_account( string account_name ) const;

      /** Returns the current wallet filename.
       *
       * This is the filename that will be used when automatically saving the wallet.
       *
       * @see set_wallet_filename()
       * @return the wallet filename
       */
      string                            get_wallet_filename() const;

      /**
       * Get the WIF private key corresponding to a public key.  The
       * private key must already be in the wallet.
       */
      string                            get_private_key( public_key_type pubkey )const;

      annotated_signed_transaction      get_transaction( transaction_id_type trx_id )const;

      /** Checks whether the wallet has just been created and has not yet had a password set.
       *
       * Calling \c set_password will transition the wallet to the locked state.
       * @return true if the wallet is new
       * @ingroup Wallet Management
       */
      bool    is_new()const;

      /** Checks whether the wallet is locked (is unable to use its private keys).
       *
       * This state can be changed by calling \c lock() or \c unlock().
       * @return true if the wallet is locked
       * @ingroup Wallet Management
       */
      bool    is_locked()const;

      /** Locks the wallet immediately.
       * @ingroup Wallet Management
       */
      void    lock();

      /** Unlocks the wallet.
       *
       * The wallet remain unlocked until the \c lock is called
       * or the program exits.
       * @param password the password previously set with \c set_password()
       * @ingroup Wallet Management
       */
      void    unlock(string password);

      /** Sets a new password on the wallet.
       *
       * The wallet must be either 'new' or 'unlocked' to
       * execute this command.
       * @ingroup Wallet Management
       */
      void    set_password(string password);

      /** Dumps all private keys owned by the wallet.
       *
       * The keys are printed in WIF format.  You can import these keys into another wallet
       * using \c import_key()
       * @returns a map containing the private keys, indexed by their public key
       */
      map<public_key_type, string> list_keys();

      /** Returns detailed help on a single API command.
       * @param method the name of the API command you want help with
       * @returns a multi-line string suitable for displaying on a terminal
       */
      string  gethelp(const string& method)const;

      /** Loads a specified Graphene wallet.
       *
       * The current wallet is closed before the new wallet is loaded.
       *
       * @warning This does not change the filename that will be used for future
       * wallet writes, so this may cause you to overwrite your original
       * wallet unless you also call \c set_wallet_filename()
       *
       * @param wallet_filename the filename of the wallet JSON file to load.
       *                        If \c wallet_filename is empty, it reloads the
       *                        existing wallet file
       * @returns true if the specified wallet is loaded
       */
      bool    load_wallet_file(string wallet_filename = "");

      /** Saves the current wallet to the given filename.
       *
       * @warning This does not change the wallet filename that will be used for future
       * writes, so think of this function as 'Save a Copy As...' instead of
       * 'Save As...'.  Use \c set_wallet_filename() to make the filename
       * persist.
       * @param wallet_filename the filename of the new wallet JSON file to create
       *                        or overwrite.  If \c wallet_filename is empty,
       *                        save to the current filename.
       */
      void    save_wallet_file(string wallet_filename = "");

      /** Sets the wallet filename used for future writes.
       *
       * This does not trigger a save, it only changes the default filename
       * that will be used the next time a save is triggered.
       *
       * @param wallet_filename the new filename to use for future saves
       */
      void    set_wallet_filename(string wallet_filename);

      /** Suggests a safe brain key to use for creating your account.
       * \c create_account_with_brain_key() requires you to specify a 'brain key',
       * a long passphrase that provides enough entropy to generate cyrptographic
       * keys.  This function will suggest a suitably random string that should
       * be easy to write down (and, with effort, memorize).
       * @returns a suggested brain_key
       */
      brain_key_info suggest_brain_key()const;

      /** Converts a signed_transaction in JSON form to its binary representation.
       *
       * TODO: I don't see a broadcast_transaction() function, do we need one?
       *
       * @param tx the transaction to serialize
       * @returns the binary form of the transaction.  It will not be hex encoded,
       *          this returns a raw string that may have null characters embedded
       *          in it
       */
      string serialize_transaction(signed_transaction tx) const;

      bool import_key( string wif_key );

      /** Transforms a brain key to reduce the chance of errors when re-entering the key from memory.
       *
       * This takes a user-supplied brain key and normalizes it into the form used
       * for generating private keys.  In particular, this upper-cases all ASCII characters
       * and collapses multiple spaces into one.
       * @param s the brain key as supplied by the user
       * @returns the brain key in its normalized form
       */
      string normalize_brain_key(string s) const;

      /**
       * This method is used by faucets to create new accounts for other users which must
       * provide their desired keys. The resulting account may not be controllable by this
       * wallet.
       */
      annotated_signed_transaction create_account_with_keys( string creator,
                                            string newname,
                                            string json_meta,
                                            public_key_type owner,
                                            public_key_type active,
                                            public_key_type posting,
                                            public_key_type memo,
                                            bool broadcast )const;

      annotated_signed_transaction update_account( string accountname,
                                         string json_meta,
                                         public_key_type owner,
                                         public_key_type active,
                                         public_key_type posting,
                                         public_key_type memo,
                                         bool broadcast )const;

      /**
       *  This method will genrate new owner, active, and memo keys for the new account which
       *  will be controlable by this wallet.
       */
      annotated_signed_transaction create_account( string creator, string new_account_name, string json_meta, bool broadcast );


      /**
       *  This method is used to convert a JSON transaction to its transactin ID.
       */
      transaction_id_type get_transaction_id( const signed_transaction& trx )const { return trx.id(); }


      /** Lists all witnesses registered in the blockchain.
       * This returns a list of all account names that own witnesses, and the associated witness id,
       * sorted by name.  This lists witnesses whether they are currently voted in or not.
       *
       * Use the \c lowerbound and limit parameters to page through the list.  To retrieve all witnesss,
       * start by setting \c lowerbound to the empty string \c "", and then each iteration, pass
       * the last witness name returned as the \c lowerbound for the next \c list_witnesss() call.
       *
       * @param lowerbound the name of the first witness to return.  If the named witness does not exist,
       *                   the list will start at the witness that comes after \c lowerbound
       * @param limit the maximum number of witnesss to return (max: 1000)
       * @returns a list of witnesss mapping witness names to witness ids
       */
      set<string>       list_witnesses(const string& lowerbound, uint32_t limit);

      /** Returns information about the given witness.
       * @param owner_account the name or id of the witness account owner, or the id of the witness
       * @returns the information about the witness stored in the block chain
       */
      optional< witness_object > get_witness(string owner_account);

      vector<convert_request_object> get_conversion_requests( string owner );


      /**
       * Update a witness object owned by the given account.
       *
       * @param witness The name of the witness's owner account.  Also accepts the ID of the owner account or the ID of the witness.
       * @param url Same as for create_witness.  The empty string makes it remain the same.
       * @param block_signing_key The new block signing public key.  The empty string makes it remain the same.
       * @param broadcast true if you wish to broadcast the transaction.
       */
      annotated_signed_transaction update_witness(string witness_name,
                                        string url,
                                        public_key_type block_signing_key,
                                        const chain_properties& props,
                                        bool broadcast = false);


      /** Set the voting proxy for an account.
       *
       * If a user does not wish to take an active part in voting, they can choose
       * to allow another account to vote their stake.
       *
       * Setting a vote proxy does not remove your previous votes from the blockchain,
       * they remain there but are ignored.  If you later null out your vote proxy,
       * your previous votes will take effect again.
       *
       * This setting can be changed at any time.
       *
       * @param account_to_modify the name or id of the account to update
       * @param proxy the name of account that should proxy to, or empty string to have no proxy
       * @param approve true if you support the witness, false otherwise
       *
       * @param broadcast true if you wish to broadcast the transaction
       * @return the signed transaction changing your vote proxy settings
       */
      annotated_signed_transaction set_voting_proxy(string account_to_modify,
                                          string proxy,
                                          bool broadcast = false);

      annotated_signed_transaction vote_for_witness(string account_to_vote_with,
                                          string witness_to_vote_for,
                                          bool approve = true,
                                          bool broadcast = false);

      annotated_signed_transaction transfer(string from, string to, asset amount, string memo, bool broadcast = false);
      annotated_signed_transaction transfer_to_vesting(string from, string to, asset amount, bool broadcast = false);
      annotated_signed_transaction withdraw_vesting( string from, share_type vesting_shares, bool broadcast = false );

      /**
       *  This method will convert STEEM to SBD or SBD to STEEM at the current_median_history price one
       *  week from the time it is executed. This method depends upon there being a valid price feed.
       */
      annotated_signed_transaction convert_sbd( string from, asset amount, bool broadcast = false );

      annotated_signed_transaction publish_feed(string witness, price exchange_rate, bool broadcast );

      /** Signs a transaction.
       *
       * Given a fully-formed transaction that is only lacking signatures, this signs
       * the transaction with the necessary keys and optionally broadcasts the transaction
       * @param tx the unsigned transaction
       * @param broadcast true if you wish to broadcast the transaction
       * @return the signed version of the transaction
       */
      annotated_signed_transaction sign_transaction(signed_transaction tx, bool broadcast = false);

      /** Returns an uninitialized object representing a given blockchain operation.
       *
       * This returns a default-initialized object of the given type; it can be used
       * during early development of the wallet when we don't yet have custom commands for
       * creating all of the operations the blockchain supports.
       *
       * Any operation the blockchain supports can be created using the transaction builder's
       * \c add_operation_to_builder_transaction() , but to do that from the CLI you need to
       * know what the JSON form of the operation looks like.  This will give you a template
       * you can fill in.  It's better than nothing.
       *
       * @param operation_type the type of operation to return, must be one of the
       *                       operations defined in `steemit/chain/operations.hpp`
       *                       (e.g., "global_parameters_update_operation")
       * @return a default-constructed operation of the given type
       */
      operation get_prototype_operation(string operation_type);

      void network_add_nodes( const vector<string>& nodes );
      vector< variant > network_get_connected_peers();


      /**
       *  Creates a limit order at the price amount_to_sell / min_to_receive and will deduct amount_to_sell from account
       *
       *  @param order_id is a unique identifier assigned by the creator of the order, it can be reused after the order has been filled
       */
      annotated_signed_transaction create_order( string owner, uint32_t order_id, asset amount_to_sell, asset min_to_receive, bool fill_or_kill, uint32_t expiration, bool broadcast );
      /**
       * Cancel an order created with create_order
       */
      annotated_signed_transaction cancel_order( string owner, uint32_t orderid, bool broadcast );

      /**
       *  Post or update a comment.
       *
       *  @param parent_author can be null if this is a top level comment
       *  @param parent_permlink becomes category if parent_author is ""
       */
      annotated_signed_transaction post_comment( string author, string permlink, string parent_author, string parent_permlink, string title, string body, string json, bool broadcast );
      annotated_signed_transaction vote( string voter, string author, string permlink, int16_t weight, bool broadcast );

      /**
       *  Account operations have sequence numbers from 0 to N where N is the most recent operation. This method
       *  returns operations in the range [from-limit, from]
       *
       *  @param from - the absolute sequence number, -1 means most recent, limit is the number of operations before from.
       *  @param limit - the maximum number of items that can be queried (0 to 1000], must be less than from
       */
      map<uint32_t,operation_object> get_account_history( string account, uint32_t from, uint32_t limit );




      std::map<string,std::function<string(fc::variant,const fc::variants&)>> get_result_formatters() const;

      fc::signal<void(bool)> lock_changed;
      std::shared_ptr<detail::wallet_api_impl> my;
      void encrypt_keys();
};

struct plain_keys {
   fc::sha512                  checksum;
   map<public_key_type,string> keys;
};

} }

FC_REFLECT( steemit::wallet::wallet_data,
            (cipher_keys)
            (ws_server)
            (ws_user)
            (ws_password)
          )

FC_REFLECT( steemit::wallet::brain_key_info, (brain_priv_key)(wif_priv_key) (pub_key));

FC_REFLECT_DERIVED( steemit::wallet::signed_block_with_info, (steemit::chain::signed_block),
   (block_id)(signing_key)(transaction_ids) )

FC_REFLECT( steemit::wallet::operation_detail, (memo)(description)(op) )
FC_REFLECT( steemit::wallet::plain_keys, (checksum)(keys) )

FC_API( steemit::wallet::wallet_api,
        /// wallet api
        (help)(gethelp)
        (about)(is_new)(is_locked)(lock)(unlock)(set_password)
        (load_wallet_file)(save_wallet_file)

        /// key api
        (import_key)
        (suggest_brain_key)
        (list_keys)
        (get_private_key)
        (normalize_brain_key)

        /// query api
        (info)
        (list_my_accounts)
        (list_accounts)
        (list_witnesses)
        (get_witness)
        (get_account)
        (get_block)
        (get_feed_history)
        (get_conversion_requests)
        (get_account_history)
        (get_state)

        /// transaction api
        (create_account)
        (create_account_with_keys)
        (update_account)
        (update_witness)
        (set_voting_proxy)
        (vote_for_witness)
        (transfer)
        (transfer_to_vesting)
        (withdraw_vesting)
        (convert_sbd)
        (publish_feed)
        (create_order)
        (cancel_order)
        (post_comment)
        (vote)

        /// helper api
        (get_prototype_operation)
        (serialize_transaction)
        (sign_transaction)

        (network_add_nodes)
        (network_get_connected_peers)

        (get_active_witnesses)
        (get_miner_queue)
        (get_transaction)
      )

