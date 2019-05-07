#pragma once
#include <string>
#include <boost/any.hpp>
#include <boost/core/demangle.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <fc/io/json.hpp>
#include <fc/exception/exception.hpp>
#include <fc/variant.hpp>
#include <fc/variant_object.hpp>
#include <rocksdb/filter_policy.h>
#include <rocksdb/table.h>
#include <rocksdb/statistics.h>
#include <rocksdb/rate_limiter.h>
#include <rocksdb/convenience.h>

namespace mira
{

class configuration
{
   configuration() = delete;
   static fc::variant_object apply_configuration_overlay( const fc::variant& base, const fc::variant& overlay );
   static fc::variant_object retrieve_active_configuration( const fc::variant_object& obj, std::string type_name );
   static fc::variant_object retrieve_global_configuration( const fc::variant_object& obj );
public:
   static ::rocksdb::Options get_options( const boost::any& cfg, std::string type_name );
   static bool gather_statistics( const boost::any& cfg );
   static size_t get_object_count( const boost::any& cfg );
};

} // mira
