/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <steemit/protocol/protocol.hpp>
#include <steemit/chain/steem_objects.hpp>
#include <fc/smart_ref_impl.hpp>
#include <iostream>

using namespace steemit::chain;
using namespace steemit::protocol;

using std::string;
using std::map;

namespace detail_ns {

string remove_tail_if( const string& str, char c, const string& match )
{
   auto last = str.find_last_of( c );
   if( last != std::string::npos )
      if( str.substr( last + 1 ) == match )
         return str.substr( 0, last );
   return str;
}
string remove_namespace_if( const string& str, const string& match )
{
   auto last = str.find( match );
   if( last != std::string::npos )
      return str.substr( match.size()+2 );
   return str;
}


string remove_namespace( string str )
{
   str = remove_tail_if( str, '_', "operation" );
   str = remove_tail_if( str, '_', "t" );
   str = remove_tail_if( str, '_', "object" );
   str = remove_tail_if( str, '_', "type" );
   str = remove_namespace_if( str, "steemit::chain" );
   str = remove_namespace_if( str, "chainbase" );
   str = remove_namespace_if( str, "std" );
   str = remove_namespace_if( str, "fc" );
   auto pos = str.find( ":" );
   if( pos != str.npos )
      str.replace( pos, 2, "_" );
   return str;
}




template<typename T>
void generate_serializer();
template<typename T>
void register_serializer();


map<string, size_t >                st;
steemit::vector<std::function<void()>>       serializers;

bool register_serializer( const string& name, std::function<void()> sr )
{
   if( st.find(name) == st.end() )
   {
      serializers.push_back( sr );
      st[name] = serializers.size() - 1;
      return true;
   }
   return false;
}

template<typename T> struct js_name { static std::string name(){ return  remove_namespace(fc::get_typename<T>::name()); }; };

template<typename T, size_t N>
struct js_name<fc::array<T,N>>
{
   static std::string name(){ return  "fixed_array "+ fc::to_string(N) + ", "  + remove_namespace(fc::get_typename<T>::name()); };
};
template<size_t N>   struct js_name<fc::array<char,N>>    { static std::string name(){ return  "bytes "+ fc::to_string(N); }; };
template<size_t N>   struct js_name<fc::array<uint8_t,N>> { static std::string name(){ return  "bytes "+ fc::to_string(N); }; };
template<typename T> struct js_name< fc::optional<T> >    { static std::string name(){ return "optional " + js_name<T>::name(); } };
template<typename T> struct js_name< fc::smart_ref<T> >   { static std::string name(){ return js_name<T>::name(); } };
template<typename T> struct js_name< fc::flat_set<T> >    { static std::string name(){ return "set " + js_name<T>::name(); } };
template<typename T> struct js_name< std::vector<T> >     { static std::string name(){ return "array " + js_name<T>::name(); } };
template<typename T> struct js_name< fc::safe<T> > { static std::string name(){ return js_name<T>::name(); } };


template<> struct js_name< std::vector<char> > { static std::string name(){ return "bytes()";     } };
template<> struct js_name<fc::uint160>         { static std::string name(){ return "bytes 20";   } };
template<> struct js_name<fc::sha224>          { static std::string name(){ return "bytes 28";   } };
template<> struct js_name<fc::sha256>          { static std::string name(){ return "bytes 32";   } };
template<> struct js_name<fc::unsigned_int>    { static std::string name(){ return "varuint32";  } };
template<> struct js_name<fc::signed_int>      { static std::string name(){ return "varint32";   } };
template<> struct js_name<fc::time_point_sec >    { static std::string name(){ return "time_point_sec"; } };

template<typename O>
struct js_name<chainbase::oid<O> >
{
   static std::string name(){
      return "protocol_id_type \"" + remove_namespace(fc::get_typename<O>::name()) + "\"";
   };
};


template<typename T> struct js_name< std::set<T> > { static std::string name(){ return "set " + js_name<T>::name(); } };

template<typename K, typename V>
struct js_name< std::map<K,V> > { static std::string name(){ return "map (" + js_name<K>::name() + "), (" + js_name<V>::name() +")"; } };

template<typename K, typename V>
struct js_name< fc::flat_map<K,V> > { static std::string name(){ return "map (" + js_name<K>::name() + "), (" + js_name<V>::name() +")"; } };


template<typename... T> struct js_sv_name;

template<typename A> struct js_sv_name<A>
{ static std::string name(){ return  "\n    " + js_name<A>::name(); } };

template<typename A, typename... T>
struct js_sv_name<A,T...> { static std::string name(){ return  "\n    " + js_name<A>::name() +"    " + js_sv_name<T...>::name(); } };

template<typename... T>
struct js_name< fc::static_variant<T...> >
{
   static std::string name( std::string n = ""){
      static const std::string name = n;
      if( name == "" )
         return "static_variant [" + js_sv_name<T...>::name() + "\n]";
      else return name;
   }
};
template<>
struct js_name< fc::static_variant<> >
{
   static std::string name( std::string n = ""){
      static const std::string name = n;
      if( name == "" )
         return "static_variant []";
      else return name;
   }
};



template<typename T, bool reflected = fc::reflector<T>::is_defined::value>
struct serializer;


struct register_type_visitor
{
   typedef void result_type;

