/**
 *  @file fc/cmt/asio.hpp
 *  @brief defines wrappers for boost::asio functions
 */
#pragma once
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <fc/thread/future.hpp>
#include <fc/io/iostream.hpp>

namespace fc { 
/**
 *  @brief defines fc wrappers for boost::asio functions.
 */
namespace asio {
    /**
     *  @brief internal implementation types/methods for fc::asio
     */
    namespace detail {
        using namespace fc;

        class read_write_handler
        {
        public:
          read_write_handler(const promise<size_t>::ptr& p);
          void operator()(const boost::system::error_code& ec, size_t bytes_transferred);
        private:
          promise<size_t>::ptr _completion_promise;
        };

        class read_write_handler_with_buffer
        {
        public:
          read_write_handler_with_buffer(const promise<size_t>::ptr& p, 
                                         const std::shared_ptr<const char>& buffer);
          void operator()(const boost::system::error_code& ec, size_t bytes_transferred);
        private:
          promise<size_t>::ptr _completion_promise;
          std::shared_ptr<const char> _buffer;
        };

        //void read_write_handler( const promise<size_t>::ptr& p, 
        //                         const boost::system::error_code& ec, 
        //                         size_t bytes_transferred );
        void read_write_handler_ec( promise<size_t>* p, 
                                    boost::system::error_code* oec, 
                                    const boost::system::error_code& ec, 
                                    size_t bytes_transferred );
        void error_handler( const promise<void>::ptr& p, 
                              const boost::system::error_code& ec );
        void error_handler_ec( promise<boost::system::error_code>* p, 
                              const boost::system::error_code& ec ); 

        template<typename C>
        struct non_blocking { 
          bool operator()( C& c ) { return c.non_blocking(); } 
          bool operator()( C& c, bool s ) { c.non_blocking(s); return true; } 
        };

        #if WIN32  // windows stream handles do not support non blocking!
	       template<>
         struct non_blocking<boost::asio::windows::stream_handle> { 
	          typedef boost::asio::windows::stream_handle C;
            bool operator()( C& ) { return false; } 
            bool operator()( C&, bool ) { return false; } 
        };
        #endif 
    }
    /**
     * @return the default boost::asio::io_service for use with fc::asio
     * 
     * This IO service is automatically running in its own thread to service asynchronous
     * requests without blocking any other threads.
     */
    boost::asio::io_service& default_io_service(bool cleanup = false);

    /** 
     *  @brief wraps boost::asio::async_read
     *  @pre s.non_blocking() == true
     *  @return the number of bytes read.
     */
    template<typename AsyncReadStream, typename MutableBufferSequence>
    size_t read( AsyncReadStream& s, const MutableBufferSequence& buf ) {
        promise<size_t>::ptr p(new promise<size_t>("fc::asio::read"));
        boost::asio::async_read( s, buf, detail::read_write_handler(p) );
        return p->wait();
    }
    /** 
     *  This method will read at least 1 byte from the stream and will
     *  cooperatively block until that byte is available or an error occurs.
     *  
     *  If the stream is not in 'non-blocking' mode it will be put in 'non-blocking'
     *  mode it the stream supports s.non_blocking() and s.non_blocking(bool).
     *
     *  If in non blocking mode, the call will be synchronous avoiding heap allocs
     *  and context switching. If the sync call returns 'would block' then an
     *  promise is created and an async read is generated.
     *
     *  @return the number of bytes read.
     */
    template<typename AsyncReadStream, typename MutableBufferSequence>
    future<size_t> read_some(AsyncReadStream& s, const MutableBufferSequence& buf)
    {
      promise<size_t>::ptr completion_promise(new promise<size_t>("fc::asio::async_read_some"));
      s.async_read_some(buf, detail::read_write_handler(completion_promise));
      return completion_promise;//->wait();
    }

    template<typename AsyncReadStream>
    future<size_t> read_some(AsyncReadStream& s, char* buffer, size_t length, size_t offset = 0)
    {
      promise<size_t>::ptr completion_promise(new promise<size_t>("fc::asio::async_read_some"));
      s.async_read_some(boost::asio::buffer(buffer + offset, length), 
                        detail::read_write_handler(completion_promise));
      return completion_promise;//->wait();
    }

