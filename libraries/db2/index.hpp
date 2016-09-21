#pragma once

#include <boost/interprocess/managed_mapped_file.hpp>

namespace graphene { namespace db2 {

   template<typename T>
   using allocator = boost::interprocess::allocator<T, boost::interprocess::managed_mapped_file::segment_manager>;

   /**
    *  Object ID type that includes the type of the object it references
    */
   template<typename T>
   struct oid {
      oid( int64_t i = 0 ):_id(i){}

      const T& operator()( const database& db )const { return db.get<T>( _id ); }

      int64_t _id = 0;
   };

   template<uint16_t TypeNumber, typename Derived>
   struct object : public Derived {
      static const uint16_t type_id = TypeNumber;

      oid<Derived> id = 0;
   };

   /** this class is ment to be specified to enable lookup of index type by object type using
    * the SET_INDEX_TYPE macro. 
    **/
   template<typename T>
   struct get_index_type {};

   /**
    *  This macro must be used at global scope and OBJECT_TYPE and INDEX_TYPE must be fully qualified 
    */
   #define SET_INDEX_TYPE( OBJECT_TYPE, INDEX_TYPE )  \
   namespace graphene { namespace db2 { template<> struct get_index_type<OBJECT_TYPE> { typedef INDEX_TYPE type; } } }

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
         typedef index_type::value_type                                value_type;
         typedef bip::allocator< generic_index, segment_manager_type > allocator_type;

         generic_index( allocator<value_type> a )
         :index_type( a ){}

