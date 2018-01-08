
#include <iostream>
#include <string>

#include <boost/algorithm/string.hpp>

#include <fc/io/json.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/variant.hpp>

#include <steem/utilities/key_conversion.hpp>

#include <steem/protocol/transaction.hpp>
#include <steem/protocol/types.hpp>

#define CHAIN_ID_PARAM "--chain-id"

struct tx_signing_request
{
   steem::protocol::transaction     tx;
   std::string                      wif;
};

struct tx_signing_result
{
   steem::protocol::transaction     tx;
   fc::sha256                       digest;
   fc::sha256                       sig_digest;
   steem::protocol::public_key_type key;
   steem::protocol::signature_type  sig;
};

struct error_result
{
   std::string error;
};

FC_REFLECT( tx_signing_request, (tx)(wif) )
FC_REFLECT( tx_signing_result, (digest)(sig_digest)(key)(sig) )
FC_REFLECT( error_result, (error) )

int main(int argc, char** argv, char** envp)
{
   fc::sha256 chainId;

   chainId = STEEM_CHAIN_ID;

   const size_t chainIdLen = strlen(CHAIN_ID_PARAM);

   if(argc > 1 && !strncmp(argv[1], CHAIN_ID_PARAM, chainIdLen))
   {
      const char* strChainId = argv[1] + chainIdLen;
      if(*strChainId == '=')
      {
         ++strChainId;
         if(*strChainId == '\0')
         {
            if(argc == 2)
            {
               /// --chain-id=
               std::cerr << "Missing parameter for `--chain-id' option. Option ignored, default chain-id used."
                  << std::endl;
               std::cerr << "Usage: sign_transaction [--chain-id=<hex-chain-id>]" << std::endl;
            }
            else
            {
               /// --chain-id= XXXXX
               strChainId = argv[2]; /// ChainId passed as another parameter
            }
         }
      }
      else
      if(argc > 2)
      {
         /// ChainId passed as another parameter
         strChainId = argv[2];
      }
      else
      {
         std::cerr << "Missing parameter for `--chain-id' option. Option ignored, default chain-id used."
            << std::endl;
         std::cerr << "Usage: sign_transaction [--chain-id=<hex-chain-id>]" << std::endl;
      }

      if(*strChainId != '\0')
      {
         try
         {
            fc::sha256 parsedId(strChainId);
            chainId = parsedId;
            std::cerr << "Using explicit chain-id: `" << strChainId << "'" << std::endl;
         }
         catch(const fc::exception& e)
         {
            std::cerr << "Specified explicit chain-id : `" << strChainId << "' is invalid. Option ignored, default chain-id used." << std::endl;
            auto error = e.to_detail_string();
            std::cerr << error << std::endl;
         }
      }
   }

   // hash key pairs on stdin
   std::string hash, wif;
   while( std::cin )
   {
      std::string line;
      std::getline( std::cin, line );
      boost::trim(line);
      if( line == "" )
         continue;

      try
      {
         fc::variant v = fc::json::from_string( line, fc::json::strict_parser );
         tx_signing_request sreq;
         fc::from_variant( v, sreq );

         tx_signing_result sres;
         sres.tx = sreq.tx;
         sres.digest = sreq.tx.digest();
         sres.sig_digest = sreq.tx.sig_digest(chainId);

         auto priv_key = steem::utilities::wif_to_key( sreq.wif );

         if(priv_key)
         {
            sres.sig = priv_key->sign_compact( sres.sig_digest );
            sres.key = steem::protocol::public_key_type( priv_key->get_public_key() );
            std::string sres_str = fc::json::to_string( sres );
            std::cout << "{\"result\":" << sres_str << "}" << std::endl;
         }
         else
         {
            if(sreq.wif.empty())
               std::cerr << "Missing Wallet Import Format in the input JSON structure" << std::endl;
            else
               std::cerr << "Passed JSON points to invalid data according to Wallet Import Format specification: `" << sreq.wif << "'" << std::endl;
         }
      }
      catch( const fc::exception& e )
      {
         error_result eresult;
         eresult.error = e.to_detail_string();
         std::cout << fc::json::to_string( eresult ) << std::endl;
      }
   }
   return 0;
}
