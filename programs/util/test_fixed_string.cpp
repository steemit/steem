
#include <steemit/protocol/fixed_string.hpp>

#include <fc/io/raw.hpp>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

inline int popcount( uint32_t x )
{
   int result = 0;
   for( size_t i=0; i<8*sizeof(x); i++ )
   {
      result += (x&1);
      x >>= 1;
   }
   return result;
}

int errors = 0;

void check( const std::string& s, const std::string& t,
    bool cond )
{
   if( !cond )
   {
      std::cout << "problem with " << s << " " << t << std::endl;
      ++errors;
   }
}

template< typename Storage >
void check_variant( const std::string& s, const steemit::protocol::fixed_string< Storage >& fs )
{
   fc::variant vs, vfs;
   fc::to_variant( s, vs );
   fc::to_variant( fs, vfs );
   if( !(vs.is_string() && vfs.is_string() &&
         (vs.get_string() == s) && (vfs.get_string() == s)
     )  )
   {
      std::cout << "to_variant() check failed on " << s << std::endl;
      ++errors;
   }

   std::string s2;
   steemit::protocol::fixed_string< Storage > fs2;
   fc::from_variant( vs, s2 );
   fc::from_variant( vfs, fs2 );
   if( s2 != s )
   {
      std::cout << "from_variant() check failed on " << s << std::endl;
      ++errors;
   }

   if( fs2 != fs )
   {
      std::cout << "from_variant() check failed on " << s << std::endl;
      ++errors;
   }
}

template< typename Storage >
void check_pack( const std::string& s, const steemit::protocol::fixed_string< Storage >& fs )
{
   std::stringstream ss, sfs;
   fc::raw::pack( ss, s );
   fc::raw::pack( sfs, fs );
   std::string packed = ss.str();
   if( packed != sfs.str() )
   {
      std::cout << "check_pack() check failed on " << s << std::endl;
      ++errors;
   }

   ss.seekg(0);
   steemit::protocol::fixed_string< Storage > unpacked;
   fc::raw::unpack( ss, unpacked );
   if( unpacked != fs )
   {
      std::cout << "check_pack() check failed on " << s << std::endl;
      ++errors;
   }
}

