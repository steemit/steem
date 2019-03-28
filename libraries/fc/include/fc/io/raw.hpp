#pragma once
#include <fc/io/raw_variant.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/io/datastream.hpp>
#include <fc/io/varint.hpp>
#include <fc/optional.hpp>
#include <fc/fwd.hpp>
#include <fc/smart_ref_fwd.hpp>
#include <fc/array.hpp>
#include <fc/int_array.hpp>
#include <fc/time.hpp>
#include <fc/filesystem.hpp>
#include <fc/exception/exception.hpp>
#include <fc/safe.hpp>
#include <fc/io/raw_fwd.hpp>
#include <map>
#include <deque>

namespace fc {
    namespace raw {

    template<typename Stream, typename Arg0, typename... Args>
    inline void pack( Stream& s, const Arg0& a0, Args... args ) {
       pack( s, a0 );
       pack( s, args... );
    }

    template<typename Stream>
    inline void pack( Stream& s, const fc::exception& e )
    {
       fc::raw::pack( s, e.code() );
       fc::raw::pack( s, std::string(e.name()) );
       fc::raw::pack( s, std::string(e.what()) );
       fc::raw::pack( s, e.get_log() );
    }
    template<typename Stream>
    inline void unpack( Stream& s, fc::exception& e, uint32_t depth )
    {
       depth++;
       FC_ASSERT( depth <= MAX_RECURSION_DEPTH );
       int64_t code;
       std::string name, what;
       log_messages msgs;

       fc::raw::unpack( s, code, depth );
       fc::raw::unpack( s, name, depth );
       fc::raw::unpack( s, what, depth );
       fc::raw::unpack( s, msgs, depth );

       e = fc::exception( fc::move(msgs), code, name, what );
    }

    template<typename Stream>
    inline void pack( Stream& s, const fc::log_message& msg )
    {
       fc::raw::pack( s, variant(msg) );
    }
    template<typename Stream>
    inline void unpack( Stream& s, fc::log_message& msg, uint32_t depth )
    {
       depth++;
       FC_ASSERT( depth <= MAX_RECURSION_DEPTH );
       fc::variant vmsg;
       fc::raw::unpack( s, vmsg, depth );
       msg = vmsg.as<log_message>();
    }

    template<typename Stream>
    inline void pack( Stream& s, const fc::path& tp )
    {
       fc::raw::pack( s, tp.generic_string() );
    }

    template<typename Stream>
    inline void unpack( Stream& s, fc::path& tp, uint32_t depth )
    {
       depth++;
       std::string p;
       fc::raw::unpack( s, p, depth );
       tp = p;
    }

    template<typename Stream>
    inline void pack( Stream& s, const fc::time_point_sec& tp )
    {
       uint32_t usec = tp.sec_since_epoch();
       s.write( (const char*)&usec, sizeof(usec) );
    }

    template<typename Stream>
    inline void unpack( Stream& s, fc::time_point_sec& tp, uint32_t )
    { try {
       uint32_t sec;
       s.read( (char*)&sec, sizeof(sec) );
       tp = fc::time_point() + fc::seconds(sec);
    } FC_RETHROW_EXCEPTIONS( warn, "" ) }

    template<typename Stream>
    inline void pack( Stream& s, const fc::time_point& tp )
    {
       uint64_t usec = tp.time_since_epoch().count();
       s.write( (const char*)&usec, sizeof(usec) );
    }

    template<typename Stream>
    inline void unpack( Stream& s, fc::time_point& tp, uint32_t )
    { try {
       uint64_t usec;
       s.read( (char*)&usec, sizeof(usec) );
       tp = fc::time_point() + fc::microseconds(usec);
    } FC_RETHROW_EXCEPTIONS( warn, "" ) }

    template<typename Stream>
    inline void pack( Stream& s, const fc::microseconds& usec )
    {
       uint64_t usec_as_int64 = usec.count();
       s.write( (const char*)&usec_as_int64, sizeof(usec_as_int64) );
    }

