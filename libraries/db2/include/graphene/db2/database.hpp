#pragma once
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <boost/interprocess/containers/deque.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/filesystem.hpp>
#include <fc/log/logger.hpp>
#include <fc/exception/exception.hpp>
#include <fc/container/flat.hpp>
#include <fc/io/raw_fwd.hpp>
#include <fc/filesystem.hpp>
#include <fc/interprocess/file_mapping.hpp>
#include <fc/io/datastream.hpp>

#include <typeindex>
#include <fstream>


namespace graphene { namespace db2 {
   template<typename T> class oid;
} }

namespace fc {
  template<typename T> void to_variant( const graphene::db2::oid<T>& var,  variant& vo );
  template<typename T> void from_variant( const variant& vo, graphene::db2::oid<T>& var );

  namespace raw {
    template<typename Stream, typename T> inline void pack( Stream& s, const graphene::db2::oid<T>& id );
    template<typename Stream, typename T> inline void unpack( Stream& s, graphene::db2::oid<T>& id );
  }
}


namespace graphene { namespace db2 {

   namespace bip = boost::interprocess;
   using std::unique_ptr;
   using std::vector;


   template<typename T>
   using allocator = bip::allocator<T, bip::managed_mapped_file::segment_manager>;

   class database;

   /**
    *  Object ID type that includes the type of the object it references
    */
   template<typename T>
   class oid {
      public:
         oid( int64_t i = 0 ):_id(i){}
         const T& operator()( const database& db )const;

         oid& operator++() { ++_id; return *this; }

         friend bool operator < ( const oid& a, const oid& b ) { return a._id < b._id; }
         friend bool operator > ( const oid& a, const oid& b ) { return a._id > b._id; }
         friend bool operator == ( const oid& a, const oid& b ) { return a._id == b._id; }
         friend bool operator != ( const oid& a, const oid& b ) { return a._id != b._id; }
         int64_t _id = 0;
   };

   template<uint16_t TypeNumber, typename Derived>
   struct object {
      typedef oid<Derived> id_type;
      static const uint16_t type_id = TypeNumber;
   };

   /** this class is ment to be specified to enable lookup of index type by object type using
    * the SET_INDEX_TYPE macro.
    **/
   template<typename T>
   struct get_index_type {};

   /**
    *  This macro must be used at global scope and OBJECT_TYPE and INDEX_TYPE must be fully qualified
    */
   #define GRAPHENE_DB2_SET_INDEX_TYPE( OBJECT_TYPE, INDEX_TYPE )  \
   namespace graphene { namespace db2 { template<> struct get_index_type<OBJECT_TYPE> { typedef INDEX_TYPE type; }; } }
   #define SET_INDEX_TYPE( OBJECT_TYPE, INDEX_TYPE ) GRAPHENE_DB2_SET_INDEX_TYPE( OBJECT_TYPE, INDEX_TYPE )

   /**
    *  The value_type stored in the multiindex container must have a integer field with the name 'id'.  This will
    *  be the primary key and it will be assigned and managed by generic_index.
    *
    *  Additionally, the constructor for value_type must take an allocator
    */
   template<typename MultiIndexType>
   class generic_index {
      public:
         typedef bip::managed_mapped_file::segment_manager             segment_manager_type;
         typedef MultiIndexType                                        index_type;
         typedef typename index_type::value_type                       value_type;
         typedef bip::allocator< generic_index, segment_manager_type > allocator_type;

         generic_index( allocator<value_type> a )
         :_stack(a),_indices( a ),_size_of_value_type( sizeof(typename MultiIndexType::node_type) ),_size_of_this(sizeof(*this)){}

         void validate()const {
            FC_ASSERT( sizeof(typename MultiIndexType::node_type) == _size_of_value_type );
            FC_ASSERT( sizeof(*this)      == _size_of_this );
         }

         void export_to_file( const fc::path& filename )const {
            std::ofstream out( filename.generic_string(),
                               std::ofstream::binary | std::ofstream::out | std::ofstream::trunc );
            fc::raw::pack( out, int32_t(value_type::type_id) );
            fc::raw::pack( out, uint64_t( _indices.size() ) );
            fc::raw::pack( out, _next_id );

            /*for( const auto& item : _indices ) {
               fc::raw::pack( out, item );
            }*/
         }

