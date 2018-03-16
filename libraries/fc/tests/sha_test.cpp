
#include <fc/crypto/restartable_sha256.hpp>

#include <cstdint>
#include <iostream>

int main( int argc, char** argv, char** envp )
{
   uint8_t chunk[64];
   for( int i=0; i<64; i++ )
      chunk[i] = 0;

   for( int i=0; i<26; i++ )
   {
      chunk[i] = uint8_t('a'+i);
      fc::restartable_sha256 s;
      s.update( chunk, i+1 );
      s.finish();
      std::cout << "hash:" << s.hexdigest() << std::endl;
   }

   return 0;
}
