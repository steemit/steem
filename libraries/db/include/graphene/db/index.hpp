/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once
#include <graphene/db/object.hpp>
#include <fc/interprocess/file_mapping.hpp>
#include <fc/io/raw.hpp>
#include <fc/io/json.hpp>
#include <fc/crypto/sha256.hpp>
#include <fstream>

namespace graphene { namespace db {
   class object_database;
   using fc::path;

   /**
    * @class index_observer
    * @brief used to get callbacks when objects change
    */
   class index_observer
   {
      public:
         virtual ~index_observer(){}
         /** called just after the object is added */
         virtual void on_add( const object& obj ){}
         /** called just before obj is removed */
         virtual void on_remove( const object& obj ){}
         /** called just after obj is modified with new value*/
         virtual void on_modify( const object& obj ){}
   };

   /**
    *  @class index
    *  @brief abstract base class for accessing objects indexed in various ways.
    *
    *  All indexes assume that there exists an object ID space that will grow
    *  forever in a seqential manner.  These IDs are used to identify the
    *  index, type, and instance of the object.
    *
    *  Items in an index can only be modified via a call to modify and
    *  all references to objects outside of that callback are const references.
    *
    *  Most implementations will probably be some form of boost::multi_index_container
    *  which means that they can covnert a reference to an object to an iterator.  When
    *  at all possible save a pointer/reference to your objects rather than constantly
    *  looking them up by ID.
    */
   class index
   {
      public:
         virtual ~index(){}

         virtual uint8_t object_space_id()const = 0;
         virtual uint8_t object_type_id()const = 0;

         virtual object_id_type get_next_id()const = 0;
         virtual void           use_next_id() = 0;
         virtual void           set_next_id( object_id_type id ) = 0;

         virtual const object&  load( const std::vector<char>& data ) = 0;
         /**
          *  Polymorphically insert by moving an object into the index.
          *  this should throw if the object is already in the database.
          */
         virtual const object& insert( object&& obj ) = 0;

         /**
          * Builds a new object and assigns it the next available ID and then
          * initializes it with constructor and lastly inserts it into the index.
          */
         virtual const object&  create( const std::function<void(object&)>& constructor ) = 0;

         /**
          *  Opens the index loading objects from a file
          */
         virtual void open( const fc::path& db ) = 0;
         virtual void save( const fc::path& db ) = 0;



         /** @return the object with id or nullptr if not found */
         virtual const object*      find( object_id_type id )const = 0;

         /**
          * This version will automatically check for nullptr and throw an exception if the
          * object ID could not be found.
          */
         const object&              get( object_id_type id )const
         {
            auto maybe_found = find( id );
            FC_ASSERT( maybe_found != nullptr, "Unable to find Object", ("id",id) );
            return *maybe_found;
         }

         virtual void               modify( const object& obj, const std::function<void(object&)>& ) = 0;
         virtual void               remove( const object& obj ) = 0;

         /**
          *   When forming your lambda to modify obj, it is natural to have Object& be the signature, but
          *   that is not compatible with the type erasue required by the virtual method.  This method
          *   provides a helper to wrap the lambda in a form compatible with the virtual modify call.
          *   @note Lambda should have the signature:  void(Object&)
          */
         template<typename Object, typename Lambda>
         void modify( const Object& obj, const Lambda& l ) {
            modify( static_cast<const object&>(obj), std::function<void(object&)>( [&]( object& o ){ l( static_cast<Object&>(o) ); } ) );
         }

         virtual void               inspect_all_objects(std::function<void(const object&)> inspector)const = 0;
         virtual fc::uint128        hash()const = 0;
         virtual void               add_observer( const shared_ptr<index_observer>& ) = 0;

         virtual void               object_from_variant( const fc::variant& var, object& obj )const = 0;
         virtual void               object_default( object& obj )const = 0;
   };

   class secondary_index
   {
      public:
         virtual ~secondary_index(){};
         virtual void object_inserted( const object& obj ){};
         virtual void object_removed( const object& obj ){};
         virtual void about_to_modify( const object& before ){};
         virtual void object_modified( const object& after  ){};
   };

   /**
    *   Defines the common implementation
    */
   class base_primary_index
   {
      public:
         base_primary_index( object_database& db ):_db(db){}

         /** called just before obj is modified */
         void save_undo( const object& obj );

         /** called just after the object is added */
         void on_add( const object& obj );

         /** called just before obj is removed */
         void on_remove( const object& obj );

         /** called just after obj is modified */
         void on_modify( const object& obj );

         template<typename T>
         void add_secondary_index()
         {
            _sindex.emplace_back( new T() );
         }

