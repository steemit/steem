#pragma once

#include <fc/reflect/reflect.hpp>

#include <string>
#include <fc/variant_object.hpp>

namespace steemit { namespace object_builder {

/**
 * Builds various strings built from the full_object_descriptor.
 * This is a "helper" function that is only necessary because fc's
 * template language is very primitive (e.g. if we used Jinja2 the
 * template itself could contain the logic for iterating over
 * template variables and do various tasks of iteration, filtering,
 * separator insertion, etc.)
 */
fc::variant_object build_context_variant( const full_object_descriptor& fdesc );

} }
