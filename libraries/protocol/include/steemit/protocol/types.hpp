#pragma once
#include <steemit/protocol/config.hpp>
#include <fc/fixed_string.hpp>

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

namespace steemit {

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

      typedef fc::ecc::private_key                   private_key_type;
      typedef fc::sha256                             chain_id_type;
      typedef fc::fixed_string<>                     account_name_type;
      //   typedef std::string                            account_name_type;

      struct string_less
      {
         bool operator()( const std::string& a, const std::string& b )const
         {
            return a < b;
         }

         bool operator()( const fc::fixed_string<>& a, const fc::fixed_string<>& b )const
         {
            const char* ap = (const char*)&a;
            const char* ab = (const char*)&b;
            int count = sizeof(a) - 1;
            while( *ap == *ab && count > 0 ) { ++ap; ++ab; --count; }
            return *ap < *ab;
         }

         bool operator()( const fc::fixed_string<>& a, const std::string& b )const
         {
            return std::string( a ) < b;
         }

         bool operator()( const std::string& a, const fc::fixed_string<>& b )const
         {
            return a < std::string( b );
         }
      };

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

      #define STEEMIT_INIT_PUBLIC_KEY (steemit::protocol::public_key_type(STEEMIT_INIT_PUBLIC_KEY_STR))

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
} }  // steemit::protocol

namespace fc
{
    void to_variant( const steemit::protocol::public_key_type& var,  fc::variant& vo );
    void from_variant( const fc::variant& var,  steemit::protocol::public_key_type& vo );
    void to_variant( const steemit::protocol::extended_public_key_type& var, fc::variant& vo );
    void from_variant( const fc::variant& var, steemit::protocol::extended_public_key_type& vo );
    void to_variant( const steemit::protocol::extended_private_key_type& var, fc::variant& vo );
    void from_variant( const fc::variant& var, steemit::protocol::extended_private_key_type& vo );
}

FC_REFLECT( steemit::protocol::public_key_type, (key_data) )
FC_REFLECT( steemit::protocol::public_key_type::binary_key, (data)(check) )
FC_REFLECT( steemit::protocol::extended_public_key_type, (key_data) )
FC_REFLECT( steemit::protocol::extended_public_key_type::binary_key, (check)(data) )
FC_REFLECT( steemit::protocol::extended_private_key_type, (key_data) )
FC_REFLECT( steemit::protocol::extended_private_key_type::binary_key, (check)(data) )

FC_REFLECT_TYPENAME( steemit::protocol::share_type )

FC_REFLECT( steemit::void_t, )
