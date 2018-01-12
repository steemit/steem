#pragma once

#include <fc/optional.hpp>
#include <fc/signals.hpp>
#include <fc/time.hpp>

namespace golos {
    namespace time {

        typedef fc::signal<void()> time_discontinuity_signal_type;
        extern time_discontinuity_signal_type time_discontinuity_signal;

        fc::optional<fc::time_point> ntp_time();

        fc::time_point now();

        fc::time_point nonblocking_now(); // identical to now() but guaranteed not to block
        void update_ntp_time();

        fc::microseconds ntp_error();

        void shutdown_ntp_time();

        void start_simulated_time(const fc::time_point sim_time);

        void advance_simulated_time_to(const fc::time_point sim_time);

        void advance_time(int32_t delta_seconds);

    }
} // golos::time