         void import_from_file( const fc::path& filename ) {
            int32_t   id = 0;
            uint64_t  size = 0;

            fc::file_mapping fm( filename.generic_string().c_str(), fc::read_only );
            fc::mapped_region mr( fm, fc::read_only, 0, fc::file_size(filename) );
            fc::datastream<const char*> in( (const char*)mr.get_address(), mr.get_size() );

            fc::raw::unpack( in, id );
            fc::raw::unpack( in, size );
            fc::raw::unpack( in, _next_id );

            FC_ASSERT( id == value_type::type_id );

            /*for( uint64_t i = 0; i < size; ++i ) {
               auto insert_result = _indices.emplace( [&]( value_type& v ) {
                  fc::raw::unpack( in, v );
               }, _indices.get_allocator() );
               FC_ASSERT( insert_result.second, "Could not import object, most likely a uniqueness constraint was violated" );
            }*/
         }

         /**
          * Construct a new element in the multi_index_container.
          * Set the ID to the next available ID, then increment _next_id and fire off on_create().
          */
         template<typename Constructor>
         const value_type& emplace( Constructor&& c ) {
            auto new_id = _next_id;

            auto constructor = [&]( value_type& v ) {
               v.id = new_id;
               c( v );
            };

            auto insert_result = _indices.emplace( constructor, _indices.get_allocator() );

            FC_ASSERT( insert_result.second, "Could not insert object, most likely a uniqueness constraint was violated" );

            ++_next_id;
            on_create( *insert_result.first );
            return *insert_result.first;
         }

         template<typename Modifier>
         void modify( const value_type& obj, Modifier&& m ) {
            on_modify( obj );
            auto ok = _indices.modify( _indices.iterator_to( obj ), m );
            FC_ASSERT( ok, "Could not modify object, most likely a uniqueness constraint was violated" );
         }

         void remove( const value_type& obj ) {
            on_remove( obj );
            _indices.erase( _indices.iterator_to( obj ) );
         }

         template<typename CompatibleKey>
         const value_type* find( CompatibleKey&& key )const {
            auto itr = _indices.find( std::forward<CompatibleKey>(key) );
            if( itr != _indices.end() ) return *itr;
            return nullptr;
         }

         const index_type& indices()const { return _indices; }

         class undo_state {
            public:
               typedef typename value_type::id_type                      id_type;
               typedef allocator< std::pair<const id_type, value_type> > id_value_allocator_type;
               typedef allocator< id_type >                              id_allocator_type;

               template<typename T>
               undo_state( allocator<T> al )
               :old_values( id_value_allocator_type( al.get_segment_manager() ) ),
                removed_values( id_value_allocator_type( al.get_segment_manager() ) ),
                new_ids( id_allocator_type( al.get_segment_manager() ) ){}


               typedef boost::interprocess::map< id_type, value_type, std::less<id_type>,  id_value_allocator_type > id_value_type_map;

               id_value_type_map                                                           old_values;
               id_value_type_map                                                           removed_values;
               boost::interprocess::set< id_type, std::less<id_type>, id_allocator_type>   new_ids;
               id_type                                                                     old_next_id = 0;
               int64_t                                                                     revision = 0;
         };

         class session {
            public:
               session( session&& mv )
               :_index(mv._index),_apply(mv._apply){ mv._apply = false; }

               ~session() {
                  if( _apply ) {
                     _index.undo();
                  }
               }

               /** leaves the UNDO state on the stack when session goes out of scope */
               void push()   { _apply = false; }
               /** combines this session with the prior session */
               void squash() { if( _apply ) _index.squash(); _apply = false; }
               void undo()   { if( _apply ) _index.undo();  _apply = false; }

               session& operator = ( session&& mv ) {
                  if( this == &mv ) return *this;
                  if( _apply ) _index.undo();
                  _apply = mv._apply;
                  mv._apply = false;
                  return *this;
               }

