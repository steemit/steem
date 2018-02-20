#include <boost/test/unit_test.hpp>

#include <fstream>
#include <iostream>
#include <fc/crypto/aes.hpp>
#include <fc/crypto/city.hpp>
#include <fc/exception/exception.hpp>

#include <fc/variant.hpp>

BOOST_AUTO_TEST_SUITE(fc_crypto)

BOOST_AUTO_TEST_CASE(aes_test)
{
    std::ifstream testfile;
    testfile.open("README.md");

    auto key = fc::sha512::hash( "hello", 5 );
    std::stringstream buffer;
    std::string line;
    std::getline( testfile, line );
    while( testfile.good() )
    {
//        std::cout << line << "\n";
        buffer << line << "\n";
        try {
            std::vector<char> data( line.c_str(),line.c_str()+line.size()+1 );
            std::vector<char> crypt = fc::aes_encrypt( key, data );
            std::vector<char> dcrypt = fc::aes_decrypt( key, crypt );
            BOOST_CHECK( data == dcrypt );

//            memset( crypt.data(), 0, crypt.size() );
//            fc::aes_encoder enc;
//            enc.init( fc::sha256::hash((char*)&key,sizeof(key) ), fc::city_hash_crc_128( (char*)&key, sizeof(key) ) );
//            auto len = enc.encode( dcrypt.data(), dcrypt.size(), crypt.data() );
//            BOOST_CHECK_EQUAL( dcrypt.size(), len );
//
//            fc::aes_decoder dec;
//            dec.init( fc::sha256::hash((char*)&key,sizeof(key) ), fc::city_hash_crc_128( (char*)&key, sizeof(key) ) );
//            len = dec.decode( crypt.data(), len, dcrypt.data() );
//            BOOST_CHECK_EQUAL( dcrypt.size(), len );
//            BOOST_CHECK( !memcmp( dcrypt.data(), data.data(), len) );
        }
        catch ( fc::exception& e )
        {
           std::cout<<e.to_detail_string()<<"\n";
        }
        std::getline( testfile, line );
    }

    line = buffer.str();
    std::vector<char> data( line.c_str(),line.c_str()+line.size()+1 );
    std::vector<char> crypt = fc::aes_encrypt( key, data );
    std::vector<char> dcrypt = fc::aes_decrypt( key, crypt );
    BOOST_CHECK( data == dcrypt );

//    memset( crypt.data(), 0, crypt.size() );
//    fc::aes_encoder enc;
//    enc.init( fc::sha256::hash((char*)&key,sizeof(key) ), fc::city_hash_crc_128( (char*)&key, sizeof(key) ) );
//    auto len = enc.encode( dcrypt.data(), dcrypt.size(), crypt.data() );
//    BOOST_CHECK_EQUAL( dcrypt.size(), len );
//
//    fc::aes_decoder dec;
//    dec.init( fc::sha256::hash((char*)&key,sizeof(key) ), fc::city_hash_crc_128( (char*)&key, sizeof(key) ) );
//    len = dec.decode( crypt.data(), len, dcrypt.data() );
//    BOOST_CHECK_EQUAL( dcrypt.size(), len );
//    BOOST_CHECK( !memcmp( dcrypt.data(), data.data(), len) );
}

BOOST_AUTO_TEST_SUITE_END()
