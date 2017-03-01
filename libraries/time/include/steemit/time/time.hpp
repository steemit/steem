#pragma once

#include <fc/time.hpp>

namespace steemit { namespace time {

void set_ntp_enabled( bool enabled );
fc::time_point now();

} } // steemit::time