   template<typename Type>
   result_type operator()( const Type& op )const { serializer<Type>::init(); }
};

class register_member_visitor;

struct serialize_type_visitor
{
   typedef void result_type;

   int t = 0;
   serialize_type_visitor(int _t ):t(_t){}

   template<typename Type>
   result_type operator()( const Type& op )const
   {
      std::cout << "    " <<remove_namespace( fc::get_typename<Type>::name() )  <<": "<<t<<"\n";
   }
};


class serialize_member_visitor
{
   public:
      template<typename Member, class Class, Member (Class::*member)>
      void operator()( const char* name )const
      {
         std::cout << "    " << name <<": " << js_name<Member>::name() <<"\n";
      }
};

template<typename T>
struct serializer<T,false>
{
   static_assert( fc::reflector<T>::is_defined::value == false, "invalid template arguments" );
   static void init()
   {}

   static void generate()
   {}
};

template<typename T, size_t N>
struct serializer<fc::array<T,N>,false>
{
   static void init() { serializer<T>::init(); }
   static void generate() {}
};
template<typename T>
struct serializer<std::vector<T>,false>
{
   static void init() { serializer<T>::init(); }
   static void generate() {}
};

template<typename T>
struct serializer<fc::smart_ref<T>,false>
{
   static void init() {
      serializer<T>::init(); }
   static void generate() {}
};

template<>
struct serializer<std::vector<operation>,false>
{
   static void init() { }
   static void generate() {}
};

template<>
struct serializer<uint64_t,false>
{
   static void init() {}
   static void generate() {}
};
#ifdef __APPLE__
// on mac, size_t is a distinct type from uint64_t or uint32_t and needs a separate specialization
template<> struct serializer<size_t,false> { static void init() {} static void generate() {} };
#endif
template<> struct serializer<int64_t,false> { static void init() {} static void generate() {} };
template<> struct serializer<int64_t,true> { static void init() {} static void generate() {} };

template<typename T>
struct serializer<fc::optional<T>,false>
{
   static void init() { serializer<T>::init(); }
   static void generate(){}
};

template<typename T>
struct serializer< chainbase::oid<T> ,true>
{
   static void init() {}
   static void generate() {}
};

template<typename... T>
struct serializer< fc::static_variant<T...>, false >
{
   static void init()
   {
      static bool init = false;
      if( !init )
      {
         init = true;
         fc::static_variant<T...> var;
         for( int i = 0; i < var.count(); ++i )
         {
            var.set_which(i);
            var.visit( register_type_visitor() );
         }
         register_serializer( js_name<fc::static_variant<T...>>::name(), [=](){ generate(); } );
      }
   }

   static void generate()
   {
      std::cout <<  js_name<fc::static_variant<T...>>::name() << " = static_variant [" + js_sv_name<T...>::name() + "\n]\n\n";
   }
};
template<>
struct serializer< fc::static_variant<>, false >
{
   static void init()
   {
      static bool init = false;
      if( !init )
      {
         init = true;
         fc::static_variant<> var;
         register_serializer( js_name<fc::static_variant<>>::name(), [=](){ generate(); } );
      }
   }

   static void generate()
   {
      std::cout <<  js_name<fc::static_variant<>>::name() << " = static_variant []\n\n";
   }
};


class register_member_visitor
{
   public:
      template<typename Member, class Class, Member (Class::*member)>
      void operator()( const char* name )const
      {
         serializer<Member>::init();
      }
};

template<typename T, bool reflected>
struct serializer
{
   static_assert( fc::reflector<T>::is_defined::value == reflected, "invalid template arguments" );
   static void init()
   {
      auto name = js_name<T>::name();
      if( st.find(name) == st.end() )
      {
         fc::reflector<T>::visit( register_member_visitor() );
         register_serializer( name, [=](){ generate(); } );
      }
   }

   static void generate()
   {
      auto name = remove_namespace( js_name<T>::name() );
      if( name == "int64" ) return;
      std::cout << "" << name
                << " = new Serializer( \n"
                << "    \"" + name + "\"\n";

      fc::reflector<T>::visit( serialize_member_visitor() );

      std::cout <<")\n\n";
   }
};

} // namespace detail_ns

int main( int argc, char** argv )
{
   try {
    operation op;

    std::cout << "ChainTypes.operations=\n";
    for( int i = 0; i < op.count(); ++i )
    {
       op.set_which(i);
       op.visit( detail_ns::serialize_type_visitor(i) );
    }
    std::cout << "\n";

    detail_ns::js_name<operation>::name("operation");
    detail_ns::js_name<future_extensions>::name("future_extensions");
    detail_ns::serializer<signed_block>::init();
    detail_ns::serializer<block_header>::init();
    detail_ns::serializer<signed_block_header>::init();
    detail_ns::serializer<operation>::init();
    detail_ns::serializer<transaction>::init();
    detail_ns::serializer<signed_transaction>::init();
    for( const auto& gen : detail_ns::serializers )
       gen();

  } catch ( const fc::exception& e ){ edump((e.to_detail_string())); }
   return 0;
}
