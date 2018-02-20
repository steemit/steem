#include <boost/test/unit_test.hpp>

#include <fc/crypto/elliptic.hpp>
#include <fc/log/logger.hpp>
#include <fc/io/raw.hpp>
#include <fc/variant.hpp>
#include <fc/reflect/variant.hpp>

BOOST_AUTO_TEST_SUITE(fc_crypto)

BOOST_AUTO_TEST_CASE(blind_test)
{
   try {
      auto InB1 = fc::sha256::hash("InB1");
      auto InB2 = fc::sha256::hash("InB2");
      auto OutB1 = fc::sha256::hash("OutB1");


      auto InC1 = fc::ecc::blind(InB1,25);
      auto InC2 = fc::ecc::blind(InB2,75);

      auto OutC1 = fc::ecc::blind(OutB1,40);

      auto OutB2 = fc::ecc::blind_sum( {InB1,InB2,OutB1}, 2 );
      auto OutC2 = fc::ecc::blind( OutB2, 60 );

      /*
      FC_ASSERT( fc::ecc::verify_sum( {},{InC1,InC2},  100 ) );
      FC_ASSERT( fc::ecc::verify_sum( {InC1,InC2}, {}, -100 ) );
      */

      //FC_ASSERT( fc::ecc::verify_sum( {InC1,InC2}, {OutC1}, -60 ) );


      BOOST_CHECK( fc::ecc::verify_sum( {InC1,InC2}, {OutC1,OutC2}, 0 ) );
      auto nonce = fc::sha256::hash("nonce");

      auto proof = fc::ecc::range_proof_sign( 0, OutC1, OutB1, nonce, 0, 0, 40 );
//      wdump( (proof.size()));

      auto result = fc::ecc::range_get_info( proof );
//      wdump((result));
      BOOST_CHECK( result.max_value >= 60 );
      BOOST_CHECK( result.min_value >= 0 );


      auto B1 = fc::sha256::hash("B1");
      auto B2 = fc::sha256::hash("B2");
      auto b3 = fc::sha256::hash("b3");
      auto B4 = fc::sha256::hash("B4");
      auto C1 = fc::ecc::blind( B1, 1  );
      auto C2 = fc::ecc::blind( B2, 2  );
      /*auto c3 = */fc::ecc::blind( b3, 3  );
      /*auto C4 = */fc::ecc::blind( B4, -1 );

      auto B3 = fc::ecc::blind_sum( {B1,B2}, 2 );
      auto C3 = fc::ecc::blind( B3, 3 );


      auto B2m1 = fc::ecc::blind_sum( {B2,B1}, 1 );
      /*auto C2m1 = */fc::ecc::blind( B2m1, 1 );

      BOOST_CHECK( fc::ecc::verify_sum( {C1,C2}, {C3}, 0 ) );
      BOOST_CHECK( fc::ecc::verify_sum( {C1,C2}, {C3}, 0 ) );
      BOOST_CHECK( fc::ecc::verify_sum( {C3}, {C1,C2}, 0 ) );
      BOOST_CHECK( fc::ecc::verify_sum( {C3}, {C1,C2}, 0 ) );


      {
         auto B1 = fc::sha256::hash("B1");
         /*auto B2 = */fc::sha256::hash("B2");
         /*auto B3 = */fc::sha256::hash("B3");
         /*auto B4 = */fc::sha256::hash("B4");

         //secp256k1_scalar_get_b32((unsigned char*)&B1, (const secp256k1_scalar_t*)&B2);
         //B1 = fc::variant("b2e5da56ef9f2a34d3e22fd12634bc99261e95c87b9960bf94ed3d27b30").as<fc::sha256>();

         auto C1 = fc::ecc::blind( B1, INT64_MAX );
         auto C2 = fc::ecc::blind( B1, 0 );
         auto C3 = fc::ecc::blind( B1, 1 );
         /*auto C4 = */fc::ecc::blind( B1, 2 );

         BOOST_CHECK( fc::ecc::verify_sum( {C2}, {C3}, -1 ) );
         BOOST_CHECK( fc::ecc::verify_sum( {C1}, {C1}, 0 ) );
         BOOST_CHECK( fc::ecc::verify_sum( {C2}, {C2}, 0 ) );
         BOOST_CHECK( fc::ecc::verify_sum( {C3}, {C2}, 1 ) );
         BOOST_CHECK( fc::ecc::verify_sum( {C1}, {C2}, INT64_MAX ) );
         BOOST_CHECK( fc::ecc::verify_sum( {C1}, {C2}, INT64_MAX ) );
         BOOST_CHECK( fc::ecc::verify_sum( {C2}, {C1}, -INT64_MAX ) );
      }


      {
         auto Out1 = fc::sha256::hash("B1");
         auto Out2 = fc::sha256::hash("B2");
         auto OutC1 = fc::ecc::blind( Out1, 250 );
         auto OutC2 = fc::ecc::blind( Out2, 750 );
         auto InBlind = fc::ecc::blind_sum( {Out1,Out2}, 2 );
         auto InC  = fc::ecc::blind( InBlind, 1000 );
         auto In0  = fc::ecc::blind( InBlind, 0 );

         BOOST_CHECK( fc::ecc::verify_sum( {InC}, {OutC1,OutC2}, 0 ) );
         BOOST_CHECK( fc::ecc::verify_sum( {InC}, {In0}, 1000 ) );

      }
   }
   catch ( const fc::exception& e )
   {
      edump((e.to_detail_string()));
   }
}

BOOST_AUTO_TEST_SUITE_END()
