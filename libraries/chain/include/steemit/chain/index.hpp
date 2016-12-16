#pragma once

#include <steemit/chain/database.hpp>

namespace steemit { namespace chain {

template< typename MultiIndexType >
void _add_index_impl( database& db )
{
   db.add_index< MultiIndexType >();
}

template< typename MultiIndexType >
void add_core_index( database& db )
{
   _add_index_impl< MultiIndexType >(db);
}

template< typename MultiIndexType >
void add_plugin_index( database& db )
{
   db._plugin_index_signal.connect( [&db](){ _add_index_impl< MultiIndexType >(db); } );
}

} }
