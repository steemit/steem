#pragma once

#include <fc/exception/exception.hpp>
#include <golos/protocol/protocol.hpp>

#define STEEMIT_ASSERT(expr, exc_type, FORMAT, ...)                \
   FC_MULTILINE_MACRO_BEGIN                                           \
   if( !(expr) )                                                      \
      FC_THROW_EXCEPTION( exc_type, FORMAT, __VA_ARGS__ );            \
   FC_MULTILINE_MACRO_END

namespace golos {
    namespace protocol {
#define FC_DECLARE_DERIVED_EXCEPTION( TYPE, BASE, CODE, WHAT ) \
    class TYPE : public BASE  \
    { \
       public: \
        enum code_enum { \
           code_value = CODE, \
        }; \
        explicit TYPE( int64_t code, const std::string& name_value, const std::string& what_value ) \
        :BASE( code, name_value, what_value ){} \
        explicit TYPE( fc::log_message&& m, int64_t code, const std::string& name_value, const std::string& what_value ) \
        :BASE( std::move(m), code, name_value, what_value ){} \
        explicit TYPE( fc::log_messages&& m, int64_t code, const std::string& name_value, const std::string& what_value )\
        :BASE( std::move(m), code, name_value, what_value ){}\
        explicit TYPE( const fc::log_messages& m, int64_t code, const std::string& name_value, const std::string& what_value )\
        :BASE( m, code, name_value, what_value ){}\
        TYPE( const std::string& what_value, const fc::log_messages& m ) \
        :BASE( m, CODE, BOOST_PP_STRINGIZE(TYPE), what_value ){} \
        TYPE( fc::log_message&& m ) \
        :BASE( fc::move(m), CODE, BOOST_PP_STRINGIZE(TYPE), WHAT ){}\
        TYPE( fc::log_messages msgs ) \
        :BASE( fc::move( msgs ), CODE, BOOST_PP_STRINGIZE(TYPE), WHAT ) {} \
        TYPE( const TYPE& c ) \
        :BASE(c){} \
        TYPE( const BASE& c ) \
        :BASE(c){} \
        TYPE():BASE(CODE, BOOST_PP_STRINGIZE(TYPE), WHAT){}\
        \
        virtual std::shared_ptr<fc::exception> dynamic_copy_exception()const\
        { return std::make_shared<TYPE>( *this ); } \
        virtual NO_RETURN void     dynamic_rethrow_exception()const \
        { if( code() == CODE ) throw *this;\
          else fc::exception::dynamic_rethrow_exception(); \
        } \
   };

#define FC_DECLARE_EXCEPTION( TYPE, CODE, WHAT ) \
      FC_DECLARE_DERIVED_EXCEPTION( TYPE, fc::exception, CODE, WHAT )

        FC_DECLARE_EXCEPTION(transaction_exception, 3000000, "transaction exception")

        FC_DECLARE_DERIVED_EXCEPTION(tx_missing_active_auth, golos::protocol::transaction_exception, 3010000, "missing required active authority")

        FC_DECLARE_DERIVED_EXCEPTION(tx_missing_owner_auth, golos::protocol::transaction_exception, 3020000, "missing required owner authority")

        FC_DECLARE_DERIVED_EXCEPTION(tx_missing_posting_auth, golos::protocol::transaction_exception, 3030000, "missing required posting authority")

        FC_DECLARE_DERIVED_EXCEPTION(tx_missing_other_auth, golos::protocol::transaction_exception, 3040000, "missing required other authority")

        FC_DECLARE_DERIVED_EXCEPTION(tx_irrelevant_sig, golos::protocol::transaction_exception, 3050000, "irrelevant signature included")

        FC_DECLARE_DERIVED_EXCEPTION(tx_duplicate_sig, golos::protocol::transaction_exception, 3060000, "duplicate signature included")

#define STEEMIT_RECODE_EXC(cause_type, effect_type) \
      catch( const cause_type& e ) \
      { throw( effect_type( e.what(), e.get_log() ) ); }

    }
} // golos::protocol
