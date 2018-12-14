/* Used in Boost.MultiIndex tests.
 *
 * Copyright 2003-2015 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#ifndef BOOST_MULTI_INDEX_TEST_EMPLOYEE_HPP
#define BOOST_MULTI_INDEX_TEST_EMPLOYEE_HPP

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <boost/move/core.hpp>
#include <boost/move/utility.hpp>
#include <boost/mpl/vector.hpp>

#if 0
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/ranked_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#endif

#include <cstddef>
#include <ostream>
#include <string>
#include "non_std_allocator.hpp"

#include <mira/multi_index_container.hpp>
#include <mira/ordered_index.hpp>
#include <mira/tag.hpp>
#include <mira/member.hpp>
#include <mira/indexed_by.hpp>

using namespace mira;

struct employee
{
  int         id;
  std::string name;
  int         age;
  int         ssn;

  employee(int id_,std::string name_,int age_,int ssn_):
    id(id_),name(name_),age(age_),ssn(ssn_)
  {}

  employee(const employee& x):
    id(x.id),name(x.name),age(x.age),ssn(x.ssn)
  {}

  employee(BOOST_RV_REF(employee) x):
    id(x.id),name(boost::move(x.name)),age(x.age),ssn(x.ssn)
  {}

  employee& operator=(BOOST_COPY_ASSIGN_REF(employee) x)
  {
    id=x.id;
    name=x.name;
    age=x.age;
    ssn=x.ssn;
    return *this;
  };

  employee& operator=(BOOST_RV_REF(employee) x)
  {
    id=x.id;
    name=boost::move(x.name);
    age=x.age;
    ssn=x.ssn;
    return *this;
  }

  bool operator==(const employee& x)const
  {
    return id==x.id&&name==x.name&&age==x.age;
  }

  bool operator<(const employee& x)const
  {
    return id<x.id;
  }

  bool operator!=(const employee& x)const{return !(*this==x);}
  bool operator> (const employee& x)const{return x<*this;}
  bool operator>=(const employee& x)const{return !(*this<x);}
  bool operator<=(const employee& x)const{return !(x<*this);}

  struct comp_id
  {
    bool operator()(int x,const employee& e2)const{return x<e2.id;}
    bool operator()(const employee& e1,int x)const{return e1.id<x;}
  };

  friend std::ostream& operator<<(std::ostream& os,const employee& e)
  {
    os<<e.id<<" "<<e.name<<" "<<e.age<<std::endl;
    return os;
  }

private:
  BOOST_COPYABLE_AND_MOVABLE(employee)
};

struct name{};
struct by_name{};
struct age{};
struct as_inserted{};
struct ssn{};
struct randomly{};

struct employee_set_indices:
  boost::mpl::vector<
    mira::multi_index::ordered_unique<
      mira::multi_index::identity<employee> >,
    mira::multi_index::hashed_non_unique<
      mira::multi_index::tag<name,by_name>,
      BOOST_MULTI_INDEX_MEMBER(employee,std::string,name)>,
    mira::multi_index::ranked_non_unique<
      mira::multi_index::tag<age>,
      BOOST_MULTI_INDEX_MEMBER(employee,int,age)>,
    mira::multi_index::sequenced<
      mira::multi_index::tag<as_inserted> >,
    mira::multi_index::hashed_unique<
      mira::multi_index::tag<ssn>,
      BOOST_MULTI_INDEX_MEMBER(employee,int,ssn)>,
    mira::multi_index::random_access<
      mira::multi_index::tag<randomly> > >
{};

typedef
  mira::multi_index::multi_index_container<
    employee,
    employee_set_indices,
    non_std_allocator<employee> >        employee_set;

#if defined(BOOST_NO_MEMBER_TEMPLATES)
typedef mira::multi_index::nth_index<
  employee_set,1>::type                  employee_set_by_name;
#else
typedef employee_set::nth_index<1>::type employee_set_by_name;
#endif

typedef mira::multi_index::index<
         employee_set,age>::type         employee_set_by_age;
typedef mira::multi_index::index<
         employee_set,as_inserted>::type employee_set_as_inserted;
typedef mira::multi_index::index<
         employee_set,ssn>::type         employee_set_by_ssn;

#if defined(BOOST_NO_MEMBER_TEMPLATES)
typedef mira::multi_index::index<
         employee_set,randomly>::type    employee_set_randomly;
#else
typedef employee_set::index<
          randomly>::type                employee_set_randomly;
#endif

#endif
