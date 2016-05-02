#pragma once
#include <steemit/chain/config.hpp>

#include <graphene/db/object_id.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/undo_database.hpp>

#include <fc/container/flat_fwd.hpp>
#include <fc/io/varint.hpp>
#include <fc/io/enum_type.hpp>
#include <fc/crypto/sha224.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/optional.hpp>
#include <fc/safe.hpp>
#include <fc/container/flat.hpp>
#include <fc/string.hpp>
#include <fc/io/raw.hpp>
#include <fc/uint128.hpp>
#include <fc/static_variant.hpp>
#include <fc/smart_ref_fwd.hpp>

#include <boost/multiprecision/cpp_int.hpp>

#include <memory>
#include <vector>
#include <deque>
#include <cstdint>

namespace steemit { namespace chain {
   using                                    graphene::db::object_id_type;
   using                                    graphene::db::object_id;
   using                                    graphene::db::abstract_object;
   using                                    graphene::db::undo_database;

   using                                    fc::uint128_t;
   typedef boost::multiprecision::uint256_t u256;
   typedef boost::multiprecision::uint512_t u512;

   using                               std::map;
   using                               std::vector;
   using                               std::unordered_map;
   using                               std::string;
   using                               std::deque;
   using                               std::shared_ptr;
   using                               std::weak_ptr;
   using                               std::unique_ptr;
   using                               std::set;
   using                               std::pair;
   using                               std::enable_shared_from_this;
   using                               std::tie;
   using                               std::make_pair;

   using                               fc::smart_ref;
   using                               fc::variant_object;
   using                               fc::variant;
   using                               fc::enum_type;
   using                               fc::optional;
   using                               fc::unsigned_int;
   using                               fc::signed_int;
   using                               fc::time_point_sec;
   using                               fc::time_point;
   using                               fc::safe;
   using                               fc::flat_map;
   using                               fc::flat_set;
   using                               fc::static_variant;
   using                               fc::ecc::range_proof_type;
   using                               fc::ecc::range_proof_info;
   using                               fc::ecc::commitment_type;
   struct void_t{};

   typedef fc::ecc::private_key        private_key_type;
   typedef fc::sha256 chain_id_type;


   enum reserved_spaces
   {
      relative_protocol_ids = 0,
      protocol_ids          = 1,
      implementation_ids    = 2
   };

   inline bool is_relative( object_id_type o ){ return o.space() == 0; }

   enum object_type
   {
      null_object_type,
      base_object_type,
      OBJECT_TYPE_COUNT ///< Sentry value which contains the number of different object types
   };

   enum impl_object_type
   {
      impl_dynamic_global_property_object_type,
      impl_reserved0_object_type,      // formerly index_meta_object_type, TODO: delete me
      impl_account_object_type,
      impl_witness_object_type,
      impl_transaction_object_type,
      impl_block_summary_object_type,
      impl_chain_property_object_type,
      impl_witness_schedule_object_type,
      impl_comment_object_type,
      impl_comment_vote_object_type,
      impl_vote_object_type,
      impl_witness_vote_object_type,
      impl_limit_order_object_type,
      impl_feed_history_object_type,
      impl_convert_request_object_type,
      impl_liquidity_reward_balance_object_type,
      impl_operation_object_type,
      impl_account_history_object_type,
      impl_category_object_type,
      impl_hardfork_property_object_type
   };

   class operation_object;
   class account_history_object;
   class comment_object;
   class category_object;
   class comment_vote_object;
   class vote_object;
   class witness_vote_object;
   class account_object;
   class feed_history_object;
   class limit_order_object;
   class convert_request_object;
   class dynamic_global_property_object;
   class transaction_object;
   class block_summary_object;
   class chain_property_object;
   class witness_schedule_object;
   class account_object;
   class witness_object;
   class liquidity_reward_balance_object;
   class hardfork_property_object;

