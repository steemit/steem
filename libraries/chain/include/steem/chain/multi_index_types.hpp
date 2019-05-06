
#include <mira/adapters.hpp>

#include <mira/multi_index_container.hpp>
#include <mira/ordered_index.hpp>
#include <mira/tag.hpp>
#include <mira/member.hpp>
#include <mira/indexed_by.hpp>
#include <mira/composite_key.hpp>
#include <mira/mem_fun.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/tag.hpp>
#include <mira/boost_adapter.hpp>

#include <boost/mpl/vector.hpp>
#include <type_traits>
#include <typeinfo>

namespace steem { namespace chain {

//using ::boost::multi_index::indexed_by;
//using ::boost::multi_index::ordered_unique;
//using ::boost::multi_index::tag;
//using ::boost::multi_index::member;
//using ::boost::multi_index::composite_key;
//using ::boost::multi_index::composite_key_compare;
//using ::boost::multi_index::const_mem_fun;

using mira::multi_index_container;
using mira::multi_index::indexed_by;
using mira::multi_index::ordered_unique;
using mira::multi_index::tag;
using mira::multi_index::member;
using mira::multi_index::composite_key;
using mira::multi_index::composite_key_compare;
using mira::multi_index::const_mem_fun;
/*
template< typename... Args >
using indexed_by = typename std::conditional< false, ::mira::multi_index::indexed_by< Args ... >, ::boost::multi_index::indexed_by< Args ... > >;

template< typename... Args >
using ordered_unique = typename std::conditional< false, ::mira::multi_index::ordered_unique< Args ... >, ::boost::multi_index::ordered_unique< Args ... > >;

template < typename T >
using tag = typename std::conditional< false, ::mira::multi_index::tag< T >, ::boost::multi_index::tag< T > >;

template< class Class, typename Type, Type Class::*PtrToMember >
using member = typename std::conditional< false, ::mira::multi_index::member< Class, Type, PtrToMember >, ::boost::multi_index::member< Class, Type, PtrToMember > >;

template< typename... Args >
using composite_key = typename std::conditional< false, ::mira::multi_index::composite_key< Args ... >, ::boost::multi_index::composite_key< Args ... > >;

template< typename... Args >
using composite_key_compare = typename std::conditional< false, ::mira::multi_index::composite_key_compare< Args ... >, ::boost::multi_index::composite_key_compare< Args ... > >;

template< class Class, typename Type, Type (Class::*PtrToMemberFunction)()const >
using const_mem_fun = typename std::conditional< false, ::mira::multi_index::const_mem_fun< Class, Type, PtrToMemberFunction >, ::boost::multi_index::const_mem_fun< Class, Type, PtrToMemberFunction > >;
*/

template< typename T1, typename T2, typename T3 >
class bmic_type : public ::mira::boost_multi_index_adapter< T1, T2, T3 >
{
public:
   using mira::boost_multi_index_adapter< T1, T2, T3 >::boost_multi_index_adapter;
};

template< typename T1, typename T2, typename T3 >
class mira_type : public ::mira::multi_index_container< T1, T2, T3 >
{
public:
   using mira::multi_index_container< T1, T2, T3 >::multi_index_container;
};

/*
template< typename Value, typename IndexSpecifierList, typename Allocator >
class multi_index_container : public mira_type< Value, IndexSpecifierList, Allocator >
{
public:
   using mira_type< Value, IndexSpecifierList, Allocator >::mira_type;
};
*/

} } // steem::chain
