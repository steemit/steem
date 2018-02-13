#pragma once
#include <steem/protocol/types_fwd.hpp>
#include <steem/protocol/config.hpp>

#include <steem/protocol/asset_symbol.hpp>
#include <steem/protocol/fixed_string.hpp>

#include <fc/container/flat_fwd.hpp>
#include <fc/io/varint.hpp>
#include <fc/io/enum_type.hpp>
#include <fc/crypto/sha224.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/safe.hpp>
#include <fc/optional.hpp>
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

namespace steem {

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

   namespace protocol {

      typedef fc::ecc::private_key        private_key_type;
      typedef fc::sha256                  chain_id_type;
      typedef fixed_string<16>            account_name_type;
      typedef fc::ripemd160               block_id_type;
      typedef fc::ripemd160               checksum_type;
      typedef fc::ripemd160               transaction_id_type;
      typedef fc::sha256                  digest_type;
      typedef fc::ecc::compact_signature  signature_type;
      typedef safe<int64_t>               share_type;
      typedef uint16_t                    weight_type;
      typedef uint32_t                    contribution_id_type;


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

      #define STEEM_INIT_PUBLIC_KEY (steem::protocol::public_key_type(STEEM_INIT_PUBLIC_KEY_STR))

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

      chain_id_type generate_chain_id( const std::string& chain_id_name );

} }  // steem::protocol

namespace fc
{
    void to_variant( const steem::protocol::public_key_type& var,  fc::variant& vo );
    void from_variant( const fc::variant& var,  steem::protocol::public_key_type& vo );
    void to_variant( const steem::protocol::extended_public_key_type& var, fc::variant& vo );
    void from_variant( const fc::variant& var, steem::protocol::extended_public_key_type& vo );
    void to_variant( const steem::protocol::extended_private_key_type& var, fc::variant& vo );
    void from_variant( const fc::variant& var, steem::protocol::extended_private_key_type& vo );
}

FC_REFLECT( steem::protocol::public_key_type, (key_data) )
FC_REFLECT( steem::protocol::public_key_type::binary_key, (data)(check) )
FC_REFLECT( steem::protocol::extended_public_key_type, (key_data) )
FC_REFLECT( steem::protocol::extended_public_key_type::binary_key, (check)(data) )
FC_REFLECT( steem::protocol::extended_private_key_type, (key_data) )
FC_REFLECT( steem::protocol::extended_private_key_type::binary_key, (check)(data) )

FC_REFLECT_TYPENAME( steem::protocol::share_type )

FC_REFLECT( steem::void_t, )
