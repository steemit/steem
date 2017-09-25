#ifndef __PTR_REF_HPP
#define __PTR_REF_HPP

#include <stdlib.h>

#include <chainbase/chainbase.hpp>
#include <chainbase/util/ra_indexed_container.hpp>

namespace chainbase
{

struct main_db_accessor
{
   database& operator()() const
   {
      return database::main_db();
   }
};

template <class ObjectType, class DbAccessor = main_db_accessor>
class ptr_ref
{
public:
   enum
   {
      NULL_ID = 0
   };

   ptr_ref(ObjectType* refPtr = nullptr)
   {
      Id = refPtr != nullptr ? refPtr->id._id : NULL_ID;
   }

   ptr_ref& operator=(const ObjectType* refPtr)
   {
      Id = refPtr != nullptr ? refPtr->id._id : NULL_ID;
      return *this;
   }

   ptr_ref& operator=(const ObjectType& refObj)
   {
      Id = refObj.id._id;
      return *this;
   }
   
   ObjectType* operator -> () const
   {
      return get_object();
   }

   operator ObjectType* () const
   {
      return get_object();
   }

private:
   ObjectType* get_object() const
   {
      typedef typename chainbase::get_index_type<ObjectType>::type multi_index_type;
      static_assert(chainbase::has_master_index<multi_index_type>::value,
         "ptr_ref  needs ra_indexed_container to be used as this type storage");

      if(Id == NULL_ID)
         return nullptr;

      DbAccessor accessor;
      database& actualDb = accessor();

      const multi_index_type& actualContainer = actualDb.get_index<multi_index_type>().indices();

      ObjectType* object = const_cast<ObjectType*>(actualContainer[Id]);
      assert(object != nullptr && "Attempting to reference deleted object");
      return object;
   }

/// Class attributes:
private:
   size_t Id = 0;
};

} /// namespace chainbase

#endif /// __PTR_REF_HPP

