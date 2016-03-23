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

#include <string>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>

#include <fc/exception/exception.hpp>

namespace steemit { namespace wallet {

   struct method_description
   {
      std::string method_name;
      std::string brief_description;
      std::string detailed_description;
   };

   class api_documentation
   {
      typedef boost::multi_index::multi_index_container<method_description,
         boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<
               boost::multi_index::member<method_description, std::string, &method_description::method_name> > > > method_description_set;
      method_description_set method_descriptions;
   public:
      api_documentation();
      std::string get_brief_description(const std::string& method_name) const
      {
         auto iter = method_descriptions.find(method_name);
         if (iter != method_descriptions.end())
            return iter->brief_description;
         else
            FC_THROW_EXCEPTION(fc::key_not_found_exception, "No entry for method ${name}", ("name", method_name));
      }
      std::string get_detailed_description(const std::string& method_name) const
      {
         auto iter = method_descriptions.find(method_name);
         if (iter != method_descriptions.end())
            return iter->detailed_description;
         else
            FC_THROW_EXCEPTION(fc::key_not_found_exception, "No entry for method ${name}", ("name", method_name));
      }
      std::vector<std::string> get_method_names() const
      {
         std::vector<std::string> method_names;
         for (const method_description& method_description: method_descriptions)
            method_names.emplace_back(method_description.method_name);
         return method_names;
      }
   };

} } // end namespace steemit::wallet
