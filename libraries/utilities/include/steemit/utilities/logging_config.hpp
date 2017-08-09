#pragma once

#include <fc/log/console_appender.hpp>
#include <fc/log/file_appender.hpp>
#include <fc/log/logger.hpp>
#include <fc/log/logger_config.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/io/json.hpp>

#include <boost/program_options.hpp>

namespace steemit { namespace utilities {

struct console_appender_args
{
   std::string appender;
   std::string stream;
};

struct file_appender_args
{
   std::string appender;
   std::string file;
};

struct logger_args
{
   std::string name;
   std::string level;
   std::string appender;
};

void set_logging_program_options( boost::program_options::options_description& options );

fc::optional<fc::logging_config> load_logging_config( const boost::program_options::variables_map& args, const boost::filesystem::path& pwd );

} } // steemit::utilities

FC_REFLECT( steemit::utilities::console_appender_args, (appender)(stream) )
FC_REFLECT( steemit::utilities::file_appender_args, (appender)(file) )
FC_REFLECT( steemit::utilities::logger_args, (name)(level)(appender) )
