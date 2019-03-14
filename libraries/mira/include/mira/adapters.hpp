#pragma once
#include <mira/multi_index_container.hpp>
#include <mira/ordered_index.hpp>
#include <mira/tag.hpp>
#include <mira/member.hpp>
#include <mira/indexed_by.hpp>
#include <mira/composite_key.hpp>
#include <mira/mem_fun.hpp>
#include <mira/boost_adapter.hpp>

#include <boost/multi_index/indexed_by.hpp>
#include <boost/multi_index/composite_key.hpp>

#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>


#define MIRA_ADAPTER_PP_ENUM_PARAMS_MIRA_M(z, n, param) BOOST_PP_COMMA_IF(n) typename adapter_conversion< param ## n >::mira_type
#define MIRA_ADAPTER_PP_ENUM_PARAMS_MIRA(count, param) BOOST_PP_REPEAT(count, MIRA_ADAPTER_PP_ENUM_PARAMS_MIRA_M, param)

#define MIRA_ADAPTER_PP_ENUM_PARAMS_BOOST_M(z, n, param) BOOST_PP_COMMA_IF(n) typename adapter_conversion< param ## n >::bmic_type
#define MIRA_ADAPTER_PP_ENUM_PARAMS_BOOST(count, param) BOOST_PP_REPEAT(count, MIRA_ADAPTER_PP_ENUM_PARAMS_BOOST_M, param)

#define MIRA_COMPOSITE_KEY_SIZE 10

#define MIRA_CK_ENUM(macro,data) \
  BOOST_PP_ENUM(MIRA_COMPOSITE_KEY_SIZE,macro,data)

#define MIRA_CK_ENUM_PARAMS(param) \
  BOOST_PP_ENUM_PARAMS(MIRA_COMPOSITE_KEY_SIZE,param)

#define MIRA_CK_TEMPLATE_PARM(z,n,text) \
   typename BOOST_PP_CAT(text,n)

#define MIRA_INDEXED_BY_SIZE 20

#define MIRA_INDEXED_BY_TEMPLATE_PARM(z,n,var) \
   typename BOOST_PP_CAT(var,n)

namespace mira {

template< typename T >
struct adapter_conversion
{
   typedef T mira_type;
   typedef T bmic_type;
};

template< typename Arg1, typename Arg2, typename Arg3 >
struct adapter_conversion< multi_index::multi_index_container< Arg1, Arg2, Arg3 > >
{
   typedef multi_index::multi_index_container<
      typename adapter_conversion< Arg1 >::mira_type,
      typename adapter_conversion< Arg2 >::mira_type,
      typename adapter_conversion< Arg3 >::mira_type
      > mira_type;

   typedef boost_multi_index_adapter<
      typename adapter_conversion< Arg1 >::bmic_type,
      typename adapter_conversion< Arg2 >::bmic_type,
      typename adapter_conversion< Arg3 >::bmic_type
      > bmic_type;
};

template<
   BOOST_PP_ENUM(MIRA_INDEXED_BY_SIZE,MIRA_INDEXED_BY_TEMPLATE_PARM,T)
>
struct adapter_conversion< multi_index::indexed_by< BOOST_PP_ENUM_PARAMS(MIRA_INDEXED_BY_SIZE,T) > >
{
   typedef multi_index::indexed_by< MIRA_ADAPTER_PP_ENUM_PARAMS_MIRA(MIRA_INDEXED_BY_SIZE,T) >           mira_type;
   typedef boost::multi_index::indexed_by< MIRA_ADAPTER_PP_ENUM_PARAMS_BOOST(MIRA_INDEXED_BY_SIZE,T) >   bmic_type;
};

template< typename Arg1, typename Arg2, typename Arg3 >
struct adapter_conversion< multi_index::ordered_unique< Arg1, Arg2, Arg3 > >
{
   typedef multi_index::ordered_unique<
      typename adapter_conversion< Arg1 >::mira_type,
      typename adapter_conversion< Arg2 >::mira_type,
      typename adapter_conversion< Arg3 >::mira_type
      > mira_type;

   typedef boost::multi_index::ordered_unique<
      typename adapter_conversion< Arg1 >::bmic_type,
      typename adapter_conversion< Arg2 >::bmic_type,
      typename adapter_conversion< Arg3 >::bmic_type
      > bmic_type;
};

template< typename Value, MIRA_CK_ENUM(MIRA_CK_TEMPLATE_PARM,KeyFromValue) >
struct adapter_conversion< multi_index::composite_key< Value, MIRA_CK_ENUM_PARAMS(KeyFromValue) > >
{
   typedef multi_index::composite_key< Value, MIRA_CK_ENUM_PARAMS(KeyFromValue) >        mira_type;
   typedef boost::multi_index::composite_key< Value, MIRA_CK_ENUM_PARAMS(KeyFromValue) > bmic_type;
};

template< MIRA_CK_ENUM(MIRA_CK_TEMPLATE_PARM,Compare) >
struct adapter_conversion< multi_index::composite_key_compare< MIRA_CK_ENUM_PARAMS(Compare) > >
{
   typedef multi_index::composite_key_compare< MIRA_CK_ENUM_PARAMS(Compare) >          mira_type;
   typedef boost::multi_index::composite_key_compare< MIRA_CK_ENUM_PARAMS(Compare) >   bmic_type;
};

} //mira

#undef MIRA_ADAPTER_PP_ENUM_PARAMS_MIRA_M
#undef MIRA_ADAPTER_PP_ENUM_PARAMS_MIRA
#undef MIRA_ADAPTER_PP_ENUM_PARAMS_BOOST_M
#undef MIRA_ADAPTER_PP_ENUM_PARAMS_BOOST
#undef MIRA_COMPOSITE_KEY_SIZE
#undef MIRA_CK_ENUM
#undef MIRA_CK_ENUM_PARAMS
#undef MIRA_CK_TEMPLATE_PARM
#undef MIRA_INDEXED_BY_SIZE
#undef MIRA_INDEXED_BY_TEMPLATE_PARM
