/*
MIT License

Copyright (c) 2017 Vincent Thiery, Steemit, Inc., and Contributors

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef STATSD_CLIENT_HPP
#define STATSD_CLIENT_HPP

#include <cstdlib>
#include <fc/optional.hpp>
#include <string>
#include "UDPSender.hpp"

namespace Statsd {
/*!
 *
 * Statsd client
 *
 * This is the Statsd client, exposing the classic methods
 * and relying on a UDP sender for the actual sending.
 *
 * The sampling frequency can be input, as well as the
 * batching size. The latter is optional and shall not be
 * set if batching is not desired.
 *
 */
class StatsdClient {
public:
    //!@name Constructor and destructor
    //!@{

    //! Constructor
    StatsdClient(const std::string &host,
                 const uint16_t port,
                 const std::string &prefix,
                 const fc::optional<uint64_t> batchsize = fc::optional<uint64_t>()) noexcept;

    //!@}

    //!@name Methods
    //!@{

    //! Sets a configuration { host, port, prefix }
    inline void setConfig(const std::string &host, const uint16_t port, const std::string &prefix) noexcept;

    //! Returns the error message as an optional std::string
    inline fc::optional<std::string> errorMessage() const noexcept;

    //! Increments the key, at a given frequency rate
    inline void increment(const std::string &key, const float frequency = 1.0f) const noexcept;

    //! Increments the key, at a given frequency rate
    inline void decrement(const std::string &key, const float frequency = 1.0f) const noexcept;

    //! Adjusts the specified key by a given delta, at a given frequency rate
    inline void count(const std::string &key, const int delta, const float frequency = 1.0f) const noexcept;

    //! Records a gauge for the key, with a given value, at a given frequency rate
    inline void gauge(const std::string &key, const unsigned int value, const float frequency = 1.0f) const noexcept;

    //! Records a timing for a key, at a given frequency
    inline void timing(const std::string &key, const unsigned int ms, const float frequency = 1.0f) const noexcept;

    //! Send a value for a key, according to its type, at a given frequency
    void send(const std::string &key, const int value, const std::string &type, const float frequency = 1.0f) const
        noexcept;

    //!@}

private:
    //! The prefix to be used for metrics
    std::string m_prefix;

    //! The UDP sender to be used for actual sending
    mutable UDPSender m_sender;
};

StatsdClient::StatsdClient(const std::string &host,
                           const uint16_t port,
                           const std::string &prefix,
                           const fc::optional<uint64_t> batchsize) noexcept
    : m_prefix(prefix), m_sender(host, port, batchsize) {
    // Initialize the randorm generator to be used for sampling
    std::srand(time(nullptr));
}

void StatsdClient::setConfig(const std::string &host, const uint16_t port, const std::string &prefix) noexcept {
    m_prefix = prefix;
    m_sender.setConfig(host, port);
}

fc::optional<std::string> StatsdClient::errorMessage() const noexcept {
    return m_sender.errorMessage();
}

void StatsdClient::decrement(const std::string &key, const float frequency) const noexcept {
    return count(key, -1, frequency);
}

void StatsdClient::increment(const std::string &key, const float frequency) const noexcept {
    return count(key, 1, frequency);
}

void StatsdClient::count(const std::string &key, const int delta, const float frequency) const noexcept {
    return send(key, delta, "c", frequency);
}

void StatsdClient::gauge(const std::string &key, const unsigned int value, const float frequency) const noexcept {
    return send(key, value, "g", frequency);
}

void StatsdClient::timing(const std::string &key, const unsigned int ms, const float frequency) const noexcept {
    return send(key, ms, "ms", frequency);
}

void StatsdClient::send(const std::string &key, const int value, const std::string &type, const float frequency) const
    noexcept {
    const auto isFrequencyOne = [](const float frequency) noexcept {
        constexpr float epsilon{0.0001f};
        return std::fabs(frequency - 1.0f) < epsilon;
    };

    // Test if one should send or not, according to the frequency rate
    if (!isFrequencyOne(frequency)) {
        if (frequency < static_cast<float>(std::rand()) / RAND_MAX) {
            return;
        }
    }

    // Prepare the buffer, with a sampling rate if specified different from 1.0f
    char buffer[256];
    if (isFrequencyOne(frequency)) {
        // Sampling rate is 1.0f, no need to specify it
        std::snprintf(buffer, sizeof(buffer), "%s%s:%d|%s", m_prefix.c_str(), key.c_str(), value, type.c_str());
    } else {
        // Sampling rate is different from 1.0f, hence specify it
        std::snprintf(
            buffer, sizeof(buffer), "%s%s:%d|%s|@%.2f", m_prefix.c_str(), key.c_str(), value, type.c_str(), frequency);
    }

    // Send the message via the UDP sender
    m_sender.send(buffer);
}

}  // namespace Statsd

#endif