    template<typename Stream>
    inline void unpack( Stream& s, fc::microseconds& usec, uint32_t )
    { try {
       uint64_t usec_as_int64;
       s.read( (char*)&usec_as_int64, sizeof(usec_as_int64) );
       usec = fc::microseconds(usec_as_int64);
    } FC_RETHROW_EXCEPTIONS( warn, "" ) }

    template<typename Stream, typename T, size_t N>
    inline void pack( Stream& s, const fc::array<T,N>& v)
    {
       s.write((const char*)&v.data[0],N*sizeof(T));
    }

    template<typename Stream, typename T, size_t N>
    inline void unpack( Stream& s, fc::array<T,N>& v, uint32_t )
    { try {
       s.read( (char*)&v.data[0], N*sizeof(T) );
    } FC_RETHROW_EXCEPTIONS( warn, "fc::array<type,length>", ("type",fc::get_typename<T>::name())("length",N) ) }

    template<typename Stream, typename T, size_t N>
    inline void pack( Stream& s, const fc::int_array<T,N>& v)
    {
       s.write( (const char*)&v.data[0], N*sizeof(T) );
    }

    template<typename Stream, typename T, size_t N>
    inline void unpack( Stream& s, fc::int_array<T,N>& v, uint32_t )
    { try {
       s.read( (char*)&v.data[0], N*sizeof(T) );
    } FC_RETHROW_EXCEPTIONS( warn, "fc::int_array<type,length>", ("type",fc::get_typename<T>::name())("length",N) ) }

    template<typename Stream, typename T>
    inline void pack( Stream& s, const std::shared_ptr<T>& v)
    {
      fc::raw::pack( s, *v );
    }

    template<typename Stream, typename T>
    inline void unpack( Stream& s, std::shared_ptr<T>& v, uint32_t depth )
    { try {
      depth++;
      FC_ASSERT( depth <= MAX_RECURSION_DEPTH );
      v = std::make_shared<T>();
      fc::raw::unpack( s, *v, depth );
    } FC_RETHROW_EXCEPTIONS( warn, "std::shared_ptr<T>", ("type",fc::get_typename<T>::name()) ) }

    template<typename Stream> inline void pack( Stream& s, const signed_int& v ) {
      uint32_t val = (v.value<<1) ^ (v.value>>31);
      do {
        uint8_t b = uint8_t(val) & 0x7f;
        val >>= 7;
        b |= ((val > 0) << 7);
        s.write((char*)&b,1);//.put(b);
      } while( val );
    }

    template<typename Stream> inline void pack( Stream& s, const unsigned_int& v ) {
      uint64_t val = v.value;
      do {
        uint8_t b = uint8_t(val) & 0x7f;
        val >>= 7;
        b |= ((val > 0) << 7);
        s.write((char*)&b,1);//.put(b);
      }while( val );
    }

    template<typename Stream> inline void unpack( Stream& s, signed_int& vi, uint32_t ) {
      uint32_t v = 0, by = 0, limit = 0; char b = 0;
      do {
        s.get(b);
        v |= uint32_t(uint8_t(b) & 0x7f) << by;
        by += 7;
        limit++;
      } while( uint8_t(b) & 0x80 && limit < 5 );
      vi.value = ((v>>1) ^ (v>>31)) + (v&0x01);
      vi.value = v&0x01 ? vi.value : -vi.value;
      vi.value = -vi.value;
    }
    template<typename Stream> inline void unpack( Stream& s, unsigned_int& vi, uint32_t ) {
      uint32_t v = 0, by = 0, limit = 0; char b = 0;
      do {
          s.get(b);
          v |= uint32_t(uint8_t(b) & 0x7f) << by;
          by += 7;
          limit++;
      } while( uint8_t(b) & 0x80 && limit < 5 );
      vi.value = static_cast<uint32_t>(v);
    }

    template<typename Stream, typename T> inline void unpack( Stream& s, const T& vi, uint32_t depth )
    {
       depth++;
       FC_ASSERT( depth <= MAX_RECURSION_DEPTH );
       T tmp;
       fc::raw::unpack( s, tmp, depth );
       FC_ASSERT( vi == tmp );
    }

    template<typename Stream> inline void pack( Stream& s, const char* v ) { fc::raw::pack( s, fc::string(v) ); }

