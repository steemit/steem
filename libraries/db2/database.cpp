#include <graphene/db2/database.hpp>
#include <fc/filesystem.hpp>
#include <fc/interprocess/file_mapping.hpp>
#include <fc/io/raw.hpp>
#include <fc/reflect/reflect.hpp>


namespace graphene { namespace db2 {

   void database::open( const fc::path& dir ) {
      if( _data_dir != dir ) close(); 
      
      if( !fc::exists( dir ) )
         fc::create_directories( dir );

      _data_dir = dir; 
      auto abs_path = fc::absolute( dir / "shared_memory" );
      _segment.reset( new bip::managed_mapped_file( bip::open_or_create, 
                                                    abs_path.generic_string().c_str(), 
                                                    uint64_t(1024*1024*64) ) );
      _mutex = _segment->find_or_construct< bip::interprocess_mutex >( "global_mutex" )();
   }

   void database::flush() {
      if( _segment )
         _segment->flush();
   }

   void database::close() {
      _segment.reset();
      _data_dir = fc::path();
   }


   void database::export_to_directory( const fc::path& dir )const {
      if( !fc::exists( dir ) ) fc::create_directories( dir );
      for( const auto& idx : _index_list ) {
         idx->export_to_file( dir / fc::to_string(idx->type_id()) );
      }
   }

   void database::import_from_directory( const fc::path& dir ) {
      FC_ASSERT( fc::exists( dir ) );
      for( const auto& idx : _index_list ) {
         idx->import_from_file( dir / fc::to_string(idx->type_id()) );
      }
   }

   void database::undo() {
      for( auto& item : _index_list ) {
         item->undo();
      }
   }
   void database::squash() {
      for( auto& item : _index_list ) {
         item->squash();
      }
   }
   void database::commit( int64_t revision ) {
      for( auto& item : _index_list ) {
         item->commit( revision );
      }
   }

   database::session database::start_undo_session( bool enabled ) {
      if( enabled ) {
         vector< std::unique_ptr<abstract_session> > _sub_sessions;
         _sub_sessions.reserve( _index_map.size() );
         for( auto& item : _index_list ) {
            _sub_sessions.push_back( item->start_undo_session( enabled ) );
         }
         return session( std::move( _sub_sessions ) );
      } else {
         return session();
      }
   }


} }


