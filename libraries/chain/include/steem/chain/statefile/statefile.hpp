#pragma once

#include <fc/static_variant.hpp>
#include <fc/reflect/reflect.hpp>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace steem { namespace chain {

class database;

namespace statefile {

// Version        : Must precisely match what is output by embedded code.
// Header         : JSON object that lists sections
// Section header : JSON object that lists count of objects
// Section footer : JSON object that lists hash/offset of section header
// Footer         : JSON object that lists hash/offset of all sections

// db_format_version : Must match STEEM_DB_FORMAT_VERSION
// network_type      : Must match STEEM_NETWORK_TYPE
// chain_id          : Must match requested chain ID and value of embedded GPO

struct steem_version_info
{
   std::string                          db_format_version;
   std::string                          network_type;
   std::map< std::string, std::string > object_schemas;
   std::map< std::string, fc::variant > config;
   std::string                          chain_id;
};

struct object_section
{
   std::string                   object_type;
   std::string                   format;
   int64_t                       object_count = 0;
};

typedef fc::static_variant< object_section > section_header;

struct state_header
{
   steem_version_info              version;
   std::vector< section_header >   sections;
};

struct section_footer
{
   std::string       hash;
   int64_t           begin_offset = 0;
   int64_t           end_offset = 0;
};

struct state_footer : public section_footer
{
   std::vector< section_footer >      section_footers;
};

struct write_state_result
{
   int64_t         size = 0;
   std::string     hash;
};

write_state_result write_state( const database& db, const std::string& state_filename );
void init_genesis_from_state( const database& db, const std::string& state_filename );

} } }

FC_REFLECT( steem::chain::statefile::steem_version_info,
   (db_format_version)
   (network_type)
   (object_schemas)
   (config)
   (chain_id)
   )

FC_REFLECT( steem::chain::statefile::state_header,
   (version)
   (sections)
   )

FC_REFLECT( steem::chain::statefile::object_section,
   (object_type)
   (format)
   (object_count)
   )

FC_REFLECT_TYPENAME( steem::chain::statefile::section_header )

FC_REFLECT( steem::chain::statefile::section_footer,
   (hash)
   (begin_offset)
   (end_offset)
   )

FC_REFLECT_DERIVED( steem::chain::statefile::state_footer,
   (steem::chain::statefile::section_footer),
   (section_footers)
   )