               int64_t revision()const { return _revision; }

            private:
               friend class generic_index;

               session( generic_index& idx, int64_t revision )
               :_index(idx),_revision(revision) {
                  if( revision == -1 )
                     _apply = false;
               }

               generic_index& _index;
               bool           _apply = true;
               int64_t        _revision = 0;
         };

         session start_undo_session( bool enabled ) {
            if( enabled ) {
               _stack.emplace_back( _indices.get_allocator() );
               _stack.back().old_next_id = _next_id;
               _stack.back().revision = ++_revision;
               return session( *this, _revision );
            } else {
               return session( *this, -1 );
            }
         }

         const index_type& indicies()const { return _indices; }
         int64_t revision()const { return _revision; }


         /**
          *  Restores the state to how it was prior to the current session discarding all changes
          *  made between the last revision and the current revision.
          */
         void undo() {
            if( !enabled() ) return;


            const auto& head = _stack.back();
            for( auto& item : head.old_values ) {
               _indices.modify( _indices.find( item.second.id ), [&]( value_type& v ) {
                  v = std::move( item.second );
               });
            }

            for( auto& item : head.removed_values ) {
               _indices.emplace( std::move( item.second ) );
            }

            if( head.new_ids.size() ) {
               for( auto id : head.new_ids ) {
                  _indices.erase( _indices.find( id ) );
               }
               _next_id = *head.new_ids.begin();
            }

            _stack.pop_back();
            --_revision;
         }

         /**
          *  This method works similar to git squash, it merges the change set from the two most
          *  recent revision numbers into one revision number (reducing the head revision number)
          *
          *  This method does not change the state of the index, only the state of the undo buffer.
          */
         void squash() {
            if( !enabled() ) return;
            if( _stack.size() == 1 ) {
               _stack.pop_front();
               return;
            }

            auto& state = _stack.back();
            auto& prev_state = _stack[_stack.size()-2];

            for( const auto& item : state.old_values ) {
               if( prev_state.new_ids.find( item.second.id ) != prev_state.new_ids.end() )
                  continue;
               if( prev_state.old_values.find( item.second.id ) != prev_state.old_values.end() )
                  continue;
               // del+upd -> N/A
               assert( prev_state.removed_values.find(item.second.id) == prev_state.removed_values.end() );
               // nop+upd(was=Y) -> upd(was=Y), type B
               prev_state.old_values.emplace( std::move(item) ); //[item.second->id] = std::move(item.second);
            }

            // *+new, but we assume the N/A cases don't happen, leaving type B nop+new -> new
            for( auto id : state.new_ids )
               prev_state.new_ids.insert(id);

            // *+del
            for( auto& obj : state.removed_values )
            {
               if( prev_state.new_ids.find(obj.second.id) != prev_state.new_ids.end() )
               {
                  // new + del -> nop (type C)
                  prev_state.new_ids.erase(obj.second.id);
                  continue;
               }
               auto it = prev_state.old_values.find(obj.second.id);
               if( it != prev_state.old_values.end() )
               {
                  // upd(was=X) + del(was=Y) -> del(was=X)
                  prev_state.removed_values.emplace( std::move(*it) );
                  prev_state.old_values.erase(obj.second.id);
                  continue;
               }
               // del + del -> N/A
               assert( prev_state.removed_values.find( obj.second.id ) == prev_state.removed_values.end() );
               // nop + del(was=Y) -> del(was=Y)
               prev_state.removed_values.emplace( std::move(obj) ); //[obj.second->id] = std::move(obj.second);
            }

            _stack.pop_back();
            --_revision;
         }

         /**
          * Discards all undo history prior to revision
          */
         void commit( int64_t revision )
         {
            while( _stack.size() && _stack[0].revision <= revision )
            {
               _stack.pop_front();
            }
         }

         void set_revision( uint64_t revision )
         {
            FC_ASSERT( _stack.size() == 0, "Revision can only be changed on empty undo state" );
            _revision = revision;
         }

      private:
         bool enabled()const { return _stack.size(); }

