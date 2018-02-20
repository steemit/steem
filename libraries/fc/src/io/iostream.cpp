#include <fc/io/iostream.hpp>
#include <fc/io/sstream.hpp>
#include <fc/thread/thread.hpp>
#include <iostream>
#include <string.h>
//#include <fc/log.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/thread/scoped_lock.hpp>
#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/thread/mutex.hpp>
#include <fc/io/stdio.hpp>
#include <fc/log/logger.hpp>

namespace fc {
  
  struct cin_buffer {
    cin_buffer():eof(false),write_pos(0),read_pos(0),cinthread("cin"){
    
      cinthread.async( [=](){read();}, "cin_buffer::read" );
    }

    void     read() {
      char c;
      std::cin.read(&c,1);
      while( !std::cin.eof() ) {
        while( write_pos - read_pos > 0xfffff ) {
          fc::promise<void>::ptr wr( new fc::promise<void>("cin_buffer::write_ready") );
          write_ready = wr;
          if( write_pos - read_pos <= 0xfffff ) {
            wr->wait();
          }
          write_ready.reset();
        }
        buf[write_pos&0xfffff] = c;
        ++write_pos;

        fc::promise<void>::ptr tmp;
        { // copy read_ready because it is accessed from multiple threads
          fc::scoped_lock<boost::mutex> lock( read_ready_mutex ); 
          tmp = read_ready; 
        }

        if( tmp && !tmp->ready() ) { 
          tmp->set_value(); 
        }
        std::cin.read(&c,1);
      }
      eof = true;
      fc::promise<void>::ptr tmp;
      {  // copy read_ready because it is accessed from multiple threads
        fc::scoped_lock<boost::mutex> lock( read_ready_mutex ); 
        tmp = read_ready;
      }
      if( tmp && !tmp->ready() ) { 
        tmp->set_exception( exception_ptr( new eof_exception() )); 
      }
    }
    boost::mutex              read_ready_mutex;
    fc::promise<void>::ptr read_ready;
    fc::promise<void>::ptr write_ready;
    
    volatile bool     eof;
    
    volatile uint64_t write_pos;
             char     buf[0xfffff+1]; // 1 mb buffer
    volatile uint64_t read_pos;
    fc::thread        cinthread;
  };

  cin_buffer& get_cin_buffer() {
    static cin_buffer* b = new cin_buffer();
    return *b;
  }


  fc::thread& cin_thread() { static fc::thread i("cin"); return i; }

  fc::istream& getline( fc::istream& i, fc::string& s, char delim  ) {
    fc::stringstream ss; 
    char c;
    i.read( &c, 1 );
    while( true ) {
      if( c == delim ) { s = ss.str();  return i; }
      if( c != '\r' ) ss.write(&c,1);
      i.read( &c, 1 );
    }
    s = ss.str();
    return i;
  }


  size_t cout_t::writesome( const char* buf, size_t len ) { std::cout.write(buf,len); return len; }
  size_t cout_t::writesome( const std::shared_ptr<const char>& buf, size_t len, size_t offset ) { return writesome(buf.get() + offset, len); }
  void   cout_t::close() {}
  void   cout_t::flush() { std::cout.flush(); }

  size_t cerr_t::writesome( const char* buf, size_t len ) { std::cerr.write(buf,len); return len; }
  size_t cerr_t::writesome( const std::shared_ptr<const char>& buf, size_t len, size_t offset ) { return writesome(buf.get() + offset, len); }
  void   cerr_t::close() {};
  void   cerr_t::flush() { std::cerr.flush(); }


  size_t cin_t::readsome( char* buf, size_t len ) {
    cin_buffer& b = get_cin_buffer();
    int64_t avail = b.write_pos - b.read_pos;
    avail = (fc::min)(int64_t(len),avail);
    int64_t u = 0;

    if( !((avail>0) && (len>0)) ) {
      read( buf, 1 );
      ++buf;
      ++u;
      --len;
    }

    while( (avail>0) && (len>0) ) {
      *buf = b.buf[b.read_pos&0xfffff]; 
      ++b.read_pos;
      ++buf;
      --avail;
      --len;
      ++u;
    }
    return size_t(u);
  }
  size_t cin_t::readsome( const std::shared_ptr<char>& buf, size_t len, size_t offset ) { return readsome(buf.get() + offset, len); }

  cin_t::~cin_t() {
    /*
    cin_buffer& b = get_cin_buffer();
    if( b.read_ready ) {
        b.read_ready->wait();
    }
    */
  }
  istream& cin_t::read( char* buf, size_t len ) {
    cin_buffer& b = get_cin_buffer();
    do {
        while( !b.eof &&  (b.write_pos - b.read_pos)==0 ){ 
           // wait for more... 
           fc::promise<void>::ptr rr( new fc::promise<void>("cin_buffer::read_ready") );
           {  // copy read_ready because it is accessed from multiple threads
             fc::scoped_lock<boost::mutex> lock( b.read_ready_mutex ); 
             b.read_ready = rr;
           }
           if( b.write_pos - b.read_pos == 0 ) {
             rr->wait();
           }
         //  b.read_ready.reset();
           {  // copy read_ready because it is accessed from multiple threads
             fc::scoped_lock<boost::mutex> lock( b.read_ready_mutex ); 
             b.read_ready.reset();
           }
        }
        if( b.eof ) FC_THROW_EXCEPTION( eof_exception, "cin" );
        size_t r = readsome( buf, len );
        buf += r;
        len -= r;

        auto tmp = b.write_ready; // copy write_writey because it is accessed from multiple thwrites
        if( tmp && !tmp->ready() ) { 
          tmp->set_value(); 
        }
    } while( len > 0 && !b.eof );
    if( b.eof ) FC_THROW_EXCEPTION( eof_exception, "cin" );
    return *this;
  }

