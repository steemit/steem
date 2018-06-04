#include <golos/plugins/mongo_db/mongo_db_types.hpp>

namespace golos {
namespace plugins {
namespace mongo_db {

    void bmi_insert_or_replace(db_map& bmi, named_document doc) {
        auto it = bmi.get<hashed_idx>().find(std::make_tuple(
            doc.collection_name,
            doc.key, doc.keyval, doc.is_removal));
        if (it != bmi.get<hashed_idx>().end())
            bmi.get<hashed_idx>().erase(it);
        bmi.push_back(std::move(doc));
    }
}
}
}