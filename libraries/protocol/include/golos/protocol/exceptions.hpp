#pragma once

#include <fc/exception/exception.hpp>
#include <golos/protocol/protocol.hpp>

#define GOLOS_ASSERT_MESSAGE(FORMAT, ...) \
    FC_LOG_MESSAGE(error, FORMAT, __VA_ARGS__)

#define GOLOS_CTOR_ASSERT(expr, exception_type, exception_ctor) \
    if (!(expr)) { \
        exception_type _E; \
        exception_ctor(_E); \
        throw _E; \
    }

#define GOLOS_ASSERT(expr, exception_type, FORMAT, ...) \
    if (!(expr)) { \
        throw exception_type(GOLOS_ASSERT_MESSAGE(FORMAT, __VA_ARGS__)); \
    }

#define GOLOS_DECLARE_DERIVED_EXCEPTION_BODY(TYPE, BASE, CODE, WHAT) \
    public: \
        enum code_enum { \
            code_value = CODE, \
        }; \
        TYPE(fc::log_message&& m) \
            : BASE(fc::move(m), CODE, BOOST_PP_STRINGIZE(TYPE), WHAT) {} \
        TYPE() \
            : BASE(CODE, BOOST_PP_STRINGIZE(TYPE), WHAT) {} \
        virtual std::shared_ptr<fc::exception> dynamic_copy_exception() const { \
            return std::make_shared<TYPE>(*this); \
        } \
        virtual NO_RETURN void dynamic_rethrow_exception() const { \
            if (code() == CODE) { \
                throw *this; \
            } else { \
                fc::exception::dynamic_rethrow_exception(); \
            } \
        } \
    protected: \
        TYPE(const BASE& c) \
            : BASE(c) {} \
        explicit TYPE(int64_t code, const std::string& name_value, const std::string& what_value) \
            : BASE(code, name_value, what_value) {} \
        explicit TYPE(fc::log_message&& m, int64_t code, const std::string& name_value, const std::string& what_value) \
            : BASE(std::move(m), code, name_value, what_value) {}

#define GOLOS_DECLARE_DERIVED_EXCEPTION(TYPE, BASE, CODE, WHAT) \
    class TYPE: public BASE { \
        GOLOS_DECLARE_DERIVED_EXCEPTION_BODY(TYPE, BASE, CODE, WHAT) \
    };

namespace golos { namespace protocol {
    GOLOS_DECLARE_DERIVED_EXCEPTION(
        transaction_exception, fc::exception,
        3000000, "transaction exception")

    class tx_missing_active_auth: public transaction_exception {
        GOLOS_DECLARE_DERIVED_EXCEPTION_BODY(
            tx_missing_active_auth, transaction_exception,
            3010000, "missing required active authority");
    public:
        std::vector<account_name_type> missing_accounts;
        std::vector<public_key_type> used_signatures;
    };

    class tx_missing_owner_auth: public transaction_exception {
        GOLOS_DECLARE_DERIVED_EXCEPTION_BODY(
            tx_missing_owner_auth, transaction_exception,
            3020000, "missing required owner authority");
    public:
        std::vector<account_name_type> missing_accounts;
        std::vector<public_key_type> used_signatures;
    };

    class tx_missing_posting_auth: public transaction_exception {
        GOLOS_DECLARE_DERIVED_EXCEPTION_BODY(
            tx_missing_posting_auth, transaction_exception,
            3030000, "missing required posting authority");
    public:
        std::vector<account_name_type> missing_accounts;
        std::vector<public_key_type> used_signatures;
    };

    class tx_missing_other_auth: public transaction_exception {
        GOLOS_DECLARE_DERIVED_EXCEPTION_BODY(
            tx_missing_other_auth, transaction_exception,
            3040000, "missing required other authority");
    public:
        std::vector<authority> missing_auths;
    };

    class tx_irrelevant_sig: public transaction_exception {
        GOLOS_DECLARE_DERIVED_EXCEPTION_BODY(
            tx_irrelevant_sig, transaction_exception,
            3050000, "irrelevant signature included");
    public:
        std::vector<public_key_type> unused_signatures;
    };

    class tx_duplicate_sig: public transaction_exception {
        GOLOS_DECLARE_DERIVED_EXCEPTION_BODY(
            tx_duplicate_sig, transaction_exception,
            3060000, "duplicate signature included");
    };

    class tx_irrelevant_approval: public transaction_exception {
        GOLOS_DECLARE_DERIVED_EXCEPTION_BODY(
            tx_irrelevant_approval, transaction_exception,
            3070000, "irrelevant approval included");
    public:
        std::vector<account_name_type> unused_approvals;
    };

} } // golos::protocol
