/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once

// This file contains various reflection methods that are used to
// support the wallet, e.g. allow specifying operations by name
// instead of ID.

namespace steemit { namespace wallet {

struct static_variant_map
{
   flat_map< string, int > name_to_which;
   vector< string > which_to_name;
};

namespace impl {

std::string clean_name( const std::string& name )
{
   std::string result;
   const static std::string prefix = "steemit::protocol::";
   const static std::string suffix = "_operation";
   // graphene::chain::.*_operation
   if(    (name.size() >= prefix.size() + suffix.size())
       && (name.substr( 0, prefix.size() ) == prefix)
       && (name.substr( name.size()-suffix.size(), suffix.size() ) == suffix )
     )
        return name.substr( prefix.size(), name.size() - prefix.size() - suffix.size() );

   // If this line spams the console, please don't just comment it out.
   // Instead, add code above to deal specifically with the names that are causing the spam.
   wlog( "don't know how to clean name: ${name}", ("name", name) );
   return name;
}

struct static_variant_map_visitor
{
   static_variant_map_visitor() {}

   typedef void result_type;

   template< typename T >
   result_type operator()( const T& dummy )
   {
      assert( which == m.which_to_name.size() );
      std::string name = clean_name( fc::get_typename<T>::name() );
      m.name_to_which[ name ] = which;
      m.which_to_name.push_back( name );
   }

   static_variant_map m;
   int which;
};

template< typename StaticVariant >
struct from_which_visitor
{
   typedef StaticVariant result_type;

   template< typename Member >   // Member is member of static_variant
   result_type operator()( const Member& dummy )
   {
      Member result;
      from_variant( v, result );
      return result;    // converted from StaticVariant to Result automatically due to return type
   }

   const variant& v;

   from_which_visitor( const variant& _v ) : v(_v) {}
};

} // namespace impl

template< typename T >
T from_which_variant( int which, const variant& v )
{
   // Parse a variant for a known which()
   T dummy;
   dummy.set_which( which );
   impl::from_which_visitor< T > vtor(v);
   return dummy.visit( vtor );
}

template<typename T>
static_variant_map create_static_variant_map()
{
   T dummy;
   int n = dummy.count();
   impl::static_variant_map_visitor vtor;
   for( int i=0; i<n; i++ )
   {
      dummy.set_which(i);
      vtor.which = i;
      dummy.visit( vtor );
   }
   return vtor.m;
}

} } // namespace steemit::wallet
