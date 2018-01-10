#include <chainbase/manually_serialized.hpp>

namespace chainbase
{

void database::Dump(ASerializeDumper& dumper) const
{
   dumper & _index_list.size();
   for (auto index : _index_list)
      index->DumpPointer(dumper);

   dumper & _data_dir.generic_string();
   dumper & _enable_require_locking;      
}

void database::Load(ASerializeLoader& loader)
{
   size_t size;
   loader & size;
   _index_list.reserve(size);
   _index_map.reserve(size);

   for (size_t i = 0; i < size; ++i)
   {
      abstract_index* index = (abstract_index*)abstract_index::LoadPointer(loader);
      _index_list.emplace_back(index);
      _index_list.emplace_back(index);
   }

   std::string data_dir;
   loader & data_dir;
   _data_dir = data_dir;
   loader & _enable_require_locking;
}

} // namespace chainbase
