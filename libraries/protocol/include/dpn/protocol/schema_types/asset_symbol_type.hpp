
#pragma once

#include <dpn/schema/abstract_schema.hpp>
#include <dpn/schema/schema_impl.hpp>

#include <dpn/protocol/asset_symbol.hpp>

namespace dpn { namespace schema { namespace detail {

//////////////////////////////////////////////
// asset_symbol_type                        //
//////////////////////////////////////////////

struct schema_asset_symbol_type_impl
   : public abstract_schema
{
   DPN_SCHEMA_CLASS_BODY( schema_asset_symbol_type_impl )
};

}

template<>
struct schema_reflect< dpn::protocol::asset_symbol_type >
{
   typedef detail::schema_asset_symbol_type_impl           schema_impl_type;
};

} }
