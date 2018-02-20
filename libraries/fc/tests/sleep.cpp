#include <fc/thread/thread.hpp>
#include <iostream>

int main( int argc, char** argv )
{
  fc::thread test("test");
  auto result = test.async( [=]() {
      while( true )
      {
        fc::usleep( fc::microseconds(1000) );
      }
  });
  char c;
  std::cin >> c;
  test.quit();
  return 0;
}