    template<typename AsyncReadStream>
    future<size_t> read_some(AsyncReadStream& s, const std::shared_ptr<char>& buffer, size_t length, size_t offset)
    {
      promise<size_t>::ptr completion_promise(new promise<size_t>("fc::asio::async_read_some"));
      s.async_read_some(boost::asio::buffer(buffer.get() + offset, length), 
                        detail::read_write_handler_with_buffer(completion_promise, buffer));
      return completion_promise;//->wait();
    }

    template<typename AsyncReadStream, typename MutableBufferSequence>
    void async_read_some(AsyncReadStream& s, const MutableBufferSequence& buf, promise<size_t>::ptr completion_promise)
    {
      s.async_read_some(buf, detail::read_write_handler(completion_promise));
    }

    template<typename AsyncReadStream>
    void async_read_some(AsyncReadStream& s, char* buffer,
                         size_t length, promise<size_t>::ptr completion_promise)
    {
      s.async_read_some(boost::asio::buffer(buffer, length), detail::read_write_handler(completion_promise));
    }

    template<typename AsyncReadStream>
    void async_read_some(AsyncReadStream& s, const std::shared_ptr<char>& buffer,
                         size_t length, size_t offset, promise<size_t>::ptr completion_promise)
    {
      s.async_read_some(boost::asio::buffer(buffer.get() + offset, length), detail::read_write_handler_with_buffer(completion_promise, buffer));
    }

    template<typename AsyncReadStream>
    size_t read_some( AsyncReadStream& s, boost::asio::streambuf& buf )
    {
        char buffer[1024];
        size_t bytes_read = read_some( s, boost::asio::buffer( buffer, sizeof(buffer) ) );
        buf.sputn( buffer, bytes_read );
        return bytes_read;
    }

    /** @brief wraps boost::asio::async_write
     *  @return the number of bytes written
     */
    template<typename AsyncWriteStream, typename ConstBufferSequence>
    size_t write( AsyncWriteStream& s, const ConstBufferSequence& buf ) {
        promise<size_t>::ptr p(new promise<size_t>("fc::asio::write"));
        boost::asio::async_write(s, buf, detail::read_write_handler(p));
        return p->wait();
    }

    /** 
     *  @pre s.non_blocking() == true
     *  @brief wraps boost::asio::async_write_some
     *  @return the number of bytes written
     */
    template<typename AsyncWriteStream, typename ConstBufferSequence>
    future<size_t> write_some( AsyncWriteStream& s, const ConstBufferSequence& buf ) {
        promise<size_t>::ptr p(new promise<size_t>("fc::asio::write_some"));
        s.async_write_some( buf, detail::read_write_handler(p));
        return p; //->wait();
    }

    template<typename AsyncWriteStream>
    future<size_t> write_some( AsyncWriteStream& s, const char* buffer, 
                               size_t length, size_t offset = 0) {
        promise<size_t>::ptr p(new promise<size_t>("fc::asio::write_some"));
        s.async_write_some( boost::asio::buffer(buffer + offset, length), detail::read_write_handler(p));
        return p; //->wait();
    }

    template<typename AsyncWriteStream>
    future<size_t> write_some( AsyncWriteStream& s, const std::shared_ptr<const char>& buffer, 
                               size_t length, size_t offset ) {
        promise<size_t>::ptr p(new promise<size_t>("fc::asio::write_some"));
        s.async_write_some( boost::asio::buffer(buffer.get() + offset, length), detail::read_write_handler_with_buffer(p, buffer));
        return p; //->wait();
    }

    /**
    *  @pre s.non_blocking() == true
    *  @brief wraps boost::asio::async_write_some
    *  @return the number of bytes written
    */
    template<typename AsyncWriteStream, typename ConstBufferSequence>
    void async_write_some(AsyncWriteStream& s, const ConstBufferSequence& buf, promise<size_t>::ptr completion_promise) {
      s.async_write_some(buf, detail::read_write_handler(completion_promise));
    }

