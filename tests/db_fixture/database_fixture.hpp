#pragma once

#include <appbase/application.hpp>
#include <steem/chain/database.hpp>
#include <fc/io/json.hpp>
#include <fc/smart_ref_impl.hpp>

#include <steem/plugins/debug_node/debug_node_plugin.hpp>

#include <steem/utilities/key_conversion.hpp>

#include <steem/plugins/database_api/database_api_plugin.hpp>
#include <steem/plugins/block_api/block_api_plugin.hpp>

#include <fc/network/http/connection.hpp>
#include <fc/network/ip.hpp>

#include <array>
#include <iostream>

#define INITIAL_TEST_SUPPLY (10000000000ll)

extern uint32_t ( STEEM_TESTING_GENESIS_TIMESTAMP );

#define PUSH_TX \
   steem::chain::test::_push_transaction

#define PUSH_BLOCK \
   steem::chain::test::_push_block

// See below
#define REQUIRE_OP_VALIDATION_SUCCESS( op, field, value ) \
{ \
   const auto temp = op.field; \
   op.field = value; \
   op.validate(); \
   op.field = temp; \
}
#define REQUIRE_OP_EVALUATION_SUCCESS( op, field, value ) \
{ \
   const auto temp = op.field; \
   op.field = value; \
   trx.operations.back() = op; \
   op.field = temp; \
   db.push_transaction( trx, ~0 ); \
}

/*#define STEEM_REQUIRE_THROW( expr, exc_type )          \
{                                                         \
   std::string req_throw_info = fc::json::to_string(      \
      fc::mutable_variant_object()                        \
      ("source_file", __FILE__)                           \
      ("source_lineno", __LINE__)                         \
      ("expr", #expr)                                     \
      ("exc_type", #exc_type)                             \
      );                                                  \
   if( fc::enable_record_assert_trip )                    \
      std::cout << "STEEM_REQUIRE_THROW begin "        \
         << req_throw_info << std::endl;                  \
   BOOST_REQUIRE_THROW( expr, exc_type );                 \
   if( fc::enable_record_assert_trip )                    \
      std::cout << "STEEM_REQUIRE_THROW end "          \
         << req_throw_info << std::endl;                  \
}*/

#define STEEM_REQUIRE_THROW( expr, exc_type )          \
   BOOST_REQUIRE_THROW( expr, exc_type );

