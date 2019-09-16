/* Copyright 2003-2015 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#pragma once

#include <mira/composite_key_fwd.hpp>
#include <mira/slice_pack.hpp>

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <boost/functional/hash_fwd.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/or.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/control/expr_if.hpp>
#include <boost/preprocessor/list/at.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/static_assert.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/utility/enable_if.hpp>
#include <functional>

#if !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING)
#include <boost/ref.hpp>
#endif

#if !defined(BOOST_NO_SFINAE)
#include <boost/type_traits/is_convertible.hpp>
#endif

#if !defined(BOOST_NO_CXX11_HDR_TUPLE)&&\
    !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
#include <mira/detail/cons_stdtuple.hpp>
#endif

/* A composite key stores n key extractors and "computes" the
 * result on a given value as a packed reference to the value and
 * the composite key itself. Actual invocations to the component
 * key extractors are lazily performed when executing an operation
 * on composite_key results (equality, comparison, hashing.)
 * As the other key extractors in Boost.MultiIndex, composite_key<T,...>
 * is  overloaded to work on chained pointers to T and reference_wrappers
 * of T.
 */

/* This user_definable macro limits the number of elements of a composite
 * key; useful for shortening resulting symbol names (MSVC++ 6.0, for
 * instance has problems coping with very long symbol names.)
 * NB: This cannot exceed the maximum number of arguments of
 * boost::tuple. In Boost 1.32, the limit is 10.
 */

#if !defined(BOOST_MULTI_INDEX_LIMIT_COMPOSITE_KEY_SIZE)
#define BOOST_MULTI_INDEX_LIMIT_COMPOSITE_KEY_SIZE 10
#endif

/* maximum number of key extractors in a composite key */

#if BOOST_MULTI_INDEX_LIMIT_COMPOSITE_KEY_SIZE<10 /* max length of a tuple */
#define BOOST_MULTI_INDEX_COMPOSITE_KEY_SIZE \
  BOOST_MULTI_INDEX_LIMIT_COMPOSITE_KEY_SIZE
#else
#define BOOST_MULTI_INDEX_COMPOSITE_KEY_SIZE 10
#endif

/* BOOST_PP_ENUM of BOOST_MULTI_INDEX_COMPOSITE_KEY_SIZE elements */

#define BOOST_MULTI_INDEX_CK_ENUM(macro,data)                                \
  BOOST_PP_ENUM(BOOST_MULTI_INDEX_COMPOSITE_KEY_SIZE,macro,data)

/* BOOST_PP_ENUM_PARAMS of BOOST_MULTI_INDEX_COMPOSITE_KEY_SIZE elements */

#define BOOST_MULTI_INDEX_CK_ENUM_PARAMS(param)                              \
  BOOST_PP_ENUM_PARAMS(BOOST_MULTI_INDEX_COMPOSITE_KEY_SIZE,param)

/* if n==0 ->   text0
 * otherwise -> textn=tuples::null_type
 */

#define BOOST_MULTI_INDEX_CK_TEMPLATE_PARM(z,n,text)                         \
  typename BOOST_PP_CAT(text,n) BOOST_PP_EXPR_IF(n,=tuples::null_type)

/* const textn& kn=textn() */

#define BOOST_MULTI_INDEX_CK_CTOR_ARG(z,n,text)                              \
  const BOOST_PP_CAT(text,n)& BOOST_PP_CAT(k,n) = BOOST_PP_CAT(text,n)()

/* typename list(0)<list(1),n>::type */

#define BOOST_MULTI_INDEX_CK_APPLY_METAFUNCTION_N(z,n,list)                  \
  BOOST_DEDUCED_TYPENAME BOOST_PP_LIST_AT(list,0)<                           \
    BOOST_PP_LIST_AT(list,1),n                                               \
  >::type

