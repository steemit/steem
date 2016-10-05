#pragma once
#include <steemit/protocol/types.hpp>
#include <fc/interprocess/container.hpp>

namespace steemit { namespace protocol {

   //typedef std::unique_ptr< abstract_authority > authority_ptr;

   /**
    * The authority class, being a dynamic sized class, needs to be allocated with the memory
    * mapped segment manager when stored on the account object. However, authorities created
    * by operations reside on the stack. They require the default memory allocator. To accomodate
    * the interoperability between these classes, all authorities are referred to by the abstract
    * authority type which defines the virtual method interface required by authorities. The
    * authority impl class is a templated wrapper around each authority implementation. This is
    * needed at a minimum for the shared authority because we cannot have virtual methods on
    * objects in the memory mapped file.
    */
   /*struct abstract_authority
   {


      virtual ~abstract_authority() = 0;

      virtual void add_authority( const public_key_type& k, weight_type w )   = 0;
      virtual void add_authority( const account_name_type& k, weight_type w ) = 0;

      template< typename AuthType >
      void add_authorities( AuthType k, weight_type w )
      {
         add_authority( k, w );
      }

      template< typename AuthType, class ...Args >
      void add_authorities( AuthType k, weight_type w, Args... auths )
      {
         add_authority( k, w );
         add_authorities( auths... );
      }

      virtual vector< public_key_type > get_keys()const = 0;

      virtual bool     is_impossible()const = 0;
      virtual uint32_t num_auths()const     = 0;
      virtual void     clear()              = 0;
      virtual void     validate()           = 0;

      virtual uint32_t weight_threshold()const = 0;

      virtual size_t account_auth_size()const = 0;
      virtual std::pair< account_name_type, weight_type >& get_account_auth_at( size_t i )const = 0;

      virtual size_t key_auth_size()const = 0;
      virtual std::pair< public_key_type, weight_type >& get_key_auth_at( size_t i )const = 0;
   };

   template< typename AuthorityType >
   struct authority_impl : public abstract_authority
   {
      explicit authority_impl( const AuthorityType* a ) : _authority( a ) {}

      virtual void add_authority( const public_key_type& k, weight_type w ) override   { _authority->add_authority( k, w ); }
      virtual void add_authority( const account_name_type& k, weight_type w ) override { _authority->add_authority( k, w ); }

      virtual vector< public_key_type > get_keys()const override { return _authority->get_keys(); }

      virtual bool     is_impossible()const override { return _authority->is_impossible(); }
      virtual uint32_t num_auths()const override     { return _authority->num_auths();     }
      virtual void     clear() override              { return _authority->clear();         }
      virtual void     validate() override           { return _authority->validate();      }

      virtual uint32_t weight_threshold()const override { return _authority->weight_threshold; }

      virtual size_t account_auth_size()const override { return _authority->account_auths.size(); }
      virtual std::pair< account_name_type, weight_type >& get_account_auth_at( size_t i )const override { return _authority->account_auths.begin() + i; }

      virtual size_t key_auth_size()const override { return _authority->key_auths.size(); }
      virtual std::pair< public_key_type, weight_type >& get_key_auth_at( size_t i )const override { return _authority->key_auths.begin() + i; }

      const AuthorityType* _authority;
   };*/

   struct authority
   {
      authority(){}

      enum classification
      {
         owner   = 0,
         active  = 1,
         key     = 2,
         posting = 3
      };

      template< class ...Args >
      authority( uint32_t threshold, Args... auths )
         : weight_threshold( threshold )
      {
         add_authorities( auths... );
      }

      void add_authority( const public_key_type& k, weight_type w );
      void add_authority( const account_name_type& k, weight_type w );

      template< typename AuthType >
      void add_authorities( AuthType k, weight_type w )
      {
         add_authority( k, w );
      }

      template< typename AuthType, class ...Args >
      void add_authorities( AuthType k, weight_type w, Args... auths )
      {
         add_authority( k, w );
         add_authorities( auths... );
      }

      vector< public_key_type > get_keys()const;

      bool     is_impossible()const;
      uint32_t num_auths()const;
      void     clear();
      void     validate()const;

      typedef flat_map< account_name_type, weight_type, string_less > account_authority_map;
      typedef flat_map< public_key_type, weight_type >                key_authority_map;

      uint32_t                                                        weight_threshold = 0;
      account_authority_map                                           account_auths;
      key_authority_map                                               key_auths;
   };

template< typename AuthorityType >
void add_authority_accounts(
   flat_set<account_name_type>& result,
   const AuthorityType& a
   )
{
   for( auto& item : a.account_auths )
      result.insert( item.first );
}

/**
 * Names must comply with the following grammar (RFC 1035):
 * <domain> ::= <subdomain> | " "
 * <subdomain> ::= <label> | <subdomain> "." <label>
 * <label> ::= <letter> [ [ <ldh-str> ] <let-dig> ]
 * <ldh-str> ::= <let-dig-hyp> | <let-dig-hyp> <ldh-str>
 * <let-dig-hyp> ::= <let-dig> | "-"
 * <let-dig> ::= <letter> | <digit>
 *
 * Which is equivalent to the following:
 *
 * <domain> ::= <subdomain> | " "
 * <subdomain> ::= <label> ("." <label>)*
 * <label> ::= <letter> [ [ <let-dig-hyp>+ ] <let-dig> ]
 * <let-dig-hyp> ::= <let-dig> | "-"
 * <let-dig> ::= <letter> | <digit>
 *
 * I.e. a valid name consists of a dot-separated sequence
 * of one or more labels consisting of the following rules:
 *
 * - Each label is three characters or more
 * - Each label begins with a letter
 * - Each label ends with a letter or digit
 * - Each label contains only letters, digits or hyphens
 *
 * In addition we require the following:
 *
 * - All letters are lowercase
 * - Length is between (inclusive) STEEMIT_MIN_ACCOUNT_NAME_LENGTH and STEEMIT_MAX_ACCOUNT_NAME_LENGTH
 */
bool is_valid_account_name( const string& name );

bool operator == ( const authority& a, const authority& b )
{
   return ( a.weight_threshold == b.weight_threshold ) &&
            ( a.account_auths  == b.account_auths )    &&
            ( a.key_auths      == b.key_auths );
}

} } // namespace steemit::protocol


FC_REFLECT_TYPENAME( steemit::protocol::authority::account_authority_map)
FC_REFLECT_TYPENAME( steemit::protocol::authority::key_authority_map)
FC_REFLECT( steemit::protocol::authority, (weight_threshold)(account_auths)(key_auths) )
FC_REFLECT_ENUM( steemit::protocol::authority::classification, (owner)(active)(key)(posting) )
