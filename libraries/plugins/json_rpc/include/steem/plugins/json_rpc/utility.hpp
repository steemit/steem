#pragma once

#include <type_traits>

#include <fc/reflect/reflect.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/cat.hpp>

#define DECLARE_API_METHOD_HELPER( r, data, method ) \
BOOST_PP_CAT( method, _return ) method( const BOOST_PP_CAT( method, _args )& );

#define FOR_EACH_API_HELPER( r, callback, method ) \
{ \
   typedef std::remove_pointer<decltype(this)>::type this_type; \
   \
   callback( \
      (*this), \
      BOOST_PP_STRINGIZE( method ), \
      &this_type::method, \
      static_cast< BOOST_PP_CAT( method, _args )* >(nullptr), \
      static_cast< BOOST_PP_CAT( method, _return )* >(nullptr) \
   ); \
}

#define DECLARE_API( METHODS ) \
   BOOST_PP_SEQ_FOR_EACH( DECLARE_API_METHOD_HELPER, _, METHODS ) \
   \
   template< typename Lambda > \
   void for_each_api( Lambda&& callback ) \
   { \
      BOOST_PP_SEQ_FOR_EACH( FOR_EACH_API_HELPER, callback, METHODS ) \
   }

#define DEFINE_API( class, api_name )                                   \
api_name ## _return class :: api_name ( const api_name ## _args& args )

namespace steem { namespace plugins { namespace json_rpc {

struct void_type {};

} } } // steem::plugins::json_rpc

FC_REFLECT( steem::plugins::json_rpc::void_type, )
