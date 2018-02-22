#include <golos/plugins/blockchain_statistics/statistics_sender.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/system/error_code.hpp>

#include <fc/exception/exception.hpp>
#include <boost/exception/all.hpp>
#include <fc/io/sstream.hpp>
#include <fc/log/logger.hpp>

#include <string>
#include <cstdio>
#include <vector>
#include <cstdlib>
#include <algorithm>


statistics_sender::statistics_sender() :
    ios( appbase::app().get_io_service() ),
    is_previous_bucket_set(false) {
}

statistics_sender::statistics_sender(uint32_t default_port) :
    default_port(default_port),
    ios( appbase::app().get_io_service() ),
    is_previous_bucket_set(false) {
}

bool statistics_sender::can_start() {
    return !recipient_endpoint_set.empty();
}

void statistics_sender::push(const std::string & str) {
    ios.post ([this, str]() {
        boost::asio::ip::udp::socket socket(this->ios, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 0));
        socket.set_option(boost::asio::socket_base::broadcast(true));

        for (auto endpoint : this->recipient_endpoint_set) {
            socket.send_to(boost::asio::buffer(str), endpoint);
        }
 
    });

}

void statistics_sender::add_address(const std::string & address) {
    // Parsing "IP:PORT". If there is no port, then use Default one from configs.
    try
    {
        boost::asio::ip::udp::endpoint ep;
        boost::asio::ip::address ip;
        uint16_t port;        
        boost::system::error_code ec;

        auto pos = address.find(':');

        if (pos != std::string::npos) {
            ip = boost::asio::ip::address::from_string( address.substr( 0, pos ) , ec);
            port = boost::lexical_cast<uint16_t>( address.substr( pos + 1, address.size() ) );
        }
        else {
            ip = boost::asio::ip::address::from_string( address , ec);
            port = default_port;
        }
        
        if (ip.is_unspecified()) {
            // TODO something with exceptions and logs!
            ep = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), port);
            recipient_endpoint_set.insert(ep);
        }
        else {
            ep = boost::asio::ip::udp::endpoint(ip, port);
            recipient_endpoint_set.insert(ep);
        }
    }
    FC_CAPTURE_AND_LOG(())
}

std::vector<std::string> statistics_sender::get_endpoint_string_vector() {
    std::vector<std::string> ep_vec;
    for (auto x : recipient_endpoint_set) {
        ep_vec.push_back( x.address().to_string() + ":" + std::to_string(x.port()));
    }
    return ep_vec;
}
