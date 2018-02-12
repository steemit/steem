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
            class msg_pack final {
            public:
                fc::variant id;
                std::string plugin;
                std::string method;
                fc::optional<std::vector<fc::variant>> args;

                // Constructor with hidden handlers types
                template <typename Handler>
                msg_pack(Handler &&);

                // Move constructor/operator move handlers, so source msg_pack can't pass result/error to connection
                msg_pack(msg_pack &&);

                msg_pack & operator=(msg_pack &&);

                bool valid() const;

                // Initialize rpc request/response id
                void rpc_id(fc::variant);

                fc::optional<fc::variant> rpc_id() const;

                // Pass result to remote connection
                void result(fc::optional<fc::variant> result);

                fc::optional<fc::variant> result() const;

                // Pass error to remote connection
                void error(int32_t code, std::string message, fc::optional<fc::variant> data = fc::optional<fc::variant>());

                void error(std::string message, fc::optional<fc::variant> data = fc::optional<fc::variant>());

                void error(int32_t code, const fc::exception &);

                void error(const fc::exception &);

                fc::optional<std::string> error() const;

            private:
                struct impl;
                std::unique_ptr<impl> pimpl;
            };

            struct void_type {
            };

        }
    }
} // golos::plugins::json_rpc

FC_REFLECT((golos::plugins::json_rpc::void_type),)
