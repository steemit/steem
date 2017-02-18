/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once

#include <fc/exception/exception.hpp>
#include <fc/io/varint.hpp>
#include <memory>

#define GRAPHENE_DB_MAX_INSTANCE_ID  (uint64_t(-1)>>16)

namespace graphene {
    namespace db {
        using std::shared_ptr;
        using std::unique_ptr;
        using std::vector;
        using fc::flat_map;
        using fc::variant;
        using fc::unsigned_int;
        using fc::signed_int;

        struct object_id_type {
            object_id_type(uint8_t s, uint8_t t, uint64_t i) {
                assert(i >> 48 == 0);
                FC_ASSERT(i >> 48 == 0, "instance overflow", ("instance", i));
                number = (uint64_t(s) << 56) | (uint64_t(t) << 48) | i;
            }

            object_id_type() {
                number = 0;
            }

            uint8_t space() const {
                return number >> 56;
            }

            uint8_t type() const {
                return number >> 48 & 0x00ff;
            }

            uint16_t space_type() const {
                return number >> 48;
            }

            uint64_t instance() const {
                return number & GRAPHENE_DB_MAX_INSTANCE_ID;
            }

            bool is_null() const {
                return number == 0;
            }

            operator uint64_t() const {
                return number;
            }

            friend bool operator==(const object_id_type &a, const object_id_type &b) {
                return a.number == b.number;
            }

            friend bool operator!=(const object_id_type &a, const object_id_type &b) {
                return a.number != b.number;
            }

            friend bool operator<(const object_id_type &a, const object_id_type &b) {
                return a.number < b.number;
            }

            friend bool operator>(const object_id_type &a, const object_id_type &b) {
                return a.number > b.number;
            }

            object_id_type &operator++(int) {
                ++number;
                return *this;
            }

            object_id_type &operator++() {
                ++number;
                return *this;
            }

            friend object_id_type operator+(const object_id_type &a, int delta) {
                return object_id_type(a.space(), a.type(),
                        a.instance() + delta);
            }

            friend object_id_type operator+(const object_id_type &a, int64_t delta) {
                return object_id_type(a.space(), a.type(),
                        a.instance() + delta);
            }

            friend size_t hash_value(object_id_type v) {
                return std::hash<uint64_t>()(v.number);
            }

            template<typename T>
            bool is() const {
                return (number >> 48) == ((T::space_id << 8) | (T::type_id));
            }

            template<typename T>
            T as() const {
                FC_ASSERT(is<T>());
                return T(*this);
            }

            explicit operator std::string() const {
                return fc::to_string(space()) + "." + fc::to_string(type()) +
                       "." + fc::to_string(instance());
            }

            uint64_t number;
        };

        class object;

        class object_database;

        template<uint8_t SpaceID, uint8_t TypeID, typename T = object>
        struct object_id {
            typedef T type;
            static const uint8_t space_id = SpaceID;
            static const uint8_t type_id = TypeID;

            object_id() {
            }

            object_id(unsigned_int i) : instance(i) {
            }

            object_id(uint64_t i) : instance(i) {
                FC_ASSERT((i >> 48) == 0);
            }

            object_id(object_id_type id) : instance(id.instance()) {
            }

            friend object_id operator+(const object_id a, int64_t delta) {
                return object_id(uint64_t(a.instance.value + delta));
            }

            friend object_id operator+(const object_id a, int delta) {
                return object_id(uint64_t(a.instance.value + delta));
            }

            operator object_id_type() const {
                return object_id_type(SpaceID, TypeID, instance.value);
            }

            operator uint64_t() const {
                return object_id_type(*this).number;
            }

            template<typename DB>
            const T &operator()(const DB &db) const {
                return db.get(*this);
            }

            friend bool operator==(const object_id &a, const object_id &b) {
                return a.instance == b.instance;
            }

            friend bool operator!=(const object_id &a, const object_id &b) {
                return a.instance != b.instance;
            }

            friend bool operator==(const object_id_type &a, const object_id &b) {
                return a == object_id_type(b);
            }

            friend bool operator!=(const object_id_type &a, const object_id &b) {
                return a != object_id_type(b);
            }

            friend bool operator==(const object_id &b, const object_id_type &a) {
                return a == object_id_type(b);
            }

            friend bool operator!=(const object_id &b, const object_id_type &a) {
                return a != object_id_type(b);
            }