    template<typename Stream, typename T>
    void pack( Stream& s, const safe<T>& v ) { fc::raw::pack( s, v.value ); }

    template<typename Stream, typename T>
    void unpack( Stream& s, fc::safe<T>& v, uint32_t depth )
    {
      depth++;
      FC_ASSERT( depth <= MAX_RECURSION_DEPTH );
      fc::raw::unpack( s, v.value, depth );
    }

    template<typename Stream, typename T, unsigned int S, typename Align>
    void pack( Stream& s, const fc::fwd<T,S,Align>& v ) {
       fc::raw::pack( *v );
    }

    template<typename Stream, typename T, unsigned int S, typename Align>
    void unpack( Stream& s, fc::fwd<T,S,Align>& v, uint32_t depth )
    {
       depth++;
       FC_ASSERT( depth <= MAX_RECURSION_DEPTH );
       fc::raw::unpack( *v, depth );
    }
    template<typename Stream, typename T>
    void pack( Stream& s, const fc::smart_ref<T>& v ) { fc::raw::pack( s, *v ); }

    template<typename Stream, typename T>
    void unpack( Stream& s, fc::smart_ref<T>& v, uint32_t depth )
    {
      depth++;
      FC_ASSERT( depth <= MAX_RECURSION_DEPTH );
      fc::raw::unpack( s, *v, depth );
    }

    // optional
    template<typename Stream, typename T>
    void pack( Stream& s, const fc::optional<T>& v ) {
      fc::raw::pack( s, bool(!!v) );
      if( !!v ) fc::raw::pack( s, *v );
    }

    template<typename Stream, typename T>
    void unpack( Stream& s, fc::optional<T>& v, uint32_t depth )
    { try {
      depth++;
      FC_ASSERT( depth <= MAX_RECURSION_DEPTH );
      bool b; fc::raw::unpack( s, b, depth );
      if( b ) { v = T(); fc::raw::unpack( s, *v, depth ); }
    } FC_RETHROW_EXCEPTIONS( warn, "optional<${type}>", ("type",fc::get_typename<T>::name() ) ) }

    // std::vector<char>
    template<typename Stream> inline void pack( Stream& s, const std::vector<char>& value ) {
      fc::raw::pack( s, unsigned_int((uint32_t)value.size()) );
      if( value.size() )
        s.write( &value.front(), (uint32_t)value.size() );
    }
    template<typename Stream> inline void unpack( Stream& s, std::vector<char>& value, uint32_t depth ) {
      depth++;
      unsigned_int size; fc::raw::unpack( s, size, depth );
      FC_ASSERT( size.value < MAX_ARRAY_ALLOC_SIZE );
      value.resize(size.value);
      if( value.size() )
        s.read( value.data(), value.size() );
    }

    // fc::string
    template<typename Stream> inline void pack( Stream& s, const fc::string& v )  {
      fc::raw::pack( s, unsigned_int((uint32_t)v.size()));
      if( v.size() ) s.write( v.c_str(), v.size() );
    }

    template<typename Stream> inline void unpack( Stream& s, fc::string& v, uint32_t depth )  {
      depth++;
      std::vector<char> tmp;
      fc::raw::unpack(s,tmp,depth);
      if( tmp.size() )
         v = fc::string(tmp.data(),tmp.data()+tmp.size());
      else v = fc::string();
    }

    // bool
    template<typename Stream> inline void pack( Stream& s, const bool& v ) { fc::raw::pack( s, uint8_t(v) );             }
    template<typename Stream> inline void unpack( Stream& s, bool& v, uint32_t depth )
    {
       depth++;
       uint8_t b;
       fc::raw::unpack( s, b, depth );
       FC_ASSERT( (b & ~1) == 0 );
       v=(b!=0);
    }

    namespace detail {

      template<typename Stream, typename Class>
      struct pack_object_visitor {
        pack_object_visitor(const Class& _c, Stream& _s)
        :c(_c),s(_s){}

        template<typename T, typename C, T(C::*p)>
        void operator()( const char* name )const {
          fc::raw::pack( s, c.*p );
        }
        private:
          const Class& c;
          Stream&      s;
      };

