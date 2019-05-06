#pragma once

#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <rocksdb/utilities/write_batch_with_index.h>

#include <vector>

namespace mira { namespace multi_index { namespace detail {

using ::rocksdb::ColumnFamilyDescriptor;

class rocksdb_base
{

   protected:
      std::vector< ColumnFamilyDescriptor > _column_defs;

};

} } } // mira::multi_index::detail