namespace mira{

template<class T> class reference_wrapper; /* fwd decl. */

namespace multi_index{

namespace detail{

/* n-th key extractor of a composite key */

template<typename CompositeKey,int N>
struct nth_key_from_value
{
  typedef typename CompositeKey::key_extractor_tuple key_extractor_tuple;
  typedef typename boost::mpl::eval_if_c<
    N<boost::tuples::length<key_extractor_tuple>::value,
    boost::tuples::element<N,key_extractor_tuple>,
    boost::mpl::identity<boost::tuples::null_type>
  >::type                                            type;
};

/* nth_composite_key_##name<CompositeKey,N>::type yields
 * functor<nth_key_from_value<CompositeKey,N> >, or tuples::null_type
 * if N exceeds the length of the composite key.
 */

#define BOOST_MULTI_INDEX_CK_NTH_COMPOSITE_KEY_FUNCTOR(name,functor)         \
template<typename KeyFromValue>                                              \
struct BOOST_PP_CAT(key_,name)                                               \
{                                                                            \
  typedef functor<typename KeyFromValue::result_type> type;                  \
};                                                                           \
                                                                             \
template<>                                                                   \
struct BOOST_PP_CAT(key_,name)<boost::tuples::null_type>                            \
{                                                                            \
  typedef boost::tuples::null_type type;                                            \
};                                                                           \
                                                                             \
template<typename CompositeKey,int  N>                                       \
struct BOOST_PP_CAT(nth_composite_key_,name)                                 \
{                                                                            \
  typedef typename nth_key_from_value<CompositeKey,N>::type key_from_value;  \
  typedef typename BOOST_PP_CAT(key_,name)<key_from_value>::type type;       \
};

/* nth_composite_key_equal_to
 * nth_composite_key_less
 * nth_composite_key_greater
 * nth_composite_key_hash
 */

BOOST_MULTI_INDEX_CK_NTH_COMPOSITE_KEY_FUNCTOR(equal_to,std::equal_to)
BOOST_MULTI_INDEX_CK_NTH_COMPOSITE_KEY_FUNCTOR(less,std::less)
BOOST_MULTI_INDEX_CK_NTH_COMPOSITE_KEY_FUNCTOR(greater,std::greater)
BOOST_MULTI_INDEX_CK_NTH_COMPOSITE_KEY_FUNCTOR(hash,boost::hash)

/* used for defining equality and comparison ops of composite_key_result */

#define BOOST_MULTI_INDEX_CK_IDENTITY_ENUM_MACRO(z,n,text) text

struct generic_operator_equal
{
  template<typename T,typename Q>
  bool operator()(const T& x,const Q& y)const{return x==y;}
};

typedef boost::tuple<
  BOOST_MULTI_INDEX_CK_ENUM(
    BOOST_MULTI_INDEX_CK_IDENTITY_ENUM_MACRO,
    detail::generic_operator_equal)>          generic_operator_equal_tuple;

struct generic_operator_less
{
  template<typename T,typename Q>
  bool operator()(const T& x,const Q& y)const{return x<y;}
};

typedef boost::tuple<
  BOOST_MULTI_INDEX_CK_ENUM(
    BOOST_MULTI_INDEX_CK_IDENTITY_ENUM_MACRO,
    detail::generic_operator_less)>           generic_operator_less_tuple;

/* Metaprogramming machinery for implementing equality, comparison and
 * hashing operations of composite_key_result.
 *
 * equal_* checks for equality between composite_key_results and
 * between those and tuples, accepting a tuple of basic equality functors.
 * compare_* does lexicographical comparison.
 * hash_* computes a combination of elementwise hash values.
 */

template< typename KeyCons1, typename KeyCons2, typename EqualCons >
struct equal_key_key; /* fwd decl. */

template< typename KeyCons1, typename KeyCons2, typename EqualCons >
struct equal_key_key_terminal
{
   static bool compare(
      const KeyCons1&,
      const KeyCons2&,
      const EqualCons& )
   {
      return true;
   }
};

template< typename KeyCons1, typename KeyCons2, typename EqualCons >
struct equal_key_key_normal
{
   static bool compare(
      const KeyCons1& k1,
      const KeyCons2& k2,
      const EqualCons& eq )
   {
      if( !eq.get_head()( k1.get_head(), k2.get_head() ) ) return false;
      return equal_key_key<
         typename KeyCons1::tail_type,
         typename KeyCons2::tail_type,
         typename EqualCons::tail_type
      >::compare( k1.get_tail(), k2.get_tail(), eq.get_tail() );
   }
};

template< typename KeyCons1, typename KeyCons2, typename EqualCons >
struct equal_key_key:
   boost::mpl::if_<
      boost::mpl::or_<
         boost::is_same< KeyCons1, boost::tuples::null_type >,
         boost::is_same< KeyCons2, boost::tuples::null_type >
      >,
      equal_key_key_terminal< KeyCons1, KeyCons2, EqualCons >,
      equal_key_key_normal< KeyCons1, KeyCons2 ,EqualCons >
   >::type
{};

template< typename KeyCons1, typename KeyCons2, typename CompareCons >
struct compare_key_key; /* fwd decl. */

template< typename KeyCons1, typename KeyCons2, typename CompareCons >
struct compare_key_key_terminal
{
   static bool compare( const KeyCons1&, const KeyCons2&, const CompareCons& )
   {
      return false;
   }
};

template< typename KeyCons1, typename KeyCons2, typename CompareCons >
struct compare_key_key_normal
{
   static bool compare(
      const KeyCons1& k1,
      const KeyCons2& k2,
      const CompareCons& comp)
   {
      if(comp.get_head()(k1.get_head(),k2.get_head()))return true;
      if(comp.get_head()(k2.get_head(),k1.get_head()))return false;
      return compare_key_key<
         typename KeyCons1::tail_type,
         typename KeyCons2::tail_type,
         typename CompareCons::tail_type
      >::compare(k1.get_tail(),k2.get_tail(),comp.get_tail());
   }
};

template< typename KeyCons1, typename KeyCons2, typename CompareCons >
struct compare_key_key:
   boost::mpl::if_<
      boost::mpl::or_<
         boost::is_same<KeyCons1,boost::tuples::null_type>,
         boost::is_same<KeyCons2,boost::tuples::null_type>
      >,
      compare_key_key_terminal< KeyCons1, KeyCons2, CompareCons >,
      compare_key_key_normal< KeyCons1, KeyCons2, CompareCons >
   >::type
{};

template<typename KeyCons,typename Value,typename HashCons>
struct hash_ckey; /* fwd decl. */

template<typename KeyCons,typename Value,typename HashCons>
struct hash_ckey_terminal
{
  static std::size_t hash(
    const KeyCons&,const Value&,const HashCons&,std::size_t carry)
  {
    return carry;
  }
};

template<typename KeyCons,typename Value,typename HashCons>
struct hash_ckey_normal
{
  static std::size_t hash(
    const KeyCons& c,const Value& v,const HashCons& h,std::size_t carry=0)
  {
    /* same hashing formula as boost::hash_combine */

    carry^=h.get_head()(c.get_head()(v))+0x9e3779b9+(carry<<6)+(carry>>2);
    return hash_ckey<
      BOOST_DEDUCED_TYPENAME KeyCons::tail_type,Value,
      BOOST_DEDUCED_TYPENAME HashCons::tail_type
    >::hash(c.get_tail(),v,h.get_tail(),carry);
  }
};

template<typename KeyCons,typename Value,typename HashCons>
struct hash_ckey:
  boost::mpl::if_<
    boost::is_same<KeyCons,boost::tuples::null_type>,
    hash_ckey_terminal<KeyCons,Value,HashCons>,
    hash_ckey_normal<KeyCons,Value,HashCons>
  >::type
{
};

template<typename ValCons,typename HashCons>
struct hash_cval; /* fwd decl. */

template<typename ValCons,typename HashCons>
struct hash_cval_terminal
{
  static std::size_t hash(const ValCons&,const HashCons&,std::size_t carry)
  {
    return carry;
  }
};

template<typename ValCons,typename HashCons>
struct hash_cval_normal
{
  static std::size_t hash(
    const ValCons& vc,const HashCons& h,std::size_t carry=0)
  {
    carry^=h.get_head()(vc.get_head())+0x9e3779b9+(carry<<6)+(carry>>2);
    return hash_cval<
      BOOST_DEDUCED_TYPENAME ValCons::tail_type,
      BOOST_DEDUCED_TYPENAME HashCons::tail_type
    >::hash(vc.get_tail(),h.get_tail(),carry);
  }
};

template<typename ValCons,typename HashCons>
struct hash_cval:
  boost::mpl::if_<
    boost::is_same<ValCons,boost::tuples::null_type>,
    hash_cval_terminal<ValCons,HashCons>,
    hash_cval_normal<ValCons,HashCons>
  >::type
{
};

} /* namespace multi_index::detail */

/* composite_key_result */

#if defined(BOOST_MSVC)
#pragma warning(push)
#pragma warning(disable:4512)
#endif

template< typename KeyExtractor, typename Key, typename Value >
struct key_from_value; // Forward Declaration

template< typename KeyExtractor, typename Key, typename Value >
struct key_from_value_terminal
{
   static Key extract( const KeyExtractor& e, const Value& v )
   {
      return boost::tuples::null_type();
   }
};

template< typename KeyExtractor, typename Key, typename Value >
struct key_from_value_normal
{
   static Key extract( const KeyExtractor& e, const Value& v )
   {
      return Key(
         e.get_head()( v ),
         key_from_value<
            typename KeyExtractor::tail_type,
            typename Key::tail_type,
            Value
         >::extract( e.get_tail(), v ) );
   }
};

template< typename KeyExtractor, typename Key, typename Value >
struct key_from_value :
   boost::mpl::if_<
      boost::is_same< KeyExtractor, boost::tuples::null_type >,
      key_from_value_terminal< KeyExtractor, Key, Value >,
      key_from_value_normal< KeyExtractor, Key, Value >
      >::type
{};

template< typename Key, typename Tuple >
struct convert_compatible_key;

template< typename Key, typename Tuple >
struct convert_compatible_key_terminal
{
   static Key convert( const Tuple& t )
   {
      return Key(
         t.get_head(),
         typename Key::tail_type() );
   }
};

template< typename Key, typename Tuple >
struct convert_compatible_key_normal
{
   static Key convert( const Tuple& t )
   {
      return Key(
         t.get_head(),
         convert_compatible_key<
            typename Key::tail_type,
            typename Tuple::tail_type
         >::convert( t.get_tail() ) );
   }
};

template< typename Key, typename Tuple >
struct convert_compatible_key :
   boost::mpl::if_<
      boost::mpl::or_<
         boost::is_same< typename Key::tail_type, boost::tuples::null_type >,
         boost::is_same< typename Tuple::tail_type, boost::tuples::null_type >
      >,
      convert_compatible_key_terminal< Key, Tuple >,
      convert_compatible_key_normal< Key, Tuple >
   >::type
{};

/*
template< typename Key, typename CompatibleKey >
struct convert_compatible_key;

template< typename Key, typename CompKeyHT >
struct convert_compatible_key< Key, boost::tuples::cons< CompKeyHT, boost::tuples::null_type > >
{
   static Key convert( const boost::tuples::cons< CompKeyHT, boost::tuples::null_type >& k )
   {
      return Key( k.get_head(), typename Key::tail_type() );
   }
};

template< typename Key, typename CompKeyHT, typename CompKeyTT >
struct convert_compatible_key< Key, boost::tuples::cons< CompKeyHT, CompKeyTT > >
{
   static Key convert( const boost::tuples::cons< CompKeyHT, CompKeyTT >& k )
   {
      return Key( k.get_head(), k.get_tail() );
   }
};

template< typename Key, typename... Args >
struct convert_compatible_key< Key, boost::tuples::tuple< Args... > >
{
   static Key convert( const boost::tuples::tuple< Args... >& k )
   {
      return convert_to_cons< Key, boost::tuples::tuple< Args ... > >::convert( k );
   }
};

template< typename Key, typename CompatibleKey >
struct convert_compatible_key
{
   static Key convert( const CompatibleKey& k )
   {
      return Key( k, typename Key::tail_type() );
   }
};
*/

template< typename KeyCons >
struct composite_key_conversion; // Forward Declaration

template< typename HT, typename TT >
struct result_type_extractor
{
   typedef typename boost::tuples::cons<
      typename HT::result_type,
      typename composite_key_conversion< TT >::type
   > type;
};

template< typename HT >
struct result_type_extractor< HT, boost::tuples::null_type >
{
   typedef typename boost::tuples::cons<
      typename HT::result_type,
      boost::tuples::null_type
   > type;
};

template< typename KeyCons >
struct composite_key_conversion
{
   typedef typename result_type_extractor<
      typename KeyCons::head_type,
      typename KeyCons::tail_type
   >::type type;
};

template<typename CompositeKey>
struct composite_key_result
{
   typedef CompositeKey                                     composite_key_type;
   typedef typename composite_key_conversion<
         typename composite_key_type::key_extractor_tuple
      >::type                                               key_type;
   typedef typename composite_key_type::value_type          value_type;