   typedef object_id< implementation_ids, impl_operation_object_type,         operation_object>                         operation_id_type;
   typedef object_id< implementation_ids, impl_account_history_object_type,   account_history_object>                   account_history_id_type;
   typedef object_id< implementation_ids, impl_comment_object_type,           comment_object>                           comment_id_type;
   typedef object_id< implementation_ids, impl_category_object_type,           category_object>                         category_id_type;
   typedef object_id< implementation_ids, impl_comment_vote_object_type,      comment_vote_object>                      comment_vote_id_type;
   typedef object_id< implementation_ids, impl_vote_object_type,              vote_object>                              vote_id_type;
   typedef object_id< implementation_ids, impl_witness_vote_object_type,      witness_vote_object>                      witness_vote_id_type;
   typedef object_id< implementation_ids, impl_limit_order_object_type,       limit_order_object>                       limit_order_id_type;
   typedef object_id< implementation_ids, impl_feed_history_object_type,      feed_history_object>                      feed_history_id_type;
   typedef object_id< implementation_ids, impl_convert_request_object_type,   convert_request_object>                   convert_request_id_type;
   typedef object_id< implementation_ids, impl_account_object_type, account_object>                                     account_id_type;
   typedef object_id< implementation_ids, impl_witness_object_type, witness_object>                                     witness_id_type;
   typedef object_id< implementation_ids, impl_dynamic_global_property_object_type,  dynamic_global_property_object>    dynamic_global_property_id_type;
   typedef object_id< implementation_ids, impl_transaction_object_type,      transaction_object>                        transaction_obj_id_type;
   typedef object_id< implementation_ids, impl_block_summary_object_type,    block_summary_object>                      block_summary_id_type;
   typedef object_id< implementation_ids, impl_chain_property_object_type,   chain_property_object>                     chain_property_id_type;
   typedef object_id< implementation_ids, impl_witness_schedule_object_type, witness_schedule_object>                   witness_schedule_id_type;
   typedef object_id< implementation_ids, impl_liquidity_reward_balance_object_type, liquidity_reward_balance_object >  liquidity_reward_balance_id_type;
   typedef object_id< implementation_ids, impl_hardfork_property_object_type, hardfork_property_object >                 hardfork_property_id_type;

   typedef fc::ripemd160                                        block_id_type;
   typedef fc::ripemd160                                        checksum_type;
   typedef fc::ripemd160                                        transaction_id_type;
   typedef fc::sha256                                           digest_type;
   typedef fc::ecc::compact_signature                           signature_type;
   typedef safe<int64_t>                                        share_type;
   typedef uint16_t                                             weight_type;


   struct public_key_type
   {
       struct binary_key
       {
          binary_key() {}
          uint32_t                 check = 0;
          fc::ecc::public_key_data data;
       };
       fc::ecc::public_key_data key_data;
       public_key_type();
       public_key_type( const fc::ecc::public_key_data& data );
       public_key_type( const fc::ecc::public_key& pubkey );
       explicit public_key_type( const std::string& base58str );
       operator fc::ecc::public_key_data() const;
       operator fc::ecc::public_key() const;
       explicit operator std::string() const;
       friend bool operator == ( const public_key_type& p1, const fc::ecc::public_key& p2);
       friend bool operator == ( const public_key_type& p1, const public_key_type& p2);
       friend bool operator < ( const public_key_type& p1, const public_key_type& p2) { return p1.key_data < p2.key_data; }
       friend bool operator != ( const public_key_type& p1, const public_key_type& p2);
   };

#define STEEMIT_INIT_PUBLIC_KEY (steemit::chain::public_key_type(STEEMIT_INIT_PUBLIC_KEY_STR))

   struct extended_public_key_type
   {
      struct binary_key
      {
         binary_key() {}
         uint32_t                   check = 0;
         fc::ecc::extended_key_data data;
      };

      fc::ecc::extended_key_data key_data;

      extended_public_key_type();
      extended_public_key_type( const fc::ecc::extended_key_data& data );
      extended_public_key_type( const fc::ecc::extended_public_key& extpubkey );
      explicit extended_public_key_type( const std::string& base58str );
      operator fc::ecc::extended_public_key() const;
      explicit operator std::string() const;
      friend bool operator == ( const extended_public_key_type& p1, const fc::ecc::extended_public_key& p2);
      friend bool operator == ( const extended_public_key_type& p1, const extended_public_key_type& p2);
      friend bool operator != ( const extended_public_key_type& p1, const extended_public_key_type& p2);
   };

