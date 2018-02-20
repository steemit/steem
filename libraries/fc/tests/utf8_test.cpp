#include <boost/test/unit_test.hpp>

#include <fc/utf8.hpp>

using namespace fc;

static const std::string TEST_INVALID_1("\375\271\261\241\201\211\001");
static const std::string TEST_INVALID_2("\371\261\241\201\211\001");
static const std::string TEST_VALID_1("\361\241\201\211\001");
static const std::string TEST_VALID_2("\361\241\201\211");
static const std::string TEST_INVALID_3("\361\241\201");
static const std::string TEST_INVALID_4("\361\241\201\001");
static const std::string TEST_INVALID_5("\361\241");
static const std::string TEST_INVALID_6("\361\241\001");
static const std::string TEST_INVALID_7("\361");
static const std::string TEST_INVALID_8("\361\001");
static const std::string TEST_INVALID_9("\355\244\200");
static const std::string TEST_INVALID_10("\355\244\200\001");
static const std::string TEST_INVALID_11("\340\214\200");
static const std::string TEST_INVALID_12("\340\214\200\001");

BOOST_AUTO_TEST_SUITE(fc)

BOOST_AUTO_TEST_CASE(utf8_test)
{
    std::wstring test(L"\0\001\002");
    test.reserve(65536);
    for (wchar_t c = 0xffff; c >= 0xe000; c--) {
        test.push_back(c);
    }
    for (wchar_t c = 0xd7ff; c > 2; c--) {
        test.push_back(c);
    }
    for (wchar_t c = 1; c < 16; c++) {
        test.push_back((c << 16) | 0xffff);
    }

    std::string storage;
    storage.reserve(257*1024);
    fc::encodeUtf8(test, &storage);
    BOOST_CHECK(fc::is_utf8(storage));

    std::wstring decoded;
    decoded.reserve(65536);
    fc::decodeUtf8(storage, &decoded);
    BOOST_CHECK(test.compare(decoded) == 0);

    BOOST_CHECK(fc::is_utf8(TEST_VALID_1));
    BOOST_CHECK(fc::is_utf8(TEST_VALID_2));
    BOOST_CHECK(!fc::is_utf8(TEST_INVALID_1));
    BOOST_CHECK(!fc::is_utf8(TEST_INVALID_2));
    BOOST_CHECK(!fc::is_utf8(TEST_INVALID_3));
    BOOST_CHECK(!fc::is_utf8(TEST_INVALID_4));
    BOOST_CHECK(!fc::is_utf8(TEST_INVALID_5));
    BOOST_CHECK(!fc::is_utf8(TEST_INVALID_6));
    BOOST_CHECK(!fc::is_utf8(TEST_INVALID_7));
    BOOST_CHECK(!fc::is_utf8(TEST_INVALID_8));
    BOOST_CHECK(!fc::is_utf8(TEST_INVALID_9));
    BOOST_CHECK(!fc::is_utf8(TEST_INVALID_10));
    BOOST_CHECK(!fc::is_utf8(TEST_INVALID_11));
    BOOST_CHECK(!fc::is_utf8(TEST_INVALID_12));

    decoded.clear();
    try {
        fc::decodeUtf8(TEST_INVALID_1, &decoded);
        BOOST_FAIL("expected invalid utf8 exception");
    } catch (const std::exception& e) {
        BOOST_CHECK(!strncmp("Invalid UTF-8", e.what(), 14));
    }
    try {
        fc::decodeUtf8(TEST_INVALID_9, &decoded);
        BOOST_FAIL("expected invalid code point exception");
    } catch (const std::exception& e) {
        BOOST_CHECK(!strncmp("Invalid code point", e.what(), 19));
    }
    try {
        fc::decodeUtf8(TEST_INVALID_11, &decoded);
        BOOST_FAIL("expected invalid utf8 exception");
    } catch (const std::exception& e) {
        BOOST_CHECK(!strncmp("Invalid UTF-8", e.what(), 14));
    }
}

BOOST_AUTO_TEST_SUITE_END()
