
#include <boost/multiprecision/cpp_int.hpp>

#include <fc/crypto/sha256.hpp>
#include <fc/exception/exception.hpp>

#include <fstream>
#include <iomanip>
#include <iostream>

uint64_t endian_reverse( uint64_t x )
{
   uint64_t x0 = ((x        ) & 0xFF);
   uint64_t x1 = ((x >> 0x08) & 0xFF);
   uint64_t x2 = ((x >> 0x10) & 0xFF);
   uint64_t x3 = ((x >> 0x18) & 0xFF);
   uint64_t x4 = ((x >> 0x20) & 0xFF);
   uint64_t x5 = ((x >> 0x28) & 0xFF);
   uint64_t x6 = ((x >> 0x30) & 0xFF);
   uint64_t x7 = ((x >> 0x38) & 0xFF);

   return (x0 << 0x38)
        | (x1 << 0x30)
        | (x2 << 0x28)
        | (x3 << 0x20)
        | (x4 << 0x18)
        | (x5 << 0x10)
        | (x6 << 0x08)
        | (x7        );
}

int main(int argc, char**argv, char** envp)
{
   std::ifstream infile("log_test.txt");
   uint32_t ref_clz;
   std::string str_h;
   uint32_t ref_log;
   uint32_t cases = 0;
   uint32_t errors = 0;

   while( true )
   {
      if( !(infile >> std::hex >> ref_clz) )
         break;
      if( !(infile >> str_h) )
         break;
      if( !(infile >> std::hex >> ref_log) )
         break;
      fc::sha256 h(str_h);
      if( ref_clz != h.clz() )
      {
         std::cerr << "got error on clz(" << str_h << ")" << std::endl;
         ++errors;
      }
      if( ref_log != h.approx_log_32() )
      {
         std::cerr << "got error on log(" << str_h << ")" << std::endl;
         ++errors;
      }
      double d_ilog_h_test = h.inverse_approx_log_32_double( ref_log );
      h.set_to_inverse_approx_log_32( ref_log );
      if( ref_log != h.approx_log_32() )
      {
         std::cerr << "got error on ilog(" << ref_log << ")" << std::endl;
         ++errors;
      }

      std::string str_ilog_h = h.str();
      boost::multiprecision::uint256_t u256_ilog_h( "0x" + str_ilog_h );
      double d_ilog_h_ref = u256_ilog_h.template convert_to<double>();
      if( d_ilog_h_ref != d_ilog_h_test )
      {
         std::cerr << "got error on d_ilog(" << ref_log << ")" << std::endl;
         ++errors;
      }

      if( h != fc::sha256() )
      {
         fc::sha256 h_before = h;
         if( h._hash[3] == 0 )
         {
            if( h._hash[2] == 0 )
            {
               if( h._hash[1] == 0 )
               {
                  h._hash[0] = endian_reverse( endian_reverse( h._hash[0] )-1 );
               }
               h._hash[1] = endian_reverse( endian_reverse( h._hash[1] )-1 );
            }
            h._hash[2] = endian_reverse( endian_reverse( h._hash[2] )-1 );
         }
         h._hash[3] = endian_reverse( endian_reverse( h._hash[3] )-1 );
         bool ok = (h.approx_log_32() < ref_log);
         if( !ok )
         {
            std::cerr << "got error on logm1 for " << ref_log << std::endl;
            std::cerr << "h0:" << str_h << std::endl;
            std::cerr << "h1:" << h_before.str() << std::endl;
            std::cerr << "h2:" << h.str() << std::endl;
            std::cerr << "ref_log:" << std::hex << std::setw(8) << ref_log << std::endl;
            std::cerr << "log(h) :" << std::hex << std::setw(8) << h.approx_log_32() << std::endl;
            std::cerr << std::endl;
            ++errors;
         }
      }

      ++cases;
   }

   std::cerr << "sha256_log_test checked " << cases << " cases, got " << errors << " errors" << std::endl;
   if( errors )
      return 1;
   return 0;
}
