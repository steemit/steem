#pragma once
#include <fc/reflect/reflect.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/cat.hpp>

#define DECLARE_API_HELPER( r, data, method ) \
BOOST_PP_CAT( method, _return ) method( const BOOST_PP_CAT( method, _args )& );

#define DECLARE_API( METHODS ) BOOST_PP_SEQ_FOR_EACH( DECLARE_API_HELPER, _, METHODS )

#define DEFINE_API( class, api_name )                                   \
api_name ## _return class :: api_name ( const api_name ## _args& args )

namespace steemit { namespace plugins { namespace json_rpc {

struct void_type {};

} } } // steemit::plugins::json_rpc

FC_REFLECT( steemit::plugins::json_rpc::void_type, )
