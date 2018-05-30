#pragma once

#include <golos/chain/database.hpp>

/*
 * This file provides with() functions which modify the database
 * temporarily, then restore it.  These functions are mostly internal
 * implementation detail of the database.
 *
 * Essentially, we want to be able to use "finally" to restore the
 * database regardless of whether an exception is thrown or not, but there
 * is no "finally" in C++.  Instead, C++ requires us to create a struct
 * and put the finally block in a destructor.  Aagh!
 */

namespace golos {
    namespace chain {
        namespace detail {
            /**
             * Class used to help the without_pending_transactions implementation.
               *
             * TODO:  Change the name of this class to better reflect the fact
             * that it restores popped transactions as well as pending transactions.
             */
            struct pending_transactions_restorer final {
                pending_transactions_restorer(
                    database &db, uint32_t skip,
                    std::vector<signed_transaction> &&pending_transactions
                )
                    : _db(db),
                      _skip(skip),
                      _pending_transactions(std::move(pending_transactions))
                {
                    _db.clear_pending();
                }

                ~pending_transactions_restorer() {
                    for (const auto &tx : _db._popped_tx) {
                        try {
                            if (!_db.is_known_transaction(tx.id())) {
                                // since push_transaction() takes a signed_transaction,
                                // the operation_results field will be ignored.
                                _db._push_transaction(tx, _skip);
                            }
                        } catch (const fc::exception &) {
                        }
                    }
                    _db._popped_tx.clear();
                    for (const signed_transaction &tx : _pending_transactions) {
                        try {
                            if (!_db.is_known_transaction(tx.id())) {
                                // since push_transaction() takes a signed_transaction,
                                // the operation_results field will be ignored.
                                _db._push_transaction(tx, _skip);
                            }
                        } catch (const fc::exception &e) {

                            //wlog( "Pending transaction became invalid after switching to block ${b}  ${t}", ("b", _db.head_block_id())("t",_db.head_block_time()) );
                            //wlog( "The invalid pending transaction caused exception ${e}", ("e", e.to_detail_string() ) );

                        }
                    }
                }

                database &_db;
                uint32_t _skip;
                std::vector<signed_transaction> _pending_transactions;
            };

            /**
             * Class is used to help the with_producing implementation
             */
            struct producing_helper final {
                producing_helper(database& db): _db(db) {
                    _db.set_producing(true);
                }

                ~producing_helper() {
                    _db.set_producing(false);
                }

                database &_db;
            };

            /**
             * Empty pending_transactions, call callback,
             * then reset pending_transactions after callback is done.
             *
             * Pending transactions which no longer validate will be culled.
             */
            template<typename Lambda>
            void without_pending_transactions(
                database& db,
                uint32_t skip,
                std::vector<signed_transaction>&& pending_transactions,
                Lambda callback
            ) {
                pending_transactions_restorer restorer(db, skip, std::move(pending_transactions));
                callback();
                return;
            }

            /**
             * Set producing flag to true, call callback, then set producing flag to false.
             */
             template <typename Lambda>
             void with_producing(
                 database& db,
                 Lambda callback
             ) {
                 producing_helper restorer(db);
                 callback();
             }
        }
    }
} // golos::chain::detail
