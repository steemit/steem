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
#include <iomanip>
#include <boost/algorithm/string/join.hpp>
#include <steemit/wallet/wallet.hpp>
#include <steemit/wallet/api_documentation.hpp>

namespace steemit { namespace wallet {
   namespace detail {
      namespace
      {
         template <typename... Args>
         struct types_to_string_list_helper;

         template <typename First, typename... Args>
         struct types_to_string_list_helper<First, Args...>
         {
            std::list<std::string> operator()() const
            {
               std::list<std::string> argsList = types_to_string_list_helper<Args...>()();
               argsList.push_front(fc::get_typename<typename std::decay<First>::type>::name());
               return argsList;
            }
         };

         template <>
         struct types_to_string_list_helper<>
         {
            std::list<std::string> operator()() const
            {
               return std::list<std::string>();
            }
         };

         template <typename... Args>
         std::list<std::string> types_to_string_list()
         {
            return types_to_string_list_helper<Args...>()();
         }
      } // end anonymous namespace

      struct help_visitor
      {
         std::vector<method_description> method_descriptions;

         template<typename R, typename... Args>
         void operator()( const char* name, std::function<R(Args...)>& memb )
         {
            method_description this_method;
            this_method.method_name = name;
            std::ostringstream ss;
            ss << std::setw(40) << std::left << fc::get_typename<R>::name() << " " << name << "(";
            ss << boost::algorithm::join(types_to_string_list<Args...>(), ", ");
            ss << ")\n";
            this_method.brief_description = ss.str();
            method_descriptions.push_back(this_method);
         }
      };
   } // end namespace detail

   api_documentation::api_documentation()
   {
      fc::api<wallet_api> tmp;
      detail::help_visitor visitor;
      tmp->visit(visitor);
      std::copy(visitor.method_descriptions.begin(), visitor.method_descriptions.end(),
                std::inserter(method_descriptions, method_descriptions.end()));
   }

} } // end namespace steemit::wallet
