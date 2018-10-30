#pragma once

namespace fc {

  /* provides access to the OpenSSL random number generator */
  void rand_bytes(char* buf, int count);
} // namespace fc
