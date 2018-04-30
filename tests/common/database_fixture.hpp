#ifndef DATABASE_FIXTURE_HPP
#define DATABASE_FIXTURE_HPP

#include <appbase/application.hpp>
#include <golos/chain/database.hpp>

#include <fc/io/json.hpp>
#include <fc/smart_ref_impl.hpp>

#include <golos/plugins/debug_node/plugin.hpp>
#include <golos/plugins/account_history/plugin.hpp>

#include <graphene/utilities/key_conversion.hpp>

#include <iostream>

#define INITIAL_TEST_SUPPLY (10000000000ll)

extern uint32_t ( STEEMIT_TESTING_GENESIS_TIMESTAMP );

#define PUSH_TX \
   golos::chain::test::_push_transaction

#define PUSH_BLOCK \
   golos::chain::test::_push_block

// See below
#define REQUIRE_OP_VALIDATION_SUCCESS(op, field, value) \
{ \
   const auto temp = op.field; \
   op.field = value; \
   op.validate(); \
   op.field = temp; \
}
#define REQUIRE_OP_EVALUATION_SUCCESS(op, field, value) \
{ \
   const auto temp = op.field; \
   op.field = value; \
   trx.operations.back() = op; \
   op.field = temp; \
   db.push_transaction( trx, ~0 ); \
}

/*#define STEEMIT_REQUIRE_THROW( expr, exc_type )          \
{                                                         \
   std::string req_throw_info = fc::json::to_string(      \
      fc::mutable_variant_object()                        \
      ("source_file", __FILE__)                           \
      ("source_lineno", __LINE__)                         \
      ("expr", #expr)                                     \
      ("exc_type", #exc_type)                             \
      );                                                  \
   if( fc::enable_record_assert_trip )                    \
      std::cout << "STEEMIT_REQUIRE_THROW begin "        \
         << req_throw_info << std::endl;                  \
   BOOST_REQUIRE_THROW( expr, exc_type );                 \
   if( fc::enable_record_assert_trip )                    \
      std::cout << "STEEMIT_REQUIRE_THROW end "          \
         << req_throw_info << std::endl;                  \
}*/

#define STEEMIT_REQUIRE_THROW(expr, exc_type)          \
   BOOST_REQUIRE_THROW( expr, exc_type );