   struct extended_private_key_type
   {
      struct binary_key
      {
         binary_key() {}
         uint32_t                   check = 0;
         fc::ecc::extended_key_data data;
      };

      fc::ecc::extended_key_data key_data;

      extended_private_key_type();
      extended_private_key_type( const fc::ecc::extended_key_data& data );
      extended_private_key_type( const fc::ecc::extended_private_key& extprivkey );
      explicit extended_private_key_type( const std::string& base58str );
      operator fc::ecc::extended_private_key() const;
      explicit operator std::string() const;
      friend bool operator == ( const extended_private_key_type& p1, const fc::ecc::extended_private_key& p2);
      friend bool operator == ( const extended_private_key_type& p1, const extended_private_key_type& p2);
      friend bool operator != ( const extended_private_key_type& p1, const extended_private_key_type& p2);
   };
} }  // steemit::chain

namespace fc
{
    void to_variant( const steemit::chain::public_key_type& var,  fc::variant& vo );
    void from_variant( const fc::variant& var,  steemit::chain::public_key_type& vo );
    void to_variant( const steemit::chain::extended_public_key_type& var, fc::variant& vo );
    void from_variant( const fc::variant& var, steemit::chain::extended_public_key_type& vo );
    void to_variant( const steemit::chain::extended_private_key_type& var, fc::variant& vo );
    void from_variant( const fc::variant& var, steemit::chain::extended_private_key_type& vo );
}

FC_REFLECT( steemit::chain::public_key_type, (key_data) )
FC_REFLECT( steemit::chain::public_key_type::binary_key, (data)(check) )
FC_REFLECT( steemit::chain::extended_public_key_type, (key_data) )
FC_REFLECT( steemit::chain::extended_public_key_type::binary_key, (check)(data) )
FC_REFLECT( steemit::chain::extended_private_key_type, (key_data) )
FC_REFLECT( steemit::chain::extended_private_key_type::binary_key, (check)(data) )

FC_REFLECT_ENUM( steemit::chain::object_type,
                 (null_object_type)
                 (base_object_type)
                 (OBJECT_TYPE_COUNT)
               )
FC_REFLECT_ENUM( steemit::chain::impl_object_type,
                 (impl_dynamic_global_property_object_type)
                 (impl_reserved0_object_type)
                 (impl_account_object_type)
                 (impl_witness_object_type)
                 (impl_transaction_object_type)
                 (impl_block_summary_object_type)
                 (impl_chain_property_object_type)
                 (impl_witness_schedule_object_type)
                 (impl_comment_object_type)
                 (impl_category_object_type)
                 (impl_comment_vote_object_type)
                 (impl_vote_object_type)
                 (impl_witness_vote_object_type)
                 (impl_limit_order_object_type)
                 (impl_feed_history_object_type)
                 (impl_convert_request_object_type)
                 (impl_liquidity_reward_balance_object_type)
                 (impl_operation_object_type)
                 (impl_account_history_object_type)
                 (impl_hardfork_property_object_type)
               )

FC_REFLECT_TYPENAME( steemit::chain::share_type )

FC_REFLECT_TYPENAME( steemit::chain::account_id_type )
FC_REFLECT_TYPENAME( steemit::chain::witness_id_type )
FC_REFLECT_TYPENAME( steemit::chain::comment_id_type )
FC_REFLECT_TYPENAME( steemit::chain::category_id_type )
FC_REFLECT_TYPENAME( steemit::chain::comment_vote_id_type )
FC_REFLECT_TYPENAME( steemit::chain::dynamic_global_property_id_type )
FC_REFLECT_TYPENAME( steemit::chain::transaction_obj_id_type )
FC_REFLECT_TYPENAME( steemit::chain::block_summary_id_type )
FC_REFLECT_TYPENAME( steemit::chain::account_history_id_type )
FC_REFLECT_TYPENAME( steemit::chain::hardfork_property_id_type )
FC_REFLECT( steemit::chain::void_t, )
