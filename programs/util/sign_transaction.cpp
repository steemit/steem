
#include <iostream>
#include <string>

#include <boost/algorithm/string.hpp>

#include <fc/io/json.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/variant.hpp>

#include <steem/utilities/key_conversion.hpp>

#include <steem/protocol/transaction.hpp>
#include <steem/protocol/types.hpp>

struct tx_signing_request
{
   steem::protocol::transaction     tx;
   std::string                        wif;
};

struct tx_signing_result
{
   steem::protocol::transaction     tx;
   fc::sha256                         digest;
   fc::sha256                         sig_digest;
   steem::protocol::public_key_type key;
   steem::protocol::signature_type  sig;
};

FC_REFLECT( tx_signing_request, (tx)(wif) )
FC_REFLECT( tx_signing_result, (digest)(sig_digest)(key)(sig) )

int main(int argc, char** argv, char** envp)
{
   steem::protocol::chain_id_type _chain_id = steem::protocol::generate_chain_id( STEEM_CHAIN_ID_NAME );
  
   // hash key pairs on stdin
   std::string chain_id, hash, wif;
   while( std::cin )
   {
      std::string line;
      std::getline( std::cin, line );
      boost::trim(line);
      if( line == "" )
         continue;

      fc::variant v = fc::json::from_string( line, fc::json::strict_parser );
      tx_signing_request sreq;
      fc::from_variant( v, sreq );
      tx_signing_result sres;
      sres.tx = sreq.tx;
      sres.digest = sreq.tx.digest();
      sres.sig_digest = sreq.tx.sig_digest( _chain_id );

      fc::ecc::private_key priv_key = *steem::utilities::wif_to_key( sreq.wif );
      sres.sig = priv_key.sign_compact( sres.sig_digest );
      sres.key = steem::protocol::public_key_type( priv_key.get_public_key() );
      std::cout << fc::json::to_string( sres ) << std::endl;
   }
   return 0;
}
