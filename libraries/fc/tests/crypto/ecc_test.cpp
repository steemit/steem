#include <fc/crypto/elliptic.hpp>
#include <fc/exception/exception.hpp>
#include <iostream>
#include <fstream>

static std::fstream interop_data;
static bool write_mode = false;

static void interop_do(const char * const data, size_t len) {
    static char buffer[256];

    if (!interop_data.is_open()) { return; }

    FC_ASSERT(len < sizeof(buffer));
    if (write_mode) {
        interop_data.write(data, len);
        return;
    }

    interop_data.read(buffer, len);
    FC_ASSERT(!interop_data.eof());
    FC_ASSERT(!memcmp(data, buffer, len));
}

static void interop_do(const fc::ecc::public_key_data &data) {
    interop_do(data.begin(), data.size());
}

static void interop_do(const fc::ecc::private_key_secret &data) {
    interop_do(data.data(), 32);
}

static void interop_do(const fc::ecc::public_key_point_data &data) {
    interop_do(data.begin(), data.size());
}

static void interop_do(const std::string &data) {
    interop_do(data.c_str(), data.length());
}

static void interop_do(const fc::sha512 &data) {
    interop_do(data.data(), 64);
}

static void interop_do(fc::ecc::compact_signature &data) {
    if (write_mode) {
        interop_data.write((char*) data.begin(), data.size());
        return;
    }

    interop_data.read((char*) data.begin(), data.size());
}

static void interop_file(const char * const name) {
    interop_data.open(name, std::fstream::in | std::fstream::binary);
    if (!interop_data.fail()) { return; }

    write_mode = true;
    interop_data.open(name, std::fstream::out | std::fstream::binary);
    if (!interop_data.fail()) { return; }

    std::cerr << "Can't read nor write " << name << "\n";
}

int main( int argc, char** argv )
{
    if (argc > 2) {
        interop_file(argv[2]);
    }

    fc::ecc::private_key nullkey;

   for( uint32_t i = 0; i < 3000; ++ i )
   {
   try {
   FC_ASSERT( argc > 1 );

   std::string  pass(argv[1]);
   fc::sha256   h = fc::sha256::hash( pass.c_str(), pass.size() );
   fc::ecc::private_key priv = fc::ecc::private_key::generate_from_seed(h);
   FC_ASSERT( nullkey != priv );
   interop_do(priv.get_secret());
   fc::ecc::public_key  pub  = priv.get_public_key();
   interop_do(pub.serialize());
   interop_do(pub.serialize_ecc_point());

   pass += "1";
   fc::sha256   h2            = fc::sha256::hash( pass.c_str(), pass.size() );
   fc::ecc::public_key  pub1  = pub.add( h2 );
   interop_do(pub1.serialize());
   interop_do(pub1.serialize_ecc_point());
   fc::ecc::private_key priv1 = fc::ecc::private_key::generate_from_seed(h, h2);
   interop_do(priv1.get_secret());

   std::string b58 = pub1.to_base58();
   interop_do(b58);
   fc::ecc::public_key pub2 = fc::ecc::public_key::from_base58(b58);
   FC_ASSERT( pub1 == pub2 );

   fc::sha512 shared = priv1.get_shared_secret( pub );
   interop_do(shared);

   auto sig = priv.sign_compact( h );
   interop_do(sig);
   auto recover = fc::ecc::public_key( sig, h );
   interop_do(recover.serialize());
   interop_do(recover.serialize_ecc_point());
   FC_ASSERT( recover == pub );
   } 
   catch ( const fc::exception& e )
   {
      edump( (e.to_detail_string()) );
   }
   }

   if (interop_data.is_open()) {
       interop_data.close();
   }

  return 0;
}
