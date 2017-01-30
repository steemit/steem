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

#include <boost/test/unit_test.hpp>

#include <steemit/chain/database.hpp>
#include <steemit/protocol/protocol.hpp>

#include <steemit/protocol/steem_operations.hpp>

#include <fc/crypto/digest.hpp>
#include <fc/crypto/hex.hpp>
#include "../common/database_fixture.hpp"

#include <algorithm>
#include <random>

using namespace steemit;
using namespace steemit::chain;
using namespace steemit::protocol;

BOOST_FIXTURE_TEST_SUITE( basic_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( parse_size_test )
{
   BOOST_CHECK_THROW( fc::parse_size( "" ), fc::parse_error_exception );
   BOOST_CHECK_THROW( fc::parse_size( "k" ), fc::parse_error_exception );

   BOOST_CHECK_EQUAL( fc::parse_size( "0" ), 0 );
   BOOST_CHECK_EQUAL( fc::parse_size( "1" ), 1 );
   BOOST_CHECK_EQUAL( fc::parse_size( "2" ), 2 );
   BOOST_CHECK_EQUAL( fc::parse_size( "3" ), 3 );
   BOOST_CHECK_EQUAL( fc::parse_size( "4" ), 4 );

   BOOST_CHECK_EQUAL( fc::parse_size( "9" ),   9 );
   BOOST_CHECK_EQUAL( fc::parse_size( "10" ), 10 );
   BOOST_CHECK_EQUAL( fc::parse_size( "11" ), 11 );
   BOOST_CHECK_EQUAL( fc::parse_size( "12" ), 12 );

   BOOST_CHECK_EQUAL( fc::parse_size( "314159265"), 314159265 );
   BOOST_CHECK_EQUAL( fc::parse_size( "1k" ), 1024 );
   BOOST_CHECK_THROW( fc::parse_size( "1a" ), fc::parse_error_exception );
   BOOST_CHECK_EQUAL( fc::parse_size( "1kb" ), 1000 );
   BOOST_CHECK_EQUAL( fc::parse_size( "1MiB" ), 1048576 );
   BOOST_CHECK_EQUAL( fc::parse_size( "32G" ), 34359738368 );
}

/**
 * Verify that names are RFC-1035 compliant https://tools.ietf.org/html/rfc1035
 * https://github.com/cryptonomex/graphene/issues/15
 */
BOOST_AUTO_TEST_CASE( valid_name_test )
{
   BOOST_CHECK( !is_valid_account_name( "a" ) );
   BOOST_CHECK( !is_valid_account_name( "A" ) );
   BOOST_CHECK( !is_valid_account_name( "0" ) );
   BOOST_CHECK( !is_valid_account_name( "." ) );
   BOOST_CHECK( !is_valid_account_name( "-" ) );

   BOOST_CHECK( !is_valid_account_name( "aa" ) );
   BOOST_CHECK( !is_valid_account_name( "aA" ) );
   BOOST_CHECK( !is_valid_account_name( "a0" ) );
   BOOST_CHECK( !is_valid_account_name( "a." ) );
   BOOST_CHECK( !is_valid_account_name( "a-" ) );

   BOOST_CHECK( is_valid_account_name( "aaa" ) );
   BOOST_CHECK( !is_valid_account_name( "aAa" ) );
   BOOST_CHECK( is_valid_account_name( "a0a" ) );
   BOOST_CHECK( !is_valid_account_name( "a.a" ) );
   BOOST_CHECK( is_valid_account_name( "a-a" ) );

   BOOST_CHECK( is_valid_account_name( "aa0" ) );
   BOOST_CHECK( !is_valid_account_name( "aA0" ) );
   BOOST_CHECK( is_valid_account_name( "a00" ) );
   BOOST_CHECK( !is_valid_account_name( "a.0" ) );
   BOOST_CHECK( is_valid_account_name( "a-0" ) );

   BOOST_CHECK(  is_valid_account_name( "aaa-bbb-ccc" ) );
   BOOST_CHECK(  is_valid_account_name( "aaa-bbb.ccc" ) );

   BOOST_CHECK( !is_valid_account_name( "aaa,bbb-ccc" ) );
   BOOST_CHECK( !is_valid_account_name( "aaa_bbb-ccc" ) );
   BOOST_CHECK( !is_valid_account_name( "aaa-BBB-ccc" ) );

   BOOST_CHECK( !is_valid_account_name( "1aaa-bbb" ) );
   BOOST_CHECK( !is_valid_account_name( "-aaa-bbb-ccc" ) );
   BOOST_CHECK( !is_valid_account_name( ".aaa-bbb-ccc" ) );
   BOOST_CHECK( !is_valid_account_name( "/aaa-bbb-ccc" ) );

   BOOST_CHECK( !is_valid_account_name( "aaa-bbb-ccc-" ) );
   BOOST_CHECK( !is_valid_account_name( "aaa-bbb-ccc." ) );
   BOOST_CHECK( !is_valid_account_name( "aaa-bbb-ccc.." ) );
   BOOST_CHECK( !is_valid_account_name( "aaa-bbb-ccc/" ) );

   BOOST_CHECK( !is_valid_account_name( "aaa..bbb-ccc" ) );
   BOOST_CHECK( is_valid_account_name( "aaa.bbb-ccc" ) );
   BOOST_CHECK( is_valid_account_name( "aaa.bbb.ccc" ) );

   BOOST_CHECK(  is_valid_account_name( "aaa--bbb--ccc" ) );
   BOOST_CHECK( !is_valid_account_name( "xn--san-p8a.de" ) );
   BOOST_CHECK(  is_valid_account_name( "xn--san-p8a.dex" ) );
   BOOST_CHECK( !is_valid_account_name( "xn-san-p8a.de" ) );
   BOOST_CHECK(  is_valid_account_name( "xn-san-p8a.dex" ) );

   BOOST_CHECK(  is_valid_account_name( "this-label-has" ) );
   BOOST_CHECK( !is_valid_account_name( "this-label-has-more-than-63-char.act.ers-64-to-be-really-precise" ) );
   BOOST_CHECK( !is_valid_account_name( "none.of.these.labels.has.more.than-63.chars--but.still.not.valid" ) );
}

BOOST_AUTO_TEST_CASE( merkle_root )
{
   signed_block block;
   vector<signed_transaction> tx;
   vector<digest_type> t;
   const uint32_t num_tx = 10;

   for( uint32_t i=0; i<num_tx; i++ )
   {
      tx.emplace_back();
      tx.back().ref_block_prefix = i;
      t.push_back( tx.back().merkle_digest() );
   }

   auto c = []( const digest_type& digest ) -> checksum_type
   {   return checksum_type::hash( digest );   };

   auto d = []( const digest_type& left, const digest_type& right ) -> digest_type
   {   return digest_type::hash( std::make_pair( left, right ) );   };

   BOOST_CHECK( block.calculate_merkle_root() == checksum_type() );

   block.transactions.push_back( tx[0] );
   BOOST_CHECK( block.calculate_merkle_root() ==
      c(t[0])
      );

   digest_type dA, dB, dC, dD, dE, dI, dJ, dK, dM, dN, dO;

   /****************
    *              *
    *   A=d(0,1)   *
    *      / \     *
    *     0   1    *
    *              *
    ****************/

   dA = d(t[0], t[1]);

   block.transactions.push_back( tx[1] );
   BOOST_CHECK( block.calculate_merkle_root() == c(dA) );

   /*************************
    *                       *
    *         I=d(A,B)      *
    *        /        \     *
    *   A=d(0,1)      B=2   *
    *      / \        /     *
    *     0   1      2      *
    *                       *
    *************************/

   dB = t[2];
   dI = d(dA, dB);

   block.transactions.push_back( tx[2] );
   BOOST_CHECK( block.calculate_merkle_root() == c(dI) );

   /***************************
    *                         *
    *       I=d(A,B)          *
    *        /    \           *
    *   A=d(0,1)   B=d(2,3)   *
    *      / \    /   \       *
    *     0   1  2     3      *
    *                         *
    ***************************
    */

   dB = d(t[2], t[3]);
   dI = d(dA, dB);

   block.transactions.push_back( tx[3] );
   BOOST_CHECK( block.calculate_merkle_root() == c(dI) );

   /***************************************
    *                                     *
    *                  __M=d(I,J)__       *
    *                 /            \      *
    *         I=d(A,B)              J=C   *
    *        /        \            /      *
    *   A=d(0,1)   B=d(2,3)      C=4      *
    *      / \        / \        /        *
    *     0   1      2   3      4         *
    *                                     *
    ***************************************/

   dC = t[4];
   dJ = dC;
   dM = d(dI, dJ);

   block.transactions.push_back( tx[4] );
   BOOST_CHECK( block.calculate_merkle_root() == c(dM) );

   /**************************************
    *                                    *
    *                 __M=d(I,J)__       *
    *                /            \      *
    *        I=d(A,B)              J=C   *
    *       /        \            /      *
    *  A=d(0,1)   B=d(2,3)   C=d(4,5)    *
    *     / \        / \        / \      *
    *    0   1      2   3      4   5     *
    *                                    *
    **************************************/

   dC = d(t[4], t[5]);
   dJ = dC;
   dM = d(dI, dJ);

   block.transactions.push_back( tx[5] );
   BOOST_CHECK( block.calculate_merkle_root() == c(dM) );

   /***********************************************
    *                                             *
    *                  __M=d(I,J)__               *
    *                 /            \              *
    *         I=d(A,B)              J=d(C,D)      *
    *        /        \            /        \     *
    *   A=d(0,1)   B=d(2,3)   C=d(4,5)      D=6   *
    *      / \        / \        / \        /     *
    *     0   1      2   3      4   5      6      *
    *                                             *
    ***********************************************/

   dD = t[6];
   dJ = d(dC, dD);
   dM = d(dI, dJ);

   block.transactions.push_back( tx[6] );
   BOOST_CHECK( block.calculate_merkle_root() == c(dM) );

   /*************************************************
    *                                               *
    *                  __M=d(I,J)__                 *
    *                 /            \                *
    *         I=d(A,B)              J=d(C,D)        *
    *        /        \            /        \       *
    *   A=d(0,1)   B=d(2,3)   C=d(4,5)   D=d(6,7)   *
    *      / \        / \        / \        / \     *
    *     0   1      2   3      4   5      6   7    *
    *                                               *
    *************************************************/

   dD = d(t[6], t[7]);
   dJ = d(dC, dD);
   dM = d(dI, dJ);

   block.transactions.push_back( tx[7] );
   BOOST_CHECK( block.calculate_merkle_root() == c(dM) );

   /************************************************************************
    *                                                                      *
    *                             _____________O=d(M,N)______________      *
    *                            /                                   \     *
    *                  __M=d(I,J)__                                  N=K   *
    *                 /            \                              /        *
    *         I=d(A,B)              J=d(C,D)                 K=E           *
    *        /        \            /        \            /                 *
    *   A=d(0,1)   B=d(2,3)   C=d(4,5)   D=d(6,7)      E=8                 *
    *      / \        / \        / \        / \        /                   *
    *     0   1      2   3      4   5      6   7      8                    *
    *                                                                      *
    ************************************************************************/

   dE = t[8];
   dK = dE;
   dN = dK;
   dO = d(dM, dN);

   block.transactions.push_back( tx[8] );
   BOOST_CHECK( block.calculate_merkle_root() == c(dO) );

   /************************************************************************
    *                                                                      *
    *                             _____________O=d(M,N)______________      *
    *                            /                                   \     *
    *                  __M=d(I,J)__                                  N=K   *
    *                 /            \                              /        *
    *         I=d(A,B)              J=d(C,D)                 K=E           *
    *        /        \            /        \            /                 *
    *   A=d(0,1)   B=d(2,3)   C=d(4,5)   D=d(6,7)   E=d(8,9)               *
    *      / \        / \        / \        / \        / \                 *
    *     0   1      2   3      4   5      6   7      8   9                *
    *                                                                      *
    ************************************************************************/

   dE = d(t[8], t[9]);
   dK = dE;
   dN = dK;
   dO = d(dM, dN);

   block.transactions.push_back( tx[9] );
   BOOST_CHECK( block.calculate_merkle_root() == c(dO) );
}

BOOST_AUTO_TEST_SUITE_END()