            friend bool operator<(const object_id &a, const object_id &b) {
                return a.instance.value < b.instance.value;
            }

            friend bool operator>(const object_id &a, const object_id &b) {
                return a.instance.value > b.instance.value;
            }

            friend size_t hash_value(object_id v) {
                return std::hash<uint64_t>()(v.instance.value);
            }

            unsigned_int instance;
        };

    }
} // graphene::db

FC_REFLECT(graphene::db::object_id_type, (number))

// REFLECT object_id manually because it has 2 template params
namespace fc {
    template<uint8_t SpaceID, uint8_t TypeID, typename T>
    struct get_typename<graphene::db::object_id<SpaceID, TypeID, T>> {
        static const char *name() {
            return typeid(get_typename).name();
            static std::string _str = string("graphene::db::object_id<") +
                                      fc::to_string(SpaceID) + ":" +
                                      fc::to_string(TypeID) + ">";
            return _str.c_str();
        }
    };

    template<uint8_t SpaceID, uint8_t TypeID, typename T>
    struct reflector<graphene::db::object_id<SpaceID, TypeID, T>> {
        typedef graphene::db::object_id<SpaceID, TypeID, T> type;
        typedef fc::true_type is_defined;
        typedef fc::false_type is_enum;
        enum member_count_enum {
            local_member_count = 1,
            total_member_count = 1
        };

        template<typename Visitor>
        static inline void visit(const Visitor &visitor) {
            typedef decltype(((type *)nullptr)->instance) member_type;
            visitor.TEMPLATE operator()<member_type, type, &type::instance>("instance");
        }
    };


    inline void to_variant(const graphene::db::object_id_type &var, fc::variant &vo) {
        vo = std::string(var);
    }

    inline void from_variant(const fc::variant &var, graphene::db::object_id_type &vo) {
        try {
            vo.number = 0;
            const auto &s = var.get_string();
            auto first_dot = s.find('.');
            auto second_dot = s.find('.', first_dot + 1);
            FC_ASSERT(first_dot != second_dot);
            FC_ASSERT(first_dot != 0 && first_dot != std::string::npos);
            vo.number = fc::to_uint64(s.substr(second_dot + 1));
            FC_ASSERT(vo.number <= GRAPHENE_DB_MAX_INSTANCE_ID);
            auto space_id = fc::to_uint64(s.substr(0, first_dot));
            FC_ASSERT(space_id <= 0xff);
            auto type_id = fc::to_uint64(s.substr(
                    first_dot + 1, second_dot - first_dot - 1));
            FC_ASSERT(type_id <= 0xff);
            vo.number |= (space_id << 56) | (type_id << 48);
        } FC_CAPTURE_AND_RETHROW((var))
    }

    template<uint8_t SpaceID, uint8_t TypeID, typename T>
    void to_variant(const graphene::db::object_id<SpaceID, TypeID, T> &var, fc::variant &vo) {
        vo = fc::to_string(SpaceID) + "." + fc::to_string(TypeID) + "." +
             fc::to_string(var.instance.value);
    }

    template<uint8_t SpaceID, uint8_t TypeID, typename T>
    void from_variant(const fc::variant &var, graphene::db::object_id<SpaceID, TypeID, T> &vo) {
        try {
            const auto &s = var.get_string();
            auto first_dot = s.find('.');
            auto second_dot = s.find('.', first_dot + 1);
            FC_ASSERT(first_dot != second_dot);
            FC_ASSERT(first_dot != 0 && first_dot != std::string::npos);
            FC_ASSERT(fc::to_uint64(s.substr(0, first_dot)) == SpaceID &&
                      fc::to_uint64(s.substr(
                              first_dot + 1, second_dot - first_dot - 1)) ==
                      TypeID,
                    "Space.Type.0 (${SpaceID}.${TypeID}.0) doesn't match expected value ${var}", ("TypeID", TypeID)("SpaceID", SpaceID)("var", var));
            vo.instance = fc::to_uint64(s.substr(second_dot + 1));
        } FC_CAPTURE_AND_RETHROW((var))
    }

} // namespace fc

namespace std {
    template<>
    struct hash<graphene::db::object_id_type> {
        size_t operator()(const graphene::db::object_id_type &x) const {
            return std::hash<uint64_t>()(x.number);
        }
    };
}
