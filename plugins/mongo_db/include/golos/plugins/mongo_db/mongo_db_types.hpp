#pragma once

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

#include <fc/crypto/sha1.hpp>

namespace golos {
namespace plugins {
namespace mongo_db {

    using namespace golos::chain;
    using namespace golos::protocol;
    using golos::chain::to_string;
    using golos::chain::shared_string;
    using bsoncxx::builder::stream::document;

    struct named_document {
        named_document() = default;
        document doc;
        std::string collection_name;
    };

    using named_document_ptr = std::unique_ptr<named_document>;

    inline void format_oid(document& doc, const std::string& name, const std::string& value) {
        auto oid = fc::sha1::hash(value).str().substr(0, 24);
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
    
}}} // golos::plugins::mongo_db