   explicit composite_key_result( const composite_key_type& composite_key, const value_type& value ) :
      key(
         key_from_value<
            typename composite_key_type::key_extractor_tuple,
            key_type,
            value_type
         >::extract( composite_key.key_extractors(), value ) )
   {}

   template< typename... Args >
   explicit composite_key_result( const boost::tuples::tuple< Args... > compat_key ) :
      key( convert_compatible_key<
         key_type,
         boost::tuples::tuple< Args... >
         >::convert( compat_key ) )
   {}

   template< typename CompatibleKey >
   explicit composite_key_result( const CompatibleKey& k ) : composite_key_result( boost::make_tuple( k ) )
   {}

   composite_key_result() {}

   key_type key;
};

#if defined(BOOST_MSVC)
#pragma warning(pop)
#endif

/* composite_key */

using namespace boost;

template<
  typename Value,
  BOOST_MULTI_INDEX_CK_ENUM(BOOST_MULTI_INDEX_CK_TEMPLATE_PARM,KeyFromValue)
>
struct composite_key:
  private tuple<BOOST_MULTI_INDEX_CK_ENUM_PARAMS(KeyFromValue)>
{
private:
  typedef boost::tuple<BOOST_MULTI_INDEX_CK_ENUM_PARAMS(KeyFromValue)> super;

public:
  typedef super                               key_extractor_tuple;
  typedef Value                               value_type;
  typedef composite_key_result<composite_key> result_type;

  composite_key(
    BOOST_MULTI_INDEX_CK_ENUM(BOOST_MULTI_INDEX_CK_CTOR_ARG,KeyFromValue)):
    super(BOOST_MULTI_INDEX_CK_ENUM_PARAMS(k))
  {}

  composite_key(const key_extractor_tuple& x):super(x){}

  const key_extractor_tuple& key_extractors()const{return *this;}
  key_extractor_tuple&       key_extractors(){return *this;}

  template<typename ChainedPtr>

#if !defined(BOOST_NO_SFINAE)
  typename disable_if<
    is_convertible<const ChainedPtr&,const value_type&>,result_type>::type
#else
  result_type
#endif

  operator()(const ChainedPtr& x)const
  {
    return operator()(*x);
  }

  result_type operator()(const value_type& x)const
  {
    return result_type( *this, x );
  }

  result_type operator()(const reference_wrapper<const value_type>& x)const
  {
    return result_type( *this, x.get() );
  }

  result_type operator()(const reference_wrapper<value_type>& x)const
  {
    return result_type( *this, x.get() );
  }
};

/* comparison operators */

/* == */

template< typename CompositeKey1, typename CompositeKey2 >
inline bool operator==(
   const composite_key_result< CompositeKey1 >& x,
   const composite_key_result< CompositeKey2 >& y)
{
  typedef typename CompositeKey1::key_extractor_tuple key_extractor_tuple1;
  typedef typename CompositeKey2::key_extractor_tuple key_extractor_tuple2;

  BOOST_STATIC_ASSERT(
    boost::tuples::length<key_extractor_tuple1>::value==
    boost::tuples::length<key_extractor_tuple2>::value);

  return detail::equal_key_key<
    typename CompositeKey1::result_type::key_type,
    typename CompositeKey2::result_type::key_type,
    detail::generic_operator_equal_tuple
  >::compare(
    x.key,
    y.key,
    detail::generic_operator_equal_tuple());
}

template<
  typename CompositeKey,
  BOOST_MULTI_INDEX_CK_ENUM_PARAMS( typename Value )
>
inline bool operator==(
   const composite_key_result< CompositeKey >& x,
   const tuple< BOOST_MULTI_INDEX_CK_ENUM_PARAMS( Value ) >& y)
{
   typedef typename CompositeKey::key_extractor_tuple    key_extractor_tuple;
   typedef boost::tuple<
      BOOST_MULTI_INDEX_CK_ENUM_PARAMS( Value ) >        key_tuple;

   BOOST_STATIC_ASSERT(
      boost::tuples::length<key_extractor_tuple>::value==
      boost::tuples::length<key_tuple>::value);

   return detail::equal_key_key<
      typename CompositeKey::result_type::key_type,
      key_tuple,
      detail::generic_operator_equal_tuple
   >::compare(
      x.key, y, detail::generic_operator_equal_tuple() );
}

template
<
  BOOST_MULTI_INDEX_CK_ENUM_PARAMS( typename Value ),
  typename CompositeKey
>
inline bool operator==(
   const tuple< BOOST_MULTI_INDEX_CK_ENUM_PARAMS( Value ) >& x,
   const composite_key_result< CompositeKey >& y)
{
   typedef typename CompositeKey::key_extractor_tuple     key_extractor_tuple;
   typedef boost::tuple<BOOST_MULTI_INDEX_CK_ENUM_PARAMS(Value)> key_tuple;

   BOOST_STATIC_ASSERT(
      boost::tuples::length<key_extractor_tuple>::value==
      boost::tuples::length<key_tuple>::value);

   return detail::equal_key_key<
      key_tuple,
      typename CompositeKey::result_type::key_type,
      detail::generic_operator_equal_tuple
   >::compare(
      x, y.key, detail::generic_operator_equal_tuple() );
}

#if !defined(BOOST_NO_CXX11_HDR_TUPLE)&&\
    !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
template< typename CompositeKey, typename... Values >
inline bool operator==(
   const composite_key_result< CompositeKey >& x,
   const std::tuple< Values... >& y)
{
   typedef typename CompositeKey::key_extractor_tuple key_extractor_tuple;
   typedef std::tuple<Values...>                      key_tuple;
   typedef typename detail::cons_stdtuple_ctor<
      key_tuple>::result_type                          cons_key_tuple;

   BOOST_STATIC_ASSERT(
      static_cast<std::size_t>(boost::tuples::length<key_extractor_tuple>::value)==
      std::tuple_size<key_tuple>::value);

   return detail::equal_key_key<
      typename CompositeKey::result_type::key_type,
      cons_key_tuple,
      detail::generic_operator_equal_tuple
   >::compare(
      x.key, detail::make_cons_stdtuple(y), detail::generic_operator_equal_tuple() );
}

template< typename CompositeKey, typename... Values >
inline bool operator==(
   const std::tuple< Values... >& x,
   const composite_key_result< CompositeKey >& y)
{
   typedef typename CompositeKey::key_extractor_tuple key_extractor_tuple;
   typedef std::tuple<Values...>                      key_tuple;
   typedef typename detail::cons_stdtuple_ctor<
      key_tuple>::result_type                         cons_key_tuple;

   BOOST_STATIC_ASSERT(
      static_cast<std::size_t>(boost::tuples::length<key_extractor_tuple>::value)==
      std::tuple_size<key_tuple>::value);

   return detail::equal_key_key<
      cons_key_tuple,
      typename CompositeKey::result_type::key_type,
      detail::generic_operator_equal_tuple
   >::compare(
      detail::make_cons_stdtuple( x ),
      y.key,
      detail::generic_operator_equal_tuple() );
}
#endif

/* < */

template<typename CompositeKey1,typename CompositeKey2>
inline bool operator<(
  const composite_key_result<CompositeKey1>& x,
  const composite_key_result<CompositeKey2>& y)
{
   return detail::compare_key_key<
      typename CompositeKey1::result_type::key_type,
      typename CompositeKey2::result_type::key_type,
      detail::generic_operator_less_tuple
   >::compare(
      x.key,
      y.key,
      detail::generic_operator_less_tuple());
}

template<typename CompositeKey,typename Value>
inline bool operator<(
   const composite_key_result<CompositeKey>& x,
   const Value& y)
{
   return x < boost::make_tuple( boost::cref( y ) );
}

template
<
   typename CompositeKey,
   BOOST_MULTI_INDEX_CK_ENUM_PARAMS( typename Value )
>
inline bool operator<(
   const composite_key_result< CompositeKey >& x,
   const tuple< BOOST_MULTI_INDEX_CK_ENUM_PARAMS( Value ) >& y )
{
   typedef boost::tuple< BOOST_MULTI_INDEX_CK_ENUM_PARAMS( Value ) > key_tuple;

   return detail::compare_key_key<
      typename CompositeKey::result_type::key_type,
      key_tuple,
      detail::generic_operator_less_tuple
   >::compare(
      x.key, y, detail::generic_operator_less_tuple() );
}

template
<
   BOOST_MULTI_INDEX_CK_ENUM_PARAMS(typename Value),
   typename CompositeKey
>
inline bool operator<(
   const tuple< BOOST_MULTI_INDEX_CK_ENUM_PARAMS( Value ) >& x,
   const composite_key_result< CompositeKey >& y)
{
   typedef boost::tuple<BOOST_MULTI_INDEX_CK_ENUM_PARAMS(Value)> key_tuple;

   return detail::compare_key_key<
      key_tuple,
      typename CompositeKey::result_type::key_type,
      detail::generic_operator_less_tuple
   >::compare(
      x, y.key, detail::generic_operator_less_tuple() );
}

#if !defined(BOOST_NO_CXX11_HDR_TUPLE)&&\
    !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
template<typename CompositeKey,typename... Values>
inline bool operator<(
   const composite_key_result<CompositeKey>& x,
   const std::tuple<Values...>& y)
{
   typedef std::tuple<Values...>                      key_tuple;
   typedef typename detail::cons_stdtuple_ctor<
      key_tuple>::result_type                          cons_key_tuple;

   return detail::compare_key_key<
      typename CompositeKey::result_type::key_type,
      cons_key_tuple,
      detail::generic_operator_less_tuple
   >::compare(
      x.key,
      detail::make_cons_stdtuple( y ),
      detail::generic_operator_less_tuple() );
}

template< typename CompositeKey, typename... Values >
inline bool operator<(
   const std::tuple< Values... >& x,
   const composite_key_result< CompositeKey >& y)
{
   typedef std::tuple<Values...>                      key_tuple;
   typedef typename detail::cons_stdtuple_ctor<
      key_tuple>::result_type                          cons_key_tuple;

   return detail::compare_key_key<
      cons_key_tuple,
      typename CompositeKey::result_type::key_type,
      detail::generic_operator_less_tuple
   >::compare(
      detail::make_cons_stdtuple( x ),
      y.key,
      detail::generic_operator_less_tuple() );
}
#endif

/* rest of comparison operators */

#define BOOST_MULTI_INDEX_CK_COMPLETE_COMP_OPS(t1,t2,a1,a2)                  \
template<t1,t2> inline bool operator!=(const a1& x,const a2& y)              \
{                                                                            \
  return !(x==y);                                                            \
}                                                                            \
                                                                             \
template<t1,t2> inline bool operator>(const a1& x,const a2& y)               \
{                                                                            \
  return y<x;                                                                \
}                                                                            \
                                                                             \
template<t1,t2> inline bool operator>=(const a1& x,const a2& y)              \
{                                                                            \
  return !(x<y);                                                             \
}                                                                            \
                                                                             \
template<t1,t2> inline bool operator<=(const a1& x,const a2& y)              \
{                                                                            \
  return !(y<x);                                                             \
}

BOOST_MULTI_INDEX_CK_COMPLETE_COMP_OPS(
  typename CompositeKey1,
  typename CompositeKey2,
  composite_key_result<CompositeKey1>,
  composite_key_result<CompositeKey2>
)

BOOST_MULTI_INDEX_CK_COMPLETE_COMP_OPS(
  typename CompositeKey,
  BOOST_MULTI_INDEX_CK_ENUM_PARAMS(typename Value),
  composite_key_result<CompositeKey>,
  tuple<BOOST_MULTI_INDEX_CK_ENUM_PARAMS(Value)>
)

BOOST_MULTI_INDEX_CK_COMPLETE_COMP_OPS(
  BOOST_MULTI_INDEX_CK_ENUM_PARAMS(typename Value),
  typename CompositeKey,
  tuple<BOOST_MULTI_INDEX_CK_ENUM_PARAMS(Value)>,
  composite_key_result<CompositeKey>
)

#if !defined(BOOST_NO_CXX11_HDR_TUPLE)&&\
    !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
BOOST_MULTI_INDEX_CK_COMPLETE_COMP_OPS(
  typename CompositeKey,
  typename... Values,
  composite_key_result<CompositeKey>,
  std::tuple<Values...>
)

BOOST_MULTI_INDEX_CK_COMPLETE_COMP_OPS(
  typename CompositeKey,
  typename... Values,
  std::tuple<Values...>,
  composite_key_result<CompositeKey>
)
#endif

/* composite_key_equal_to */

template
<
  BOOST_MULTI_INDEX_CK_ENUM(BOOST_MULTI_INDEX_CK_TEMPLATE_PARM,Pred)
>
struct composite_key_equal_to:
  private tuple<BOOST_MULTI_INDEX_CK_ENUM_PARAMS(Pred)>
{
private:
  typedef boost::tuple<BOOST_MULTI_INDEX_CK_ENUM_PARAMS(Pred)> super;

public:
  typedef super key_eq_tuple;

  composite_key_equal_to(
    BOOST_MULTI_INDEX_CK_ENUM(BOOST_MULTI_INDEX_CK_CTOR_ARG,Pred)):
    super(BOOST_MULTI_INDEX_CK_ENUM_PARAMS(k))
  {}

  composite_key_equal_to(const key_eq_tuple& x):super(x){}

  const key_eq_tuple& key_eqs()const{return *this;}
  key_eq_tuple&       key_eqs(){return *this;}

   template<typename CompositeKey1,typename CompositeKey2>
   bool operator()(
      const composite_key_result<CompositeKey1> & x,
      const composite_key_result<CompositeKey2> & y)const
   {
      typedef typename CompositeKey1::key_extractor_tuple key_extractor_tuple1;
      typedef typename CompositeKey2::key_extractor_tuple key_extractor_tuple2;

      BOOST_STATIC_ASSERT(
         boost::tuples::length<key_extractor_tuple1>::value<=
         boost::tuples::length<key_eq_tuple>::value&&
         boost::tuples::length<key_extractor_tuple1>::value==
         boost::tuples::length<key_extractor_tuple2>::value);

      return detail::equal_key_key<
         typename CompositeKey1::result_type::key_type,
         typename CompositeKey2::result_type::key_type,
         detail::generic_operator_less_tuple
      >::compare(
         x.key,
         y.key,
         key_eqs());
   }

   template
   <
      typename CompositeKey,
      BOOST_MULTI_INDEX_CK_ENUM_PARAMS( typename Value )
   >
   bool operator()(
      const composite_key_result< CompositeKey >& x,
      const tuple< BOOST_MULTI_INDEX_CK_ENUM_PARAMS( Value ) >& y)const
   {
      typedef typename CompositeKey::key_extractor_tuple     key_extractor_tuple;
      typedef boost::tuple<BOOST_MULTI_INDEX_CK_ENUM_PARAMS(Value)> key_tuple;

      BOOST_STATIC_ASSERT(
         boost::tuples::length<key_extractor_tuple>::value<=
         boost::tuples::length<key_eq_tuple>::value&&
         boost::tuples::length<key_extractor_tuple>::value==
         boost::tuples::length<key_tuple>::value);

      return detail::equal_key_key<
         typename CompositeKey::result_type::key_type,
         key_tuple,
         key_eq_tuple
      >::compare( x.key, y, key_eqs() );
   }

   template
   <
      BOOST_MULTI_INDEX_CK_ENUM_PARAMS(typename Value),
      typename CompositeKey
   >
   bool operator()(
      const tuple< BOOST_MULTI_INDEX_CK_ENUM_PARAMS( Value ) >& x,
      const composite_key_result< CompositeKey >& y)const
   {
      typedef typename CompositeKey::key_extractor_tuple     key_extractor_tuple;
      typedef boost::tuple<BOOST_MULTI_INDEX_CK_ENUM_PARAMS(Value)> key_tuple;

      BOOST_STATIC_ASSERT(
         boost::tuples::length<key_tuple>::value<=
         boost::tuples::length<key_eq_tuple>::value&&
         boost::tuples::length<key_tuple>::value==
         boost::tuples::length<key_extractor_tuple>::value);

      return detail::equal_key_key<
         key_tuple,
         typename CompositeKey::result_type::key_type,
         key_eq_tuple
      >::compare( x, y.key, key_eqs() );
   }

#if !defined(BOOST_NO_CXX11_HDR_TUPLE)&&\
    !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
   template< typename CompositeKey, typename... Values >
   bool operator()(
      const composite_key_result< CompositeKey >& x,
      const std::tuple< Values... >& y )const
   {
      typedef typename CompositeKey::key_extractor_tuple key_extractor_tuple;
      typedef std::tuple<Values...>                      key_tuple;
      typedef typename detail::cons_stdtuple_ctor<
         key_tuple>::result_type                          cons_key_tuple;

      BOOST_STATIC_ASSERT(
         boost::tuples::length<key_extractor_tuple>::value<=
         boost::tuples::length<key_eq_tuple>::value&&
         static_cast<std::size_t>(boost::tuples::length<key_extractor_tuple>::value)==
         std::tuple_size<key_tuple>::value);

      return detail::equal_key_key<
         typename CompositeKey::result_type::key_type,
         cons_key_tuple,
         key_eq_tuple
      >::compare(
         x.key, detail::make_cons_stdtuple( y ), key_eqs() );
   }

   template< typename CompositeKey, typename... Values >
   bool operator()(
      const std::tuple< Values... >& x,
      const composite_key_result< CompositeKey >& y)const
   {
      typedef typename CompositeKey::key_extractor_tuple key_extractor_tuple;
      typedef std::tuple<Values...>                      key_tuple;
      typedef typename detail::cons_stdtuple_ctor<
         key_tuple>::result_type                          cons_key_tuple;

      BOOST_STATIC_ASSERT(
         std::tuple_size<key_tuple>::value<=
         static_cast<std::size_t>(boost::tuples::length<key_eq_tuple>::value)&&
         std::tuple_size<key_tuple>::value==
         static_cast<std::size_t>(boost::tuples::length<key_extractor_tuple>::value));

      return detail::equal_key_key<
         cons_key_tuple,
         typename CompositeKey::result_type::key_type,
         key_eq_tuple
      >::compare(
         detail::make_cons_stdtuple( x ), y.key, key_eqs() );
   }
#endif
};

/* composite_key_compare */

template
<
  BOOST_MULTI_INDEX_CK_ENUM(BOOST_MULTI_INDEX_CK_TEMPLATE_PARM,Compare)
>
struct composite_key_compare:
  private tuple<BOOST_MULTI_INDEX_CK_ENUM_PARAMS(Compare)>
{
private:
  typedef boost::tuple<BOOST_MULTI_INDEX_CK_ENUM_PARAMS(Compare)> super;

public:
  typedef super key_comp_tuple;

  composite_key_compare(
    BOOST_MULTI_INDEX_CK_ENUM(BOOST_MULTI_INDEX_CK_CTOR_ARG,Compare)):
    super(BOOST_MULTI_INDEX_CK_ENUM_PARAMS(k))
  {}

  composite_key_compare(const key_comp_tuple& x):super(x){}

  const key_comp_tuple& key_comps()const{return *this;}
  key_comp_tuple&       key_comps(){return *this;}

  template<typename CompositeKey1,typename CompositeKey2>
  bool operator()(
    const composite_key_result<CompositeKey1> & x,
    const composite_key_result<CompositeKey2> & y)const
  {
    typedef typename CompositeKey1::key_extractor_tuple key_extractor_tuple1;
    typedef typename CompositeKey2::key_extractor_tuple key_extractor_tuple2;

    BOOST_STATIC_ASSERT(
      boost::tuples::length<key_extractor_tuple1>::value<=
      boost::tuples::length<key_comp_tuple>::value||
      boost::tuples::length<key_extractor_tuple2>::value<=
      boost::tuples::length<key_comp_tuple>::value);

    return detail::compare_key_key<
      typename CompositeKey1::result_type::key_type,
      typename CompositeKey2::result_type::key_type,
      key_comp_tuple
    >::compare(
      x.key,
      y.key,
      key_comps());
  }

#if !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING)
  template<typename CompositeKey,typename Value>
  bool operator()(
    const composite_key_result<CompositeKey>& x,
    const Value& y)const
  {
    return operator()(x,boost::make_tuple(boost::cref(y)));
  }
#endif

   template
   <
      typename CompositeKey,
      BOOST_MULTI_INDEX_CK_ENUM_PARAMS(typename Value)
   >
   bool operator()(
      const composite_key_result< CompositeKey >& x,
      const tuple< BOOST_MULTI_INDEX_CK_ENUM_PARAMS( Value ) >& y )const
   {
      typedef typename CompositeKey::key_extractor_tuple     key_extractor_tuple;
      typedef boost::tuple<BOOST_MULTI_INDEX_CK_ENUM_PARAMS(Value)> key_tuple;

      BOOST_STATIC_ASSERT(
         boost::tuples::length<key_extractor_tuple>::value<=
         boost::tuples::length<key_comp_tuple>::value||
         boost::tuples::length<key_tuple>::value<=
         boost::tuples::length<key_comp_tuple>::value);

      return detail::compare_key_key<
         typename CompositeKey::result_type::key_type,
         key_tuple,
         key_comp_tuple
      >::compare( x.key, y, key_comps() );
   }

#if !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING)
  template<typename Value,typename CompositeKey>
  bool operator()(
    const Value& x,
    const composite_key_result<CompositeKey>& y)const
  {
    return operator()(boost::make_tuple(boost::cref(x)),y);
  }
#endif

   template
   <
      BOOST_MULTI_INDEX_CK_ENUM_PARAMS( typename Value ),
      typename CompositeKey
   >
   bool operator()(
      const tuple< BOOST_MULTI_INDEX_CK_ENUM_PARAMS( Value ) >& x,
      const composite_key_result< CompositeKey >& y )const
   {
      typedef typename CompositeKey::key_extractor_tuple     key_extractor_tuple;
      typedef boost::tuple<BOOST_MULTI_INDEX_CK_ENUM_PARAMS(Value)> key_tuple;

      BOOST_STATIC_ASSERT(
         boost::tuples::length<key_tuple>::value<=
         boost::tuples::length<key_comp_tuple>::value||
         boost::tuples::length<key_extractor_tuple>::value<=
         boost::tuples::length<key_comp_tuple>::value);

      return detail::compare_key_key<
         key_tuple,
         typename CompositeKey::result_type::key_type,
         key_comp_tuple
      >::compare( x, y.key, key_comps() );
   }

#if !defined(BOOST_NO_CXX11_HDR_TUPLE)&&\
    !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
   template< typename CompositeKey, typename... Values >
   bool operator()(
      const composite_key_result< CompositeKey >& x,
      const std::tuple< Values... >& y )const
   {
      typedef typename CompositeKey::key_extractor_tuple key_extractor_tuple;
      typedef std::tuple<Values...>                      key_tuple;
      typedef typename detail::cons_stdtuple_ctor<
         key_tuple>::result_type                          cons_key_tuple;

      BOOST_STATIC_ASSERT(
         boost::tuples::length<key_extractor_tuple>::value<=
         boost::tuples::length<key_comp_tuple>::value||
         std::tuple_size<key_tuple>::value<=
         static_cast<std::size_t>(boost::tuples::length<key_comp_tuple>::value));

      return detail::compare_key_key<
         typename CompositeKey::result_type::key_type,
         cons_key_tuple,
         key_comp_tuple
      >::compare(
         x.key,
         detail::make_cons_stdtuple(y),
         key_comps() );
   }

   template< typename CompositeKey, typename... Values >
   bool operator()(
      const std::tuple< Values... >& x,
      const composite_key_result< CompositeKey >& y )const
   {
      typedef typename CompositeKey::key_extractor_tuple key_extractor_tuple;
      typedef std::tuple<Values...>                      key_tuple;
      typedef typename detail::cons_stdtuple_ctor<
         key_tuple>::result_type                          cons_key_tuple;

      BOOST_STATIC_ASSERT(
         std::tuple_size<key_tuple>::value<=
         static_cast<std::size_t>(boost::tuples::length<key_comp_tuple>::value)||
         boost::tuples::length<key_extractor_tuple>::value<=
         boost::tuples::length<key_comp_tuple>::value);

      return detail::compare_key_key<
         cons_key_tuple,
         typename CompositeKey::result_type::key_type,
         key_comp_tuple
      >::compare(
         detail::make_cons_stdtuple(x),
         y.key,
         y.value,key_comps() );
   }
#endif
};

/* composite_key_hash */

template
<
  BOOST_MULTI_INDEX_CK_ENUM(BOOST_MULTI_INDEX_CK_TEMPLATE_PARM,Hash)
>
struct composite_key_hash:
  private tuple<BOOST_MULTI_INDEX_CK_ENUM_PARAMS(Hash)>
{
private:
  typedef boost::tuple<BOOST_MULTI_INDEX_CK_ENUM_PARAMS(Hash)> super;

public:
  typedef super key_hasher_tuple;

  composite_key_hash(
    BOOST_MULTI_INDEX_CK_ENUM(BOOST_MULTI_INDEX_CK_CTOR_ARG,Hash)):
    super(BOOST_MULTI_INDEX_CK_ENUM_PARAMS(k))
  {}

  composite_key_hash(const key_hasher_tuple& x):super(x){}

  const key_hasher_tuple& key_hash_functions()const{return *this;}
  key_hasher_tuple&       key_hash_functions(){return *this;}

  template<typename CompositeKey>
  std::size_t operator()(const composite_key_result<CompositeKey> & x)const
  {
    typedef typename CompositeKey::key_extractor_tuple key_extractor_tuple;
    typedef typename CompositeKey::value_type          value_type;

    BOOST_STATIC_ASSERT(
      boost::tuples::length<key_extractor_tuple>::value==
      boost::tuples::length<key_hasher_tuple>::value);

    return detail::hash_ckey<
      key_extractor_tuple,value_type,
      key_hasher_tuple
    >::hash(x.composite_key.key_extractors(),x.value,key_hash_functions());
  }

  template<BOOST_MULTI_INDEX_CK_ENUM_PARAMS(typename Value)>
  std::size_t operator()(
    const tuple<BOOST_MULTI_INDEX_CK_ENUM_PARAMS(Value)>& x)const
  {
    typedef boost::tuple<BOOST_MULTI_INDEX_CK_ENUM_PARAMS(Value)> key_tuple;

    BOOST_STATIC_ASSERT(
      boost::tuples::length<key_tuple>::value==
      boost::tuples::length<key_hasher_tuple>::value);

    return detail::hash_cval<
      key_tuple,key_hasher_tuple
    >::hash(x,key_hash_functions());
  }

#if !defined(BOOST_NO_CXX11_HDR_TUPLE)&&\
    !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
  template<typename... Values>
  std::size_t operator()(const std::tuple<Values...>& x)const
  {
    typedef std::tuple<Values...>                key_tuple;
    typedef typename detail::cons_stdtuple_ctor<
      key_tuple>::result_type                    cons_key_tuple;

    BOOST_STATIC_ASSERT(
      std::tuple_size<key_tuple>::value==
      static_cast<std::size_t>(boost::tuples::length<key_hasher_tuple>::value));

    return detail::hash_cval<
      cons_key_tuple,key_hasher_tuple
    >::hash(detail::make_cons_stdtuple(x),key_hash_functions());
  }
#endif
};

/* Instantiations of the former functors with "natural" basic components:
 * composite_key_result_equal_to uses std::equal_to of the values.
 * composite_key_result_less     uses std::less.
 * composite_key_result_greater  uses std::greater.
 * composite_key_result_hash     uses boost::hash.
 */

#define BOOST_MULTI_INDEX_CK_RESULT_EQUAL_TO_SUPER                           \
composite_key_equal_to<                                                      \
    BOOST_MULTI_INDEX_CK_ENUM(                                               \
      BOOST_MULTI_INDEX_CK_APPLY_METAFUNCTION_N,                             \
      /* the argument is a PP list */                                        \
      (detail::nth_composite_key_equal_to,                                   \
        (BOOST_DEDUCED_TYPENAME CompositeKeyResult::composite_key_type,      \
          BOOST_PP_NIL)))                                                    \
  >

template<typename CompositeKeyResult>
struct composite_key_result_equal_to:
BOOST_MULTI_INDEX_PRIVATE_IF_USING_DECL_FOR_TEMPL_FUNCTIONS
BOOST_MULTI_INDEX_CK_RESULT_EQUAL_TO_SUPER
{
private:
  typedef BOOST_MULTI_INDEX_CK_RESULT_EQUAL_TO_SUPER super;

public:
  typedef CompositeKeyResult  first_argument_type;
  typedef first_argument_type second_argument_type;
  typedef bool                result_type;

  using super::operator();
};

#define BOOST_MULTI_INDEX_CK_RESULT_LESS_SUPER                               \
composite_key_compare<                                                       \
    BOOST_MULTI_INDEX_CK_ENUM(                                               \
      BOOST_MULTI_INDEX_CK_APPLY_METAFUNCTION_N,                             \
      /* the argument is a PP list */                                        \
      (detail::nth_composite_key_less,                                       \
        (BOOST_DEDUCED_TYPENAME CompositeKeyResult::composite_key_type,      \
          BOOST_PP_NIL)))                                                    \
  >

template<typename CompositeKeyResult>
struct composite_key_result_less:
BOOST_MULTI_INDEX_PRIVATE_IF_USING_DECL_FOR_TEMPL_FUNCTIONS
BOOST_MULTI_INDEX_CK_RESULT_LESS_SUPER
{
private:
  typedef BOOST_MULTI_INDEX_CK_RESULT_LESS_SUPER super;

public:
  typedef CompositeKeyResult  first_argument_type;
  typedef first_argument_type second_argument_type;
  typedef bool                result_type;

  using super::operator();
};

#define BOOST_MULTI_INDEX_CK_RESULT_GREATER_SUPER                            \
composite_key_compare<                                                       \
    BOOST_MULTI_INDEX_CK_ENUM(                                               \
      BOOST_MULTI_INDEX_CK_APPLY_METAFUNCTION_N,                             \
      /* the argument is a PP list */                                        \
      (detail::nth_composite_key_greater,                                    \
        (BOOST_DEDUCED_TYPENAME CompositeKeyResult::composite_key_type,      \
          BOOST_PP_NIL)))                                                    \
  >

template<typename CompositeKeyResult>
struct composite_key_result_greater:
BOOST_MULTI_INDEX_PRIVATE_IF_USING_DECL_FOR_TEMPL_FUNCTIONS
BOOST_MULTI_INDEX_CK_RESULT_GREATER_SUPER
{
private:
  typedef BOOST_MULTI_INDEX_CK_RESULT_GREATER_SUPER super;

public:
  typedef CompositeKeyResult  first_argument_type;
  typedef first_argument_type second_argument_type;
  typedef bool                result_type;

  using super::operator();
};

#define BOOST_MULTI_INDEX_CK_RESULT_HASH_SUPER                               \
composite_key_hash<                                                          \
    BOOST_MULTI_INDEX_CK_ENUM(                                               \
      BOOST_MULTI_INDEX_CK_APPLY_METAFUNCTION_N,                             \
      /* the argument is a PP list */                                        \
      (detail::nth_composite_key_hash,                                       \
        (BOOST_DEDUCED_TYPENAME CompositeKeyResult::composite_key_type,      \
          BOOST_PP_NIL)))                                                    \
  >

template<typename CompositeKeyResult>
struct composite_key_result_hash:
BOOST_MULTI_INDEX_PRIVATE_IF_USING_DECL_FOR_TEMPL_FUNCTIONS
BOOST_MULTI_INDEX_CK_RESULT_HASH_SUPER
{
private:
  typedef BOOST_MULTI_INDEX_CK_RESULT_HASH_SUPER super;

public:
  typedef CompositeKeyResult argument_type;
  typedef std::size_t        result_type;

  using super::operator();
};

} /* namespace multi_index */
//*

template< typename T > struct is_static_length< multi_index::composite_key_result< T > >
   : public is_static_length< typename multi_index::composite_key_result< T >::key_type > {};
/*
template< typename T >
struct slice_packer< multi_index::composite_key_result< T > >
{
   static void pack( PinnableSlice& s , const T& t )
   {
      if( is_static_length< typename multi_index::composite_key_result< T >::key_type >::value )
      {
         static_packer< multi_index::composite_key_result< T >::key_type >::pack( s, t.key );
      }
      else
      {
         dynamic_packer< multi_index::composite_key_result< T >::key_type >::pack( s, t.key );
      }
   }

   static void unpack( PinnableSlice& s , const T& t )
   {
      if( is_static_length< typename multi_index::composite_key_result< T >::key_type >::value )
      {
         static_packer< multi_index::composite_key_result< T > >::unpack( s, t );
      }
      else
      {
         dynamic_packer< multi_index::composite_key_result< T > >::unpack( s, t );
      }
   }
};
*/
/*
template< typename T >
inline void pack_to_slice< multi_index::composite_key_result< T > >( PinnableSlice& s, const multi_index::composite_key_result< T >& t )
{

}

template< typename T >
inline void unpack_from_slice< multi_index::composite_key_result< T > >( const Slice& s, multi_index::composite_key_result< T >& t )
{
   if( is_static_length< typename multi_index::composite_key_result< T >::key_type >::value )
   {
      t = *(multi_index::composite_key_result< T >*)s.data();
   }
   else
   {
      fc::raw::unpack_from_char_array< multi_index::composite_key_result< T > >( s.data(), s.size(), t );
   }
}
//*/
} /* namespace mira */

