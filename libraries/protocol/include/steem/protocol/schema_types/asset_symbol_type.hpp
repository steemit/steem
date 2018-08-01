
#pragma once

#include <steem/schema/abstract_schema.hpp>
#include <steem/schema/schema_impl.hpp>

#include <steem/protocol/asset_symbol.hpp>

namespace steem { namespace schema { namespace detail {

//////////////////////////////////////////////
// asset_symbol_type                        //
//////////////////////////////////////////////

struct schema_asset_symbol_type_impl
   : public abstract_schema
{
   STEEM_SCHEMA_CLASS_BODY( schema_asset_symbol_type_impl )
};

}

template<>
struct schema_reflect< steem::protocol::asset_symbol_type >
{
   typedef detail::schema_asset_symbol_type_impl           schema_impl_type;
};

} }
