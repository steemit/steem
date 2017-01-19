#include <fc/interprocess/container.hpp>
#include <steemit/chain2/block_objects.hpp>
#include <steemit/chain2/chain_database.hpp>
#include <fc/io/raw.hpp>

namespace steemit {
    namespace chain2 {
        database::database() {
        }

        database::~database() {
        }

        void database::open(const fc::path &dir) {
            graphene::db2::database::open(dir);

            add_index<block_index>();
            add_index<transaction_index>();

            if (head_block_num()) {
                const auto &head = head_block();
                signed_block front;
                front.previous = head.previous;
                //_fork_db.set_head( branches.second.front() );
            }
        }


        void database::push_block(const signed_block &b) {
            try {
                if (!head_block_num()) {
                    _fork_db.start_block(b);
                }

                auto restore_pending = clear_pending();

                const auto &head = head_block();

                auto new_head = _fork_db.push_block(b);
                if (new_head->previous_id() == head.previous) {
                    try {
                        apply(*this, b,
                                skip_undo_transaction | skip_undo_operation);
                    } catch (const fc::exception &e) {
                        _fork_db.remove(b.id());
                        throw;
                    }
                } else {

                    auto branches = _fork_db.fetch_branch_from(new_head->id, head.block_id);
                    while (head_block().block_id !=
                           branches.second.back()->previous_id()) {
                        undo();
                    }

                    for (auto ritr = branches.first.rbegin();
                         ritr != branches.first.rend(); ++ritr) {
                        optional <fc::exception> except;
                        try {
                            apply(*this, (*ritr)->data, skip_undo_transaction |
                                                        skip_undo_operation);
                        } catch (const fc::exception &e) {
                            except = e;
                        }

                        if (except) {
                            // wlog( "exception thrown while switching forks ${e}", ("e",except->to_detail_string() ) );
                            // remove the rest of branches.first from the fork_db, those blocks are invalid
                            while (ritr != branches.first.rend()) {
                                _fork_db.remove((*ritr)->id); //data.id() );
                                ++ritr;
                            }
                            _fork_db.set_head(branches.second.front());

                            // pop all blocks from the bad fork
                            while (head_block().block_id !=
                                   branches.second.back()->data.previous) {
                                       undo();
                            }

                            // restore all blocks from the good fork
                            for (auto ritr = branches.second.rbegin();
                                 ritr != branches.second.rend(); ++ritr) {
                                     apply(*this, (*ritr)->data,
                                             skip_undo_transaction |
                                             skip_undo_operation);
                            }
                            throw *except;
                        }
                    }
                }
            } FC_CAPTURE_AND_RETHROW((b))
        }

        void database::push_transaction(const signed_transaction &trx) {
            try {

                if (!_pending_tx_session) {
                    _pending_tx_session = start_undo_session(true);
                }

                auto undos = start_undo_session(true);
                apply(*this, trx);
                _pending_transactions.emplace_back(trx);

            } FC_CAPTURE_AND_RETHROW((trx))
        }

        signed_block database::generate_block(time_point_sec time, const account_name_type &witness, const fc::ecc::private_key &block_signing_key) {
            signed_block result;

            return result;
        }

        const block_object &database::head_block() const {
            const auto &block_idx = get_index<block_index, by_block_num>();
            auto head_block_itr = block_idx.rbegin();
            FC_ASSERT(head_block_itr != block_idx.rend());
            return *head_block_itr;
        }

        uint32_t database::head_block_num() const {
            const auto &block_idx = get_index<block_index, by_block_num>();
            auto head_block_itr = block_idx.rbegin();
            if (head_block_itr != block_idx.rend()) {
                return head_block_itr->block_num;
            }
            return 0;
        }

        void apply(database &db, const signed_block &b, const options_type &opts) {
            auto undo_session = db.start_undo_session(!(opts &
                                                        skip_undo_block));
            db.pre_apply_block(b);

            if (!(opts & skip_validation)) {
                FC_ASSERT(b.timestamp.sec_since_epoch() % 3 == 0);
                if (b.block_num() > 1) {
                    idump((b.block_num()));
                    const auto &head = db.head_block();
                    FC_ASSERT(b.block_num() == head.block_num + 1);
                    FC_ASSERT(b.timestamp >= head.timestamp + fc::seconds(3));
                }
            }

            db.create<block_object>([&](block_object &obj) {
                obj.block_num = b.block_num();
                obj.block_id = b.id();
                obj.ref_prefix = obj.block_id._hash[1];
                obj.previous = b.previous;
                obj.timestamp = b.timestamp;
                obj.witness = b.witness;
                obj.transaction_merkle_root = b.transaction_merkle_root;
                obj.witness_signature = b.witness_signature;

                obj.transactions.reserve(b.transactions.size());
                for (const auto &t : b.transactions) {
                    obj.transactions.emplace_back(t.id());
                }
            });

            for (const auto &trx : b.transactions) {
                apply(db, trx, opts);
            }

            db.post_apply_block(b);
            undo_session.push();
        }

        void apply(database &db, const signed_transaction &t, const options_type &opts) {
            auto undo_session = db.start_undo_session(!(opts &
                                                        skip_undo_transaction));
            db.pre_apply_transaction(t);

            db.create<transaction_object>([&](transaction_object &trx) {
                trx.trx_id = t.id();
                trx.block_num = db.head_block().block_num;
                auto pack_size = fc::raw::pack_size(t);
                trx.packed_transaction.resize(pack_size);
                fc::datastream<char *> ds(trx.packed_transaction.data(), pack_size);
                fc::raw::pack(ds, t);
            });

            for (const auto &op : t.operations) {
                apply(db, op, opts);
            }

            db.post_apply_transaction(t);
            undo_session.squash();
        }

        struct apply_operation_visitor {
            apply_operation_visitor(database &db, const options_type &opts)
                    : _db(db), _opts(opts) {
            }

            typedef void result_type;

            template<typename T>
            void operator()(T &&op) const {
                apply(_db, std::forward<T>(op), _opts);
            }

            database &_db;
            const options_type &_opts;
        };

        void apply(database &db, const operation &o, const options_type &opts) {
            auto undo_session = db.start_undo_session(!(opts &
                                                        skip_undo_operation));
            db.pre_apply_operation(o);
            o.visit(apply_operation_visitor(db, opts));
            db.post_apply_operation(o);
            undo_session.squash();
        }

        struct validate_operation_visitor {
            validate_operation_visitor(const options_type &opts) : _opts(opts) {
            }

            typedef void result_type;

            template<typename T>
            void operator()(T &&op) const {
                validate(std::forward<T>(op), _opts);
            }

            const options_type &_opts;
        };

        void validate(const operation &o, const options_type &opts) {
            o.visit(validate_operation_visitor(opts));
        }

    }
} // steemit::chain2