         template<typename Constructor>
         const value_type& emplace( Constructor&& c ) {
            auto new_id = _next_id;

            auto constructor = [&]( value_type& v ) {
               v.id = new_id;
               c( v );
            };

            auto insert_result = _indices.emplace( constructor, _indices.get_allocator() ) 
            if( insert_result.second ) {
               ++_next_id;

               on_create( *insert_result.first );

               return *insert_result.first;
            }

            FC_ASSERT( insert_result.second, "Could not insert object, most likely a uniqueness constraint was violated" );
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

         template<typename CompatibleKey&>
         const object_type* find( CompatibleKey& key )const {
            auto itr = _indices.find( key );
            if( itr != _indices.end() ) return *itr;
            return nullptr;
         }

         const index_type& indices()const { return _indices; }


         int64_t get_next_id()const override              { return _next_id; }
         void    use_next_id()override                    { ++_next_id;      }
         void    set_next_id( int64_t id )override        { _next_id = id;   }

         class undo_state {
            public:
               typedef allocator< std::pair<int64_t, value_type> > id_value_allocator_type;
               typedef allocator< int64_t> >                       id_allocator_type;

               undo_state( allocator<undo_type> al )
               :old_values( id_value_allocator_type( al.get_segment_manager() ) ), 
                removed_values( id_value_allocator_type( al.get_segment_manager() ) ), 
                new_ids( id_allocator_type( al.get_segment_manager() ) ){}


               boost::interprocess::map< int64_t, value_type, id_value_allocator_type > old_values;
               boost::interprocess::map< int64_t, value_type, id_value_allocator_type > removed_values;
               boost::interprocess::set< int64_t, id_allocator_type>                    new_ids;
               int64_t                                                                  old_next_id = 0;
         };

         class session {
            public:
               session( session&& mv )
               :_index(mv._index),_apply(mv._apply),_version(mv._version){}

               ~session() {
                  if( _apply ) {
                     _index.undo();
                  }
               }

               /** leaves the UNDO state on the stack when session goes out of scope */
               void push()   { _apply = false; }
               /** combines this session with the prior session */
               void merge()  {  if( _apply ) _index.merge(); _apply = false; }
               void undo()   {  if( _apply ) _index.undo();  _apply = false; }

               int64_t version()const { return _version; }

               session& operator = ( session&& mv ) {
                  if( this == &mv ) return *this;
                  if( _apply ) _index.undo();
                  _apply = mv._apply;
                  _version = mv._version;
                  mv._apply = false;
                  mv._version = 0;
                  return *this;
               }

            private:
               session( generic_index& idx, int64_t version )
               :_index(idx),_version(version){
                  if( version == -1 )
                     _apply = false;
               } 

               generic_index& _index;
               bool           _apply = true;
               int64_t        _version;
         };

         session start_undo_session( bool enabled ) {
            if( enabled ) {
               _stack.emplace_back();
               _stack.back().old_next_id = _next_id;
               ++_version;
               return session( *this, _version );
            } else {
               return session( *this, -1 );
            }
         }

      private:
         bool enabled()const { return _stack.size(); }

         /**
          *  Restores the state to how it was prior to the current session
          */
         void undo() {
            if( !enabled() ) return;

            const auto& head = _stack.back();
            for( auto& item : head.old_values ) {
               _indices.modify( *find( item.id ), [&]( value_type& v ) {
                  v = std::move( item.second );
               });
            }

            for( auto& item : head.removed_values ) {
               _indices.emplace( std::move( item ) );
            }

            if( head.new_ids.size() ) {
               for( auto id : head.new_ids ) {
                  _indices.erase( _indices.find( id ) );
               }
               _next_id = *head.new_ids.begin();
            }

            _stack.pop_back();
         }

         /**
          *  Combines most recent undo state with the prior undo state
          */
         void merge() {
            if( !enabled() ) return;
            if( _stack.size() == 1 )
               _stack.pop_front();

            _stack.pop_back();

            auto& state = _stack.back();
            auto& prev_state = _stack[stack.size()-2];

            for( const auto& item : state.old_values ) {
               if( prev_state.new_ids.find( item.second.id ) != prev_state.new_ids.end() )
                  continue;
               if( prev_state.old_values.find(obj.second->id) != prev_state.old_values.end() )
                  continue;
               // del+upd -> N/A
               assert( prev_state.removed_values.find(obj.second->id) == prev_state.removed_values.end() );
               // nop+upd(was=Y) -> upd(was=Y), type B
               prev_state.old_values.emplace( std::move(item) ); //[obj.second->id] = std::move(item.second);
            }

            // *+new, but we assume the N/A cases don't happen, leaving type B nop+new -> new
            for( auto id : state.new_ids )
               prev_state.new_ids.insert(id);

            // *+del
            for( auto& obj : state.removed_values )
            {
               if( prev_state.new_ids.find(obj.second->id) != prev_state.new_ids.end() )
               {
                  // new + del -> nop (type C)
                  prev_state.new_ids.erase(obj.second->id);
                  continue;
               }
               auto it = prev_state.old_values.find(obj.second->id);
               if( it != prev_state.old_values.end() )
               {
                  // upd(was=X) + del(was=Y) -> del(was=X)
                  prev_state.removed_values.emplace( *[obj.second->id] = std::move(it->second);
                  prev_state.old_values.erase(obj.second->id);
                  continue;
               }
               // del + del -> N/A
               assert( prev_state.removed_values.find( obj.second->id ) == prev_state.removed_values.end() );
               // nop + del(was=Y) -> del(was=Y)
               prev_state.removed_values.emplace( std::move(obj) ); //[obj.second->id] = std::move(obj.second);
            }
         }

         /**
          * Discards all undo history prior to version
          */
         void commit( int64_t version ) {
            while( _stack.size() ) {
                if( _stack[0].version <= version )
                  _stack.pop_front();
            }
         }

         void on_modify( const value_type& v ) {
            if( !enabled() ) return;

            auto& head = _stack.back();
            
            if( head.new_id.find( v.id ) != head.new_ids.end() ) 
               return;

            auto itr = head.old_values.find( v.id );
            if( itr != head.old_values.end() ) 
               return;

            state.old_values.emplace( std::pair<int64_t, const value_type&>( v.id, v ) );
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

            head.old_values.emplace( std::pair<int64_t, const value_type&>( v.id, v ) );
         }

         void on_create( const value_type& v ) {
            if( !enabled() ) return;
            auto& head = _stack.back();

            head.new_ids.insert( v.id );
         }

         boost::interprocess::deque< undo_state, allocator<undo_state> > _stack;

         /**
          *  Each new session increments the version, a merge will decrement the version by combining
          *  the two most recent versions into one version. 
          *
          *  Commit will discard all versions prior to the committed version.
          */
         int64_t     _version = 0;

         int64_t     _next_id = 0;
         index_type  _indices;

   };

   class abstract_session {
      virtual ~abstract_session();
      virtual void push() = 0;
      virtual void merge() = 0;
      virtual void undo()  = 0;
      virtual int64_t version()const  = 0;
   };

   template<typename SessionType>
   class session_impl : public abstract_index 
   {
      public:
         session_impl( SessionType&& s ):_session( std::move( s ) ){}

         virtual void push() override  { _session.push();  }
         virtual void merge() override { _session.merge(); }
         virtual void undo() override  { _session.undo();  }
         virtual int64_t version()const override  { return _session.version();  }
      private:
         SessionType _session;
   };

   class abstract_index 
   {
      public:
         abstract_index( void* i ):_idx_ptr(i){}
         virtual ~abstract_index(){}
         virtual unique_ptr<abstract_session> start_undo_session( bool enabled ) = 0;

         void* get()const { return _idx_ptr; }
      private:
         void* _idx_ptr;
   };

   template<typename BaseIndex>
   class index_impl : public abstract_index {
      public:
         index_impl( BaseIndex& base ):abstract_index( &base ),_base(base){}

         virtual unique_ptr<abstract_session> start_undo_session( bool enabled ) {
            return new session_impl<typename BaseIndex::session>( _base.start_undo_session( enabled ) );
         }
      private:
         BaseIndex& _base;
   };

   template<typename IndexType>
   class index : index_impl<IndexType> {
      public:
         index( IndexType& i ):index_impl( i ){}
   };


   /**
    *  This class 
    */
   class database 
   {
      public:
         typedef allocator< std::pair<int32_t,offset_ptr<void>> > index_pair_allocator;
         typedef flat_map<int32_t, offset_ptr<void>, index_pair_allocator >* index_map_type; 
         
         void open( const fc::path& file ) {
            auto abs_path = fc::absolute( file );
            _segment.reset( new bip::managed_mapped_file( abs_path.generic_string(), 1024*1024*1024*64 ) );
            _mutex.reset( new bip::named_mutex( bip::open_or_create, abs_path.generic_string() ) );
            _mutex = _segment.find_or_construct< bip::interprocess_mutex >( "global_mutex" )();
            //                 bip::allocator_type< bip::interprocess_mutex, bip::managed_mapped_file::segment_manager>( *_segment ) );
         }

         template<typename MultiIndexType>
         generic_index<MultiIndexType>& add_index() {
             const uint16_t type_id = generic_index<MultiIndexType>::value_type::type_id;
             typedef generic_index<MultiIndexType> index_type;
             typedef index_type::allocator_type    index_alloc;
             auto idx_ptr =  _segment.find_or_construct< index_type >( std::type_index(typeid(index_type)).name() )
                                                         ( index_alloc( _segment ) );
             _index_map[ type_id ] = new index<index_type>( *idx_ptr );
         }

         struct session {
            public:
               session( session&& s ):_index_sessions( std::move(s) ){}

               void push(){
                  for( auto& i : _index_sessions ) i->push();
               }
               void merge(){
                  for( auto& i : _index_sessions ) i->merge();
               }
               void undo(){
                  for( auto& i : _index_sessions ) i->undo();
               }

               int64_t version()const {
                  return _index_sessions[0]->version();
               }

            private:
               friend class database;
               session(){}

               vector< std::unique_ptr<abstract_session> > _index_sessions;
         };

         session start_undo_session( bool enabled ) {
            if( enabled ) {
               vector< std::unique_ptr<abstract_session> > _sub_sessions;
               _sub_sessions.reserve( _index_map.size() );
               for( auto& item : _index_map ) {
                  _sub_sessions.push_back( item->second->start_undo_session( enabled ) );
               }
            } else {
               return session();
            }
         }

         template<typename MultiIndexType>
         const generic_index<MultiIndexType>& get()const {
            typedef generic_index<MultiIndexType> index_type;
            typedef index_type*                   index_type_ptr;
            return *id::index_type_ptr( _index_map[IndexType::type_id]->get() );
         }

         template<typename ObjectType, typename Modifier>
         void modify( const ObjectType& obj, Modifier&& m ) {
            typedef get_index_type<ObjectType>::type index_type;
            get<index_type>().modify( obj, m );
         }

         template<typename ObjectType, typename CompatibleKey>
         const ObjectType& get( CompatibleKey&& key ) {
            typedef get_index_type<ObjectType>::type index_type;
            return get<index_type>().get( std::forward<CompatibleKey>(key) );
         }

         template<typename ObjectType, typename CompatibleKey>
         const ObjectType* find( CompatibleKey&& key ) {
            typedef get_index_type<ObjectType>::type index_type;
            return get<index_type>().find( std::forward<CompatibleKey>(key) );
         }

         template<typename ObjectType>
         void remove( const ObjectType& obj ) {
            typedef get_index_type<ObjectType>::type index_type;
            return get<index_type>().remove( obj );
         }

         template<typename ObjectType, typename Constructor>
         const ObjectType& create( Constructor&& con ) {
            typedef get_index_type<ObjectType>::type index_type;
            return index_type.emplace( std::forward<Constructor>(con) );
         }

         bip::interprocess_mutex& get_mutex()const { return *_mutex; }

      private:
         unique_ptr<bip::managed_mapped_file>           _segment;
         bip::interprocess_mutex*                        _mutex;
         flat_map<uint16_t,unique_ptr<abstract_index>>  _index_map;
   };

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
