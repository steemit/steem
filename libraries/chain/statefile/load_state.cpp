
#include <steem/chain/database.hpp>
#include <steem/chain/index.hpp>
#include <steem/chain/statefile/statefile.hpp>

#include <iostream>

namespace steem { namespace chain { namespace statefile {

void init_genesis_from_state( database& db, const std::string& state_filename )
{
   std::ifstream input_stream( state_filename, std::ios::binary );

   vector< char > top_header_bin;
   char c;
   input_stream.read( &c, 1 );

   while( c != '\n' )
   {
      top_header_bin.push_back( c );
      input_stream.read( &c, 1 );
   }

   state_header top_header = fc::json::from_string( std::string( top_header_bin.begin(), top_header_bin.end() ) ).as< state_header >();
   steem_version_info expected_version = steem_version_info( db );

   // Compare versions

   flat_map< std::string, std::shared_ptr< index_info > > index_map;
   db.for_each_index_extension< index_info >( [&]( std::shared_ptr< index_info > info )
   {
      std::string name;
      info->get_schema()->get_name( name );
      index_map[ name ] = info;
   });

   flat_map< std::string, section_header > header_map;
   for( const auto& header : top_header.sections )
   {
      object_section obj_header = header.get< object_section >();
      header_map.insert_or_assign( obj_header.object_type, std::move( obj_header ) );
   }

   for( const auto& idx : index_map )
   {
      auto itr = header_map.find( idx.first );
      FC_ASSERT( itr != header_map.end(), "Did not find expected object index: ${o}", ("o", idx.first) );

      std::string expected_schema;
      idx.second->get_schema()->get_str_schema( expected_schema );

      FC_TODO( "Version object data to allow for upgrading during load" );
      FC_ASSERT( expected_schema.compare( itr->second.get< object_section >().schema ) == 0,
         "Unexpected incoming schema for object ${o}.\nExpected: ${e}\nActual: ${a}",
         ("o", idx.first)("e", expected_schema)("a", itr->second.get< object_section >().schema) );
   }

   for( const auto& header_sv : top_header.sections )
   {
      vector< char > section_header_bin;

      do
      {
         input_stream.read( &c, 1 );
         section_header_bin.push_back( c );
      } while( c != '\n' );

      object_section header = header_sv.get< object_section >();
      object_section s_header = fc::json::from_string( std::string( section_header_bin.begin(), section_header_bin.end() ) ).as< object_section >();

      FC_ASSERT( header.object_type.compare( s_header.object_type ) == 0, "Expected next object type: ${e} actual: ${a}",
         ("e", header.object_type)("a", s_header.object_type) );
      FC_ASSERT( header.format.compare( s_header.format ) == 0, "Mismatched object format for ${o}.",
         ("o", header.object_type) );
      FC_ASSERT( header.object_count == s_header.object_count, "Mismatched object count for ${o}.",
         ("o", header.object_type) );
      FC_ASSERT( header.schema.compare( s_header.schema ) == 0, "Mismatched object schema for ${o}.",
         ("o", header.object_type) );

      int64_t begin = input_stream.tellg();
      std::shared_ptr< index_info > idx = index_map[ header.object_type ];

      for( int64_t i = 0; i < header.object_count; i++ )
      {
         if( header.format == FORMAT_BINARY )
            idx->create_object_from_binary( db, input_stream );

      }

      int64_t end = input_stream.tellg();

      vector< char > section_footer_bin;

      do
      {
         input_stream.read( &c, 1 );
         section_footer_bin.push_back( c );
      } while( c != '\n' );

      section_footer s_footer = fc::json::from_string( std::string( section_footer_bin.begin(), section_footer_bin.end() ) ).as< section_footer >();

      FC_ASSERT( s_footer.begin_offset == begin, "Begin offset mismatch for ${o}",
         ("o", header.object_type) );
      FC_ASSERT( s_footer.end_offset == end, "Begin offset mismatch for ${o}",
         ("o", header.object_type) );
      FC_TODO( "Compare hashes" );
   }
}

} } }