      template<typename Stream, typename Class>
      struct unpack_object_visitor {
        unpack_object_visitor(Class& _c, Stream& _s)
        :c(_c),s(_s){}

        template<typename T, typename C, T(C::*p)>
        inline void operator()( const char* name )const
        { try {
          fc::raw::unpack( s, c.*p );
        } FC_RETHROW_EXCEPTIONS( warn, "Error unpacking field ${field}", ("field",name) ) }
        private:
          Class&  c;
          Stream& s;
      };

      template<typename IsClass=fc::true_type>
      struct if_class{
        template<typename Stream, typename T>
        static inline void pack( Stream& s, const T& v ) { s << v; }
        template<typename Stream, typename T>
        static inline void unpack( Stream& s, T& v, uint32_t depth = 0 ) { s >> v; }
      };

      template<>
      struct if_class<fc::false_type> {
        template<typename Stream, typename T>
        static inline void pack( Stream& s, const T& v ) {
          s.write( (char*)&v, sizeof(v) );
        }
        template<typename Stream, typename T>
        static inline void unpack( Stream& s, T& v, uint32_t ) {
          s.read( (char*)&v, sizeof(v) );
        }
      };

      template<typename IsEnum=fc::false_type>
      struct if_enum {
        template<typename Stream, typename T>
        static inline void pack( Stream& s, const T& v ) {
          fc::reflector<T>::visit( pack_object_visitor<Stream,T>( v, s ) );
        }
        template<typename Stream, typename T>
        static inline void unpack( Stream& s, T& v, uint32_t ) {
          fc::reflector<T>::visit( unpack_object_visitor<Stream,T>( v, s ) );
        }
      };
      template<>
      struct if_enum<fc::true_type> {
        template<typename Stream, typename T>
        static inline void pack( Stream& s, const T& v ) {
          fc::raw::pack(s, (int64_t)v);
        }
        template<typename Stream, typename T>
        static inline void unpack( Stream& s, T& v, uint32_t depth = 0 ) {
          depth++;
          FC_ASSERT( depth <= MAX_RECURSION_DEPTH );
          int64_t temp;
          fc::raw::unpack(s, temp, depth);
          v = (T)temp;
        }
      };

      template<typename IsReflected=fc::false_type>
      struct if_reflected {
        template<typename Stream, typename T>
        static inline void pack( Stream& s, const T& v ) {
          if_class<typename fc::is_class<T>::type>::pack(s,v);
        }
        template<typename Stream, typename T>
        static inline void unpack( Stream& s, T& v, uint32_t depth = 0 ) {
          depth++;
          FC_ASSERT( depth <= MAX_RECURSION_DEPTH );
          if_class<typename fc::is_class<T>::type>::unpack(s,v,depth);
        }
      };
      template<>
      struct if_reflected<fc::true_type> {
        template<typename Stream, typename T>
        static inline void pack( Stream& s, const T& v ) {
          if_enum< typename fc::reflector<T>::is_enum >::pack(s,v);
        }
        template<typename Stream, typename T>
        static inline void unpack( Stream& s, T& v, uint32_t depth = 0 ) {
          depth++;
          FC_ASSERT( depth <= MAX_RECURSION_DEPTH );
          if_enum< typename fc::reflector<T>::is_enum >::unpack(s,v,depth);
        }
      };

    } // namespace detail

    template<typename Stream, typename T>
    inline void pack( Stream& s, const std::unordered_set<T>& value ) {
      fc::raw::pack( s, unsigned_int((uint32_t)value.size()) );
      auto itr = value.begin();
      auto end = value.end();
      while( itr != end ) {
        fc::raw::pack( s, *itr );
        ++itr;
      }
    }
    template<typename Stream, typename T>
    inline void unpack( Stream& s, std::unordered_set<T>& value, uint32_t depth ) {
      depth++;
      FC_ASSERT( depth <= MAX_RECURSION_DEPTH );
      unsigned_int size; fc::raw::unpack( s, size, depth );
      value.clear();
      FC_ASSERT( size.value*sizeof(T) < MAX_ARRAY_ALLOC_SIZE );
      for( uint32_t i = 0; i < size.value; ++i )
      {
          T tmp;
          fc::raw::unpack( s, tmp, depth );
          value.insert( std::move(tmp) );
      }
    }