/* Specializations of std::equal_to, std::less, std::greater and boost::hash
 * for composite_key_results enabling interoperation with tuples of values.
 */

namespace std{

template<typename CompositeKey>
struct equal_to<mira::multi_index::composite_key_result<CompositeKey> >:
  mira::multi_index::composite_key_result_equal_to<
    mira::multi_index::composite_key_result<CompositeKey>
  >
{
};

template<typename CompositeKey>
struct less<mira::multi_index::composite_key_result<CompositeKey> >:
  mira::multi_index::composite_key_result_less<
    mira::multi_index::composite_key_result<CompositeKey>
  >
{
};

template<typename CompositeKey>
struct greater<mira::multi_index::composite_key_result<CompositeKey> >:
  mira::multi_index::composite_key_result_greater<
    mira::multi_index::composite_key_result<CompositeKey>
  >
{
};

} /* namespace std */

namespace boost{

template<typename CompositeKey>
struct hash<mira::multi_index::composite_key_result<CompositeKey> >:
  mira::multi_index::composite_key_result_hash<
    mira::multi_index::composite_key_result<CompositeKey>
  >
{
};

} /* namespace boost */

#undef BOOST_MULTI_INDEX_CK_RESULT_HASH_SUPER
#undef BOOST_MULTI_INDEX_CK_RESULT_GREATER_SUPER
#undef BOOST_MULTI_INDEX_CK_RESULT_LESS_SUPER
#undef BOOST_MULTI_INDEX_CK_RESULT_EQUAL_TO_SUPER
#undef BOOST_MULTI_INDEX_CK_COMPLETE_COMP_OPS
#undef BOOST_MULTI_INDEX_CK_IDENTITY_ENUM_MACRO
#undef BOOST_MULTI_INDEX_CK_NTH_COMPOSITE_KEY_FUNCTOR
#undef BOOST_MULTI_INDEX_CK_APPLY_METAFUNCTION_N
#undef BOOST_MULTI_INDEX_CK_CTOR_ARG
#undef BOOST_MULTI_INDEX_CK_TEMPLATE_PARM
#undef BOOST_MULTI_INDEX_CK_ENUM_PARAMS
#undef BOOST_MULTI_INDEX_CK_ENUM
#undef BOOST_MULTI_INDEX_COMPOSITE_KEY_SIZE

