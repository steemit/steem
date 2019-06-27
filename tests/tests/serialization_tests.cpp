/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <dpn/chain/dpn_fwd.hpp>

#include <dpn/chain/dpn_objects.hpp>
#include <dpn/chain/database.hpp>

#include <dpn/plugins/condenser_api/condenser_api_legacy_asset.hpp>
#include <dpn/plugins/condenser_api/condenser_api_legacy_objects.hpp>

#include <fc/crypto/digest.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/reflect/variant.hpp>

#include "../db_fixture/database_fixture.hpp"

#include <cmath>

using namespace dpn;
using namespace dpn::chain;
using namespace dpn::protocol;

BOOST_FIXTURE_TEST_SUITE( serialization_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( serialization_raw_test )
{
   try {
      ACTORS( (alice)(bob) )
      transfer_operation op;
      op.from = "alice";
      op.to = "bob";
      op.amount = asset(100,DPN_SYMBOL);

      trx.operations.push_back( op );
      auto packed = fc::raw::pack_to_vector( trx );
      signed_transaction unpacked = fc::raw::unpack_from_vector<signed_transaction>(packed);
      unpacked.validate();
      BOOST_CHECK( trx.digest() == unpacked.digest() );
   } catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE( serialization_json_test )
{
   try {
      ACTORS( (alice)(bob) )
      transfer_operation op;
      op.from = "alice";
      op.to = "bob";
      op.amount = asset(100,DPN_SYMBOL);

      fc::variant test(op.amount);
      auto tmp = test.as<asset>();
      BOOST_REQUIRE( tmp == op.amount );

      trx.operations.push_back( op );
      fc::variant packed(trx);
      signed_transaction unpacked = packed.as<signed_transaction>();
      unpacked.validate();
      BOOST_CHECK( trx.digest() == unpacked.digest() );
   } catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE( legacy_asset_test )
{
   try
   {
      using dpn::plugins::condenser_api::legacy_asset;

      BOOST_CHECK_EQUAL( legacy_asset().symbol.decimals(), 3 );
      BOOST_CHECK_EQUAL( legacy_asset().to_string(), "0.000 TESTS" );

      BOOST_TEST_MESSAGE( "Asset Test" );
      legacy_asset dpn = legacy_asset::from_string( "123.456 TESTS" );
      legacy_asset dbd = legacy_asset::from_string( "654.321 TBD" );
      legacy_asset tmp = legacy_asset::from_string( "0.456 TESTS" );
      BOOST_CHECK_EQUAL( tmp.amount.value, 456 );
      tmp = legacy_asset::from_string( "0.056 TESTS" );
      BOOST_CHECK_EQUAL( tmp.amount.value, 56 );

      BOOST_CHECK_EQUAL( dpn.amount.value, 123456 );
      BOOST_CHECK_EQUAL( dpn.symbol.decimals(), 3 );
      BOOST_CHECK_EQUAL( dpn.to_string(), "123.456 TESTS" );
      BOOST_CHECK( dpn.symbol == DPN_SYMBOL );
      BOOST_CHECK_EQUAL( legacy_asset::from_asset( asset( 50, DPN_SYMBOL ) ).to_string(), "0.050 TESTS" );
      BOOST_CHECK_EQUAL( legacy_asset::from_asset( asset(50000, DPN_SYMBOL ) ) .to_string(), "50.000 TESTS" );

      BOOST_CHECK_EQUAL( dbd.amount.value, 654321 );
      BOOST_CHECK_EQUAL( dbd.symbol.decimals(), 3 );
      BOOST_CHECK_EQUAL( dbd.to_string(), "654.321 TBD" );
      BOOST_CHECK( dbd.symbol == DBD_SYMBOL );
      BOOST_CHECK_EQUAL( legacy_asset::from_asset( asset(50, DBD_SYMBOL ) ).to_string(), "0.050 TBD" );
      BOOST_CHECK_EQUAL( legacy_asset::from_asset( asset(50000, DBD_SYMBOL ) ).to_string(), "50.000 TBD" );

      BOOST_CHECK_THROW( legacy_asset::from_string( "1.00000000000000000000 TESTS" ), fc::exception );
      BOOST_CHECK_THROW( legacy_asset::from_string( "1.000TESTS" ), fc::exception );
      BOOST_CHECK_THROW( legacy_asset::from_string( "1. 333 TESTS" ), fc::exception ); // Fails because symbol is '333 TESTS', which is too long
      BOOST_CHECK_THROW( legacy_asset::from_string( "1 .333 TESTS" ), fc::exception );
      BOOST_CHECK_THROW( legacy_asset::from_string( "1. 333 X" ), fc::exception ); // Not a system asset
      BOOST_CHECK_THROW( legacy_asset::from_string( "1 .333 X" ), fc::exception );
      BOOST_CHECK_THROW( legacy_asset::from_string( "1 .333" ), fc::exception );
      BOOST_CHECK_THROW( legacy_asset::from_string( "1 1.1" ), fc::exception );
      BOOST_CHECK_THROW( legacy_asset::from_string( "11111111111111111111111111111111111111111111111 TESTS" ), fc::exception );
      BOOST_CHECK_THROW( legacy_asset::from_string( "1.1.1 TESTS" ), fc::exception );
      BOOST_CHECK_THROW( legacy_asset::from_string( "1.abc TESTS" ), fc::exception );
      BOOST_CHECK_THROW( legacy_asset::from_string( " TESTS" ), fc::exception );
      BOOST_CHECK_THROW( legacy_asset::from_string( "TESTS" ), fc::exception );
      BOOST_CHECK_THROW( legacy_asset::from_string( "1.333" ), fc::exception );
      BOOST_CHECK_THROW( legacy_asset::from_string( "1.333 " ), fc::exception );
      BOOST_CHECK_THROW( legacy_asset::from_string( "" ), fc::exception );
      BOOST_CHECK_THROW( legacy_asset::from_string( " " ), fc::exception );
      BOOST_CHECK_THROW( legacy_asset::from_string( "  " ), fc::exception );
      BOOST_CHECK_THROW( legacy_asset::from_string( "100 TESTS" ), fc::exception ); // Does not match system asset precision
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( asset_test )
{
   try
   {
      fc::string s;

      BOOST_CHECK_EQUAL( asset().symbol.decimals(), 3 );
      BOOST_CHECK_EQUAL( fc::json::to_string( asset() ), "{\"amount\":\"0\",\"precision\":3,\"nai\":\"@@000000021\"}" );

      asset dpn = fc::json::from_string( "{\"amount\":\"123456\",    \"precision\":3, \"nai\":\"@@000000021\"}" ).as< asset >();
      asset dbd =   fc::json::from_string( "{\"amount\":\"654321\",    \"precision\":3, \"nai\":\"@@000000013\"}" ).as< asset >();
      asset vests = fc::json::from_string( "{\"amount\":\"123456789\", \"precision\":6, \"nai\":\"@@000000037\"}" ).as< asset >();
      asset tmp =   fc::json::from_string( "{\"amount\":\"456\",       \"precision\":3, \"nai\":\"@@000000021\"}" ).as< asset >();
      BOOST_CHECK_EQUAL( tmp.amount.value, 456 );
      tmp = fc::json::from_string( "{\"amount\":\"56\", \"precision\":3, \"nai\":\"@@000000021\"}" ).as< asset >();
      BOOST_CHECK_EQUAL( tmp.amount.value, 56 );

      BOOST_CHECK_EQUAL( dpn.amount.value, 123456 );
      BOOST_CHECK_EQUAL( dpn.symbol.decimals(), 3 );
      BOOST_CHECK_EQUAL( fc::json::to_string( dpn ), "{\"amount\":\"123456\",\"precision\":3,\"nai\":\"@@000000021\"}" );
      BOOST_CHECK( dpn.symbol.asset_num == DPN_ASSET_NUM_DPN );
      BOOST_CHECK_EQUAL( fc::json::to_string( asset( 50, DPN_SYMBOL ) ), "{\"amount\":\"50\",\"precision\":3,\"nai\":\"@@000000021\"}" );
      BOOST_CHECK_EQUAL( fc::json::to_string( asset( 50000, DPN_SYMBOL ) ), "{\"amount\":\"50000\",\"precision\":3,\"nai\":\"@@000000021\"}" );

      BOOST_CHECK_EQUAL( dbd.amount.value, 654321 );
      BOOST_CHECK_EQUAL( dbd.symbol.decimals(), 3 );
      BOOST_CHECK_EQUAL( fc::json::to_string( dbd ), "{\"amount\":\"654321\",\"precision\":3,\"nai\":\"@@000000013\"}" );
      BOOST_CHECK( dbd.symbol.asset_num == DPN_ASSET_NUM_DBD );
      BOOST_CHECK_EQUAL( fc::json::to_string( asset( 50, DBD_SYMBOL ) ), "{\"amount\":\"50\",\"precision\":3,\"nai\":\"@@000000013\"}" );
      BOOST_CHECK_EQUAL( fc::json::to_string( asset( 50000, DBD_SYMBOL ) ), "{\"amount\":\"50000\",\"precision\":3,\"nai\":\"@@000000013\"}" );

      BOOST_CHECK_EQUAL( vests.amount.value, 123456789 );
      BOOST_CHECK_EQUAL( vests.symbol.decimals(), 6 );
      BOOST_CHECK_EQUAL( fc::json::to_string( vests ), "{\"amount\":\"123456789\",\"precision\":6,\"nai\":\"@@000000037\"}" );
      BOOST_CHECK( vests.symbol.asset_num == DPN_ASSET_NUM_VESTS );
      BOOST_CHECK_EQUAL( fc::json::to_string( asset( 50, VESTS_SYMBOL ) ), "{\"amount\":\"50\",\"precision\":6,\"nai\":\"@@000000037\"}" );
      BOOST_CHECK_EQUAL( fc::json::to_string( asset( 50000, VESTS_SYMBOL ) ), "{\"amount\":\"50000\",\"precision\":6,\"nai\":\"@@000000037\"}" );

      // amount overflow
      BOOST_CHECK_THROW( fc::json::from_string( "{\"amount\":\"9223372036854775808\",\"precision\":3,\"nai\":\"@@000000021\"}" ).as< asset >(), fc::exception );
      // amount underflow
      BOOST_CHECK_THROW( fc::json::from_string( "{\"amount\":\"-1\",\"precision\":3,\"nai\":\"@@000000021\"}" ).as< asset >(), fc::exception );

      // precision overflow
      BOOST_CHECK_THROW( fc::json::from_string( "{\"amount\":\"10\",\"precision\":256,\"nai\":\"@@000000021\"}" ).as< asset >(), fc::exception );
      // precision underflow
      BOOST_CHECK_THROW( fc::json::from_string( "{\"amount\":\"10\",\"precision\":-1,\"nai\":\"@@000000021\"}" ).as< asset >(), fc::exception );

      // Check wrong size tuple
      BOOST_CHECK_THROW( fc::json::from_string( "{\"amount\":\"0\",3]" ).as< asset >(), fc::exception );
      BOOST_CHECK_THROW( fc::json::from_string( "{\"amount\":\"0\",\"precision\":3,\"nai\":\"@@000000021\",1}" ).as< asset >(), fc::exception );

      // Check non-numeric characters in amount
      BOOST_CHECK_THROW( fc::json::from_string( "{\"amount\":\"foobar\",\"precision\":3,\"nai\":\"@@000000021\"}" ).as< asset >(), fc::exception );
      BOOST_CHECK_THROW( fc::json::from_string( "{\"amount\":\"10a\",\"precision\":3,\"nai\":\"@@000000021\"}" ).as< asset >(), fc::exception );
      BOOST_CHECK_THROW( fc::json::from_string( "{\"amount\":\"10a00\",\"precision\":3,\"nai\":\"@@000000021\"}" ).as< asset >(), fc::exception );

      // Check hex value
      BOOST_CHECK_THROW( fc::json::from_string( "{\"amount\":\"0x8000\",\"precision\":3,\"nai\":\"@@000000021\"}" ).as< asset >(), fc::exception );

      // Check octal value
      BOOST_CHECK_EQUAL( fc::json::from_string( "{\"amount\":\"08000\",\"precision\":3,\"nai\":\"@@000000021\"}" ).as< asset >().amount.value, 8000 );
   }
   FC_LOG_AND_RETHROW()
}

template< typename T >
std::string hex_bytes( const T& obj )
{
   std::vector<char> data = fc::raw::pack_to_vector( obj );
   std::ostringstream ss;
   static const char hexdigits[] = "0123456789abcdef";

   for( char c : data )
   {
      ss << hexdigits[((c >> 4) & 0x0F)] << hexdigits[c & 0x0F] << ' ';
   }
   return ss.str();
}

void old_pack_symbol(vector<char>& v, asset_symbol_type sym)
{
   if( sym == DPN_SYMBOL )
   {
      v.push_back('\x03'); v.push_back('T' ); v.push_back('E' ); v.push_back('S' );
      v.push_back('T'   ); v.push_back('S' ); v.push_back('\0'); v.push_back('\0');
      // 03 54 45 53 54 53 00 00
   }
   else if( sym == DBD_SYMBOL )
   {
      v.push_back('\x03'); v.push_back('T' ); v.push_back('B' ); v.push_back('D' );
      v.push_back('\0'  ); v.push_back('\0'); v.push_back('\0'); v.push_back('\0');
      // 03 54 42 44 00 00 00 00
   }
   else if( sym == VESTS_SYMBOL )
   {
      v.push_back('\x06'); v.push_back('V' ); v.push_back('E' ); v.push_back('S' );
      v.push_back('T'   ); v.push_back('S' ); v.push_back('\0'); v.push_back('\0');
      // 06 56 45 53 54 53 00 00
   }
   else
   {
      FC_ASSERT( false, "This method cannot serialize this symbol" );
   }
   return;
}

void old_pack_asset( vector<char>& v, const asset& a )
{
   uint64_t x = uint64_t( a.amount.value );
   v.push_back( char( x & 0xFF ) );   x >>= 8;
   v.push_back( char( x & 0xFF ) );   x >>= 8;
   v.push_back( char( x & 0xFF ) );   x >>= 8;
   v.push_back( char( x & 0xFF ) );   x >>= 8;
   v.push_back( char( x & 0xFF ) );   x >>= 8;
   v.push_back( char( x & 0xFF ) );   x >>= 8;
   v.push_back( char( x & 0xFF ) );   x >>= 8;
   v.push_back( char( x & 0xFF ) );
   old_pack_symbol( v, a.symbol );
   return;
}

std::string old_json_asset( const asset& a )
{
   size_t decimal_places = 0;
   if( (a.symbol == DPN_SYMBOL) || (a.symbol == DBD_SYMBOL) )
      decimal_places = 3;
   else if( a.symbol == VESTS_SYMBOL )
      decimal_places = 6;
   std::ostringstream ss;
   ss << std::setfill('0') << std::setw(decimal_places+1) << a.amount.value;
   std::string result = ss.str();
   result.insert( result.length() - decimal_places, 1, '.' );
   if( a.symbol == DPN_SYMBOL )
      result += " TESTS";
   else if( a.symbol == DBD_SYMBOL )
      result += " TBD";
   else if( a.symbol == VESTS_SYMBOL )
      result += " VESTS";
   result.insert(0, 1, '"');
   result += '"';
   return result;
}

BOOST_AUTO_TEST_CASE( asset_raw_test )
{
   try
   {
      BOOST_CHECK( DBD_SYMBOL < DPN_SYMBOL );
      BOOST_CHECK( DPN_SYMBOL < VESTS_SYMBOL );

      // get a bunch of random bits
      fc::sha256 h = fc::sha256::hash("");

      std::vector< share_type > amounts;

      for( int i=0; i<64; i++ )
      {
         uint64_t s = (uint64_t(1) << i);
         uint64_t x = (h._hash[0] & (s-1)) | s;
         if( x >= DPN_MAX_SHARE_SUPPLY )
            break;
         amounts.push_back( share_type( x ) );
      }
      // ilog( "h0:${h0}", ("h0", h._hash[0]) );

/*      asset dpn = asset::from_string( "0.001 TESTS" );
#define VESTS_SYMBOL  (uint64_t(6) | (uint64_t('V') << 8) | (uint64_t('E') << 16) | (uint64_t('S') << 24) | (uint64_t('T') << 32) | (uint64_t('S') << 40)) ///< VESTS with 6 digits of precision
#define DPN_SYMBOL  (uint64_t(3) | (uint64_t('T') << 8) | (uint64_t('E') << 16) | (uint64_t('S') << 24) | (uint64_t('T') << 32) | (uint64_t('S') << 40)) ///< DPN with 3 digits of precision
#define DBD_SYMBOL    (uint64_t(3) | (uint64_t('T') << 8) | (uint64_t('B') << 16) | (uint64_t('D') << 24) ) ///< Test Backed Dollars with 3 digits of precision
*/
      std::vector< asset_symbol_type > symbols;

      symbols.push_back( DPN_SYMBOL );
      symbols.push_back( DBD_SYMBOL   );
      symbols.push_back( VESTS_SYMBOL );

      for( const share_type& amount : amounts )
      {
         for( const asset_symbol_type& symbol : symbols )
         {
            // check raw::pack() works
            asset a = asset( amount, symbol );
            vector<char> v_old;
            old_pack_asset( v_old, a );
            vector<char> v_cur = fc::raw::pack_to_vector(a);
            // ilog( "${a} : ${d}", ("a", a)("d", hex_bytes( v_old )) );
            // ilog( "${a} : ${d}", ("a", a)("d", hex_bytes( v_cur )) );
            BOOST_CHECK( v_cur == v_old );

            // check raw::unpack() works
            std::istringstream ss( string(v_cur.begin(), v_cur.end()) );
            asset a2;
            fc::raw::unpack( ss, a2 );
            BOOST_CHECK( a == a2 );

            // check conversion to JSON works
            //std::string json_old = old_json_asset(a);
            //std::string json_cur = fc::json::to_string(a);
            // ilog( "json_old: ${j}", ("j", json_old) );
            // ilog( "json_cur: ${j}", ("j", json_cur) );
            //BOOST_CHECK( json_cur == json_old );

            // check JSON serialization is symmetric
            std::string json_cur = fc::json::to_string(a);
            a2 = fc::json::from_string(json_cur).as< asset >();
            BOOST_CHECK( a == a2 );
         }
      }
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( json_tests )
{
   try {
   auto var = fc::json::variants_from_string( "10.6 " );
   var = fc::json::variants_from_string( "10.5" );
   } catch ( const fc::exception& e )
   {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE( extended_private_key_type_test )
{
   try
   {
     fc::ecc::extended_private_key key = fc::ecc::extended_private_key( fc::ecc::private_key::generate(),
                                                                       fc::sha256(),
                                                                       0, 0, 0 );
      extended_private_key_type type = extended_private_key_type( key );
      std::string packed = std::string( type );
      extended_private_key_type unpacked = extended_private_key_type( packed );
      BOOST_CHECK( type == unpacked );
   } catch ( const fc::exception& e )
   {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE( extended_public_key_type_test )
{
   try
   {
      fc::ecc::extended_public_key key = fc::ecc::extended_public_key( fc::ecc::private_key::generate().get_public_key(),
                                                                       fc::sha256(),
                                                                       0, 0, 0 );
      extended_public_key_type type = extended_public_key_type( key );
      std::string packed = std::string( type );
      extended_public_key_type unpacked = extended_public_key_type( packed );
      BOOST_CHECK( type == unpacked );
   } catch ( const fc::exception& e )
   {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE( version_test )
{
   try
   {
      BOOST_REQUIRE_EQUAL( string( version( 1, 2, 3) ), "1.2.3" );

      fc::variant ver_str( "3.0.0" );
      version ver;
      fc::from_variant( ver_str, ver );
      BOOST_REQUIRE( ver == version( 3, 0 , 0 ) );

      ver_str = fc::variant( "0.0.0" );
      fc::from_variant( ver_str, ver );
      BOOST_REQUIRE( ver == version() );

      ver_str = fc::variant( "1.0.1" );
      fc::from_variant( ver_str, ver );
      BOOST_REQUIRE( ver == version( 1, 0, 1 ) );

      ver_str = fc::variant( "1_0_1" );
      fc::from_variant( ver_str, ver );
      BOOST_REQUIRE( ver == version( 1, 0, 1 ) );

      ver_str = fc::variant( "12.34.56" );
      fc::from_variant( ver_str, ver );
      BOOST_REQUIRE( ver == version( 12, 34, 56 ) );

      ver_str = fc::variant( "256.0.0" );
      DPN_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::exception );

      ver_str = fc::variant( "0.256.0" );
      DPN_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::exception );

      ver_str = fc::variant( "0.0.65536" );
      DPN_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::exception );

      ver_str = fc::variant( "1.0" );
      DPN_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::exception );

      ver_str = fc::variant( "1.0.0.1" );
      DPN_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::exception );
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( hardfork_version_test )
{
   try
   {
      BOOST_REQUIRE_EQUAL( string( hardfork_version( 1, 2 ) ), "1.2.0" );

      fc::variant ver_str( "3.0.0" );
      hardfork_version ver;
      fc::from_variant( ver_str, ver );
      BOOST_REQUIRE( ver == hardfork_version( 3, 0 ) );

      ver_str = fc::variant( "0.0.0" );
      fc::from_variant( ver_str, ver );
      BOOST_REQUIRE( ver == hardfork_version() );

      ver_str = fc::variant( "1.0.0" );
      fc::from_variant( ver_str, ver );
      BOOST_REQUIRE( ver == hardfork_version( 1, 0 ) );

      ver_str = fc::variant( "1_0_0" );
      fc::from_variant( ver_str, ver );
      BOOST_REQUIRE( ver == hardfork_version( 1, 0 ) );

      ver_str = fc::variant( "12.34.00" );
      fc::from_variant( ver_str, ver );
      BOOST_REQUIRE( ver == hardfork_version( 12, 34 ) );

      ver_str = fc::variant( "256.0.0" );
      DPN_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::exception );

      ver_str = fc::variant( "0.256.0" );
      DPN_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::exception );

      ver_str = fc::variant( "0.0.1" );
      fc::from_variant( ver_str, ver );
      BOOST_REQUIRE( ver == hardfork_version( 0, 0 ) );

      ver_str = fc::variant( "1.0" );
      DPN_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::exception );

      ver_str = fc::variant( "1.0.0.1" );
      DPN_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::exception );
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( min_block_size )
{
   signed_block b;
   while( b.witness.length() < DPN_MIN_ACCOUNT_NAME_LENGTH )
      b.witness += 'a';
   size_t min_size = fc::raw::pack_size( b );
   BOOST_CHECK( min_size == DPN_MIN_BLOCK_SIZE );
}

BOOST_AUTO_TEST_CASE( legacy_signed_transaction )
{
   using dpn::plugins::condenser_api::legacy_signed_transaction;

   signed_transaction tx;
   vote_operation op;
   op.voter = "alice";
   op.author = "bob";
   op.permlink = "foobar";
   op.weight = DPN_100_PERCENT;
   tx.ref_block_num = 4000;
   tx.ref_block_prefix = 4000000000;
   tx.expiration = fc::time_point_sec( 1514764800 );
   tx.operations.push_back( op );

   signed_transaction tx2 = signed_transaction( fc::json::from_string( "{\"ref_block_num\":4000,\"ref_block_prefix\":4000000000,\"expiration\":\"2018-01-01T00:00:00\",\"operations\":[[\"vote\",{\"voter\":\"alice\",\"author\":\"bob\",\"permlink\":\"foobar\",\"weight\":10000}]],\"extensions\":[],\"signatures\":[\"\"]}" ).as< legacy_signed_transaction >() );

   BOOST_REQUIRE( tx.id() == tx2.id() );
}

BOOST_AUTO_TEST_CASE( static_variant_json_test )
{
   try
   {
      typedef static_variant<
                  transfer_operation,
                  comment_operation
               > test_operation;

      test_operation op;
      op = transfer_operation();

      auto json_str = fc::json::to_string( op );
      BOOST_CHECK_EQUAL( json_str, "{\"type\":\"transfer_operation\",\"value\":{\"from\":\"\",\"to\":\"\",\"amount\":{\"amount\":\"0\",\"precision\":3,\"nai\":\"@@000000021\"},\"memo\":\"\"}}" );

      json_str = "{\"type\":\"transfer_operation\",\"value\":{\"from\":\"foo\",\"to\":\"bar\",\"amount\":{\"amount\":\"1000\",\"precision\":3,\"nai\":\"@@000000021\"},\"memo\":\"\"}}";
      from_variant( fc::json::from_string( json_str ), op );
      BOOST_CHECK_EQUAL( op.which(), 0 );

      transfer_operation t = op.get< transfer_operation >();
      BOOST_CHECK( t.from == "foo" );
      BOOST_CHECK( t.to == "bar" );
      BOOST_CHECK( t.amount == asset( 1000, DPN_SYMBOL ) );
      BOOST_CHECK( t.memo == "" );

      json_str = "{\"type\":1,\"value\":{\"parent_author\":\"foo\",\"parent_permlink\":\"bar\",\"author\":\"foo1\",\"permlink\":\"bar1\",\"title\":\"\",\"body\":\"\",\"json_metadata\":\"\"}}";

      from_variant( fc::json::from_string( json_str ), op );
      BOOST_CHECK_EQUAL( op.which(), 1 );

      comment_operation c = op.get< comment_operation >();
      BOOST_CHECK( c.parent_author == "foo" );
      BOOST_CHECK( c.parent_permlink == "bar" );
      BOOST_CHECK( c.author == "foo1" );
      BOOST_CHECK( c.permlink == "bar1" );
      BOOST_CHECK( c.title == "" );
      BOOST_CHECK( c.body == "" );
      BOOST_CHECK( c.json_metadata == "" );

      json_str = "{\"type\":\"not_a_type\",\"value\":{\"from\":\"foo\",\"to\":\"bar\",\"amount\":{\"amount\":\"1000\",\"precision\":3,\"nai\":\"@@000000021\"},\"memo\":\"\"}}";
      DPN_REQUIRE_THROW( from_variant( fc::json::from_string( json_str ), op ), fc::assert_exception );
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( legacy_operation_test )
{
   try
   {
      auto v = fc::json::from_string( "{\"ref_block_num\": 41047, \"ref_block_prefix\": 4089157749, \"expiration\": \"2018-03-28T19:05:47\", \"operations\": [[\"witness_update\", {\"owner\": \"test\", \"url\": \"foo\", \"block_signing_key\": \"TST1111111111111111111111111111111114T1Anm\", \"props\": {\"account_creation_fee\": \"0.500 TESTS\", \"maximum_block_size\": 65536, \"dbd_interest_rate\": 0}, \"fee\": \"0.000 TESTS\"}]], \"extensions\": [], \"signatures\": [\"1f1b2d47427a46513777ae9ed032b761b504423b18350e673beb991a1b52d2381c26c36368f9cc4a72c9de3cc16bca83b269c2ea1960e28647caf151e17c35bf3f\"]}" );
      auto ls = v.as< dpn::plugins::condenser_api::legacy_signed_transaction >();
      // not throwing an error here is success
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( asset_symbol_type_test )
{
   try
   {
      uint32_t asset_num = 10000000 << DPN_NAI_SHIFT; // Shift NAI value in to position
      asset_num |= SMT_ASSET_NUM_CONTROL_MASK;          // Flip the control bit
      asset_num |= 3;                                   // Add the precision

      auto symbol = asset_symbol_type::from_asset_num( asset_num );

      BOOST_REQUIRE( symbol == asset_symbol_type::from_nai( 100000006, 3 ) );
      BOOST_REQUIRE( symbol == asset_symbol_type::from_nai_string( "@@100000006", 3 ) );
      BOOST_REQUIRE( asset_num == asset_symbol_type::asset_num_from_nai( 100000006, 3 ) );
      BOOST_REQUIRE( symbol.to_nai_string() == "@@100000006" );
      BOOST_REQUIRE( symbol.to_nai() == 100000006 );
      BOOST_REQUIRE( symbol.asset_num == asset_num );
      BOOST_REQUIRE( symbol.space() == asset_symbol_type::asset_symbol_space::smt_nai_space );
      BOOST_REQUIRE( symbol.get_paired_symbol() == asset_symbol_type::from_asset_num( asset_num ^ SMT_ASSET_NUM_VESTING_MASK ) );
      BOOST_REQUIRE( asset_symbol_type::from_nai( symbol.to_nai(), 3 ) == symbol );

      asset_symbol_type dpn = asset_symbol_type::from_asset_num( DPN_ASSET_NUM_DPN );
      asset_symbol_type dbd = asset_symbol_type::from_asset_num( DPN_ASSET_NUM_DBD );
      asset_symbol_type vests = asset_symbol_type::from_asset_num( DPN_ASSET_NUM_VESTS );

      BOOST_REQUIRE( dpn.space() == asset_symbol_type::asset_symbol_space::legacy_space );
      BOOST_REQUIRE( dbd.space() == asset_symbol_type::asset_symbol_space::legacy_space );
      BOOST_REQUIRE( vests.space() == asset_symbol_type::asset_symbol_space::legacy_space );

      BOOST_REQUIRE( asset_symbol_type::from_nai( dpn.to_nai(), DPN_PRECISION_DPN ) == dpn );
      BOOST_REQUIRE( asset_symbol_type::from_nai( dbd.to_nai(), DPN_PRECISION_DBD ) == dbd );
      BOOST_REQUIRE( asset_symbol_type::from_nai( vests.to_nai(), DPN_PRECISION_VESTS ) == vests );

      DPN_REQUIRE_THROW( asset_symbol_type::from_nai_string( "@@100000006", DPN_ASSET_MAX_DECIMALS + 1 ), fc::assert_exception ); // More than max decimals
      DPN_REQUIRE_THROW( asset_symbol_type::from_nai_string( "@0100000006", 3 ), fc::assert_exception );                            // Invalid NAI prefix
      DPN_REQUIRE_THROW( asset_symbol_type::from_nai_string( "@@00000006", 3 ), fc::assert_exception );                             // Length too short
      DPN_REQUIRE_THROW( asset_symbol_type::from_nai_string( "@@0100000006", 3 ), fc::assert_exception );                           // Length too long
      DPN_REQUIRE_THROW( asset_symbol_type::from_nai_string( "@@invalid00", 3 ), fc::exception );                                   // Boost lexical bad cast
      DPN_REQUIRE_THROW( asset_symbol_type::from_nai_string( nullptr, 3 ), fc::exception );                                         // Null pointer
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( unpack_clear_test )
{
   try
   {
      std::stringstream ss1;
      std::stringstream ss2;

      signed_block b1;

      for ( int i = 0; i < 10; i++ )
      {
         signed_transaction tx;

         vote_operation op;
         op.voter = "alice";
         op.author = "bob";
         op.permlink = "permlink1";
         op.weight = DPN_100_PERCENT;
         tx.operations.push_back( op );

         vote_operation op2;
         op2.voter = "charlie";
         op2.author = "sam";
         op2.permlink = "permlink2";
         op2.weight = DPN_100_PERCENT;
         tx.operations.push_back( op2 );

         tx.ref_block_num = 1000;
         tx.ref_block_prefix = 1000000000;
         tx.expiration = fc::time_point_sec( 1514764800 + i );

         b1.transactions.push_back( tx );
      }

      signed_block b2;

      for ( int i = 0; i < 20; i++ )
      {
         signed_transaction tx;
         vote_operation op;
         op.voter = "dave";
         op.author = "greg";
         op.permlink = "foobar";
         op.weight = DPN_100_PERCENT/2;
         tx.ref_block_num = 4000;
         tx.ref_block_prefix = 4000000000;
         tx.expiration = fc::time_point_sec( 1714764800 + i );
         tx.operations.push_back( op );

         b2.transactions.push_back( tx );
      }

      fc::raw::pack( ss2, b2 );
      fc::raw::pack( ss1, b1 );

      signed_block unpacked_block;
      fc::raw::unpack( ss2, unpacked_block );

      // This operation should completely overwrite signed block 'b2'
      fc::raw::unpack( ss1, unpacked_block );

      BOOST_REQUIRE( b1.transactions.size() == unpacked_block.transactions.size() );
      for ( size_t i = 0; i < unpacked_block.transactions.size(); i++ )
      {
         signed_transaction tx = unpacked_block.transactions[ i ];
         BOOST_REQUIRE( unpacked_block.transactions[ i ].operations.size() == b1.transactions[ i ].operations.size() );

         vote_operation op = tx.operations[ 0 ].get< vote_operation >();
         BOOST_REQUIRE( op.voter == "alice" );
         BOOST_REQUIRE( op.author == "bob" );
         BOOST_REQUIRE( op.permlink == "permlink1" );
         BOOST_REQUIRE( op.weight == DPN_100_PERCENT );

         vote_operation op2 = tx.operations[ 1 ].get< vote_operation >();
         BOOST_REQUIRE( op2.voter == "charlie" );
         BOOST_REQUIRE( op2.author == "sam" );
         BOOST_REQUIRE( op2.permlink == "permlink2" );
         BOOST_REQUIRE( op2.weight == DPN_100_PERCENT );

         BOOST_REQUIRE( tx.ref_block_num == 1000 );
         BOOST_REQUIRE( tx.ref_block_prefix == 1000000000 );
         BOOST_REQUIRE( tx.expiration == fc::time_point_sec( 1514764800 + i ) );
      }
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( unpack_recursion_test )
{
   try
   {
      std::stringstream ss;
      int recursion_level = 100000;
      uint64_t allocation_per_level = 500000;

      for ( int i = 0; i < recursion_level; i++ )
      {
         fc::raw::pack( ss, unsigned_int( allocation_per_level ) );
         fc::raw::pack( ss, static_cast< uint8_t >( variant::array_type ) );
      }

      std::vector< fc::variant > v;
      DPN_REQUIRE_THROW( fc::raw::unpack( ss, v ), fc::assert_exception );
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_SUITE_END()
#endif
