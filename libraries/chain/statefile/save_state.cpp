
#define BOOST_THREAD_PROVIDES_FUTURE

#include <steem/chain/database.hpp>
#include <steem/chain/index.hpp>
#include <steem/chain/statefile/statefile.hpp>

#include <boost/thread/future.hpp>
#include <boost/thread/sync_bounded_queue.hpp>

#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace steem { namespace chain { namespace statefile {

// Version        : Must precisely match what is output by embedded code.
// Header         : JSON object that lists sections
// Section header : JSON object that lists count of objects
// Section footer : JSON object that lists hash/offset of section header
// Footer         : JSON object that lists hash/offset of all sections

// db_format_version : Must match STEEM_DB_FORMAT_VERSION
// network_type      : Must match STEEM_NETWORK_TYPE
// chain_id          : Must match requested chain ID and value of embedded GPO

steem_version_info compute_embedded_version( const database& db )
{
   steem_version_info vinfo;
   vinfo.db_format_version = STEEM_DB_FORMAT_VERSION;
   vinfo.network_type = STEEM_NETWORK_TYPE;
   db.for_each_index_extension< index_info >( [&]( std::shared_ptr< index_info > info )
   {
      std::shared_ptr< abstract_schema > sch = info->get_schema();
      std::string schema_name, str_schema;
      sch->get_name( schema_name );
      sch->get_str_schema( str_schema );
      vinfo.object_schemas.emplace( schema_name, str_schema );
   } );

   vinfo.chain_id = db.get_chain_id().str();
   return vinfo;
}

/**
 * abstract_sink is an abstract output stream for the state file.
 */
class abstract_sink
{
   public:
      abstract_sink() {}
      virtual ~abstract_sink() {}
      virtual void write( const std::string& s ) = 0;
};

/**
 * section_producer is responsible for producing the section header and body
 * for an arbitrary section type.
 */
class section_producer
{
   public:
      section_producer() {}
      virtual ~section_producer() {}
      virtual void get_section_header( section_header& header ) = 0;
      virtual void write_section_body( abstract_sink& sink ) = 0;
};

/**
 * sink_impl keeps multiple running hashes for the current section's
 * footer, the toplevel footer, and the whole file.
 */
class sink_impl : public abstract_sink
{
   public:
      sink_impl( std::ostream& o ) : out(o) {}
      virtual ~sink_impl() {}
      virtual void write( const std::string& s );
      void begin_section();
      void end_section(section_footer& footer);
      void end_toplevel(section_footer& footer);
      void end_file(section_footer& footer);

   private:
      std::ostream&       out;
      fc::sha256::encoder enc_file;
      bool                in_toplevel = true;
      fc::sha256::encoder enc_toplevel;
      bool                in_section = false;
      fc::sha256::encoder enc_section;
      int64_t             begin_section_offset = 0;
      int64_t             size = 0;
};

/**
 * object_serializer takes objects as input, serializes them,
 * and outputs them to a stream.  The serialization is multi-threaded
 * to enhance performance.
 */
class object_serializer
{
   public:
      object_serializer();
      virtual ~object_serializer();

      void start_threads();
      void stop_threads();
      void write_table( const database& db, std::shared_ptr< index_info > info, abstract_sink& sink );

   private:

      void convert_thread_main();
      void input_thread_main();

      struct work_item
      {
         std::shared_ptr< index_info >                      info;
         int64_t                                            id = 0;
         const database*                                    db = nullptr;
         boost::promise< std::shared_ptr< std::string > >   done_promise;
         boost::future< std::shared_ptr< std::string > >    done_future = done_promise.get_future();

         static const int64_t ID_TABLE_WORK  = -1;
      };

      size_t                                 _max_queue_size = 100;
      boost::concurrent::sync_bounded_queue< std::shared_ptr< work_item > >    _table_queue;
      boost::concurrent::sync_bounded_queue< std::shared_ptr< work_item > >    _work_queue;
      boost::concurrent::sync_bounded_queue< std::shared_ptr< work_item > >    _output_queue;
      size_t                                 _thread_stack_size = 4096*1024;
      std::shared_ptr< boost::thread >       _input_thread;
      std::vector< boost::thread >           _serialization_threads;
};

