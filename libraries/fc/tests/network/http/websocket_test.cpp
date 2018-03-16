#include <boost/test/unit_test.hpp>

#include <fc/network/http/websocket.hpp>

#include <iostream>

BOOST_AUTO_TEST_SUITE(fc_network)

BOOST_AUTO_TEST_CASE(websocket_test)
{ 
    fc::http::websocket_client client;
    fc::http::websocket_connection_ptr s_conn, c_conn;
    {
        fc::http::websocket_server server;
        server.on_connection([&]( const fc::http::websocket_connection_ptr& c ){
                s_conn = c;
                c->on_message_handler([&](const std::string& s){
                    c->send_message("echo: " + s);
                });
            });

        server.listen( 8090 );
        server.start_accept();

        std::string echo;
        c_conn = client.connect( "ws://localhost:8090" );
        c_conn->on_message_handler([&](const std::string& s){
                    echo = s;
                });
        c_conn->send_message( "hello world" );
        fc::usleep( fc::seconds(1) );
        BOOST_CHECK_EQUAL("echo: hello world", echo);
        c_conn->send_message( "again" );
        fc::usleep( fc::seconds(1) );
        BOOST_CHECK_EQUAL("echo: again", echo);

        s_conn->close(0, "test");
        fc::usleep( fc::seconds(1) );
        try {
            c_conn->send_message( "again" );
            BOOST_FAIL("expected assertion failure");
        } catch (const fc::assert_exception& e) {
            //std::cerr << e.to_string() << "\n";
        }

        c_conn = client.connect( "ws://localhost:8090" );
        c_conn->on_message_handler([&](const std::string& s){
                    echo = s;
                });
        c_conn->send_message( "hello world" );
        fc::usleep( fc::seconds(1) );
        BOOST_CHECK_EQUAL("echo: hello world", echo);
    }

    try {
        c_conn->send_message( "again" );
        BOOST_FAIL("expected assertion failure");
    } catch (const fc::assert_exception& e) {
        std::cerr << e.to_string() << "\n";
    }

    try {
        c_conn = client.connect( "ws://localhost:8090" );
        BOOST_FAIL("expected assertion failure");
    } catch (const fc::assert_exception& e) {
        std::cerr << e.to_string() << "\n";
    }
}

BOOST_AUTO_TEST_SUITE_END()
