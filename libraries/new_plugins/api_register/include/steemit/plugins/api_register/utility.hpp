#pragma once
#include <boost/preprocessor/cat.hpp>

#include <fc/reflect/reflect.hpp>

#define DEFINE_API_ARGS( api_name, arg_type, return_type )  \
typedef arg_type api_name ## _args;                         \
typedef return_type api_name ## _return;

#define DECLARE_API( api_name )                             \
api_name ## _return api_name( const api_name ## _args& )const;     \

#define DEFINE_API( class, api_name )                                   \
api_name ## _return class :: api_name ( const api_name ## _args& args )const  \

namespace steemit { namespace plugins { namespace api_register {

struct void_args {};

} } } // steemit::plugins::api_register

FC_REFLECT( steemit::plugins::api_register::void_args, )
