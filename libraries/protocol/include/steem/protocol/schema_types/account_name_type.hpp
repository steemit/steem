
#pragma once

#include <steem/schema/abstract_schema.hpp>
#include <steem/schema/schema_impl.hpp>

#include <steem/protocol/types.hpp>

namespace steem { namespace schema { namespace detail {

//////////////////////////////////////////////
// account_name_type                        //
//////////////////////////////////////////////

struct schema_account_name_type_impl
   : public abstract_schema
{
   STEEM_SCHEMA_CLASS_BODY( schema_account_name_type_impl )
};

}

template<>
struct schema_reflect< steem::protocol::account_name_type >
{
   typedef detail::schema_account_name_type_impl           schema_impl_type;
};

} }

namespace fc {

template<>
struct get_typename< steem::protocol::account_name_type >
{
   static const char* name()
   {
      return "steem::protocol::account_name_type";
   }
};

}