         template<typename T>
         const T& get_secondary_index()const
         {
            for( const auto& item : _sindex )
            {
               const T* result = dynamic_cast<const T*>(item.get());
               if( result != nullptr ) return *result;
            }
            FC_THROW_EXCEPTION( fc::assert_exception, "invalid index type" );
         }

      protected:
         vector< shared_ptr<index_observer> >   _observers;
         vector< unique_ptr<secondary_index> >  _sindex;

      private:
         object_database& _db;
   };


   /**
    * @class primary_index
    * @brief  Wraps a derived index to intercept calls to create, modify, and remove so that
    *  callbacks may be fired and undo state saved.
    *
    *  @see http://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
    */
   template<typename DerivedIndex>
   class primary_index  : public DerivedIndex, public base_primary_index
   {
      public:
         typedef typename DerivedIndex::object_type object_type;

         primary_index( object_database& db )
         :base_primary_index(db),_next_id(object_type::space_id,object_type::type_id,0) {}

         virtual uint8_t object_space_id()const override
         { return object_type::space_id; }

         virtual uint8_t object_type_id()const override
         { return object_type::type_id; }

         virtual object_id_type get_next_id()const override              { return _next_id;    }
         virtual void           use_next_id()override                    { ++_next_id.number;  }
         virtual void           set_next_id( object_id_type id )override { _next_id = id;      }

         fc::sha256 get_object_version()const
         {
            std::string desc = "1.0";//get_type_description<object_type>();
            return fc::sha256::hash(desc);
         }

         virtual void open( const path& db )override
         { 
            if( !fc::exists( db ) ) return;
            fc::file_mapping fm( db.generic_string().c_str(), fc::read_only );
            fc::mapped_region mr( fm, fc::read_only, 0, fc::file_size(db) );
            fc::datastream<const char*> ds( (const char*)mr.get_address(), mr.get_size() );
            fc::sha256 open_ver;

            fc::raw::unpack(ds, _next_id);
            fc::raw::unpack(ds, open_ver);
            FC_ASSERT( open_ver == get_object_version(), "Incompatible Version, the serialization of objects in this index has changed" );
            try {
               vector<char> tmp;
               while( true ) 
               {
                  fc::raw::unpack( ds, tmp );
                  load( tmp );
               }
            } catch ( const fc::exception&  ){}
         }

         virtual void save( const path& db ) override 
         {
            std::ofstream out( db.generic_string(), 
                               std::ofstream::binary | std::ofstream::out | std::ofstream::trunc );
            FC_ASSERT( out );
            auto ver  = get_object_version();
            fc::raw::pack( out, _next_id );
            fc::raw::pack( out, ver );
            this->inspect_all_objects( [&]( const object& o ) {
                auto vec = fc::raw::pack( static_cast<const object_type&>(o) );
                auto packed_vec = fc::raw::pack( vec );
                out.write( packed_vec.data(), packed_vec.size() );
            });
         }

         virtual const object&  load( const std::vector<char>& data )override
         {
            const auto& result = DerivedIndex::insert( fc::raw::unpack<object_type>( data ) );
            for( const auto& item : _sindex )
               item->object_inserted( result );
            return result;
         }


         virtual const object&  create(const std::function<void(object&)>& constructor )override
         {
            const auto& result = DerivedIndex::create( constructor );
            for( const auto& item : _sindex )
               item->object_inserted( result );
            on_add( result );
            return result;
         }

         virtual void  remove( const object& obj ) override
         {
            for( const auto& item : _sindex )
               item->object_removed( obj );
            on_remove(obj);
            DerivedIndex::remove(obj);
         }

         virtual void modify( const object& obj, const std::function<void(object&)>& m )override
         {
            save_undo( obj );
            for( const auto& item : _sindex )
               item->about_to_modify( obj );
            DerivedIndex::modify( obj, m );
            for( const auto& item : _sindex )
               item->object_modified( obj );
            on_modify( obj );
         }

         virtual void add_observer( const shared_ptr<index_observer>& o ) override
         {
            _observers.emplace_back( o );
         }

         virtual void object_from_variant( const fc::variant& var, object& obj )const override
         {
            object_id_type id = obj.id;
            object_type* result = dynamic_cast<object_type*>( &obj );
            FC_ASSERT( result != nullptr );
            fc::from_variant( var, *result );
            obj.id = id;
         }

         virtual void object_default( object& obj )const override
         {
            object_id_type id = obj.id;
            object_type* result = dynamic_cast<object_type*>( &obj );
            FC_ASSERT( result != nullptr );
            (*result) = object_type();
            obj.id = id;
         }

      private:
         object_id_type _next_id;
   };

} } // graphene::db