    template<typename AsyncWriteStream>
    void async_write_some(AsyncWriteStream& s, const char* buffer, 
                          size_t length, promise<size_t>::ptr completion_promise) {
      s.async_write_some(boost::asio::buffer(buffer, length), 
                         detail::read_write_handler(completion_promise));
    }

    template<typename AsyncWriteStream>
    void async_write_some(AsyncWriteStream& s, const std::shared_ptr<const char>& buffer, 
                          size_t length, size_t offset, promise<size_t>::ptr completion_promise) {
      s.async_write_some(boost::asio::buffer(buffer.get() + offset, length), 
                         detail::read_write_handler_with_buffer(completion_promise, buffer));
    }

    namespace tcp {
        typedef boost::asio::ip::tcp::endpoint endpoint;
        typedef boost::asio::ip::tcp::resolver::iterator resolver_iterator;
        typedef boost::asio::ip::tcp::resolver resolver;
        std::vector<endpoint> resolve( const std::string& hostname, const std::string& port );

        /** @brief wraps boost::asio::async_accept
          * @post sock is connected
          * @post sock.non_blocking() == true  
          * @throw on error.
          */
        template<typename SocketType, typename AcceptorType>
        void accept( AcceptorType& acc, SocketType& sock ) {
            //promise<boost::system::error_code>::ptr p( new promise<boost::system::error_code>("fc::asio::tcp::accept") );
            promise<void>::ptr p( new promise<void>("fc::asio::tcp::accept") );
            acc.async_accept( sock, boost::bind( fc::asio::detail::error_handler, p, _1 ) );
            p->wait();
            //if( ec ) BOOST_THROW_EXCEPTION( boost::system::system_error(ec) );
        }

        /** @brief wraps boost::asio::socket::async_connect
          * @post sock.non_blocking() == true  
          * @throw on error
          */
        template<typename AsyncSocket, typename EndpointType>
        void connect( AsyncSocket& sock, const EndpointType& ep ) {
            promise<void>::ptr p(new promise<void>("fc::asio::tcp::connect"));
            sock.async_connect( ep, boost::bind( fc::asio::detail::error_handler, p, _1 ) );
            p->wait();
            //if( ec ) BOOST_THROW_EXCEPTION( boost::system::system_error(ec) );
        }
    }
    namespace udp {
        typedef boost::asio::ip::udp::endpoint endpoint;
        typedef boost::asio::ip::udp::resolver::iterator resolver_iterator;
        typedef boost::asio::ip::udp::resolver resolver;
        /// @brief resolve all udp::endpoints for hostname:port
        std::vector<endpoint> resolve( resolver& r, const std::string& hostname, 
                                                         const std::string& port );
    }

    template<typename AsyncReadStream>
    class istream : public virtual fc::istream
    {
       public:
          istream( std::shared_ptr<AsyncReadStream> str )
          :_stream( fc::move(str) ){}

          virtual size_t readsome( char* buf, size_t len )
          {
             return fc::asio::read_some(*_stream, buf, len).wait();
          }
          virtual size_t readsome( const std::shared_ptr<char>& buf, size_t len, size_t offset )
          {
             return fc::asio::read_some(*_stream, buf, len, offset).wait();
          }
    
       private:
          std::shared_ptr<AsyncReadStream> _stream;
    };

    template<typename AsyncWriteStream>
    class ostream : public virtual fc::ostream
    {
       public:
          ostream( std::shared_ptr<AsyncWriteStream> str )
          :_stream( fc::move(str) ){}

          virtual size_t writesome( const char* buf, size_t len )
          {
             return fc::asio::write_some(*_stream, buf, len).wait();
          }
    
          virtual size_t     writesome( const std::shared_ptr<const char>& buf, size_t len, size_t offset )
          {
             return fc::asio::write_some(*_stream, buf, len, offset).wait();
          }
    
          virtual void       close(){ _stream->close(); }
          virtual void       flush() {}
       private:
          std::shared_ptr<AsyncWriteStream> _stream;
    };


} } // namespace fc::asio

