#pragma once

// Provides PLUGIN_API_VALIDATE_ARGS macro usable in DEFINE_API methods
// to generate arguments processing boilerplate.

//Usage:
// PLUGIN_API_VALIDATE_ARGS(
//     (std::string, json_filename)
// )
//Expands to:
// std::string json_filename;
// FC_ASSERT(args.args.valid(), "Invalid parameters");
// auto n_args = args.args->size();
// FC_ASSERT(n_args == 1, "Expected 1 parameter, received ${n}", ("n", n_args));
// json_filename = args.args->at(0).as<std::string>();
//
//And this:
// PLUGIN_API_VALIDATE_ARGS(
//     (std::string, json_filename)
//     (uint32_t, count)
//     (uint32_t, skip_flags, golos::chain::database::skip_nothing)
// )
//Expands to:
// std::string json_filename;
// uint32_t count;
// uint32_t skip_flags = golos::chain::database::skip_nothing;
// FC_ASSERT(args.args.valid(), "Invalid parameters");
// auto n_args = args.args->size();
// FC_ASSERT(n_args >= 2 && n_args <= 3, "Expected at least 2 and up to 3 parameters, received ${n}", ("n", n_args));
// json_filename = args.args->at(0).as<std::string>();
// count = args.args->at(1).as<uint32_t>();
// if (n_args > 2) {
//     skip_flags = args.args->at(2).as<uint32_t>();
// }

#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/control.hpp>
#include <boost/preprocessor/control/while.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/seq/pop_front.hpp>
#include <boost/preprocessor/seq/variadic_seq_to_seq.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/variadic/elem.hpp>
#include <boost/preprocessor/logical/and.hpp>
#include <boost/preprocessor/comparison/greater.hpp>
#include <boost/preprocessor/arithmetic/add.hpp>
#include <boost/preprocessor/facilities/empty.hpp>

#define REMOVE_PARENTHESES(...) __VA_ARGS__

// prepare
#define PLUGIN_API_VALIDATE_ARGS(ARGS)   PLUGIN_API_VALIDATE_ARGS_I(BOOST_PP_VARIADIC_SEQ_TO_SEQ(ARGS))
#define PLUGIN_API_VALIDATE_ARGS_I(ARGS) PLUGIN_API_VALIDATE_ARGS_II(ARGS, COUNT_REQUIRED_ARGS(ARGS), BOOST_PP_SEQ_SIZE(ARGS))

// main
#define PLUGIN_API_VALIDATE_ARGS_II(ARGS, N_REQ, N_ARGS)  \
    PLUGIN_API_DECLARE_ARG_VARS(ARGS)   \
    FC_ASSERT(args.args.valid(), "Invalid parameters"); /* is it possible to get invalid?*/     \
    auto n_args = args.args->size();    \
    PLUGIN_API_ARGS_NUM_ASSERT(N_REQ, N_ARGS) \
    PLUGIN_API_LOAD_AND_CHECK_ARGS(ARGS, N_REQ)

// helper
#define COUNT_REQUIRED_ARGS(args)       COUNT_REQUIRED_ARGS_I(args, BOOST_PP_SEQ_SIZE(args))
#define COUNT_REQUIRED_ARGS_I(args, sz) BOOST_PP_SEQ_ELEM(2, BOOST_PP_WHILE(COUNT_PR, COUNT_OP, (args(end))(sz)(0)))
#define     STATE_OP(op, state) op(BOOST_PP_SEQ_ELEM(0, state), BOOST_PP_SEQ_ELEM(1, state), BOOST_PP_SEQ_ELEM(2, state))
#define     COUNT_OP(d, state) STATE_OP(COUNT_OP_I, state)
#define     COUNT_PR(d, state) STATE_OP(COUNT_PR_I, state)
#define     COUNT_OP_I(args, sz, i) (args)(sz)(BOOST_PP_INC(i))
#define     COUNT_PR_I(args, sz, i) BOOST_PP_AND(HAVE_NEXT_ARG(sz, i), GOT_REQUIRED(args, i))
#define         HAVE_NEXT_ARG(sz, i)    BOOST_PP_LESS(i, sz)
#define         GOT_REQUIRED(args, i)   BOOST_PP_LESS(ARG_SIZE(BOOST_PP_SEQ_ELEM(i, args)), 3)
#define     ARG_SIZE(arg) BOOST_PP_VARIADIC_SIZE(REMOVE_PARENTHESES arg)

// steps:
#define PLUGIN_API_DECLARE_ARG_VARS(ARGS) BOOST_PP_SEQ_FOR_EACH(PLUGIN_API_DECLARE_ARG_VAR, _, ARGS)
#define     PLUGIN_API_DECLARE_ARG_VAR(r, data, arg) \
                ARG_TYPE(arg) ARG_NAME(arg) ARG_OPT_VALUE(arg);

#define     ARG_ELEM(arg, n) BOOST_PP_VARIADIC_ELEM(n, REMOVE_PARENTHESES arg, nop)
#define     ARG_TYPE(arg)   ARG_ELEM(arg, 0)
#define     ARG_NAME(arg)   ARG_ELEM(arg, 1)
#define     ARG_VALUE(arg)  ARG_ELEM(arg, 2)
#define     ARG_HAVE_VALUE(arg) BOOST_PP_GREATER(ARG_SIZE(arg), 2)
#define     ARG_OPT_VALUE(arg) BOOST_PP_IF(ARG_HAVE_VALUE(arg), = ARG_VALUE(arg), BOOST_PP_EMPTY())

#define PLUGIN_API_ARGS_NUM_ASSERT(req, all) PLUGIN_API_ARGS_NUM_ASSERT_I(BOOST_PP_LESS(req, all), req, all)
#define     PLUGIN_API_ARGS_NUM_ASSERT_I(less, req, all) \
                PLUGIN_API_ARGS_NUM_ASSERT_II(AN_ASSERT_COND(less, req, all), AN_ASSERT_MSG(less, req, all))
#define     PLUGIN_API_ARGS_NUM_ASSERT_II(cond, msg) \
                FC_ASSERT(cond, msg, ("n", n_args));

#define     AN_ASSERT_COND(less, req, all) BOOST_PP_IF(less, n_args >= req && n_args <= all, n_args == all)
#define     AN_ASSERT_MSG(LESS, REQ, ALL) \
                "Expected " BOOST_PP_STRINGIZE( BOOST_PP_IF(LESS, at least REQ and up to ALL, REQ) ) \
                " parameter" PLURAL_FINAL(ALL) ", received ${n}"
#define     PLURAL_FINAL(n) BOOST_PP_IF(BOOST_PP_LESS(n, 2), "", "s")

#define PLUGIN_API_LOAD_AND_CHECK_ARGS(ARGS, n_req) BOOST_PP_SEQ_FOR_EACH_I(LOAD_AND_CHECK_ARG, n_req, ARGS)
#define     LOAD_AND_CHECK_ARG(r, n_req, i, arg) LOAD_AND_CHECK_ARG_I(arg, i, BOOST_PP_LESS(i, n_req))
#define     LOAD_AND_CHECK_ARG_I(arg, i, req) BOOST_PP_IF(req, LOAD_ARG, LOAD_OPTIONAL_ARG)(arg, i)
#define     LOAD_ARG(arg, i) ARG_NAME(arg) = args.args->at(i).as<ARG_TYPE(arg)>();
#define     LOAD_OPTIONAL_ARG(arg, i)   \
                if (n_args > i) {       \
                    LOAD_ARG(arg, i)    \
                }