#define STEEM_CHECK_THROW( expr, exc_type )            \
{                                                         \
   std::string req_throw_info = fc::json::to_string(      \
      fc::mutable_variant_object()                        \
      ("source_file", __FILE__)                           \
      ("source_lineno", __LINE__)                         \
      ("expr", #expr)                                     \
      ("exc_type", #exc_type)                             \
      );                                                  \
   if( fc::enable_record_assert_trip )                    \
      std::cout << "STEEM_CHECK_THROW begin "          \
         << req_throw_info << std::endl;                  \
   BOOST_CHECK_THROW( expr, exc_type );                   \
   if( fc::enable_record_assert_trip )                    \
      std::cout << "STEEM_CHECK_THROW end "            \
         << req_throw_info << std::endl;                  \
}

#define REQUIRE_OP_VALIDATION_FAILURE_2( op, field, value, exc_type ) \
{ \
   const auto temp = op.field; \
   op.field = value; \
   STEEM_REQUIRE_THROW( op.validate(), exc_type ); \
   op.field = temp; \
}
#define REQUIRE_OP_VALIDATION_FAILURE( op, field, value ) \
   REQUIRE_OP_VALIDATION_FAILURE_2( op, field, value, fc::exception )

#define REQUIRE_THROW_WITH_VALUE_2(op, field, value, exc_type) \
{ \
   auto bak = op.field; \
   op.field = value; \
   trx.operations.back() = op; \
   op.field = bak; \
   STEEM_REQUIRE_THROW(db.push_transaction(trx, ~0), exc_type); \
}

#define REQUIRE_THROW_WITH_VALUE( op, field, value ) \
   REQUIRE_THROW_WITH_VALUE_2( op, field, value, fc::exception )

///This simply resets v back to its default-constructed value. Requires v to have a working assingment operator and
/// default constructor.
#define RESET(v) v = decltype(v)()
///This allows me to build consecutive test cases. It's pretty ugly, but it works well enough for unit tests.
/// i.e. This allows a test on update_account to begin with the database at the end state of create_account.
#define INVOKE(test) ((struct test*)this)->test_method(); trx.clear()

#define PREP_ACTOR(name) \
   fc::ecc::private_key name ## _private_key = generate_private_key(BOOST_PP_STRINGIZE(name));   \
   fc::ecc::private_key name ## _post_key = generate_private_key(std::string( BOOST_PP_STRINGIZE(name) ) + "_post" ); \
   public_key_type name ## _public_key = name ## _private_key.get_public_key();

#define ACTOR(name) \
   PREP_ACTOR(name) \
   const auto& name = account_create(BOOST_PP_STRINGIZE(name), name ## _public_key, name ## _post_key.get_public_key()); \
   account_id_type name ## _id = name.id; (void)name ## _id;

#define GET_ACTOR(name) \
   fc::ecc::private_key name ## _private_key = generate_private_key(BOOST_PP_STRINGIZE(name)); \
   const account_object& name = get_account(BOOST_PP_STRINGIZE(name)); \
   account_id_type name ## _id = name.id; \
   (void)name ##_id

#define ACTORS_IMPL(r, data, elem) ACTOR(elem)
#define ACTORS(names) BOOST_PP_SEQ_FOR_EACH(ACTORS_IMPL, ~, names) \
   validate_database();

#define SMT_SYMBOL( name, decimal_places ) \
   asset_symbol_type name ## _symbol = name_to_asset_symbol( #name , decimal_places );

#define ASSET( s ) \
   asset::from_string( s )

namespace steem { namespace chain {

using namespace steem::protocol;

struct database_fixture {
   // the reason we use an app is to exercise the indexes of built-in
   //   plugins
   chain::database* db = nullptr;
   signed_transaction trx;
   public_key_type committee_key;
   account_id_type committee_account;
   fc::ecc::private_key private_key = fc::ecc::private_key::generate();
   fc::ecc::private_key init_account_priv_key = fc::ecc::private_key::regenerate( fc::sha256::hash( string( "init_key" ) ) );
   string debug_key = steem::utilities::key_to_wif( init_account_priv_key );
   public_key_type init_account_pub_key = init_account_priv_key.get_public_key();
   uint32_t default_skip = 0 | database::skip_undo_history_check | database::skip_authority_check;

   plugins::debug_node::debug_node_plugin* db_plugin;

   optional<fc::temp_directory> data_dir;
   bool skip_key_index_test = false;
   uint32_t anon_acct_count;

   database_fixture() {}
   virtual ~database_fixture() { appbase::reset(); }

   static fc::ecc::private_key generate_private_key( string seed = "init_key" );
   static asset_symbol_type name_to_asset_symbol( const std::string& name, uint8_t decimal_places );
   string generate_anon_acct_name();
   void open_database();
   void generate_block(uint32_t skip = 0,
                               const fc::ecc::private_key& key = generate_private_key("init_key"),
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

   const account_object& account_create(
      const string& name,
      const string& creator,
      const private_key_type& creator_key,
      const share_type& fee,
      const public_key_type& key,
      const public_key_type& post_key,
      const string& json_metadata
   );

   const account_object& account_create(
      const string& name,
      const public_key_type& key,
      const public_key_type& post_key
   );

   const account_object& account_create(
      const string& name,
      const public_key_type& key
   );


   const witness_object& witness_create(
      const string& owner,
      const private_key_type& owner_key,
      const string& url,
      const public_key_type& signing_key,
      const share_type& fee
   );

   void fund( const string& account_name, const share_type& amount = 500000 );
   void fund( const string& account_name, const asset& amount );
   void transfer( const string& from, const string& to, const share_type& steem );
   void convert( const string& account_name, const asset& amount );
   void vest( const string& from, const share_type& amount );
   void vest( const string& account, const asset& amount );
   void proxy( const string& account, const string& proxy );
   void set_price_feed( const price& new_price );
   const asset& get_balance( const string& account_name )const;
   void sign( signed_transaction& trx, const fc::ecc::private_key& key );

   vector< operation > get_last_operations( uint32_t ops );

   void validate_database( void );
};

struct clean_database_fixture : public database_fixture
{
   clean_database_fixture();
   virtual ~clean_database_fixture();

   void resize_shared_mem( uint64_t size );
};

struct live_database_fixture : public database_fixture
{
   live_database_fixture();
   virtual ~live_database_fixture();

   fc::path _chain_dir;
};

#ifdef STEEM_ENABLE_SMT
struct smt_database_fixture : public clean_database_fixture
{
   smt_database_fixture();
   virtual ~smt_database_fixture();

   asset_symbol_type create_smt( signed_transaction& trx, const string& account_name, const fc::ecc::private_key& key,
      uint8_t token_decimal_places );

   void transfer_smt( const string& from, const string& to, const asset& steem );

   /// Creates 3 different SMTs for provided control account, one with 0 precision, the other two with the same non-zero precision.
   std::array<asset_symbol_type, 3> create_smt_3(const char* control_account_name, const fc::ecc::private_key& key);
   /// Tries to create SMTs with too big precision or invalid name.
   void create_invalid_smt( const char* control_account_name, const fc::ecc::private_key& key );
   /// Tries to create SMTs matching existing one. First attempt with matching precision, second one with different (but valid) precision.
   void create_conflicting_smt( const asset_symbol_type existing_smt, const char* control_account_name, const fc::ecc::private_key& key );
};
#endif

struct json_rpc_database_fixture : public database_fixture
{
   private:
      steem::plugins::json_rpc::json_rpc_plugin* rpc_plugin;

      fc::variant get_answer( std::string& request );
      void review_answer( fc::variant& answer, int64_t code, bool is_warning, bool is_fail, fc::optional< fc::variant > id );

   public:

      json_rpc_database_fixture();
      virtual ~json_rpc_database_fixture();

      void make_array_request( std::string& request, int64_t code = 0, bool is_warning = false, bool is_fail = true );
      fc::variant make_request( std::string& request, int64_t code = 0, bool is_warning = false, bool is_fail = true );
      void make_positive_request( std::string& request );
};

namespace test
{
   bool _push_block( database& db, const signed_block& b, uint32_t skip_flags = 0 );
   void _push_transaction( database& db, const signed_transaction& tx, uint32_t skip_flags = 0 );
}

} }
