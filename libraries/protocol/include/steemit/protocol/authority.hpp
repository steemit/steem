#pragma once
#include <steemit/protocol/types.hpp>
#include <fc/interprocess/container.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>

namespace steemit { namespace protocol {
   namespace bip = boost::interprocess;

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
      void add_authority( const account_name_type& k, weight_type w )
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


      struct string_less {
         bool operator()( const account_name_type& a, const account_name_type& b )const {
            return string(a) < string(b);
         }
      };

      typedef flat_map<account_name_type,weight_type,string_less>  account_authority_map;
      typedef flat_map<public_key_type,weight_type>                key_authority_map;

      uint32_t                                             weight_threshold = 0;
      account_authority_map                                account_auths;
      key_authority_map                                    key_auths;

      void validate()const;
   };

   /**
    *  The purpose of this class is to represent an authority object in a manner compatiable with
    *  shared memory storage.  This requires all dynamic fields to be allocated with the same allocator
    *  that allocated the shared_authority.
    */
   struct shared_authority
   {
      typedef bip::allocator<std::pair<account_name_type,weight_type>, bip::managed_mapped_file::segment_manager> account_pair_allocator_type;
      typedef bip::allocator<std::pair<public_key_type,weight_type>, bip::managed_mapped_file::segment_manager> key_pair_allocator_type;

      typedef bip::flat_map<account_name_type, weight_type, authority::string_less, account_pair_allocator_type> account_authority_map;
      typedef bip::flat_map<public_key_type, weight_type, std::less<public_key_type>, key_pair_allocator_type> key_authority_map;

      template<typename Allocator>
      shared_authority( const authority& a, const Allocator& alloc )
      :account_auths( account_pair_allocator_type( alloc.get_segment_manager() )  ), key_auths( key_pair_allocator_type( alloc.get_segment_manager() ) ) {
         account_auths.reserve( a.account_auths.size() );
         key_auths.reserve( a.key_auths.size() );
         for( const auto& item : a.account_auths )
            account_auths.insert( item );
         for( const auto& item : a.key_auths )
            key_auths.insert( item );
         weight_threshold = a.weight_threshold;
      }

      shared_authority( const shared_authority& cpy )
      :weight_threshold(cpy.weight_threshold),account_auths(cpy.account_auths), key_auths( cpy.key_auths ){}

      template<typename Allocator>
      shared_authority( const Allocator& alloc )
      :account_auths( account_pair_allocator_type( alloc.get_segment_manager() )  ),
       key_auths( key_pair_allocator_type( alloc.get_segment_manager() ) ){}

      operator authority()const {
         authority result;
         result.account_auths.reserve( account_auths.size() );
         for( const auto& item : account_auths )
            result.account_auths.insert( item );
         result.key_auths.reserve( key_auths.size() );
         for( const auto& item : key_auths )
            result.key_auths.insert( item );
         result.weight_threshold = weight_threshold;
         return result;
      }

      shared_authority& operator=( const authority& a ) {
         for( const auto& item : a.account_auths )
            account_auths.insert( item );
         for( const auto& item : a.key_auths )
            key_auths.insert( item );
         weight_threshold = a.weight_threshold;
         return *this;
      }

      uint32_t                                                                                weight_threshold = 0;
      account_authority_map                                                                   account_auths;
      key_authority_map                                                                       key_auths;
   };

void add_authority_accounts(
   flat_set<account_name_type>& result,
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

} } // namespace steemit::protocol

FC_REFLECT_TYPENAME( steemit::protocol::authority::account_authority_map)
FC_REFLECT_TYPENAME( steemit::protocol::authority::key_authority_map)
FC_REFLECT_TYPENAME( steemit::protocol::shared_authority::account_authority_map)
FC_REFLECT( steemit::protocol::authority, (weight_threshold)(account_auths)(key_auths) )
FC_REFLECT( steemit::protocol::shared_authority, (weight_threshold)(account_auths)(key_auths) )
FC_REFLECT_ENUM( steemit::protocol::authority::classification, (owner)(active)(key)(posting) )
