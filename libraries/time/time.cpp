/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <graphene/time/time.hpp>

#include <fc/exception/exception.hpp>
#include <fc/network/ntp.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/thread/scoped_lock.hpp>

#include <atomic>

namespace graphene { namespace time {

static int32_t simulated_time    = 0;
static int32_t adjusted_time_sec = 0;

time_discontinuity_signal_type time_discontinuity_signal;

namespace detail
{
  std::atomic<fc::ntp*> ntp_service(nullptr);
  fc::mutex ntp_service_initialization_mutex;
}

fc::optional<fc::time_point> ntp_time()
{
  fc::ntp* actual_ntp_service = detail::ntp_service.load();
  if (!actual_ntp_service)
  {
    fc::scoped_lock<fc::mutex> lock(detail::ntp_service_initialization_mutex);
    actual_ntp_service = detail::ntp_service.load();
    if (!actual_ntp_service)
    {
      actual_ntp_service = new fc::ntp;
      detail::ntp_service.store(actual_ntp_service);
    }
  }
  return actual_ntp_service->get_time();
}

void shutdown_ntp_time()
{
  fc::ntp* actual_ntp_service = detail::ntp_service.exchange(nullptr);
  delete actual_ntp_service;
}

fc::time_point now()
{
   if( simulated_time )
       return fc::time_point() + fc::seconds( simulated_time + adjusted_time_sec );

   fc::optional<fc::time_point> current_ntp_time = ntp_time();
   if( current_ntp_time.valid() )
      return *current_ntp_time + fc::seconds( adjusted_time_sec );
   else
      return fc::time_point::now() + fc::seconds( adjusted_time_sec );
}

fc::time_point nonblocking_now()
{
  if (simulated_time)
    return fc::time_point() + fc::seconds(simulated_time + adjusted_time_sec);

  fc::ntp* actual_ntp_service = detail::ntp_service.load();
  fc::optional<fc::time_point> current_ntp_time;
  if (actual_ntp_service)
    current_ntp_time = actual_ntp_service->get_time();

  if (current_ntp_time)
    return *current_ntp_time + fc::seconds(adjusted_time_sec);
  else
    return fc::time_point::now() + fc::seconds(adjusted_time_sec);
}

void update_ntp_time()
{
  detail::ntp_service.load()->request_now();
}

fc::microseconds ntp_error()
{
  fc::optional<fc::time_point> current_ntp_time = ntp_time();
  FC_ASSERT( current_ntp_time, "We don't have NTP time!" );
  return *current_ntp_time - fc::time_point::now();
}

void start_simulated_time( const fc::time_point sim_time )
{
   simulated_time = sim_time.sec_since_epoch();
   adjusted_time_sec = 0;
}
void advance_simulated_time_to( const fc::time_point sim_time )
{
   simulated_time = sim_time.sec_since_epoch();
   adjusted_time_sec = 0;
}

void advance_time( int32_t delta_seconds )
{
   adjusted_time_sec += delta_seconds;
   time_discontinuity_signal();
}

} } // graphene::time
