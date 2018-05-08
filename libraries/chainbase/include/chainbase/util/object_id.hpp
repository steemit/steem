#ifndef __OBJECT_ID_HPP
#define __OBJECT_ID_HPP

#include <stdint.h>
#include <stdlib.h>

namespace chainbase
{

/**
*  Object ID type that includes the type of the object it references
*/
template<typename T>
class oid
{
public:
   oid( int64_t i = 0 ):_id(i){}

   oid& operator++() { ++_id; return *this; }

   operator size_t () const
   {
      return _id;
   }

   friend bool operator < ( const oid& a, const oid& b ) { return a._id < b._id; }
   friend bool operator > ( const oid& a, const oid& b ) { return a._id > b._id; }
   friend bool operator == ( const oid& a, const oid& b ) { return a._id == b._id; }
   friend bool operator != ( const oid& a, const oid& b ) { return a._id != b._id; }
   int64_t _id = 0;
};

} /// namespace chainbase

#endif /// __OBJECT_ID_HPP
