#pragma once
#include <mira/multi_index_container.hpp>
#include <mira/ordered_index.hpp>
#include <mira/tag.hpp>
#include <mira/member.hpp>
#include <mira/indexed_by.hpp>
#include <mira/composite_key.hpp>
#include <mira/mem_fun.hpp>

#include <boost/multi_index/indexed_by.hpp>

#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>


#define MIRA_ADAPTER_PP_ENUM_PARAMS_MIRA_M(z, n, param) BOOST_PP_COMMA_IF(n) adapter_conversion< param ## n >::mira_type
#define MIRA_ADAPTER_PP_ENUM_PARAMS_MIRA(count, param) BOOST_PP_REPEAT(count, MIRA_ADAPTER_PP_ENUM_PARAMS_M, param)

#define MIRA_ADAPTER_PP_ENUM_PARAMS_BOOST_M(z, n, param) BOOST_PP_COMMA_IF(n) adapter_conversion< param ## n >::boost_type
#define MIRA_ADAPTER_PP_ENUM_PARAMS_BOOST(count, param) BOOST_PP_REPEAT(count, MIRA_ADAPTER_PP_ENUM_PARAMS_BOOST_M, param)

#define BOOST_MULTI_INDEX_COMPOSITE_KEY_SIZE 10

/* BOOST_PP_ENUM of BOOST_MULTI_INDEX_COMPOSITE_KEY_SIZE elements */

#define BOOST_MULTI_INDEX_CK_ENUM(macro,data)                                \
  BOOST_PP_ENUM(BOOST_MULTI_INDEX_COMPOSITE_KEY_SIZE,macro,data)

/* BOOST_PP_ENUM_PARAMS of BOOST_MULTI_INDEX_COMPOSITE_KEY_SIZE elements */

#define BOOST_MULTI_INDEX_CK_ENUM_PARAMS(param)                              \
  BOOST_PP_ENUM_PARAMS(BOOST_MULTI_INDEX_COMPOSITE_KEY_SIZE,param)

#define BOOST_MULTI_INDEX_CK_TEMPLATE_PARM(z,n,text)                         \
  typename BOOST_PP_CAT(text,n) BOOST_PP_EXPR_IF(n,=boost::tuples::null_type)

#define BOOST_MULTI_INDEX_INDEXED_BY_SIZE 20
#define BOOST_MULTI_INDEX_INDEXED_BY_TEMPLATE_PARM(z,n,var) \
  typename BOOST_PP_CAT(var,n) BOOST_PP_EXPR_IF(n,=boost::mpl::na)

namespace mira {

template< typename Arg1, typename Arg2, typename Arg3 >
struct multi_index_container_adapter{};

template<
   BOOST_PP_ENUM( BOOST_MULTI_INDEX_INDEXED_BY_SIZE, BOOST_MULTI_INDEX_INDEXED_BY_TEMPLATE_PARM, T )
>
struct indexed_by_adapter{};

template< typename Arg1, typename Arg2, typename Arg3 >
struct ordered_unique_adapter {};

template< typename T >
using tag_adapter = multi_index::tag< T >;

template< class Class, typename Type, Type Class::*PtrToMember >
struct member_adapter {};

template< typename Value, BOOST_MULTI_INDEX_CK_ENUM(BOOST_MULTI_INDEX_CK_TEMPLATE_PARM,KeyFromValue) >
struct composite_key_adapter {};

template< BOOST_MULTI_INDEX_CK_ENUM(BOOST_MULTI_INDEX_CK_TEMPLATE_PARM,Compare) >
struct composite_key_compare_adapter {};

template< class Class, typename Type, Type (Class::*PtrToMemberFunction)()const >
using const_mem_fun_adapter = multi_index::const_mem_fun< Class, Type, PtrToMemberFunction >;



template< typename T >
struct adapter_conversion
{
   typedef T mira_type;
   typedef T bmic_type;
};

template< template< typename Arg1, typename Arg2, typename Arg3 > class >
struct adapter_conversion< multi_index_container_adapter< Arg1, Arg2, Arg3 > >
{
   typedef multi_index::multi_index_container<
      adapter_conversion< Arg1 >::mira_type,
      adapter_conversion< Arg2 >::mira_type,
      adapter_conversion< Arg3 >::mira_type
      > mira_type;

