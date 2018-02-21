#pragma once
#include <fc/vector.hpp>
#include <fc/network/ip.hpp>

namespace fc
{
  std::vector<fc::ip::endpoint> resolve( const std::string& host, uint16_t port );
}
