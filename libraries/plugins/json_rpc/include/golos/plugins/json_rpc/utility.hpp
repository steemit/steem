#pragma once

#include <type_traits>

#include <fc/reflect/reflect.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/cat.hpp>
#include <fc/optional.hpp>
#include <fc/variant.hpp>

#define DEFINE_API_ARGS(api_name, arg_type, return_type)  \
typedef arg_type api_name ## _args;                         \
typedef return_type api_name ## _return;

#define DECLARE_API_METHOD_HELPER(r, data, method) \
BOOST_PP_CAT( method, _return ) method( const BOOST_PP_CAT( method, _args )& );

#define FOR_EACH_API_HELPER(r, callback, method) \
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

#define DECLARE_API(METHODS) \
   BOOST_PP_SEQ_FOR_EACH( DECLARE_API_METHOD_HELPER, _, METHODS ) \
   \
   template< typename Lambda > \
   void for_each_api( Lambda&& callback ) \
   { \
      BOOST_PP_SEQ_FOR_EACH( FOR_EACH_API_HELPER, callback, METHODS ) \
   }

#define DEFINE_API(class, api_name)                                   \
api_name ## _return class :: api_name ( const api_name ## _args& args )

namespace golos {
    namespace plugins {
        namespace json_rpc {
            struct msg_pack final {
                std::string plugin;
                std::string method;
                fc::optional<std::vector<fc::variant>> args;
            };
            struct void_type {
            };

        }
    }
} // steem::plugins::json_rpc

FC_REFLECT((golos::plugins::json_rpc::void_type),)
