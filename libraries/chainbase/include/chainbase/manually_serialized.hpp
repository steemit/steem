#include <chainbase/chainbase.hpp>

#include <serialize3/h/storage/serializedumper.h>
#include <serialize3/h/storage/serializeloader.h>

namespace chainbase
{

template<typename BaseIndex>
void index_impl::Dump(ASerializeDumper& dumper) const
{
   _base->Dump(dumper);
}

template<typename BaseIndex>
void index_impl::Load(ASerializeLoader& loader)
{
   //_base = _segment->find_or_construct< index_type >( type_name.c_str() )( index_alloc( _segment->get_segment_manager() ) );
   //_base = new BaseIndex;
   _base->Load(loader);
   _idx_ptr = _base;
}

} // namespace chainbase
