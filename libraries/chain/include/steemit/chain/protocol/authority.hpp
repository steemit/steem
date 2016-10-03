#pragma once
#include <steemit/chain/protocol/types.hpp>

namespace steemit { namespace chain {

   struct authority
   {
      authority(){}
      template<class ...Args>
      authority(uint32_t threshhold, Args... auths)
         : weight_threshold(threshhold)
      {
         add_authorities(auths...);
      }

      enum classification
      {
         owner   = 0,
         active  = 1,
         key     = 2,
         posting = 3
      };
      void add_authority( const public_key_type& k, weight_type w )
      {
         key_auths[k] = w;
      }
      void add_authority( string k, weight_type w )
      {
         account_auths[k] = w;
      }
      bool is_impossible()const
      {
         uint64_t auth_weights = 0;
         for( const auto& item : account_auths ) auth_weights += item.second;
         for( const auto& item : key_auths ) auth_weights += item.second;
         return auth_weights < weight_threshold;
      }

      template<typename AuthType>
      void add_authorities(AuthType k, weight_type w)
      {
         add_authority(k, w);
      }
      template<typename AuthType, class ...Args>
      void add_authorities(AuthType k, weight_type w, Args... auths)
      {
         add_authority(k, w);
         add_authorities(auths...);
      }

      vector<public_key_type> get_keys() const
      {
         vector<public_key_type> result;
         result.reserve( key_auths.size() );
         for( const auto& k : key_auths )
            result.push_back(k.first);
         return result;
      }

      friend bool operator == ( const authority& a, const authority& b )
      {
         return (a.weight_threshold == b.weight_threshold) &&
                (a.account_auths == b.account_auths) &&
                (a.key_auths == b.key_auths);
      }
      uint32_t num_auths()const { return account_auths.size() + key_auths.size(); }
      void     clear() { account_auths.clear(); key_auths.clear(); }

      uint32_t                              weight_threshold = 0;
      flat_map<string,weight_type>          account_auths;
      flat_map<public_key_type,weight_type> key_auths;

      void validate()const;
   };

void add_authority_accounts(
   flat_set<string>& result,
   const authority& a
   );

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

} } // namespace steemit::chain

FC_REFLECT( steemit::chain::authority, (weight_threshold)(account_auths)(key_auths) )
FC_REFLECT_ENUM( steemit::chain::authority::classification, (owner)(active)(key)(posting) )
