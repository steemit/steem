#pragma once
#include <fc/reflect/typename.hpp>
#include <boost/tuple/tuple.hpp>

namespace mira { namespace multi_index {

template< typename T > struct composite_key_result;

} } // mira::multi_index

namespace fc {

class variant;

template< typename T > void to_variant( const mira::multi_index::composite_key_result< T >& var,  variant& vo );
template< typename T > void from_variant( const variant& vo, mira::multi_index::composite_key_result< T >& var );

//template< typename... Args > void to_variant( const boost::tuples::tuple< Args... >& var, variant& vo );
//template< typename... Args > void from_variant( const variant& vo, boost::tuples::tuple< Args... >& var );

template< typename H, typename T > void to_variant( const boost::tuples::cons< H, T >& var, variant& vo );
template< typename H, typename T > void from_variant( const variant& vo, boost::tuples::cons< H, T >& var );

template< typename T > struct get_typename< mira::multi_index::composite_key_result< T > >;

template< typename H, typename T > struct get_typename< boost::tuples::cons< H, T > >;

template<> struct get_typename< boost::tuples::null_type >;

namespace raw {

template< typename Stream, typename T > void pack( Stream& s, const mira::multi_index::composite_key_result< T >& var );
template< typename Stream, typename T > void unpack( Stream& s, mira::multi_index::composite_key_result< T >& var, uint32_t depth = 0 );

template< typename Stream > void pack( Stream&, const boost::tuples::null_type );
template< typename Stream > void unpack( Stream& s, boost::tuples::null_type, uint32_t depth = 0 );

template< typename Stream, typename H, typename T > void pack( Stream& s, const boost::tuples::cons< H, T >& var );
template< typename Stream, typename H, typename T > void unpack( Stream& s, boost::tuples::cons< H, T >& var, uint32_t depth = 0 );

} } // fc::raw