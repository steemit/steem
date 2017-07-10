#pragma once
#include <fc/reflect/reflect.hpp>

#define DEFINE_API_ARGS( api_name, arg_type, return_type )  \
typedef arg_type api_name ## _args;                         \
typedef return_type api_name ## _return;

#define DECLARE_API( api_name )                             \
api_name ## _return api_name( const api_name ## _args& )const;     \

#define DEFINE_API( class, api_name )                                   \
api_name ## _return class :: api_name ( const api_name ## _args& args )const  \

namespace steemit { namespace plugins { namespace json_rpc {

struct void_args {};

} } } // steemit::plugins::json_rpc

FC_REFLECT( steemit::plugins::json_rpc::void_args, )