         void on_modify( const value_type& v ) {
            if( !enabled() ) return;

            auto& head = _stack.back();

            if( head.new_ids.find( v.id ) != head.new_ids.end() )
               return;

            auto itr = head.old_values.find( v.id );
            if( itr != head.old_values.end() )
               return;

            head.old_values.emplace( std::pair<typename value_type::id_type, const value_type&>( v.id, v ) );
         }

         void on_remove( const value_type& v ) {
            if( !enabled() ) return;

            auto& head = _stack.back();
            if( head.new_ids.count(v.id) ) {
               head.new_ids.erase( v.id );
               return;
            }

            auto itr = head.old_values.find( v.id );
            if( itr != head.old_values.end() ) {
               head.removed_values.emplace( std::move( *itr ) );
               head.old_values.erase( v.id );
               return;
            }

            if( head.removed_values.count( v.id ) )
               return;

            head.old_values.emplace( std::pair<typename value_type::id_type, const value_type&>( v.id, v ) );
         }

         void on_create( const value_type& v ) {
            if( !enabled() ) return;
            auto& head = _stack.back();

            head.new_ids.insert( v.id );
         }

         boost::interprocess::deque< undo_state, allocator<undo_state> > _stack;

         /**
          *  Each new session increments the revision, a squash will decrement the revision by combining
          *  the two most recent revisions into one revision.
          *
          *  Commit will discard all revisions prior to the committed revision.
          */
         int64_t                         _revision = 0;
         typename value_type::id_type    _next_id = 0;
         index_type                      _indices;
         uint32_t                        _size_of_value_type = 0;
         uint32_t                        _size_of_this = 0;
   };

   class abstract_session {
      public:
         virtual ~abstract_session(){};
         virtual void push()             = 0;
         virtual void squash()           = 0;
         virtual void undo()             = 0;
         virtual int64_t revision()const  = 0;
   };

   template<typename SessionType>
   class session_impl : public abstract_session
   {
      public:
         session_impl( SessionType&& s ):_session( std::move( s ) ){}

         virtual void push() override  { _session.push();  }
         virtual void squash() override{ _session.squash(); }
         virtual void undo() override  { _session.undo();  }
         virtual int64_t revision()const override  { return _session.revision();  }
      private:
         SessionType _session;
   };

   class abstract_index
   {
      public:
         abstract_index( void* i ):_idx_ptr(i){}
         virtual ~abstract_index(){}
         virtual void     set_revision( uint64_t revision ) = 0;
         virtual unique_ptr<abstract_session> start_undo_session( bool enabled ) = 0;

         virtual int64_t revision()const = 0;
         virtual void    undo()const = 0;
         virtual void    squash()const = 0;
         virtual void    commit( int64_t revision )const = 0;
         virtual void    export_to_file( const fc::path& filename ) const = 0;
         virtual void    import_from_file( const fc::path& filename ) = 0;
         virtual uint32_t type_id()const  = 0;
         void* get()const { return _idx_ptr; }
      private:
         void* _idx_ptr;
   };

   template<typename BaseIndex>
   class index_impl : public abstract_index {
      public:
         index_impl( BaseIndex& base ):abstract_index( &base ),_base(base){}

         virtual unique_ptr<abstract_session> start_undo_session( bool enabled ) override {
            return unique_ptr<abstract_session>(new session_impl<typename BaseIndex::session>( _base.start_undo_session( enabled ) ) );
         }
         virtual void     set_revision( uint64_t revision ) override { _base.set_revision( revision ); }
         virtual int64_t revision()const  override { return _base.revision(); }
         virtual void    undo()const  override { _base.undo(); }
         virtual void    squash()const  override { _base.squash(); }
         virtual void    commit( int64_t revision )const  override { _base.commit(revision); }
         virtual void    export_to_file( const fc::path& filename )const override { _base.export_to_file( filename ); }
         virtual void    import_from_file( const fc::path& filename ) override { _base.import_from_file( filename ); }
         virtual uint32_t type_id()const override { return BaseIndex::value_type::type_id; }
      private:
         BaseIndex& _base;
   };