    template<typename Stream, typename K, typename V>
    inline void pack( Stream& s, const std::pair<K,V>& value ) {
       fc::raw::pack( s, value.first );
       fc::raw::pack( s, value.second );
    }
    template<typename Stream, typename K, typename V>
    inline void unpack( Stream& s, std::pair<K,V>& value, uint32_t depth )
    {
       depth++;
       FC_ASSERT( depth <= MAX_RECURSION_DEPTH );
       fc::raw::unpack( s, value.first, depth );
       fc::raw::unpack( s, value.second, depth );
    }

   template<typename Stream, typename K, typename V>
    inline void pack( Stream& s, const std::unordered_map<K,V>& value ) {
      fc::raw::pack( s, unsigned_int((uint32_t)value.size()) );
      auto itr = value.begin();
      auto end = value.end();
      while( itr != end ) {
        fc::raw::pack( s, *itr );
        ++itr;
      }
    }
    template<typename Stream, typename K, typename V>
    inline void unpack( Stream& s, std::unordered_map<K,V>& value, uint32_t depth )
    {
      depth++;
      FC_ASSERT( depth <= MAX_RECURSION_DEPTH );
      unsigned_int size; fc::raw::unpack( s, size, depth );
      value.clear();
      FC_ASSERT( size.value*(sizeof(K)+sizeof(V)) < MAX_ARRAY_ALLOC_SIZE );
      for( uint32_t i = 0; i < size.value; ++i )
      {
          std::pair<K,V> tmp;
          fc::raw::unpack( s, tmp, depth );
          value.insert( std::move(tmp) );
      }
    }
    template<typename Stream, typename K, typename V>
    inline void pack( Stream& s, const std::map<K,V>& value ) {
      fc::raw::pack( s, unsigned_int((uint32_t)value.size()) );
      auto itr = value.begin();
      auto end = value.end();
      while( itr != end ) {
        fc::raw::pack( s, *itr );
        ++itr;
      }
    }
    template<typename Stream, typename K, typename V>
    inline void unpack( Stream& s, std::map<K,V>& value, uint32_t depth )
    {
      depth++;
      FC_ASSERT( depth <= MAX_RECURSION_DEPTH );
      unsigned_int size; fc::raw::unpack( s, size, depth );
      value.clear();
      FC_ASSERT( size.value*(sizeof(K)+sizeof(V)) < MAX_ARRAY_ALLOC_SIZE );
      for( uint32_t i = 0; i < size.value; ++i )
      {
          std::pair<K,V> tmp;
          fc::raw::unpack( s, tmp, depth );
          value.insert( std::move(tmp) );
      }
    }

    template<typename Stream, typename T>
    inline void pack( Stream& s, const std::deque<T>& value ) {
      fc::raw::pack( s, unsigned_int((uint32_t)value.size()) );
      auto itr = value.begin();
      auto end = value.end();
      while( itr != end ) {
        fc::raw::pack( s, *itr );
        ++itr;
      }
    }

    template<typename Stream, typename T>
    inline void unpack( Stream& s, std::deque<T>& value, uint32_t depth ) {
      depth++;
      FC_ASSERT( depth <= MAX_RECURSION_DEPTH );
      unsigned_int size; fc::raw::unpack( s, size, depth );
      FC_ASSERT( size.value*sizeof(T) < MAX_ARRAY_ALLOC_SIZE );
      value.clear();
      for ( size_t i = 0; i < size.value; i++ )
      {
         T tmp;
         fc::raw::unpack( s, tmp, depth );
         value.emplace_back( std::move( tmp ) );
      }
    }

    template<typename Stream, typename T>
    inline void pack( Stream& s, const std::vector<T>& value ) {
      fc::raw::pack( s, unsigned_int((uint32_t)value.size()) );
      auto itr = value.begin();
      auto end = value.end();
      while( itr != end ) {
        fc::raw::pack( s, *itr );
        ++itr;
      }
    }

