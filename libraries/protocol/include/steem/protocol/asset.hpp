#pragma once
#include <steem/protocol/types.hpp>
#include <steem/protocol/config.hpp>
#include <steem/protocol/asset_symbol.hpp>

namespace steem { namespace protocol {

   struct asset
   {
      asset( const asset& _asset, asset_symbol_type id )
      :amount( _asset.amount ),symbol(id){}

      asset( share_type a, asset_symbol_type id )
         :amount(a),symbol(id){}

      asset()
         :amount(0),symbol(STEEM_SYMBOL){}

      share_type        amount;
      asset_symbol_type symbol;

      void validate()const;

      asset& operator += ( const asset& o )
      {
         FC_ASSERT( symbol == o.symbol );
         amount += o.amount;
         return *this;
      }

      asset& operator -= ( const asset& o )
      {
         FC_ASSERT( symbol == o.symbol );
         amount -= o.amount;
         return *this;
      }
      asset operator -()const { return asset( -amount, symbol ); }

      friend bool operator == ( const asset& a, const asset& b )
      {
         return std::tie( a.symbol, a.amount ) == std::tie( b.symbol, b.amount );
      }
      friend bool operator < ( const asset& a, const asset& b )
      {
         FC_ASSERT( a.symbol == b.symbol );
         return a.amount < b.amount;
      }

      friend bool operator <= ( const asset& a, const asset& b )
      {
         FC_ASSERT( a.symbol == b.symbol );
         return a.amount <= b.amount;
      }

      friend bool operator != ( const asset& a, const asset& b )
      {
         FC_ASSERT( a.symbol == b.symbol );
         return a.amount != b.amount;
      }

      friend bool operator > ( const asset& a, const asset& b )
      {
         FC_ASSERT( a.symbol == b.symbol );
         return a.amount > b.amount;
      }

      friend bool operator >= ( const asset& a, const asset& b )
      {
         FC_ASSERT( a.symbol == b.symbol );
         return a.amount >= b.amount;
      }

      friend asset operator - ( const asset& a, const asset& b )
      {
         FC_ASSERT( a.symbol == b.symbol );
         return asset( a.amount - b.amount, a.symbol );
      }
      friend asset operator + ( const asset& a, const asset& b )
      {
         FC_ASSERT( a.symbol == b.symbol );
         return asset( a.amount + b.amount, a.symbol );
      }

      friend asset operator * ( const asset& a, const asset& b )
      {
         FC_ASSERT( a.symbol == b.symbol );
         return asset( a.amount * b.amount, a.symbol );
      }
   };

   /** Represents quotation of the relative value of asset against another asset.
       Similar to 'currency pair' used to determine value of currencies.

       For example:
       1 EUR / 1.25 USD where:
       1 EUR is an asset specified as a base
       1.25 USD us an asset specified as a qute

       can determine value of EUR against USD.
   */
   struct price
   {
      /** Even non-single argument, lets make it an explicit one to avoid implicit calls for
          initialization lists.

          \param base  - represents a value of the price object to be expressed relatively to quote
                         asset. Cannot have amount == 0 if you want to build valid price.
          \param quote - represents an relative asset. Cannot have amount == 0, otherwise
                         asertion fail.

        Both base and quote shall have different symbol defined, since it also results in
        creation of invalid price object. \see validate() method.
      */
      explicit price(const asset& base, const asset& quote) : base(base),quote(quote)
      {
          /// Call validate to verify passed arguments. \warning It throws on error.
          validate();
      }

      /** Default constructor is needed because of fc::variant::as method requirements.
      */
      price() = default;

      asset base;
      asset quote;

      static price max(asset_symbol_type base, asset_symbol_type quote );
      static price min(asset_symbol_type base, asset_symbol_type quote );

      price max()const { return price::max( base.symbol, quote.symbol ); }
      price min()const { return price::min( base.symbol, quote.symbol ); }

      bool is_null()const;
      void validate()const;

   }; /// price

   price operator / ( const asset& base, const asset& quote );
   inline price operator~( const price& p ) { return price{p.quote,p.base}; }

   bool  operator <  ( const asset& a, const asset& b );
   bool  operator <= ( const asset& a, const asset& b );
   bool  operator <  ( const price& a, const price& b );
   bool  operator <= ( const price& a, const price& b );
   bool  operator >  ( const price& a, const price& b );
   bool  operator >= ( const price& a, const price& b );
   bool  operator == ( const price& a, const price& b );
   bool  operator != ( const price& a, const price& b );
   asset operator *  ( const asset& a, const price& b );

} } // steem::protocol

namespace fc {
    void to_variant( const steem::protocol::asset& var,  fc::variant& vo );
    void from_variant( const fc::variant& var,  steem::protocol::asset& vo );
}

FC_REFLECT( steem::protocol::asset, (amount)(symbol) )
FC_REFLECT( steem::protocol::price, (base)(quote) )
