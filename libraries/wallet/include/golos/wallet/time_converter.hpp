#pragma once

#include <fc/time.hpp>

namespace golos { namespace wallet {

class time_converter {
private:
    time_point_sec tps;
public: 
    time_converter(const std::string &s, const time_point_sec &start_tps, const time_point_sec &default_tps) {
        if (s.empty()) {
            tps = default_tps;
            return;
        }
        if (s.at(0) == '+') {
            tps = start_tps;
            tps += std::stoi(s.substr(1));
            return;
        }
        tps = time_point_sec::from_iso_string(s);
        if (tps.sec_since_epoch() == 0) {
            tps = default_tps;
            return;
        }
    }

    time_point_sec time() {
        return tps;
    }
};

}
} // golos::wallet