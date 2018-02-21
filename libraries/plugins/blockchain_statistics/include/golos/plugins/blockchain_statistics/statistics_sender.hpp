#pragma once

#include <vector>
#include <boost/asio/ip/udp.hpp>
#include <fc/uint128_t.hpp>
#include <golos/chain/steem_object_types.hpp>
#include <golos/plugins/blockchain_statistics/bucket_object.hpp>


using namespace golos::chain;

class statistics_sender final {
public:
    statistics_sender() ;
    statistics_sender(uint32_t default_port);

    ~statistics_sender() = default;

    bool can_start();

    // sends a string to all endpoints
    void push(const std::string & str);
    
    // adds address to recipient_endpoint_set.
    void add_address(const std::string & address);

    /// returns statistics recievers endpoints
    std::vector<std::string> get_endpoint_string_vector();

private:
    // Stat sender will send data to all endpoints from recipient_endpoint_set
    std::set<boost::asio::ip::udp::endpoint> recipient_endpoint_set;
    // DefaultPort for asio broadcasting 
    uint32_t default_port;
    void init();
    // bucket_id_type tmp_bucket;
    boost::asio::io_service & ios;
};
