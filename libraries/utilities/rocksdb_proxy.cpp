#include <steem/utilities/rocksdb_proxy.hpp>

#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/utilities/options_util.h>
#include <rocksdb/slice.h>
#include <rocksdb/utilities/write_batch_with_index.h>

namespace steem { namespace utilities {

namespace rocksdb_types
{
const Comparator* by_id_Comparator()
{
   static by_id_ComparatorImpl c;
   return &c;
}

const Comparator* by_location_Comparator()
{
   static by_location_ComparatorImpl c;
   return &c;
}

const Comparator* by_account_name_Comparator()
{
   static by_account_name_ComparatorImpl c;
   return &c;
}

const Comparator* by_ah_info_operation_Comparator()
{
   static by_ah_info_operation_ComparatorImpl c;
   return &c;
}

}

} }