object_serializer::object_serializer() :
  _table_queue( _max_queue_size ),
  _work_queue( _max_queue_size ),
  _output_queue( _max_queue_size ) {}
object_serializer::~object_serializer() {}

void object_serializer::input_thread_main()
{
   while( true )
   {
      std::shared_ptr< work_item > table_work;
      try
      {
         _table_queue.pull_front( table_work );
      }
      catch( const boost::concurrent::sync_queue_is_closed& e )
      {
         break;
      }

      std::string table_name;
      table_work->info->get_schema()->get_name( table_name );
      ilog( "Input thread: table ${t}", ("t", table_name) );

      table_work->info->for_each_object_id( *(table_work->db), [&]( int64_t id )
      {
         std::shared_ptr< work_item > work = std::make_shared< work_item >();
         work->info = table_work->info;
         work->id = id;
         work->db = table_work->db;
         _work_queue.push_back( work );
         _output_queue.push_back( work );
      } );
      table_work->done_promise.set_value( std::make_shared< std::string >() );
      _output_queue.push_back( table_work );
   }
}

void object_serializer::convert_thread_main()
{
   while( true )
   {
      std::shared_ptr< work_item > work;
      try
      {
         _work_queue.pull_front( work );
      }
      catch( const boost::concurrent::sync_queue_is_closed& e )
      {
         break;
      }

      // TODO exception handling
      std::shared_ptr< abstract_object > obj = work->info->get_object_from_db( *(work->db), work->id );
      std::shared_ptr< std::string > result = std::make_shared< std::string >();
      obj->to_json( *result );
      result->push_back( '\n' );
      work->done_promise.set_value( result );
   }
}

void object_serializer::write_table( const database& db, std::shared_ptr< index_info > info, abstract_sink& sink )
{
   std::shared_ptr< work_item > table_work = std::make_shared< work_item >();
   table_work->info = info;
   table_work->id = work_item::ID_TABLE_WORK;
   table_work->db = &db;
   _table_queue.push_back( table_work );

   while( true )
   {
      std::shared_ptr< work_item > work;
      try
      {
         _output_queue.pull_front( work );
      }
      catch( const boost::concurrent::sync_queue_is_closed& e )
      {
         break;
      }

      std::shared_ptr< std::string > value = work->done_future.get();
      if( work->id == work_item::ID_TABLE_WORK )
         break;
      sink.write( *value );
   }
}

void object_serializer::start_threads()
{
   boost::thread::attributes attrs;
   attrs.set_stack_size( _thread_stack_size );

   size_t num_threads = boost::thread::hardware_concurrency()+1;
   for( size_t i=0; i<num_threads; i++ )
   {
      _serialization_threads.emplace_back( attrs, [this]() { convert_thread_main(); } );
   }

   _input_thread = std::make_shared< boost::thread >( attrs, [this]() { input_thread_main(); } );
}

void object_serializer::stop_threads()
{
   _table_queue.close();
   _input_thread->join();
   _input_thread.reset();

   _work_queue.close();
   for( boost::thread& t : _serialization_threads )
      t.join();

   _output_queue.close();
}

/**
 * object_section_producer produces object_section which contains
 * objects of a single type.
 */
class object_section_producer : public section_producer
{
   public:
      object_section_producer(
         const database& d,
         std::shared_ptr< index_info > i,
         object_serializer& s ) : db(d), info(i), ser(s) {}
      virtual ~object_section_producer() {}

      virtual void get_section_header( section_header& header );
      virtual void write_section_body( abstract_sink& sink );

   private:
      const database& db;
      std::shared_ptr< index_info > info;
      object_serializer& ser;
};

