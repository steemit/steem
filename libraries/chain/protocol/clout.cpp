

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
 * - Length is between (inclusive) GRAPHENE_MIN_ACCOUNT_NAME_LENGTH and GRAPHENE_MAX_ACCOUNT_NAME_LENGTH
 */
bool is_valid_name( const string& name )
{
#if GRAPHENE_MIN_ACCOUNT_NAME_LENGTH < 3
#error This is_valid_name implementation implicitly enforces minimum name length of 3.
#endif

    const size_t len = name.size();
    if( len < GRAPHENE_MIN_ACCOUNT_NAME_LENGTH )
        return false;

    if( len > GRAPHENE_MAX_ACCOUNT_NAME_LENGTH )
        return false;

    size_t begin = 0;
    while( true )
    {
       size_t end = name.find_first_of( '.', begin );
       if( end == std::string::npos )
          end = len;
       if( end - begin < 3 )
          return false;
       switch( name[begin] )
       {
          case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h':
          case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
          case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x':
          case 'y': case 'z':
             break;
          default:
             return false;
       }
       switch( name[end-1] )
       {
          case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h':
          case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
          case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x':
          case 'y': case 'z':
          case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
          case '8': case '9':
             break;
          default:
             return false;
       }
       for( size_t i=begin+1; i<end-1; i++ )
       {
          switch( name[i] )
          {
             case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h':
             case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
             case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x':
             case 'y': case 'z':
             case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
             case '8': case '9':
             case '-':
                break;
             default:
                return false;
          }
       }
       if( end == len )
          break;
       begin = end+1;
    }
    return true;
}


void account_update_operation::validate()const
{
   FC_ASSERT( account != GRAPHENE_TEMP_ACCOUNT );
   FC_ASSERT( fee.amount >= 0 );
   FC_ASSERT( account != string() );
   FC_ASSERT( owner || active || new_options );
   if( owner )
   {
      FC_ASSERT( owner->num_auths() != 0 );
      FC_ASSERT( !owner->is_impossible(), "cannot update an account with an imposible owner authority threshold" );
   }
   if( active )
   {
      FC_ASSERT( active->num_auths() != 0 );
      FC_ASSERT( !active->is_impossible(), "cannot update an account with an imposible active authority threshold" );
   }

   if( new_options )
      new_options->validate();
}