  bool cin_t::eof()const { return get_cin_buffer().eof; }

  
  std::shared_ptr<cin_t>  cin_ptr = std::make_shared<cin_t>();
  std::shared_ptr<cout_t> cout_ptr = std::make_shared<cout_t>();
  std::shared_ptr<cerr_t> cerr_ptr = std::make_shared<cerr_t>();
  cout_t& cout = *cout_ptr;
  cerr_t& cerr = *cerr_ptr;
  cin_t&  cin  = *cin_ptr;


  ostream& operator<<( ostream& o, const char v )
  {
     o.write( &v, 1 );
     return o;
  }
  ostream& operator<<( ostream& o, const char* v )
  {
     o.write( v, strlen(v) );
     return o;
  }

  ostream& operator<<( ostream& o, const std::string& v )
  {
     o.write( v.c_str(), v.size() );
     return o;
  }
#ifdef USE_FC_STRING
  ostream& operator<<( ostream& o, const fc::string& v )
  {
     o.write( v.c_str(), v.size() );
     return o;
  }
#endif

  ostream& operator<<( ostream& o, const double& v )
  {
     return o << boost::lexical_cast<std::string>(v).c_str();
  }

  ostream& operator<<( ostream& o, const float& v )
  {
     return o << boost::lexical_cast<std::string>(v).c_str();
  }

  ostream& operator<<( ostream& o, const int64_t& v )
  {
     return o << boost::lexical_cast<std::string>(v).c_str();
  }

  ostream& operator<<( ostream& o, const uint64_t& v )
  {
     return o << boost::lexical_cast<std::string>(v).c_str();
  }

  ostream& operator<<( ostream& o, const int32_t& v )
  {
     return o << boost::lexical_cast<std::string>(v).c_str();
  }

  ostream& operator<<( ostream& o, const uint32_t& v )
  {
     return o << boost::lexical_cast<std::string>(v).c_str();
  }

  ostream& operator<<( ostream& o, const int16_t& v )
  {
     return o << boost::lexical_cast<std::string>(v).c_str();
  }

  ostream& operator<<( ostream& o, const uint16_t& v )
  {
     return o << boost::lexical_cast<std::string>(v).c_str();
  }

  ostream& operator<<( ostream& o, const int8_t& v )
  {
     return o << boost::lexical_cast<std::string>(v).c_str();
  }

  ostream& operator<<( ostream& o, const uint8_t& v )
  {
     return o << boost::lexical_cast<std::string>(v).c_str();
  }

#ifdef __APPLE__
  ostream& operator<<( ostream& o, const size_t& v )
  {
     return o << boost::lexical_cast<std::string>(v).c_str();
  }

#endif

  istream& operator>>( istream& o, std::string& v )
  {
     assert(false && "not implemented");
     return o;
  }

#ifdef USE_FC_STRING
  istream& operator>>( istream& o, fc::string& v )
  {
     assert(false && "not implemented");
     return o;
  }
#endif

  istream& operator>>( istream& o, char& v )
  {
     o.read(&v,1);
     return o;
  }

  istream& operator>>( istream& o, double& v )
  {
     assert(false && "not implemented");
     return o;
  }

  istream& operator>>( istream& o, float& v )
  {
     assert(false && "not implemented");
     return o;
  }

  istream& operator>>( istream& o, int64_t& v )
  {
     assert(false && "not implemented");
     return o;
  }

  istream& operator>>( istream& o, uint64_t& v )
  {
     assert(false && "not implemented");
     return o;
  }

  istream& operator>>( istream& o, int32_t& v )
  {
     assert(false && "not implemented");
     return o;
  }

  istream& operator>>( istream& o, uint32_t& v )
  {
     assert(false && "not implemented");
     return o;
  }

  istream& operator>>( istream& o, int16_t& v )
  {
     assert(false && "not implemented");
     return o;
  }

  istream& operator>>( istream& o, uint16_t& v )
  {
     assert(false && "not implemented");
     return o;
  }

  istream& operator>>( istream& o, int8_t& v )
  {
     assert(false && "not implemented");
     return o;
  }

  istream& operator>>( istream& o, uint8_t& v )
  {
     assert(false && "not implemented");
     return o;
  }


  char istream::get()
  {
    char tmp;
    read(&tmp,1);
    return tmp;
  }

  istream& istream::read( char* buf, size_t len )
  {
      char* pos = buf;
      while( size_t(pos-buf) < len )
         pos += readsome( pos, len - (pos - buf) );
      return *this;
  }

  istream& istream::read( const std::shared_ptr<char>& buf, size_t len, size_t offset )
  {
    size_t bytes_read = 0;
    while( bytes_read < len )
      bytes_read += readsome(buf, len - bytes_read, bytes_read + offset);
    return *this;
  }

  ostream& ostream::write( const char* buf, size_t len )
  {
      const char* pos = buf;
      while( size_t(pos-buf) < len )
         pos += writesome( pos, len - (pos - buf) );
      return *this;
  }

  ostream& ostream::write( const std::shared_ptr<const char>& buf, size_t len, size_t offset )
  {
    size_t bytes_written = 0;
    while( bytes_written < len )
      bytes_written += writesome(buf, len - bytes_written, bytes_written + offset);
    return *this;
  }

} // namespace fc