void object_section_producer::get_section_header( section_header& header )
{
   object_section oheader;
   std::shared_ptr< abstract_schema > sch = info->get_schema();
   std::string schema_name;
   sch->get_name( oheader.object_type );
   oheader.format = "json";
   oheader.object_count = info->count( db );
   header = oheader;
}

void object_section_producer::write_section_body( abstract_sink& sink )
{
   ser.write_table( db, info, sink );
}

void sink_impl::begin_section()
{
   FC_ASSERT( in_section == false );
   in_section = true;
   enc_section.reset();
   begin_section_offset = size;
}

void sink_impl::end_section( section_footer& footer )
{
   FC_ASSERT( in_section == true );
   //
   // We want to be able to extend the format, so let's add some kind of prefix
   // that defines the type of hash that is used and how it is encoded.  The IPFS
   // project's multibase / multihash specs attempt to standardize such prefixes,
   // so let's use the prefixes they've defined instead of coming up with our own.
   // For now we only support the f1220 (32 bytes of lowercase hex encoded sha256),
   // so we just unconditionally add this prefix when writing and require/strip it
   // when reading.
   //
   footer.hash = "f1220" + enc_section.result().str();
   footer.begin_offset = begin_section_offset;
   footer.end_offset = size;
   begin_section_offset = 0;
   in_section = false;
}

void sink_impl::end_toplevel( section_footer& footer )
{
   FC_ASSERT( in_toplevel == true );
   footer.hash = "f1220" + enc_toplevel.result().str();
   footer.begin_offset = 0;
   footer.end_offset = size;
   in_toplevel = false;
}

void sink_impl::end_file( section_footer& footer )
{
   footer.hash = "f1220" + enc_file.result().str();
   footer.begin_offset = 0;
   footer.end_offset = size;
}

void sink_impl::write( const std::string& s )
{
   const char* p = s.c_str();
   size_t n = s.size();
   out.write( p, n );
   enc_file.write( p, n );
   if( in_toplevel )
      enc_toplevel.write( p, n );
   if( in_section )
      enc_section.write( p, n );
   size += int64_t(n);
}

write_state_result write_state( const database& db, const std::string& state_filename )
{
   std::ofstream out( state_filename, std::ios::binary );
   //
   // We have three layers:
   // - A running hash of the whole file for write_state_result
   // - A running hash of the whole file for the footer
   // - A running hash of each section for the section footer
   //
   // The sink will keep track of all of these hashes.
   //
   sink_impl sink( out );

   state_header top_header;
   state_footer top_footer;
   top_header.version = compute_embedded_version( db );

   std::vector< object_section_producer > producers;

   object_serializer ser;
   ser.start_threads();
   // Grab the object sections
   db.for_each_index_extension< index_info >(
   [&]( std::shared_ptr< index_info > info )
   {
      producers.emplace_back( db, info, ser );
      top_header.sections.emplace_back();
      producers.back().get_section_header( top_header.sections.back() );
   } );

   std::string top_header_json = fc::json::to_string( top_header );
   top_header_json.push_back('\n');
   sink.write( top_header_json );

   for( size_t i=0; i<producers.size(); i++ )
   {
      sink.begin_section();
      std::string section_header_json = fc::json::to_string( top_header.sections[i] );
      section_header_json.push_back('\n');
      sink.write( section_header_json );
      producers[i].write_section_body( sink );

      top_footer.section_footers.emplace_back();
      section_footer& footer = top_footer.section_footers.back();
      sink.end_section( footer );

      std::string footer_json = fc::json::to_string( footer );
      footer_json.push_back('\n');
      sink.write( footer_json );
   }
   ser.stop_threads();

   sink.end_toplevel( top_footer );
   std::string top_footer_json = fc::json::to_string( top_footer );
   top_footer_json.push_back('\n');
   sink.write( top_footer_json );
   out.flush();
   out.close();

   write_state_result result;
   section_footer temp;
   sink.end_file( temp );
   result.size = temp.end_offset;
   result.hash = temp.hash;
   return result;
}

} } }
