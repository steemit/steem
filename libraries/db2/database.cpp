#include <graphene/db2/database.hpp>
#include <fc/filesystem.hpp>
#include <fc/interprocess/file_mapping.hpp>
#include <fc/io/raw.hpp>
#include <fc/reflect/reflect.hpp>


namespace graphene { namespace db2 {

   struct environment_check {
      environment_check() {
         memcpy( &compiler_version.at(0), __VERSION__, std::min<size_t>( strlen(__VERSION__), 256 ) );
#ifndef NDEBUG
         debug = true;
#endif
#ifdef __APPLE__
         apple = true;
#endif
#ifdef WIN32
         windows = true;
#endif
      }
      friend bool operator == ( const environment_check& a, const environment_check& b ) {
         return std::make_tuple( a.compiler_version, a.debug, a.apple, a.windows )
            ==  std::make_tuple( b.compiler_version, b.debug, b.apple, b.windows );
      }

      fc::array<char,256>  compiler_version;
      bool                 debug = false;
      bool                 apple = false;
      bool                 windows = false;
   };

   void database::open( const fc::path& dir, uint64_t shared_file_size ) {
      if( _data_dir != dir ) close();

      if( !fc::exists( dir ) )
         fc::create_directories( dir );

      _data_dir = dir;
      auto abs_path = fc::absolute( dir / "shared_memory" );

      if( fc::exists( abs_path ) )
         shared_file_size = fc::file_size( abs_path );

      _segment.reset( new bip::managed_mapped_file( bip::open_or_create,
                                                    abs_path.generic_string().c_str(),
                                                    shared_file_size ) );
      _mutex = _segment->find_or_construct< bip::interprocess_mutex >( "global_mutex" )();
      auto env = _segment->find_or_construct< environment_check >( "environment" )();
      FC_ASSERT( *env == environment_check() );
   }

   void database::flush() {
      if( _segment )
         _segment->flush();
   }

   void database::close()
   {
      _segment.reset();
      _data_dir = fc::path();
   }

   void database::wipe( const fc::path& dir )
   {
      _segment.reset();
      fc::remove_all( dir / "shared_memory" );
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

   void database::undo()
   {
      for( auto& item : _index_list )
      {
         item->undo();
      }
   }
   void database::squash()
   {
      for( auto& item : _index_list )
      {
         item->squash();
      }
   }
   void database::commit( int64_t revision )
   {
      for( auto& item : _index_list )
      {
         item->commit( revision );
      }
   }

   void database::undo_all()
   {
      for( auto& item : _index_list )
      {
         item->undo_all();
      }
   }

   database::session database::start_undo_session( bool enabled )
   {
      try
      {
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
      } FC_LOG_AND_RETHROW()
   }

   generic_id database::generic_id_from_variant( const fc::variant& var )const
   {
      const fc::variants& array = var.get_array();
      FC_ASSERT( array.size() == 2 );
      uint16_t type_id;
      if( array[0].is_string() )
      {
         const std::string& name = array[0].get_string();
         auto it = _type_name_to_id.find( name );
         FC_ASSERT( it != _type_name_to_id.end() );
         type_id = it->second;
      }
      else
      {
         FC_ASSERT( array[0].is_int64() );
         int64_t i = array[0].as_int64();
         FC_ASSERT( (i >= 0) && (i <= std::numeric_limits< uint16_t >::max()) );
         type_id = uint16_t(i);;
      }
      FC_ASSERT( array[1].is_int64() );
      int64_t instance_id = array[1].as_int64();
      return generic_id( type_id, instance_id );
   }

   void database::remove_object( generic_id gid )
   {
      abstract_index* idx = _index_map[ gid._type_id ].get();
      FC_ASSERT( idx != nullptr );
      idx->remove_object( gid._id );
   }

   fc::variant database::find_variant( generic_id gid )const
   {
      abstract_index* idx = _index_map[ gid._type_id ].get();
      FC_ASSERT( idx != nullptr );
      return idx->find_variant( gid._id );
   }

   fc::variant database::create_variant( generic_id gid, const fc::variant& var )
   {
      abstract_index* idx = _index_map[ gid._type_id ].get();
      FC_ASSERT( idx != nullptr );
      return idx->create_variant( gid._id, var );
   }

   void database::modify_variant( generic_id gid, const fc::variant& var )
   {
      abstract_index* idx = _index_map[ gid._type_id ].get();
      FC_ASSERT( idx != nullptr );
      idx->modify_variant( gid._id, var );
   }

   std::vector<char> database::find_binary( generic_id gid )const
   {
      abstract_index* idx = _index_map[ gid._type_id ].get();
      FC_ASSERT( idx != nullptr );
      return idx->find_binary( gid._id );
   }

   std::vector<char> database::create_binary( generic_id gid, const std::vector<char>& bin )
   {
      abstract_index* idx = _index_map[ gid._type_id ].get();
      FC_ASSERT( idx != nullptr );
      return idx->create_binary( gid._id, bin );
   }


} }