   typedef boost::multi_index::multi_index_container<
      adapter_conversion< Arg1 >::boost_type,
      adapter_conversion< Arg2 >::boost_type,
      adapter_conversion< Arg3 >::boost_type
      > boost_type;
};

template<
   BOOST_PP_ENUM( BOOST_MULTI_INDEX_INDEXED_BY_SIZE, BOOST_MULTI_INDEX_INDEXED_BY_TEMPLATE_PARM, T )
>
struct adapter_conversion< indexed_by_adapter< BOOST_PP_ENUM_PARAMS(BOOST_MULTI_INDEX_INDEXED_BY_SIZE,T) > >
{
   typedef multi_index::indexed_by< MIRA_ADAPTER_PP_ENUM_PARAMS_MIRA(BOOST_MULTI_INDEX_INDEXED_BY_SIZE,T) > mira_type;
   typedef boost::multi_index::indexed_by< MIRA_ADAPTER_PP_ENUM_PARAMS_BOOST(BOOST_MULTI_INDEX_INDEXED_BY_SIZE,T) > boost_type;
};

template< typename Arg1, typename Arg2, typename Arg3 >
struct adapter_conversion< ordered_unique_adapter< Arg1, Arg2, Arg3 > >
{
   typedef multi_index::ordered_unique<
      adapter_conversion< Arg1 >::mira_type,
      adapter_conversion< Arg2 >::mira_type,
      adapter_conversion< Arg3 >::mira_type
      > mira_type;

   typedef boost::multi_index::ordered_unique<
      adapter_conversion< Arg1 >::boost_type,
      adapter_conversion< Arg2 >::boost_type,
      adapter_conversion< Arg3 >::boost_type
      > boost_type;
};

template< typename Value, BOOST_MULTI_INDEX_CK_ENUM(BOOST_MULTI_INDEX_CK_TEMPLATE_PARM,KeyFromValue) >
struct adapter_conversion< composite_key_adapter< Value, BOOST_MULTI_INDEX_CK_ENUM_PARAMS(KeyFromValue) > >
{
   typedef multi_index::composite_key< Value, BOOST_MULTI_INDEX_CK_ENUM_PARAMS(KeyFromValue) >        mira_type;
   typedef boost::multi_index::composite_key< Value, BOOST_MULTI_INDEX_CK_ENUM_PARAMS(KeyFromValue) > boost_type;
};

template< BOOST_MULTI_INDEX_CK_ENUM(BOOST_MULTI_INDEX_CK_TEMPLATE_PARM,Compare) >
struct adapter_conversion< composite_key_compare_adapter< BOOST_MULTI_INDEX_CK_ENUM_PARAMS(Compare) > >
{
   typedef multi_index::detail::composite_key_compare< BOOST_MULTI_INDEX_CK_ENUM_PARAMS(Compare) > mira_type;
   typedef boost::multi_index::composite_key_compare< BOOST_MULTI_INDEX_CK_ENUM_PARAMS(Compare) >  boost_type;
};

} //mira

#undef MIRA_ADAPTER_PP_ENUM_PARAMS_MIRA_M(z, n, param)
#undef MIRA_ADAPTER_PP_ENUM_PARAMS_MIRA(count, param)
#undef MIRA_ADAPTER_PP_ENUM_PARAMS_BOOST_M(z, n, param)
#undef MIRA_ADAPTER_PP_ENUM_PARAMS_BOOST(count, param)
#undef BOOST_MULTI_INDEX_CK_ENUM_PARAMS
#undef BOOST_MULTI_INDEX_CK_ENUM
