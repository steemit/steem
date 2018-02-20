#pragma once
#include <fc/utility.hpp>
#include <fc/string.hpp>
#include <memory>

namespace fc {

  /**
   *  Provides a fc::thread friendly cooperatively multi-tasked stream that
   *  will block 'cooperatively' instead of hard blocking.
   */
  class istream
  {
    public:
      virtual ~istream(){};

      /** read at least 1 byte or throw, if no data is available
       *  this method should block cooperatively until data is
       *  available or fc::eof is thrown.
       *
       *  @throws fc::eof if at least 1 byte cannot be read
       **/
      virtual size_t     readsome( char* buf, size_t len ) = 0;
      virtual size_t     readsome( const std::shared_ptr<char>& buf, size_t len, size_t offset ) = 0;

      /** read len bytes or throw, this method is implemented
       *  in terms of readsome.
       *
       *  @throws fc::eof_exception if len bytes cannot be read
       **/
      istream&   read( char* buf, size_t len );
      istream&   read( const std::shared_ptr<char>& buf, size_t len, size_t offset = 0 );
      virtual char get();
      void get( char& c ) { c = get(); }
  };
  typedef std::shared_ptr<istream> istream_ptr;

  /**
   *  Provides a fc::thread friendly cooperatively multi-tasked stream that
   *  will block 'cooperatively' instead of hard blocking.
   */
  class ostream
  {
     public:
       virtual ~ostream(){};
       virtual size_t     writesome( const char* buf, size_t len ) = 0;
       virtual size_t     writesome( const std::shared_ptr<const char>& buf, size_t len, size_t offset ) = 0;
       virtual void       close() = 0;
       virtual void       flush() = 0;

       void put( char c ) { write(&c,1); }

       /** implemented in terms of writesome, guarantees len bytes are sent
        * but not flushed.
        **/
       ostream&   write( const char* buf, size_t len );
       ostream&   write( const std::shared_ptr<const char>& buf, size_t len, size_t offset = 0 );
  };

  typedef std::shared_ptr<ostream> ostream_ptr;

  class iostream : public virtual ostream, public virtual istream {};

  fc::istream& getline( fc::istream&, fc::string&, char delim = '\n' );

  template<size_t N>
  ostream& operator<<( ostream& o, char (&array)[N] )
  {
     return o.write( array, N );
  }

  ostream& operator<<( ostream& o, char );
  ostream& operator<<( ostream& o, const char* v );
  ostream& operator<<( ostream& o, const std::string& v );
  ostream& operator<<( ostream& o, const fc::string& v );
  ostream& operator<<( ostream& o, const double& v );
  ostream& operator<<( ostream& o, const float& v );
  ostream& operator<<( ostream& o, const int64_t& v );
  ostream& operator<<( ostream& o, const uint64_t& v );
  ostream& operator<<( ostream& o, const int32_t& v );
  ostream& operator<<( ostream& o, const uint32_t& v );
  ostream& operator<<( ostream& o, const int16_t& v );
  ostream& operator<<( ostream& o, const uint16_t& v );
  ostream& operator<<( ostream& o, const int8_t& v );
  ostream& operator<<( ostream& o, const uint8_t& v );
#ifndef _MSC_VER
  ostream& operator<<( ostream& o, const size_t& v );
#endif

  istream& operator>>( istream& o, std::string& v );
  istream& operator>>( istream& o, fc::string& v );
  istream& operator>>( istream& o, char& v );
  istream& operator>>( istream& o, double& v );
  istream& operator>>( istream& o, float& v );
  istream& operator>>( istream& o, int64_t& v );
  istream& operator>>( istream& o, uint64_t& v );
  istream& operator>>( istream& o, int32_t& v );
  istream& operator>>( istream& o, uint32_t& v );
  istream& operator>>( istream& o, int16_t& v );
  istream& operator>>( istream& o, uint16_t& v );
  istream& operator>>( istream& o, int8_t& v );
  istream& operator>>( istream& o, uint8_t& v );
}
