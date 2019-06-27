
#pragma once

#include <dpn/schema/abstract_schema.hpp>
#include <dpn/schema/schema_impl.hpp>

#include <dpn/protocol/types.hpp>

namespace dpn { namespace schema { namespace detail {

//////////////////////////////////////////////
// account_name_type                        //
//////////////////////////////////////////////

struct schema_account_name_type_impl
   : public abstract_schema
{
   DPN_SCHEMA_CLASS_BODY( schema_account_name_type_impl )
};

}

template<>
struct schema_reflect< dpn::protocol::account_name_type >
{
   typedef detail::schema_account_name_type_impl           schema_impl_type;
};

} }

namespace fc {

template<>
struct get_typename< dpn::protocol::account_name_type >
{
   static const char* name()
   {
      return "dpn::protocol::account_name_type";
   }
};

}