   template<typename IndexType>
   class index : public index_impl<IndexType> {
      public:
         index( IndexType& i ):index_impl<IndexType>( i ){}
   };


   /**
    *  This class
    */
   class database
   {
      public:
         void open( const fc::path& dir, uint64_t shared_file_size );
         void close();
         void flush();
         void wipe( const fc::path& dir );

         struct session {
            public:
               session( session&& s ):_index_sessions( std::move(s._index_sessions) ){}
               session( vector<std::unique_ptr<abstract_session>>&& s ):_index_sessions( std::move(s) ){}

               ~session() {
                  undo();
               }

               void push(){ for( auto& i : _index_sessions ) i->push(); }
               void squash(){ for( auto& i : _index_sessions ) i->squash(); }
               void undo(){ for( auto& i : _index_sessions ) i->undo(); }
               int64_t revision()const { return _index_sessions[0]->revision(); }

            private:
               friend class database;
               session(){}

               vector< std::unique_ptr<abstract_session> > _index_sessions;
         };

         session start_undo_session( bool enabled );

         int64_t revision()const {
            FC_ASSERT( _index_list.size() );
            return _index_list[0]->revision();
         }
         void undo();
         void squash();
         void commit( int64_t revision );


         void set_revision( uint64_t revision )
         {
            try
            {
               for( auto i : _index_list ) i->set_revision( revision );
            }
            FC_CAPTURE_AND_RETHROW()
         }


         template<typename MultiIndexType>
         void add_index() {
             const uint16_t type_id = generic_index<MultiIndexType>::value_type::type_id;
             typedef generic_index<MultiIndexType>          index_type;
             typedef typename index_type::allocator_type    index_alloc;

             index_type* idx_ptr =  _segment->find_or_construct< index_type >( std::type_index(typeid(index_type)).name() )
                                                                              ( index_alloc( _segment->get_segment_manager() ) );

             idx_ptr->validate();

             if( type_id >= _index_map.size() )
                _index_map.resize( type_id + 1 );

             auto new_index = new index<index_type>( *idx_ptr );
             _index_map[ type_id ].reset( new_index );
             _index_list.push_back( new_index );
         }

         template<typename MultiIndexType>
         const generic_index<MultiIndexType>& get_index()const {
            typedef generic_index<MultiIndexType> index_type;
            typedef index_type*                   index_type_ptr;
            return *index_type_ptr( _index_map[index_type::value_type::type_id]->get() );
         }

         template<typename MultiIndexType, typename ByIndex>
         //const auto& get_index()const {
         auto get_index()const -> decltype( ((generic_index<MultiIndexType>*)( nullptr ))->indicies().template get<ByIndex>() ) {
            typedef generic_index<MultiIndexType> index_type;
            typedef index_type*                   index_type_ptr;
            return index_type_ptr( _index_map[index_type::value_type::type_id]->get() )->indicies().template get<ByIndex>();
         }

         template<typename MultiIndexType>
         generic_index<MultiIndexType>& get_mutable_index() {
            typedef generic_index<MultiIndexType> index_type;
            typedef index_type*                   index_type_ptr;
            return *index_type_ptr( _index_map[index_type::value_type::type_id]->get() );
         }

         template< typename ObjectType, typename IndexedByType, typename CompatibleKey >
         const ObjectType* find( CompatibleKey&& key )const
         {
            typedef typename get_index_type< ObjectType >::type index_type;
            const auto& idx = get_index< index_type >().indicies().template get< IndexedByType >();
            auto itr = idx.find( std::forward< CompatibleKey >( key ) );
            if( itr == idx.end() ) return nullptr;
            return &*itr;
         }

         template< typename ObjectType >
         const ObjectType* find( oid< ObjectType > key = oid< ObjectType >() ) const
         {
            typedef typename get_index_type< ObjectType >::type index_type;
            const auto& idx = get_index< index_type >().indices();
            auto itr = idx.find( key );
            if( itr == idx.end() ) return nullptr;
            return &*itr;
         }