namespace fc
{

class variant;

template< typename T >
void to_variant( const mira::multi_index::composite_key_result< T >& var,  variant& vo )
{
   to_variant( var.key, vo );
}

template< typename T >
void from_variant( const variant& vo, mira::multi_index::composite_key_result< T >& var )
{
   from_variant( vo, var.key );
}

template< typename Cons > struct cons_to_variant;

template< typename Cons >
struct cons_to_variant_terminal
{
   static void push_variant( const Cons& c, fc::variants& vars )
   {
      fc::variant v;
      to_variant( c.get_head(), v );
      vars.push_back( v );
   }
};

template< typename Cons >
struct cons_to_variant_normal
{
   static void push_variant( const Cons& c, fc::variants& vars )
   {
      fc::variant v;
      to_variant( c.get_head(), v );
      vars.push_back( v );
      cons_to_variant< typename Cons::tail_type >::push_variant( c.get_tail(), vars );
   }
};

template< typename Cons >
struct cons_to_variant :
   boost::mpl::if_<
      boost::is_same< typename Cons::tail_type, boost::tuples::null_type >,
      cons_to_variant_terminal< Cons >,
      cons_to_variant_normal< Cons >
   >::type
{};

template< typename HT, typename TT > void to_variant( const boost::tuples::cons< HT, TT >& var, variant& vo )
{
   fc::variants v;
   cons_to_variant< boost::tuples::cons< HT, TT > >::push_variant( var, v );
   vo = v;
}

template< typename T >
struct get_typename< mira::multi_index::composite_key_result< T > >
{
   static const char* name()
   {
      static std::string n = std::string( "mira::multi_index::composite_key_result<" )
         + get_typename< typename T::value_type >::name()
         + ">";
      return n.c_str();
   }
};

template <>
struct get_typename< boost::tuples::null_type >
{
   static const char* name()
   {
      static std::string n = std::string( "boost::tuples::null_type" );
      return n.c_str();
   }
};

template< typename H, typename T >
struct get_typename< boost::tuples::cons< H, T > >
{
   static const char* name()
   {
      static std::string n = std::string( "boost::tuples::cons<" )
         + get_typename< H >::name()
         + ","
         + get_typename< T >::name()
         + ">";
      return n.c_str();
   }
};

namespace raw
{

template< typename Stream, typename T >
void inline pack( Stream& s, const mira::multi_index::composite_key_result< T >& var )
{
   pack( s, var.key );
}

template<typename Stream, typename T>
void inline unpack( Stream& s, mira::multi_index::composite_key_result< T >& var, uint32_t depth )
{
   depth++;
   unpack( s, var.key, depth + 1 );
}


template< typename Stream > void pack( Stream&, const boost::tuples::null_type ) {}
template< typename Stream > void unpack( Stream& s, boost::tuples::null_type, uint32_t depth ) {}

template< typename Stream, typename H, typename T > void pack( Stream& s, const boost::tuples::cons< H, T >& var )
{
   pack( s, var.get_head() );
   pack( s, var.get_tail() );
}

template< typename Stream, typename H, typename T > void unpack( Stream& s, boost::tuples::cons< H, T >& var, uint32_t depth )
{
   depth++;
   unpack( s, var.get_head(), depth );
   unpack( s, var.get_tail(), depth );
}

} // raw

} // fc
