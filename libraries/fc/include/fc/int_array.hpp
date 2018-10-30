#pragma once

#include <fc/variant.hpp>

#include <limits>
#include <vector>

namespace fc {

/**
 * Like fc::array, but does zero-initialization and has friendly JSON format
 */

template<typename T, size_t N>
class int_array
{
    public:
       int_array(){ memset( data, 0, sizeof(data) ); }

       T&       at( size_t pos )      { FC_ASSERT( pos < N); return data[pos]; }
       const T& at( size_t pos )const { FC_ASSERT( pos < N); return data[pos]; }

       T&       operator[]( size_t pos )      { FC_ASSERT( pos < N); return data[pos]; }
       const T& operator[]( size_t pos )const { FC_ASSERT( pos < N); return data[pos]; }

       const T*     begin()const  {  return &data[0]; }
       const T*     end()const    {  return &data[N]; }

       T*           begin()       {  return &data[0]; }
       T*           end()         {  return &data[N]; }

       size_t       size()const   { return N; }

       T data[N];
};

template<typename T,size_t N> struct get_typename< fc::int_array<T,N> >
{
   static const char* name()
   {
      static std::string _name = std::string("fc::int_array<")+std::string(fc::get_typename<T>::name())+","+ fc::to_string(N) + ">";
      return _name.c_str();
   }
};

template<typename T, size_t N>
void from_variant( const variant& var, fc::int_array<T,N>& a )
{
   memset( a.data, 0, sizeof(a.data) );
   const variants& varray = var.get_array();    // throws if is not array
   FC_ASSERT( varray.size() == N );

   for( size_t i=0; i<N; i++ )
   {
      from_variant( varray[i], a[i] );
   }
}

template<typename T, size_t N>
void to_variant( const fc::int_array<T,N>& a, variant& v )
{
   std::vector<variant> vars(N);
   for( size_t i=0; i<N; i++ )
      vars[i] = variant(a[i]);
   v = std::move(vars);
}

} // fc