         template< typename ObjectType, typename IndexedByType, typename CompatibleKey >
         const ObjectType& get( CompatibleKey&& key )const
         {
            auto obj = find< ObjectType, IndexedByType >( std::forward< CompatibleKey >( key ) );
            FC_ASSERT( obj != nullptr, "Object could not be found ${o}", ("o",std::string(fc::get_typename< ObjectType >::name())) );
            return *obj;
         }

         template< typename ObjectType >
         const ObjectType& get( oid< ObjectType > key = oid< ObjectType >() )const
         {
            auto obj = find< ObjectType >( key );
            FC_ASSERT( obj != nullptr, "Object could not be found ${o}", ("o",std::string(fc::get_typename< ObjectType >::name())) );
            return *obj;
         }

         template<typename ObjectType, typename Modifier>
         void modify( const ObjectType& obj, Modifier&& m )
         {
            try
            {
               typedef typename get_index_type<ObjectType>::type index_type;
               get_mutable_index<index_type>().modify( obj, m );
            } FC_LOG_AND_RETHROW()
         }

         template<typename ObjectType>
         void remove( const ObjectType& obj )
         {
            try
            {
               typedef typename get_index_type<ObjectType>::type index_type;
               return get_mutable_index<index_type>().remove( obj );
            } FC_LOG_AND_RETHROW()
         }

         template<typename ObjectType, typename Constructor>
         const ObjectType& create( Constructor&& con )
         {
            try
            {
               typedef typename get_index_type<ObjectType>::type index_type;
               return get_mutable_index<index_type>().emplace( std::forward<Constructor>(con) );
            } FC_LOG_AND_RETHROW()
         }

         bip::interprocess_mutex& get_mutex()const { return *_mutex; }


         void export_to_directory( const fc::path& dir )const;
         void import_from_directory( const fc::path& dir );

      private:
         unique_ptr<bip::managed_mapped_file>            _segment;
         bip::interprocess_mutex*                        _mutex;

         /**
          * This is a sparse list of known indicies kept to accelerate creation of undo sessions
          */
         vector<abstract_index*>                        _index_list;
         /**
          * This is a full map (size 2^16) of all possible index designed for constant time lookup
          */
         vector<unique_ptr<abstract_index>>             _index_map;

         fc::path                                       _data_dir;
   };


   template<typename T>
   const T& oid<T>::operator()( const database& db )const { return db.get<T>( _id ); }
} } // namepsace graphene::db2

namespace fc {
  template<typename T>
  void to_variant( const graphene::db2::oid<T>& var,  variant& vo )
  {
     vo = var._id;
  }
  template<typename T>
  void from_variant( const variant& vo, graphene::db2::oid<T>& var )
  {
     var._id = vo.as_int64();
  }

  namespace raw {
    template<typename Stream, typename T>
    inline void pack( Stream& s, const graphene::db2::oid<T>& id )
    {
       s.write( (const char*)&id._id, sizeof(id._id) );
    }
    template<typename Stream, typename T>
    inline void unpack( Stream& s, graphene::db2::oid<T>& id )
    {
       s.read( (char*)&id._id, sizeof(id._id));
    }
  }
}

/*
struct example_object : public object<1,example_object> {
   template<typename Allocator>
   example_object( Allocator&& al ){}

   int a;
   int b;
};

typedef multi_index_container<
  example_object,
  indexed_by<
     ordered_non_unique< BOOST_MULTI_INDEX_MEMBER(example_object,int,a) >,
     ordered_non_unique< BOOST_MULTI_INDEX_MEMBER(example_object,int,b) >
  >,
  bip::allocator<example_object,bip::managed_mapped_file::segment_manager>
> example_index;

SET_INDEX_TYPE( example_object, example_index )

int main( int argc, char** argv ) {

   database db;
   db.open( "./testdb" );
   const auto& idx = db.add_index<book_index>();

   const auto& eo = db.create<example_object>( [&]( example_object& o ){
      o.a = 1;
      o.b = 2;
   });

   auto undos = db.start_undo_session();
   db.modify( eo, [&]( example_object& o ) {
      o.b = 3;
   });
   undo.merge();
}
*/
