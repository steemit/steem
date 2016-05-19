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

#include <steemit/app/plugin.hpp>
#include <steemit/chain/database.hpp>

#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/composite_key.hpp>

#include <fc/thread/future.hpp>
#include <fc/api.hpp>

namespace steemit { namespace private_message {
using namespace chain;

//
// Plugins should #define their SPACE_ID's so plugins with
// conflicting SPACE_ID assignments can be compiled into the
// same binary (by simply re-assigning some of the conflicting #defined
// SPACE_ID's in a build script).
//
// Assignment of SPACE_ID's cannot be done at run-time because
// various template automagic depends on them being known at compile
// time.
//
#ifndef PRIVATE_MESSAGE_SPACE_ID
#define PRIVATE_MESSAGE_SPACE_ID 6
#endif

#define STEEMIT_PRIVATE_MESSAGE_COP_ID 777

enum private_message_object_type
{
   key_account_object_type = 0,
   bucket_object_type = 1,///< used in market_history_plugin
   message_object_type = 2
};


namespace detail
{
    class private_message_plugin_impl;
}

struct message_body {
    fc::time_point    thread_start; /// the sent_time of the original message, if any
    string            subject;
    string            body;
    string            json_meta;
    flat_set<string>  cc;
};



class  message_object : public abstract_object<message_object> {
   public:
      static const uint8_t space_id = PRIVATE_MESSAGE_SPACE_ID;
      static const uint8_t type_id  = message_object_type;

      string             from;
      string             to;
      public_key_type    from_memo_key;
      public_key_type    to_memo_key;
      uint64_t           sent_time; /// used as seed to secret generation
      fc::time_point_sec receive_time; /// time received by blockchain
      uint32_t           checksum = 0;
      vector<char>       encrypted_message;
};

struct extended_message_object : public message_object {
   extended_message_object(){}
   extended_message_object( const message_object& o ):message_object(o){}
   message_body   message;
};

struct private_message_operation {
    string             from;
    string             to;
    public_key_type    from_memo_key;
    public_key_type    to_memo_key;
    uint64_t           sent_time; /// used as seed to secret generation
    uint32_t           checksum = 0;
    vector<char>       encrypted_message;
};

struct by_to_date;
struct by_from_date;
struct by_id;

using namespace boost::multi_index;

typedef multi_index_container<
   message_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< object, object_id_type, &object::id > >,
      ordered_unique< tag< by_to_date >, 
            composite_key< message_object,
               member< message_object, string, &message_object::to >,
               member< message_object, time_point_sec, &message_object::receive_time >,
               member<object, object_id_type, &object::id >
            >,
            composite_key_compare< std::less<string>, std::greater< time_point_sec >, std::less< object_id_type > >
      >,
      ordered_unique< tag< by_from_date >, 
            composite_key< message_object,
               member< message_object, string, &message_object::from >,
               member< message_object, time_point_sec, &message_object::receive_time >,
               member<object, object_id_type, &object::id >
            >,
            composite_key_compare< std::less<string>, std::greater< time_point_sec >, std::less< object_id_type > >
      >
   >
> message_multi_index_type;

typedef graphene::db::generic_index< message_object, message_multi_index_type> private_message_index;



/**
 *   This plugin scans the blockchain for custom operations containing a valid message and authorized
 *   by the posting key.
 *  
 */
class private_message_plugin : public steemit::app::plugin
{
   public:
      private_message_plugin();
      virtual ~private_message_plugin();

      std::string plugin_name()const override;
      virtual void plugin_set_program_options(
         boost::program_options::options_description& cli,
         boost::program_options::options_description& cfg) override;
      virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
      virtual void plugin_startup() override;


      flat_map<string,string> tracked_accounts()const; /// map start_range to end_range

      friend class detail::private_message_plugin_impl;
      std::unique_ptr<detail::private_message_plugin_impl> my;
};

class private_message_api : public std::enable_shared_from_this<private_message_api> {
   public:
      private_message_api(){};
      private_message_api(const app::api_context& ctx):_app(&ctx.app){
         ilog( "creating private message api" );
      }
      void on_api_startup(){
         wlog( "on private_message api startup" );
      }
      
      /**
       *
       */
      vector<message_object> get_inbox( string to, time_point newest, uint16_t limit )const;
      vector<message_object> get_outbox( string from, time_point newest, uint16_t limit )const;

   private:
      app::application* _app = nullptr;
};



} } //steemit::private_message

FC_API( steemit::private_message::private_message_api, (get_inbox)(get_outbox) );

FC_REFLECT( steemit::private_message::message_body, (thread_start)(subject)(body)(json_meta)(cc) );
FC_REFLECT_DERIVED( steemit::private_message::message_object, (graphene::db::object), (from)(to)(from_memo_key)(to_memo_key)(sent_time)(receive_time)(checksum)(encrypted_message) );
FC_REFLECT_DERIVED( steemit::private_message::extended_message_object, (steemit::private_message::message_object), (message) );

FC_REFLECT( steemit::private_message::private_message_operation, (from)(to)(from_memo_key)(to_memo_key)(sent_time)(checksum)(encrypted_message) );


