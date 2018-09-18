#pragma once

#include <steem/protocol/asset_symbol.hpp>

namespace steem { namespace chain {
   class nai_generator {
      /** FNV-1a implementation https://en.wikipedia.org/wiki/Fowler-Noll-Vo_hash_function#FNV-1a_hash */
      struct hasher {
         uint32_t operator()( uint8_t byte, uint32_t hash )
         {
            return (byte ^ hash) * prime;
         }
         
         uint32_t operator()( uint32_t four_bytes, uint32_t hash = offset_basis )
         {
            uint8_t* ptr = (uint8_t*) &four_bytes;
            hash = this->operator()(*ptr++, hash);
            hash = this->operator()(*ptr++, hash);
            hash = this->operator()(*ptr++, hash);
            return this->operator()(*ptr,   hash);
         }
         
      private:
         // Recommended by http://isthe.com/chongo/tech/comp/fnv/
         static const uint32_t prime         = 0x01000193; //   16777619
         static const uint32_t offset_basis  = 0x811C9DC5; // 2166136261
      } hash;
   public:
      uint32_t operator( uint32_t seed )()
      {
         uint32_t nai = ( hash( seed ) % ( SMT_MAX_NAI - SMT_MIN_NON_RESERVED_NAI ) ) + SMT_MIN_NON_RESERVED_NAI;
         
         return nai;
      }
   };
}  } // steem::chain
