#pragma once

// This header forward-declare spack/unpack and to/from variant functions for Steem types.
// These declarations need to be as early as possible to prevent compiler errors.

#include <chainbase/allocators.hpp>

namespace chainbase {
template< typename T >
class oid;
}

namespace fc { namespace raw {

template<typename Stream, typename T>
inline void pack( Stream& s, const chainbase::oid<T>& id );
template<typename Stream, typename T>
inline void unpack( Stream& s, chainbase::oid<T>& id );

template<typename Stream>
inline void pack( Stream& s, const chainbase::shared_string& ss );
template<typename Stream>
inline void unpack( Stream& s, chainbase::shared_string& ss );

template<typename Stream, typename E, typename A>
void pack( Stream& s, const boost::interprocess::deque< E, A >& value );
template<typename Stream, typename E, typename A>
void unpack( Stream& s, boost::interprocess::deque< E, A >& value );

template<typename Stream, typename K, typename V, typename C, typename A>
void pack( Stream& s, const boost::interprocess::flat_map< K, V, C, A >& value );
template<typename Stream, typename K, typename V, typename C, typename A>
void unpack( Stream& s, boost::interprocess::flat_map< K, V, C, A >& value );

/*
template<typename Stream, typename K, typename V, typename C, typename A>
void pack( Stream& s, const boost::interprocess::map< K, V, C, A >& value );
template<typename Stream, typename K, typename V, typename C, typename A>
void unpack( Stream& s, boost::interprocess::map< K, V, C, A >& value );

template<typename Stream, typename E, typename C, typename A>
void pack( Stream& s, const boost::interprocess::set< E, C, A >& value );
template<typename Stream, typename E, typename C, typename A>
void unpack( Stream& s, boost::interprocess::set< E, C, A >& value );
*/

} }
