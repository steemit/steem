
#include <fc/bitutil.hpp>
#include <fc/crypto/restartable_sha256.hpp>

namespace fc {

static uint32_t kvalues[] = {
   0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
   0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
   0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
   0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
   0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
   0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
   0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
   0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

#define RIGHT_ROT(a, n) \
   ((uint32_t(a) >> n) | (uint32_t(a) << (32-n)))

restartable_sha256::restartable_sha256()
{
   _h[0] = 0x6a09e667;
   _h[1] = 0xbb67ae85;
   _h[2] = 0x3c6ef372;
   _h[3] = 0xa54ff53a;
   _h[4] = 0x510e527f;
   _h[5] = 0x9b05688c;
   _h[6] = 0x1f83d9ab;
   _h[7] = 0x5be0cd19;
}

void restartable_sha256::update( const void* data, size_t count )
{
   if( count == 0 )
      return;

   const uint8_t* p = (const uint8_t*) data;
   uint8_t* chunk = (uint8_t*) (&_data);

   size_t offset = size_t(_length & 0x3F);
   size_t to_aligned = 0x40 - offset;
   size_t to_copy = std::min( to_aligned, count );
   memcpy( chunk+offset, p, to_copy );
   count -= to_copy;
   _length += uint64_t(to_copy);
   p += to_copy;

   if( (_length & 0x3F) == 0 )
      process_chunk( chunk );
   ((uint64_t*) (&_data))[7] = 0;

   _length += count;
   while( count >= 0x40 )
   {
      process_chunk( p );
      count -= 0x40;
      p += 0x40;
   }

   memcpy( chunk, p, count );

   return;
}

void restartable_sha256::finish()
{
   if( _finished )
      return;

   uint8_t* chunk = (uint8_t*) (&_data);
   size_t offset = size_t(_length & 0x3F);
   chunk[offset++] = 0x80;
   if( offset > 0x38 )
   {
      process_chunk( chunk );
      memset( chunk, 0, 0x38 );
   }
   else
   {
      memset( chunk+offset, 0, 0x38-offset );
   }
   ((uint64_t*) (&_data))[7] = endian_reverse_u64( _length << 3 );
   process_chunk( chunk );
   _h[0] = endian_reverse_u32(_h[0]);
   _h[1] = endian_reverse_u32(_h[1]);
   _h[2] = endian_reverse_u32(_h[2]);
   _h[3] = endian_reverse_u32(_h[3]);
   _h[4] = endian_reverse_u32(_h[4]);
   _h[5] = endian_reverse_u32(_h[5]);
   _h[6] = endian_reverse_u32(_h[6]);
   _h[7] = endian_reverse_u32(_h[7]);
   _finished = true;
}

std::string restartable_sha256::hexdigest()const
{
   static const char hex_digit[] = "0123456789abcdef";

   if( !_finished )
   {
      restartable_sha256 temp = *this;
      temp.finish();
      return temp.hexdigest();
   }

   char buf[65];
   const char* h = (const char *)&_h;
   for( int i=0,j=0; i<32; i++,j+=2 )
   {
      char c = h[i];
      buf[j  ] = hex_digit[ (c >> 4) & 0x0F ];
      buf[j+1] = hex_digit[  c       & 0x0F ];
   }
   buf[64] = '\0';
   return std::string( buf );
}

// chunk must be exactly be 512 bytes
void restartable_sha256::process_chunk( const void* chunk )
{
   uint32_t w[64];
   const uint32_t* chnk = (const uint32_t*) chunk;

   for( int i=0; i<16; i++ )
   {
      w[i] = endian_reverse_u32( chnk[i] );
   }

   for( int i=16; i<64; i++ )
   {
      uint32_t s0 = RIGHT_ROT( w[i-15],  7 ) ^ RIGHT_ROT( w[i-15], 18 ) ^ ( w[i-15] >>  3 );
      uint32_t s1 = RIGHT_ROT( w[i- 2], 17 ) ^ RIGHT_ROT( w[i- 2], 19 ) ^ ( w[i- 2] >> 10 );
      w[i] = w[i-16] + s0 + w[i-7] + s1;
   }

   uint32_t a = _h[0];
   uint32_t b = _h[1];
   uint32_t c = _h[2];
   uint32_t d = _h[3];
   uint32_t e = _h[4];
   uint32_t f = _h[5];
   uint32_t g = _h[6];
   uint32_t h = _h[7];

   for( int i=0; i<64; i++ )
   {
      uint32_t s1 = RIGHT_ROT( e, 6 ) ^ RIGHT_ROT( e, 11 ) ^ RIGHT_ROT( e, 25 );
      uint32_t ch = (e & f) ^ ((~e) & g);
      uint32_t temp1 = h + s1 + ch + kvalues[i] + w[i];
      uint32_t s0 = RIGHT_ROT( a, 2 ) ^ RIGHT_ROT( a, 13 ) ^ RIGHT_ROT( a, 22 );
      uint32_t maj = (a&b) ^ (a&c) ^ (b&c);
      uint32_t temp2 = s0 + maj;

      h = g;
      g = f;
      f = e;
      e = d + temp1;
      d = c;
      c = b;
      b = a;
      a = temp1 + temp2;
   }

   _h[0] += a;
   _h[1] += b;
   _h[2] += c;
   _h[3] += d;
   _h[4] += e;
   _h[5] += f;
   _h[6] += g;
   _h[7] += h;
}

}