#define STEEMIT_CHECK_THROW(expr, exc_type)            \
{                                                         \
   std::string req_throw_info = fc::json::to_string(      \
      fc::mutable_variant_object()                        \
      ("source_file", __FILE__)                           \
      ("source_lineno", __LINE__)                         \
      ("expr", #expr)                                     \
      ("exc_type", #exc_type)                             \
      );                                                  \
   if( fc::enable_record_assert_trip )                    \
      std::cout << "STEEMIT_CHECK_THROW begin "          \
         << req_throw_info << std::endl;                  \
   BOOST_CHECK_THROW( expr, exc_type );                   \
   if( fc::enable_record_assert_trip )                    \
      std::cout << "STEEMIT_CHECK_THROW end "            \
         << req_throw_info << std::endl;                  \
}

#define REQUIRE_OP_VALIDATION_FAILURE_2(op, field, value, exc_type) \
{ \
   const auto temp = op.field; \
   op.field = value; \
   STEEMIT_REQUIRE_THROW( op.validate(), exc_type ); \
   op.field = temp; \
}
#define REQUIRE_OP_VALIDATION_FAILURE(op, field, value) \
   REQUIRE_OP_VALIDATION_FAILURE_2( op, field, value, fc::exception )

#define REQUIRE_THROW_WITH_VALUE_2(op, field, value, exc_type) \
{ \
   auto bak = op.field; \
   op.field = value; \
   trx.operations.back() = op; \
   op.field = bak; \
   STEEMIT_REQUIRE_THROW(db.push_transaction(trx, ~0), exc_type); \
}

#define REQUIRE_THROW_WITH_VALUE(op, field, value) \
   REQUIRE_THROW_WITH_VALUE_2( op, field, value, fc::exception )

///This simply resets v back to its default-constructed value. Requires v to have a working assingment operator and
/// default constructor.
#define RESET(v) v = decltype(v)()
///This allows me to build consecutive test cases. It's pretty ugly, but it works well enough for unit tests.
/// i.e. This allows a test on update_account to begin with the database at the end state of create_account.
#define INVOKE(test) ((struct test*)this)->test_method(); trx.clear()

#define PREP_ACTOR(name) \
   fc::ecc::private_key name ## _private_key = generate_private_key(BOOST_PP_STRINGIZE(name));   \
   fc::ecc::private_key name ## _post_key = generate_private_key(BOOST_PP_STRINGIZE(name) "_post"); \
   public_key_type name ## _public_key = name ## _private_key.get_public_key();

#define ACTOR(name) \
   PREP_ACTOR(name) \
   const auto& name = account_create(BOOST_PP_STRINGIZE(name), name ## _public_key, name ## _post_key.get_public_key()); \
   account_id_type name ## _id = name.id; (void)name ## _id;

#define GET_ACTOR(name) \
   fc::ecc::private_key name ## _private_key = generate_private_key(BOOST_PP_STRINGIZE(name)); \
   const account_object& name = get_account(BOOST_PP_STRINGIZE(name)); \
   account_id_type name ## _id = name.id; (void)name ## _id;

#define ACTORS_IMPL(r, data, elem) ACTOR(elem)
#define ACTORS(names) BOOST_PP_SEQ_FOR_EACH(ACTORS_IMPL, ~, names) \
   validate_database();


// Note: testnet and mainnet can have different asset names
#define ASSET(s) asset::from_string(s)

#define ASSET_GBG(x)   asset(int64_t((x)*1e3), SBD_SYMBOL)
#define ASSET_GOLOS(x) asset(int64_t((x)*1e3), STEEM_SYMBOL)
#define ASSET_GESTS(x) asset(int64_t((x)*1e6), VESTS_SYMBOL)


// get_vesting_share_price() is a dynamic value which depends on funds,
//   that is why comparision can be done only with some correction
#define GOLOS_VEST_REQUIRE_EQUAL(left, right) \
    BOOST_REQUIRE( \
            std::abs((left).amount.value - (right).amount.value) < 5 && \
            (left).symbol == (right).symbol \
    )



#ifndef STEEMIT_INIT_PRIVATE_KEY
#  define STEEMIT_INIT_PRIVATE_KEY (fc::ecc::private_key::regenerate(fc::sha256::hash(BLOCKCHAIN_NAME)))
#endif

namespace golos { namespace chain {

        using namespace golos::protocol;

        struct database_fixture {
            // the reason we use an app is to exercise the indexes of built-in
            //   plugins
            chain::database *db;
            signed_transaction trx;
            fc::ecc::private_key init_account_priv_key = STEEMIT_INIT_PRIVATE_KEY;
            string debug_key = golos::utilities::key_to_wif(init_account_priv_key);
            public_key_type init_account_pub_key = init_account_priv_key.get_public_key();
            uint32_t default_skip = 0 | database::skip_undo_history_check |
                                    database::skip_authority_check;

            golos::plugins::chain::plugin *ch_plugin = nullptr;
            golos::plugins::debug_node::plugin *db_plugin = nullptr;
            golos::plugins::account_history::plugin *ah_plugin = nullptr;

            optional<fc::temp_directory> data_dir;
            bool skip_key_index_test = false;

            uint32_t anon_acct_count;

            database_fixture() {
            }

            virtual ~database_fixture();

            static fc::ecc::private_key generate_private_key(string seed);

            string generate_anon_acct_name();

            void initialize();
            void startup(bool generate_hardfork = true);

            void open_database();
            void close_database();

            void generate_block(uint32_t skip = 0,
                    const fc::ecc::private_key &key = STEEMIT_INIT_PRIVATE_KEY,
                    int miss_blocks = 0);

            /**
             * @brief Generates block_count blocks
             * @param block_count number of blocks to generate
             */
            void generate_blocks(uint32_t block_count);

            /**
             * @brief Generates blocks until the head block time matches or exceeds timestamp
             * @param timestamp target time to generate blocks until
             */
            void generate_blocks(fc::time_point_sec timestamp, bool miss_intermediate_blocks = true);

            const account_object &account_create(
                    const string &name,
                    const string &creator,
                    const private_key_type &creator_key,
                    const share_type &fee,
                    const public_key_type &key,
                    const public_key_type &post_key,
                    const string &json_metadata
            );

            const account_object &account_create(
                    const string &name,
                    const public_key_type &key,
                    const public_key_type &post_key
            );

            const account_object &account_create(
                    const string &name,
                    const public_key_type &key
            );


            const witness_object &witness_create(
                    const string &owner,
                    const private_key_type &owner_key,
                    const string &url,
                    const public_key_type &signing_key,
                    const share_type &fee
            );

            void fund(const string &account_name, const share_type &amount = 500000);

            void fund(const string &account_name, const asset &amount);

            void transfer(const string &from, const string &to, const share_type &steem);

            void convert(const string &account_name, const asset &amount);

            void vest(const string &from, const share_type &amount);

            void vest(const string &account, const asset &amount);

            void proxy(const string &account, const string &proxy);

            void set_price_feed(const price &new_price);

            const asset &get_balance(const string &account_name) const;

            void sign(signed_transaction &trx, const fc::ecc::private_key &key);

            vector<operation> get_last_operations(uint32_t ops);

            void validate_database(void);


            // helpers to remove boilerplate code
        private:
            void tx_push_ops(signed_transaction& tx, operation op) {
                tx.operations.push_back(op);
            }
            template<typename... Ops>
            void tx_push_ops(signed_transaction& tx, operation op, Ops... ops) {
                tx.operations.push_back(op);
                tx_push_ops(tx, ops...);
            }

        public:
            template<typename... Ops>
            void sign_tx_with_ops(signed_transaction& tx, const fc::ecc::private_key& k, Ops... ops) {
                tx.clear();
                tx.set_expiration(db->head_block_time() + STEEMIT_MAX_TIME_UNTIL_EXPIRATION);
                tx_push_ops(tx, ops...);
                sign(tx, k);
            }

            template<typename... Ops>
            void push_tx_with_ops(signed_transaction& tx, const fc::ecc::private_key& k, Ops... ops) {
                sign_tx_with_ops(tx, k, ops...);
                db->push_transaction(tx, 0);
            }
        };

        struct clean_database_fixture : public database_fixture {
            clean_database_fixture();

            ~clean_database_fixture() override;

            void resize_shared_mem(uint64_t size);
        };

        struct live_database_fixture : public database_fixture {
            live_database_fixture();

            ~live_database_fixture() override;

            fc::path _chain_dir;
        };

        namespace test {
            bool _push_block(database &db, const signed_block &b, uint32_t skip_flags = 0);

            void _push_transaction(database &db, const signed_transaction &tx, uint32_t skip_flags = 0);
        }

} } // golos:chain
#endif