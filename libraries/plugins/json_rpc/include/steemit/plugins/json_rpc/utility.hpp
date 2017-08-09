#pragma once
#include <fc/reflect/reflect.hpp>

#define DEFINE_API_ARGS( api_name, arg_type, return_type )  \
typedef arg_type api_name ## _args;                         \
typedef return_type api_name ## _return;

#define DECLARE_API( api_name )                             \
api_name ## _return api_name( const api_name ## _args& );   \

#define DEFINE_API( class, api_name )                                   \
api_name ## _return class :: api_name ( const api_name ## _args& args ) \

namespace steemit { namespace plugins { namespace json_rpc {

struct void_type {};

} } } // steemit::plugins::json_rpc

FC_REFLECT( steemit::plugins::json_rpc::void_type, )
