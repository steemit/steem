#pragma once

#include <golos/protocol/block.hpp>
#include <golos/chain/database.hpp>
#include <golos/protocol/transaction.hpp>
#include <golos/protocol/operations.hpp>

#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/builder/stream/value_context.hpp>
#include <bsoncxx/json.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/key_extractors.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/optional.hpp>
#include <boost/interprocess/containers/flat_set.hpp>

#include <bson.h>

#define MONGO_ID_SINGLE "__single__"

namespace golos {
namespace plugins {
namespace mongo_db {

    namespace bip = boost::interprocess;
    using namespace golos::chain;
    using namespace golos::protocol;
    using golos::chain::to_string;
    using golos::chain::shared_string;
    using bsoncxx::builder::stream::array;
    using bsoncxx::builder::stream::document;

    struct named_document {
        std::string collection_name;
        document doc;
        bool is_removal;
        //bool is_virtual;
        std::vector<document> indexes_to_create;
        std::string key;
        std::string keyval;
    };

    struct hashed_idx;

        typedef boost::multi_index_container<
            named_document,
            boost::multi_index::indexed_by<
                boost::multi_index::random_access<
                >,
                boost::multi_index::hashed_unique<
                    boost::multi_index::tag<hashed_idx>,
                    boost::multi_index::composite_key<
                        named_document,
                        boost::multi_index::member<named_document,std::string,&named_document::collection_name>,
                        boost::multi_index::member<named_document,std::string,&named_document::key>,
                        boost::multi_index::member<named_document,std::string,&named_document::keyval>,
                        boost::multi_index::member<named_document,bool,&named_document::is_removal>
                    >
                >
            >
        > db_map;

    void bmi_insert_or_replace(db_map& bmi, named_document doc);

    using named_document_ptr = std::unique_ptr<named_document>;

    inline std::string hex_md5(const std::string& input)
    {
       uint8_t digest[16];
       bson_md5_t md5;
       char digest_str[33];
       unsigned int i;

       bson_md5_init (&md5);
       bson_md5_append (&md5, (const uint8_t *) input.c_str(), (uint32_t) input.size());
       bson_md5_finish (&md5, digest);

       for (i = 0; i < sizeof digest; i++) {
          bson_snprintf (&digest_str[i * 2], 3, "%02x", digest[i]);
       }
       digest_str[sizeof digest_str - 1] = '\0';

       return std::string(bson_strdup (digest_str));
    }

    inline std::string hash_oid(const std::string& value) {
        return hex_md5(value).substr(0, 24);
    }

    inline void format_oid(document& doc, const std::string& name, const std::string& value) {
        auto oid = hash_oid(value);
        doc << name << bsoncxx::oid(oid);
    }

    inline void format_oid(document& doc, const std::string& value) {
        format_oid(doc, "_id", value);
    }

    // Helper functions
    inline void format_value(document& doc, const std::string& name, const asset& value) {
        doc << name + "_value" << value.to_real();
        doc << name + "_symbol" << value.symbol_name();
    }

    inline void format_value(document& doc, const std::string& name, const price& value) {
        format_value(doc, name + "_base", value.base);
        format_value(doc, name + "_quote", value.quote);
    }

    inline void format_value(document& doc, const std::string& name, const std::string& value) {
        doc << name << value;
    }

    inline void format_value(document& doc, const std::string& name, const bool value) {
        doc << name << value;
    }

    inline void format_value(document& doc, const std::string& name, const double value) {
        doc << name << value;
    }

    inline void format_value(document& doc, const std::string& name, const fc::uint128_t& value) {
        doc << name << static_cast<int64_t>(value.lo);
    }

    inline void format_value(document& doc, const std::string& name, const fc::time_point_sec& value) {
        doc << name << value.to_iso_string();
    }

    inline void format_value(document& doc, const std::string& name, const shared_string& value) {
        doc << name << to_string(value);
    }

    template <typename T>
    inline void format_value(document& doc, const std::string& name, const fc::fixed_string<T>& value) {
        doc << name << static_cast<std::string>(value);
    }

    template <typename T>
    inline void format_value(document& doc, const std::string& name, const T& value) {
        doc << name << static_cast<int64_t>(value);
    }

    template <typename T>
    inline void format_value(document& doc, const std::string& name, const fc::safe<T>& value) {
        doc << name << static_cast<int64_t>(value.value);
    }

    template <typename Iterable, typename Func>
    inline void format_array_value(document& doc, const std::string& name, const Iterable& value,
        Func each_item_converter) {
            array results_array;
            for (auto& item: value) {
                results_array << each_item_converter(item);
            }
            doc << name << results_array;
    }

    template <typename T, typename Pred, typename Alloc, typename Func>
    inline void format_array_value(document& doc, const std::string& name, const bip::flat_set<T, Pred, Alloc>& value,
        Func each_item_converter) {
            if (!value.empty()) {
                array results_array;
                for (auto& item: value) {
                    results_array << each_item_converter(item);
                }
                doc << name << results_array;
            }
    }
    
}}} // golos::plugins::mongo_db