    template<typename Stream, typename T>
    inline void unpack( Stream& s, std::vector<T>& value, uint32_t depth ) {
      depth++;
      FC_ASSERT( depth <= MAX_RECURSION_DEPTH );
      unsigned_int size; fc::raw::unpack( s, size );
      FC_ASSERT( size.value*sizeof(T) < MAX_ARRAY_ALLOC_SIZE );
      value.clear();
      for ( size_t i = 0; i < size.value; i++ )
      {
         T tmp;
         fc::raw::unpack( s, tmp, depth );
         value.emplace_back( std::move( tmp ) );
      }
    }

    template<typename Stream, typename... T>
    inline void pack( Stream& s, const std::set<T...>& value ) {
      fc::raw::pack( s, unsigned_int((uint32_t)value.size()) );
      auto itr = value.begin();
      auto end = value.end();
      while( itr != end ) {
        fc::raw::pack( s, *itr );
        ++itr;
      }
    }

    template<typename Stream, typename... T>
    inline void unpack( Stream& s, std::set<T...>& value, uint32_t depth ) {
      depth++;
      FC_ASSERT( depth <= MAX_RECURSION_DEPTH );
      unsigned_int size; fc::raw::unpack( s, size, depth );
      value.clear();
      for( uint64_t i = 0; i < size.value; ++i )
      {
        typename std::set<T...>::value_type tmp;
        fc::raw::unpack( s, tmp, depth );
        value.insert( std::move(tmp) );
      }
    }

    template<typename Stream, typename... T>
    inline void pack( Stream& s, const std::multiset<T...>& value ) {
      fc::raw::pack( s, unsigned_int((uint32_t)value.size()) );
      auto itr = value.begin();
      auto end = value.end();
      while( itr != end ) {
        fc::raw::pack( s, *itr );
        ++itr;
      }
    }

    template<typename Stream, typename... T>
    inline void unpack( Stream& s, std::multiset<T...>& value, uint32_t depth ) {
      depth++;
      FC_ASSERT( depth <= MAX_RECURSION_DEPTH );
      unsigned_int size; fc::raw::unpack( s, size, depth );
      value.clear();
      for( uint64_t i = 0; i < size.value; ++i )
      {
        typename std::multiset<T...>::value_type tmp;
        fc::raw::unpack( s, tmp, depth );
        value.insert( std::move(tmp) );
      }
    }



    template<typename Stream, typename T>
    inline void pack( Stream& s, const T& v ) {
      fc::raw::detail::if_reflected< typename fc::reflector<T>::is_defined >::pack(s,v);
    }
    template<typename Stream, typename T>
    inline void unpack( Stream& s, T& v, uint32_t depth )
    { try {
      depth++;
      FC_ASSERT( depth <= MAX_RECURSION_DEPTH );
      fc::raw::detail::if_reflected< typename fc::reflector<T>::is_defined >::unpack(s,v,depth);
    } FC_RETHROW_EXCEPTIONS( warn, "error unpacking ${type}", ("type",fc::get_typename<T>::name() ) ) }

    template<typename T>
    inline size_t pack_size(  const T& v )
    {
      datastream<size_t> ps;
      fc::raw::pack(ps,v );
      return ps.tellp();
    }

    template<typename T>
    inline std::vector<char> pack_to_vector( const T& v )
    {
      datastream<size_t> ps;
      fc::raw::pack(ps,v );
      std::vector<char> vec(ps.tellp());

      if( vec.size() ) {
        datastream<char*>  ds( vec.data(), size_t(vec.size()) );
        fc::raw::pack(ds,v);
      }
      return vec;
    }

   template< typename Stream, int Index, typename Tuple >
   struct tuple_packer; // Forward Declaration

   template< typename Stream, typename Tuple >
   struct tuple_packer< Stream, 0, Tuple >
   {
      static void tuple_pack( Stream& s, const Tuple& t )
      {
         pack( s, boost::tuples::get< 0 >( t ) );
      }

      static void tuple_unpack( Stream& s, Tuple& t )
      {
         unpack( s, boost::tuples::get< 0 >( t ) );
      }
   };

   template< typename Stream, int Index, typename Tuple >
   struct tuple_packer
   {
      static void tuple_pack( Stream& s, const Tuple& t )
      {
         tuple_packer< Stream, Index - 1, Tuple >::tuple_pack( s, t );
         pack( s, boost::tuples::get< Index >( t ) );
      }