int main( int argc, char** argv, char** envp )
{
   std::vector< std::string > all_strings;
   std::vector< steemit::protocol::fixed_string_16 > all_fixed_string_16s;
   std::vector< std::vector< uint32_t > > sim_index;

   std::cout << "setting up LUT's" << std::endl;
   for( int len=0; len<=16; len++ )
   {
      int b_max = 1 << len;
      size_t begin_offset = all_strings.size();

      std::vector< uint32_t > hops;
      for( int b=0; b<b_max; b++ )
      {
         uint32_t ub = uint32_t(b);
         if( popcount(ub) <= 3 )
            hops.push_back( ub );
      }

      for( int b=0; b<b_max; b++ )
      {
         all_strings.emplace_back(len, '\0');
         for( int i=0,m=1; i<len; i++,m+=m )
         {
            all_strings.back()[len-i-1] = ((b&m)==0) ? 'a' : 'b';
         }

         sim_index.emplace_back();
         for( const uint32_t& h : hops )
         {
            sim_index.back().push_back(begin_offset + (b^h));
         }
      }
   }

   /*
   for( size_t i=0; i<all_strings.size(); i++ )
   {
      std::cout << all_strings[i] << std::endl;
      for( const uint32_t& j : sim_index[i] )
         std::cout << "   " << all_strings[j] << std::endl;
   }
   */

   std::cout << "checking conversions, size(), comparison operators" << std::endl;

   for( size_t i=0; i<all_strings.size(); i++ )
   {
      const std::string& s = all_strings[i];
      steemit::protocol::fixed_string_16 fs(s);
      std::string sfs = fs;
      if( s != fs )
      {
         std::cout << "problem on " << s << std::endl;
         ++errors;
      }
      if( fs.size() != s.size() )
      {
         std::cout << "problem on " << s << std::endl;
         ++errors;
      }

      check_variant( s, fs );
      check_pack( s, fs );

      for( const uint32_t& j : sim_index[i] )
      {
         const std::string& t = all_strings[j];
         steemit::protocol::fixed_string_16 ft(t);
         check( s, t, (s< t) == (fs< ft) );
         check( s, t, (s<=t) == (fs<=ft) );
         check( s, t, (s> t) == (fs> ft) );
         check( s, t, (s>=t) == (fs>=ft) );
         check( s, t, (s==t) == (fs==ft) );
         check( s, t, (s!=t) == (fs!=ft) );
      }
   }

   std::cout << "test_fixed_string_16 found " << errors << " errors" << std::endl;

   int result = (errors == 0) ? 0 : 1;

   errors = 0;
   all_strings = std::vector< std::string >();
   std::vector< steemit::protocol::fixed_string_24 > all_fixed_string_24s;
   sim_index = std::vector< std::vector< uint32_t > >();

   std::cout << "setting up LUT's" << std::endl;
   for( int len=0; len<=16; len++ )
   {
      int b_max = 1 << len;
      size_t begin_offset = all_strings.size();

      std::vector< uint32_t > hops;
      for( int b=0; b<b_max; b++ )
      {
         uint32_t ub = uint32_t(b);
         if( popcount(ub) <= 3 )
            hops.push_back( ub );
      }

      for( int b=0; b<b_max; b++ )
      {
         all_strings.emplace_back(len, '\0');
         for( int i=0,m=1; i<len; i++,m+=m )
         {
            all_strings.back()[len-i-1] = ((b&m)==0) ? 'a' : 'b';
         }

         sim_index.emplace_back();
         for( const uint32_t& h : hops )
         {
            sim_index.back().push_back(begin_offset + (b^h));
         }
      }
   }

   std::cout << "checking conversions, size(), comparison operators" << std::endl;

   for( size_t i=0; i<all_strings.size(); i++ )
   {
      const std::string& s = all_strings[i];
      steemit::protocol::fixed_string_24 fs(s);
      std::string sfs = fs;
      if( s != fs )
      {
         std::cout << "problem on " << s << std::endl;
         ++errors;
      }
      if( fs.size() != s.size() )
      {
         std::cout << "problem on " << s << std::endl;
         ++errors;
      }

      check_variant( s, fs );
      check_pack( s, fs );

      for( const uint32_t& j : sim_index[i] )
      {
         const std::string& t = all_strings[j];
         steemit::protocol::fixed_string_24 ft(t);
         check( s, t, (s< t) == (fs< ft) );
         check( s, t, (s<=t) == (fs<=ft) );
         check( s, t, (s> t) == (fs> ft) );
         check( s, t, (s>=t) == (fs>=ft) );
         check( s, t, (s==t) == (fs==ft) );
         check( s, t, (s!=t) == (fs!=ft) );
      }
   }

   std::cout << "test_fixed_string_24 found " << errors << " errors" << std::endl;

   result |= (errors == 0) ? 0 : 1;

   errors = 0;
   all_strings = std::vector< std::string >();
   std::vector< steemit::protocol::fixed_string_32 > all_fixed_string_32s;
   sim_index = std::vector< std::vector< uint32_t > >();

   std::cout << "setting up LUT's" << std::endl;
   for( int len=0; len<=16; len++ )
   {
      int b_max = 1 << len;
      size_t begin_offset = all_strings.size();

      std::vector< uint32_t > hops;
      for( int b=0; b<b_max; b++ )
      {
         uint32_t ub = uint32_t(b);
         if( popcount(ub) <= 3 )
            hops.push_back( ub );
      }

      for( int b=0; b<b_max; b++ )
      {
         all_strings.emplace_back(len, '\0');
         for( int i=0,m=1; i<len; i++,m+=m )
         {
            all_strings.back()[len-i-1] = ((b&m)==0) ? 'a' : 'b';
         }

         sim_index.emplace_back();
         for( const uint32_t& h : hops )
         {
            sim_index.back().push_back(begin_offset + (b^h));
         }
      }
   }

   std::cout << "checking conversions, size(), comparison operators" << std::endl;

   for( size_t i=0; i<all_strings.size(); i++ )
   {
      const std::string& s = all_strings[i];
      steemit::protocol::fixed_string_32 fs(s);
      std::string sfs = fs;
      if( s != fs )
      {
         std::cout << "problem on " << s << std::endl;
         ++errors;
      }
      if( fs.size() != s.size() )
      {
         std::cout << "problem on " << s << std::endl;
         ++errors;
      }

      check_variant( s, fs );
      check_pack( s, fs );

      for( const uint32_t& j : sim_index[i] )
      {
         const std::string& t = all_strings[j];
         steemit::protocol::fixed_string_32 ft(t);
         check( s, t, (s< t) == (fs< ft) );
         check( s, t, (s<=t) == (fs<=ft) );
         check( s, t, (s> t) == (fs> ft) );
         check( s, t, (s>=t) == (fs>=ft) );
         check( s, t, (s==t) == (fs==ft) );
         check( s, t, (s!=t) == (fs!=ft) );
      }
   }

   std::cout << "test_fixed_string_32 found " << errors << " errors" << std::endl;

   result |= (errors == 0) ? 0 : 1;

   return result;
}
