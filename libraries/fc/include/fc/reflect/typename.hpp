#pragma once

#include <deque>
#include <map>
#include <set>
#include <vector>
#include <sstream>

#include <boost/tuple/tuple.hpp>

#include <fc/string.hpp>
#include <fc/optional.hpp>

#include <fc/container/flat_fwd.hpp>
#include <fc/container/deque_fwd.hpp>

namespace fc {
  class value;
  class exception;
  namespace ip { class address; }

  template<typename... T> struct get_typename;
  template<> struct get_typename<int32_t>  { static const char* name()  { return "int32_t";  } };
  template<> struct get_typename<int64_t>  { static const char* name()  { return "int64_t";  } };
  template<> struct get_typename<int16_t>  { static const char* name()  { return "int16_t";  } };
  template<> struct get_typename<int8_t>   { static const char* name()  { return "int8_t";   } };
  template<> struct get_typename<uint32_t> { static const char* name()  { return "uint32_t"; } };
  template<> struct get_typename<uint64_t> { static const char* name()  { return "uint64_t"; } };
  template<> struct get_typename<uint16_t> { static const char* name()  { return "uint16_t"; } };
  template<> struct get_typename<uint8_t>  { static const char* name()  { return "uint8_t";  } };
  template<> struct get_typename<double>   { static const char* name()  { return "double";   } };
  template<> struct get_typename<float>    { static const char* name()  { return "float";    } };
  template<> struct get_typename<bool>     { static const char* name()  { return "bool";     } };
  template<> struct get_typename<char>     { static const char* name()  { return "char";     } };
  template<> struct get_typename<void>     { static const char* name()  { return "char";     } };
  template<> struct get_typename<string>   { static const char* name()  { return "string";   } };
  template<> struct get_typename<value>    { static const char* name()   { return "value";   } };
  template<> struct get_typename<fc::exception>   { static const char* name()   { return "fc::exception";   } };
  template<> struct get_typename<std::vector<char>>   { static const char* name()   { return "std::vector<char>";   } };
  template<typename T> struct get_typename<std::vector<T>>
  {
     static const char* name()  {
         static std::string n = std::string("std::vector<") + get_typename<T>::name() + ">";
         return n.c_str();
     }
  };
  template<typename T> struct get_typename<flat_set<T>>
  {
     static const char* name()  {
         static std::string n = std::string("flat_set<") + get_typename<T>::name() + ">";
         return n.c_str();
     }
  };
  template<typename T> struct get_typename< std::deque<T> >
  {
     static const char* name()
     {
        static std::string n = std::string("std::deque<") + get_typename<T>::name() + ">";
        return n.c_str();
     }
  };
  template<typename T> struct get_typename<optional<T>>
  {
     static const char* name()  {
         static std::string n = std::string("optional<") + get_typename<T>::name() + ">";
         return n.c_str();
     }
  };
  template<typename K,typename V> struct get_typename<std::map<K,V>>
  {
     static const char* name()  {
         static std::string n = std::string("std::map<") + get_typename<K>::name() + ","+get_typename<V>::name()+">";
         return n.c_str();
     }
  };

  template<typename E> struct get_typename< std::set<E> >
  {
     static const char* name()
     {
        static std::string n = std::string("std::set<") + std::string(get_typename<E>::name()) + std::string(">");
        return n.c_str();
     }
  };

  template<typename A, typename B> struct get_typename< std::pair<A,B> >
  {
      static const char* name()
      {
         static std::string n = std::string("std::pair<") + get_typename<A>::name() + "," + get_typename<B>::name() + ">";
         return n.c_str();
      }
  };

   template< typename T, typename... Args >
   struct var_template_args_typename_helper
   {
      static void name( std::stringstream& ss )
      {
         ss << get_typename< T >::name() << ',';
         var_template_args_typename_helper< Args... >::name( ss );
      }
   };

   template< typename T >
   struct var_template_args_typename_helper< T >
   {
      static void name( std::stringstream& ss )
      {
         ss << get_typename< T >::name();
      }
   };

   template< typename... Args > struct get_typename< std::tuple< Args... > >
   {
      static const char* name()
      {
         static std::string n;
         if( n.length() == 0 )
         {
            std::stringstream ss;
            var_template_args_typename_helper< Args... >::name( ss );
            n = ss.str();
         }
         return n.c_str();
      }
   };

   template< typename... Args >
   struct get_typename< boost::tuples::tuple< Args... > >
   {
      static const char* name()
      {
         static std::string n;
         if( n.length() == 0 )
         {
            std::stringstream ss;
            var_template_args_typename_helper< Args... >::name( ss );
            n = ss.str();
         }
         return n.c_str();
      }
   };

  struct signed_int;
  struct unsigned_int;
  template<> struct get_typename<signed_int>   { static const char* name()   { return "signed_int";   } };
  template<> struct get_typename<unsigned_int>   { static const char* name()   { return "unsigned_int";   } };

   std::string trim_typename_namespace( const std::string& n );
}
