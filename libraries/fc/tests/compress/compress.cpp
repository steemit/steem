#include <boost/test/unit_test.hpp>

#include <fstream>
#include <iostream>
#include <fc/compress/smaz.hpp>
#include <fc/compress/zlib.hpp>
#include <fc/exception/exception.hpp>

BOOST_AUTO_TEST_SUITE(compress)

BOOST_AUTO_TEST_CASE(smaz_test)
{
    std::ifstream testfile;
    testfile.open("README.md");

    std::stringstream buffer;
    std::string line;
    std::getline( testfile, line );
    while( testfile.good() )
    {
        buffer << line << "\n";
        try {
            std::string compressed = fc::smaz_compress( line );
            std::string decomp = fc::smaz_decompress( compressed );
            BOOST_CHECK_EQUAL( decomp, line );
        }
        catch ( fc::exception& e )
        {
           std::cout<<e.to_detail_string()<<"\n";
        }

        std::getline( testfile, line );
    }

    line = buffer.str();
    std::string compressed = fc::smaz_compress( line );
    std::string decomp = fc::smaz_decompress( compressed );
    BOOST_CHECK_EQUAL( decomp, line );
}


extern "C" {

enum
{
  TINFL_FLAG_PARSE_ZLIB_HEADER = 1,
  TINFL_FLAG_HAS_MORE_INPUT = 2,
  TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF = 4,
  TINFL_FLAG_COMPUTE_ADLER32 = 8
};

extern char* tinfl_decompress_mem_to_heap(const void *pSrc_buf, size_t src_buf_len, size_t *pOut_len, int flags);

}

static std::string zlib_decompress( const std::string compressed )
{
    size_t decomp_len;
    char* decomp = tinfl_decompress_mem_to_heap( compressed.c_str(), compressed.length(), &decomp_len, TINFL_FLAG_PARSE_ZLIB_HEADER );
    std::string result( decomp, decomp_len );
    free( decomp );
    return result;
}

BOOST_AUTO_TEST_CASE(zlib_test)
{
    std::ifstream testfile;
    testfile.open("README.md");

    std::stringstream buffer;
    std::string line;
    std::getline( testfile, line );
    while( testfile.good() )
    {
        buffer << line << "\n";
        std::string compressed = fc::zlib_compress( line );
        std::string decomp = zlib_decompress( compressed );
        BOOST_CHECK_EQUAL( decomp, line );

        std::getline( testfile, line );
    }

    line = buffer.str();
    std::string compressed = fc::zlib_compress( line );
    std::string decomp = zlib_decompress( compressed );
    BOOST_CHECK_EQUAL( decomp, line );
}

BOOST_AUTO_TEST_SUITE_END()
