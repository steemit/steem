
#include <steemit/time/time.hpp>

#include <fc/optional.hpp>
#include <fc/time.hpp>

#include <fc/exception/exception.hpp>

namespace steemit { namespace time {

fc::time_point now()
{
   return fc::time_point::now();
}

} } // steemit::time
