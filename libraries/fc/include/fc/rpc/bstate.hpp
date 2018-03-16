#pragma once
#include <fc/variant.hpp>
#include <functional>
#include <fc/thread/future.hpp>
#include <fc/rpc/state.hpp>

namespace fc { namespace rpc {
   typedef std::vector<char> params_type;
   typedef std::vector<char> result_type;

   struct brequest
   {
      optional<uint64_t>  id;
      std::string         method;
      params_type         params;
   };

   struct bresponse
   {
      bresponse(){}
      bresponse( int64_t i, result_type r ):id(i),result(r){}
      bresponse( int64_t i, error_object r ):id(i),error(r){}
      int64_t                id = 0;
      optional<result_type>  result;
      optional<error_object> error;
   };

   /** binary RPC state */
   class bstate
   {
      public:
         typedef std::function<result_type(const params_type&)>  method;
         ~bstate();

         void add_method( const fc::string& name, method m );
         void remove_method( const fc::string& name );

         result_type local_call( const string& method_name, const params_type& args );
         void    handle_reply( const bresponse& response );

         brequest start_remote_call( const string& method_name, params_type args );
         result_type wait_for_response( uint64_t request_id );

         void close();

         void on_unhandled( const std::function<result_type(const string&,const params_type&)>& unhandled );

      private:
         uint64_t                                                      _next_id = 1;
         std::unordered_map<uint64_t, fc::promise<result_type>::ptr>   _awaiting;
         std::unordered_map<std::string, method>                       _methods;
         std::function<result_type(const string&,const params_type&)>         _unhandled;
   };
} }  // namespace  fc::rpc

FC_REFLECT( fc::rpc::brequest, (id)(method)(params) );
FC_REFLECT( fc::rpc::bresponse, (id)(result)(error) )

