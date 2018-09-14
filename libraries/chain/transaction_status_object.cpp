#include <steem/chain/transaction_status_object.hpp>

namespace steem { namespace chain {

const object* transaction_status_index::create(const std::function<void (object*)>& constructor, object_id_type)
{
   transaction_status_object obj;

   obj.id = get_next_available_id();
   constructor(&obj);

   auto result = _index.insert(std::move(obj));
   FC_ASSERT(result.second, "Could not create transaction_status_object! Most likely a uniqueness constraint is violated.");
   return &*result.first;
}

void transaction_status_index::modify(const object* obj,
                               const std::function<void (object*)>& m)
{
   assert(obj != nullptr);
   FC_ASSERT(obj->id < _index.size());

   const transaction_status_object* t = dynamic_cast<const transaction_status_object*>(obj);
   assert(t != nullptr);

   auto itr = _index.find(obj->id.instance());
   assert(itr != _index.end());
   _index.modify(itr, [&m](transaction_status_object& o) { m(&o); });
}

void transaction_status_index::add(unique_ptr<object> o)
{
   assert(o);
   object_id_type id = o->id;
   assert(id.space() == transaction_status_object::space_id);
   assert(id.type() == transaction_status_object::type_id);
   assert(id.instance() == size());

   auto trx = dynamic_cast<transaction_status_object*>(o.get());
   assert(trx != nullptr);
   o.release();

   auto result = _index.insert(std::move(*trx));
   FC_ASSERT(result.second, "Could not insert transaction_status_object! Most likely a uniqueness constraint is violated.");
}

void transaction_status_index::remove(object_id_type id)
{
   auto& index = _index.get<instance>();
   auto itr = index.find(id.instance());
   if( itr == index.end() )
      return;

   assert(id.space() == transaction_status_object::space_id);
   assert(id.type() == transaction_status_object::type_id);

   index.erase(itr);
}

const object*transaction_status_index::get(object_id_type id) const
{
   if( id.type() != transaction_status_object::type_id ||
       id.space() != transaction_status_object::space_id )
      return nullptr;

   auto itr = _index.find(id.instance());
   if( itr == _index.end() )
      return nullptr;
   return &*itr;
}

} } // steem::chain
