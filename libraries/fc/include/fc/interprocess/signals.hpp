#pragma once
#include <functional>

namespace fc 
{
  /// handler will be called from ASIO thread
  void set_signal_handler( std::function<void(int)> handler, int signal_num ); 
}
