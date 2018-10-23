#include <openssl/rand.h>
#include <fc/crypto/openssl.hpp>
#include <fc/exception/exception.hpp>
#include <fc/fwd_impl.hpp>
#include <fc/macros.hpp>

namespace fc {

void rand_bytes(char* buf, int count)
{
  static int init = init_openssl();
  FC_UNUSED(init);

  int result = RAND_bytes((unsigned char*)buf, count);
  if (result != 1)
    FC_THROW("Error calling OpenSSL's RAND_bytes(): ${code}", ("code", (uint32_t)ERR_get_error()));
}

}  // namespace fc
