#pragma once

#include <fc/string.hpp>

namespace steemit { namespace chain {

/*
 * This class represents the basic versioning scheme of the Steem blockchain.
 * All versions are a triple consisting of a major version, hardfork version, and release version.
 * It allows easy comparison between versions. A version is a read only object.
 */
struct version
{
   version():v_num(0) {}
   version( uint8_t m, uint8_t h, uint8_t r );

   bool operator == ( const version& o )const { return v_num == o.v_num; }
   bool operator != ( const version& o )const { return v_num != o.v_num; }
   bool operator <  ( const version& o )const { return v_num <  o.v_num; }
   bool operator <= ( const version& o )const { return v_num <= o.v_num; }
   bool operator >  ( const version& o )const { return v_num >  o.v_num; }
   bool operator >= ( const version& o )const { return v_num >= o.v_num; }

   operator fc::string()const;

   uint32_t v_num;
};

} } // steemit::chain

namespace fc
{
   class variant;
   void to_variant( const steemit::chain::version& v, variant& var );
   void from_variant( const variant& var, steemit::chain::version& v );
} // fc

#include <fc/reflect/reflect.hpp>
FC_REFLECT( steemit::chain::version, (v_num) )