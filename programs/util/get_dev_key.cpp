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

#include <iostream>
#include <string>

#include <fc/crypto/elliptic.hpp>
#include <fc/io/json.hpp>

#include <steem/protocol/types.hpp>
#include <steem/utilities/key_conversion.hpp>

#ifndef WIN32
#include <csignal>
#endif

using namespace std;

int main( int argc, char** argv )
{
   try
   {
      std::string dev_key_prefix;
      bool need_help = false;
      if( argc < 2 )
         need_help = true;
      else
      {
         dev_key_prefix = argv[1];
         if(  (dev_key_prefix == "-h")
           || (dev_key_prefix == "--help")
           )
           need_help = true;
      }

      if( need_help )
      {
         std::cerr << argc << " " << argv[1]  << "\n";
         std::cerr << "get-dev-key <secret> <account> ...\n"
             "\n"
	     "Takes a secret string and a list of account names, and returns a json formatted\n"
	     "  list of associated private/public key pairs.\n"
	     "\n"
	     "Parameters:\n"
	     "\n"
	     "  secret:\n"
	     "    The secret string that will be hashed with the account names to form the keys.\n"
	     "\n"
	     "  account:\n"
	     "    Account name strings for which keys will be generated. There can be an arbitrary\n"
	     "      number of these passed as parameters.\n"
	     "    If you want to include multiple accounts with the same name but different integer\n"
	     "      values at the end, a range can be given after a dash in the form a:b, and the\n"
             "      input will be exapnded to include each combination in the range (a:b].\n"
             "      E.g. name-0:3 would expand to abc-0 abc-1 abc-2\n"
	     "\n"
             "Examples:\n"
             "\n"
             "  get-dev-key xyz owner-5 active-7 balance-9 wit-block-signing-3 wit-owner-5 wit-active-33\n"
             "  get-dev-key xyz wit-block-signing-0:101\n"
             "\n";
         return 1;
      }

      bool comma = false;

      auto show_key = [&]( const fc::ecc::private_key& priv_key, const std::string& name )
      {
         fc::mutable_variant_object mvo;
         steem::protocol::public_key_type pub_key = priv_key.get_public_key();
         mvo( "private_key",    steem::utilities::key_to_wif( priv_key ) )
	    ( "public_key",     std::string( pub_key ) )
	    ( "account_name", name )
	 ;
         if( comma )
            std::cout << ",\n";
         std::cout << fc::json::to_string( mvo );
         comma = true;
      };

      std::cout << "[";

      for( int i=2; i<argc; i++ )
      {
         std::string arg = argv[i];
         std::string prefix;
         int lep = -1, rep = -1;
         auto dash_pos = arg.rfind('-');
         if( dash_pos != string::npos )
         {
            std::string lhs = arg.substr( 0, dash_pos+1 );
            std::string rhs = arg.substr( dash_pos+1 );
            auto colon_pos = rhs.find(':');
            if( colon_pos != string::npos )
            {
               prefix = lhs;
               lep = std::stoi( rhs.substr( 0, colon_pos ) );
               rep = std::stoi( rhs.substr( colon_pos+1 ) );
            }
         }
         vector< fc::ecc::private_key > keys;
         if( lep >= 0 )
         {
            for( int k=lep; k<rep; k++ )
            {
	       std::string suffix = prefix + std::to_string(k);
               std::string s = dev_key_prefix + suffix;
               show_key( fc::ecc::private_key::regenerate( fc::sha256::hash( s ) ), suffix );
            }
         }
         else
         {
	   show_key( fc::ecc::private_key::regenerate( fc::sha256::hash( dev_key_prefix + arg ) ), arg );
         }
      }
      std::cout << "]\n";
   }
   catch ( const fc::exception& e )
   {
      std::cout << e.to_detail_string() << "\n";
      return 1;
   }
   return 0;
}
