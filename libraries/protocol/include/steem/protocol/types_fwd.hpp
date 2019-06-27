#pragma once
#include <cstdint>
#include <fc/uint128.hpp>
#include <boost/container/flat_set.hpp>

namespace fc {
class variant;
} // fc

namespace steem { namespace protocol {
template< typename Storage = fc::uint128 >
class fixed_string_impl;

class asset_symbol_type;
class legacy_steem_asset_symbol_type;
struct legacy_steem_asset;
} } // steem::protocol

using boost::container::flat_set;

template< class Key >
class flat_set_ex: public flat_set< Key >
{
   public:

      flat_set_ex& operator=( const flat_set< Key >& obj )
      {
         flat_set< Key >::operator=( obj );
         return *this;
      }

      flat_set_ex& operator=( const flat_set_ex& obj )
      {
         flat_set< Key >::operator=( obj );
         return *this;
      }
};

namespace fc { namespace raw {

template<typename Stream, typename T>
void pack( Stream& s, const flat_set_ex<T>& value );
template<typename Stream, typename T>
void unpack( Stream& s, flat_set_ex<T>& value, uint32_t depth = 0 );

template< typename Stream, typename Storage >
inline void pack( Stream& s, const steem::protocol::fixed_string_impl< Storage >& u );
template< typename Stream, typename Storage >
inline void unpack( Stream& s, steem::protocol::fixed_string_impl< Storage >& u, uint32_t depth = 0 );

template< typename Stream >
inline void pack( Stream& s, const steem::protocol::asset_symbol_type& sym );
template< typename Stream >
inline void unpack( Stream& s, steem::protocol::asset_symbol_type& sym, uint32_t depth = 0 );

template< typename Stream >
inline void pack( Stream& s, const steem::protocol::legacy_steem_asset_symbol_type& sym );
template< typename Stream >
inline void unpack( Stream& s, steem::protocol::legacy_steem_asset_symbol_type& sym, uint32_t depth = 0 );

} // raw

template<typename T>
void to_variant( const flat_set_ex<T>& var,  variant& vo );

template<typename T>
void from_variant( const variant& var, flat_set_ex<T>& vo );

template< typename Storage >
inline void to_variant( const steem::protocol::fixed_string_impl< Storage >& s, fc::variant& v );
template< typename Storage >
inline void from_variant( const variant& v, steem::protocol::fixed_string_impl< Storage >& s );

inline void to_variant( const steem::protocol::asset_symbol_type& sym, fc::variant& v );

inline void from_variant( const fc::variant& v, steem::protocol::legacy_steem_asset& leg );
inline void to_variant( const steem::protocol::legacy_steem_asset& leg, fc::variant& v );

template<typename T> struct get_typename<flat_set_ex<T>>
{
   static const char* name()  {
      static std::string n = std::string("flat_set<") + get_typename< fc::flat_set<T> >::name() + ">";
      return n.c_str();
   }
};

} // fc