      static void tuple_unpack( Stream& s, Tuple& t )
      {
         tuple_packer< Stream, Index - 1, Tuple >::tuple_unpack( s, t );
         unpack( s, boost::tuples::get< Index >( t ) );
      }
   };

   template< typename Stream, typename... Args > void pack( Stream& s, const boost::tuples::tuple< Args... >& var )
   {
      tuple_packer<
         Stream,
         boost::tuples::length< boost::tuples::tuple< Args... > >::value - 1,
         boost::tuples::tuple< Args... >
      >::tuple_pack( s, var );
   }

   template< typename Stream, typename... Args > void unpack( Stream& s, boost::tuples::tuple< Args... >& var )
   {
      tuple_packer<
         Stream,
         boost::tuples::length< boost::tuples::tuple< Args... > >::value - 1,
         boost::tuples::tuple< Args... >
      >::tuple_unpack( s, var );
   }

    template<typename T>
    inline T unpack_from_vector( const std::vector<char>& s, uint32_t depth )
    { try  {
      depth++;
      T tmp;
      if( s.size() ) {
        datastream<const char*>  ds( s.data(), size_t(s.size()) );
        fc::raw::unpack(ds,tmp,depth);
      }
      return tmp;
    } FC_RETHROW_EXCEPTIONS( warn, "error unpacking ${type}", ("type",fc::get_typename<T>::name() ) ) }

    template<typename T>
    inline void unpack_from_vector( const std::vector<char>& s, T& tmp, uint32_t depth = 0 )
    { try  {
      depth++;
      if( s.size() ) {
        datastream<const char*>  ds( s.data(), size_t(s.size()) );
        fc::raw::unpack(ds,tmp,depth);
      }
    } FC_RETHROW_EXCEPTIONS( warn, "error unpacking ${type}", ("type",fc::get_typename<T>::name() ) ) }

    template<typename T>
    inline void pack_to_char_array( char* d, uint32_t s, const T& v ) {
      datastream<char*> ds(d,s);
      fc::raw::pack(ds,v );
    }

    template<typename T>
    inline T unpack_from_char_array( const char* d, uint32_t s, uint32_t depth )
    { try {
      depth++;
      T v;
      datastream<const char*>  ds( d, s );
      fc::raw::unpack(ds,v,depth);
      return v;
    } FC_RETHROW_EXCEPTIONS( warn, "error unpacking ${type}", ("type",fc::get_typename<T>::name() ) ) }

    template<typename T>
    inline void unpack_from_char_array( const char* d, uint32_t s, T& v, uint32_t depth )
    { try {
      depth++;
      datastream<const char*>  ds( d, s );
      fc::raw::unpack(ds,v,depth);
    } FC_RETHROW_EXCEPTIONS( warn, "error unpacking ${type}", ("type",fc::get_typename<T>::name() ) ) }

   template<typename Stream>
   struct pack_static_variant
   {
      Stream& stream;
      pack_static_variant( Stream& s ):stream(s){}

      typedef void result_type;
      template<typename T> void operator()( const T& v )const
      {
         fc::raw::pack( stream, v );
      }
   };

   template<typename Stream>
   struct unpack_static_variant
   {
      Stream& stream;
      unpack_static_variant( Stream& s ):stream(s){}

      typedef void result_type;
      template<typename T> void operator()( T& v )const
      {
         fc::raw::unpack( stream, v );
      }
   };


    template<typename Stream, typename... T>
    void pack( Stream& s, const static_variant<T...>& sv )
    {
       fc::raw::pack( s, unsigned_int(sv.which()) );
       sv.visit( pack_static_variant<Stream>(s) );
    }

    template<typename Stream, typename... T> void unpack( Stream& s, static_variant<T...>& sv, uint32_t depth )
    {
       depth++;
       FC_ASSERT( depth <= MAX_RECURSION_DEPTH );
       unsigned_int w;
       fc::raw::unpack( s, w, depth );
       sv.set_which(w.value);
       sv.visit( unpack_static_variant<Stream>(s) );
    }

} } // namespace fc::raw
