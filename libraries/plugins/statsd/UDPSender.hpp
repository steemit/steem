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

#ifndef UDP_SENDER_HPP
#define UDP_SENDER_HPP

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <cmath>
#include <cstring>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

#include <fc/log/logger.hpp>
#include <fc/optional.hpp>

#ifdef DEFAULT_LOGGER
#pragma push_macro( "DEFAULT_LOGGER" )
#define OLD_DEFAULT
#undef DEFAULT_LOGGER
#endif
#define DEFAULT_LOGGER "statsd"

namespace Statsd {
/*!
 *
 * UDP sender
 *
 * A simple UDP sender handling batching.
 * Its configuration can be changed at runtime for
 * more flexibility.
 *
 */
class UDPSender final {
public:
    //!@name Constructor and destructor
    //!@{

    //! Constructor
    UDPSender(const std::string &host,
              const uint16_t port,
              const fc::optional<uint64_t> batchsize = fc::optional<uint64_t>()) noexcept;

    //! Destructor
    ~UDPSender();

    //!@}

    //!@name Methods
    //!@{

    //! Sets a configuration { host, port }
    inline void setConfig(const std::string &host, const uint16_t port) noexcept;

    //! Send a message
    void send(const std::string &message) noexcept;

    //! Returns the error message as an optional string
    inline fc::optional<std::string> errorMessage() const noexcept;

    //!@}

private:
    // @name Private methods
    // @{

    //! Initialize the sender and returns true when it is initialized
    bool initialize() noexcept;

    //! Send a message to the daemon
    void sendToDaemon(const std::string &message) noexcept;

    //!@}

private:
    // @name State variables
    // @{

    //! Is the sender initialized?
    bool m_isInitialized{false};

    //! Shall we exit?
    bool m_mustExit{false};

    //! Disable udp?
    bool m_disable_udp{false};

    //!@}

    // @name Network info
    // @{

    //! The hostname
    std::string m_host;

    //! The port
    uint16_t m_port;

    //! The structure holding the server
    struct sockaddr_in m_server;

    //! The socket to be used
    int m_socket{-1};

    //!@}

    // @name Batching info
    // @{

    //! Shall the sender use batching?
    bool m_batching{false};

    //! The batching size
    uint64_t m_batchsize;

    //! The queue batching the messages
    mutable std::deque<std::string> m_batchingMessageQueue;

    //! The mutex used for batching
    std::mutex m_batchingMutex;

    //! The thread dedicated to the batching
    std::thread m_batchingThread;

    //!@}

    //! Error message (optional string)
    fc::optional<std::string> m_errorMessage;
};

UDPSender::UDPSender(const std::string &host,
                     const uint16_t port,
                     const fc::optional<uint64_t> batchsize) noexcept
    : m_host(host), m_port(port) {

    m_disable_udp = ( host == "" ) || ( port == 0 );

    // If batching is on, use a dedicated thread to send every now and then
    if (batchsize.valid()) {
        // Thread' sleep duration between batches
        // TODO: allow to input this
        constexpr unsigned int batchingWait{1000U};

        m_batching = true;
        m_batchsize = *batchsize;

        // Define the batching thread
        m_batchingThread = std::thread([this, batchingWait] {
            while (!m_mustExit) {
                std::deque<std::string> stagedMessageQueue;

                std::unique_lock<std::mutex> batchingLock(m_batchingMutex);
                m_batchingMessageQueue.swap(stagedMessageQueue);
                batchingLock.unlock();

                // Flush the queue
                while (!stagedMessageQueue.empty()) {
                    sendToDaemon(stagedMessageQueue.front());
                    stagedMessageQueue.pop_front();
                }

                // Wait before sending the next batch
                std::this_thread::sleep_for(std::chrono::milliseconds(batchingWait));
            }
        });
    }
}

UDPSender::~UDPSender() {
    if (m_batching) {
        m_mustExit = true;
        m_batchingThread.join();
    }

    if (m_socket >= 0) {
        close(m_socket);
        m_socket = -1;
    }
}

void UDPSender::setConfig(const std::string &host, const uint16_t port) noexcept {
    m_host = host;
    m_port = port;

    m_isInitialized = false;

    m_disable_udp = ( host == "" ) || ( port == 0 );

    if (m_socket >= 0) {
        close(m_socket);
    }
    m_socket = -1;
}

void UDPSender::send(const std::string &message) noexcept {
    // If batching is on, accumulate messages in the queue
    if (m_batching) {
        std::unique_lock<std::mutex> batchingLock(m_batchingMutex);
        if (m_batchingMessageQueue.empty() || m_batchingMessageQueue.back().length() > m_batchsize) {
            m_batchingMessageQueue.push_back(message);
        } else {
            std::rbegin(m_batchingMessageQueue)->append("\n").append(message);
        }
    } else {
        sendToDaemon(message);
    }
}

fc::optional<std::string> UDPSender::errorMessage() const noexcept {
    return m_errorMessage;
}

bool UDPSender::initialize() noexcept {
    using namespace std::string_literals;

    if (m_isInitialized) {
        return true;
    }

    if (m_disable_udp) {
       return false;
    }

    // Connect the socket
    m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_socket == -1) {
        m_errorMessage = "Could not create socket, err="s + std::strerror(errno);
        return false;
    }

    std::memset(&m_server, 0, sizeof(m_server));
    m_server.sin_family = AF_INET;
    m_server.sin_port = htons(m_port);

    if (inet_aton(m_host.c_str(), &m_server.sin_addr) == 0) {
        // An error code has been returned by inet_aton

        // Specify the criteria for selecting the socket address structure
        struct addrinfo hints;
        std::memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;

        // Get the address info using the hints
        struct addrinfo *results = nullptr;
        const int ret{getaddrinfo(m_host.c_str(), nullptr, &hints, &results)};
        if (ret != 0) {
            // An error code has been returned by getaddrinfo
            close(m_socket);
            m_socket = -1;
            m_errorMessage = "getaddrinfo failed: error="s + std::to_string(ret) + ", msg=" + gai_strerror(ret);
            return false;
        }

        // Copy the results in m_server
        struct sockaddr_in *host_addr = (struct sockaddr_in *)results->ai_addr;
        std::memcpy(&m_server.sin_addr, &host_addr->sin_addr, sizeof(struct in_addr));

        // Free the memory allocated
        freeaddrinfo(results);
    }

    m_isInitialized = true;
    return true;
}

void UDPSender::sendToDaemon(const std::string &message) noexcept {
    ilog( message );

    // Can't send until the sender is initialized
    if (!initialize()) {
        return;
    }

    // Try sending the message
    const long int ret{
        sendto(m_socket, message.data(), message.size(), 0, (struct sockaddr *)&m_server, sizeof(m_server))};
    if (ret == -1) {
        using namespace std::string_literals;
        m_errorMessage =
            "sendto server failed: host="s + m_host + ":" + std::to_string(m_port) + ", err=" + std::strerror(errno);
    }
}

}  // namespace Statsd

#undef DEFAULT_LOGGER
#ifdef OLD_DEFAULT
#undef DEFAULT_LOGGER
#undef OLD_DEFAULT
#pragma pop_macro( "DEFAULT_LOGGER" )
#endif

#endif
