/*--------------------------------------------------------------------------*/
/*-------------------------- File Expressions.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of Expressions.h, with their contracts.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

namespace detail {

/// the tokens of e: numbers, operators and parentheses, the spaces dropped
/// \pre e has no other character
inline std::vector< std::string > tokens( std::string_view e )
{
 std::vector< std::string > T;
 for( std::size_t i = 0 ; i < e.size() ; ) {
  const char c = e[ i ];
  if( c == ' ' )
   ++i;
  else if( ( c >= '0' ) && ( c <= '9' ) ) {
   std::size_t j = i;
   while( ( j < e.size() ) && ( e[ j ] >= '0' ) && ( e[ j ] <= '9' ) )
    ++j;
   T.emplace_back( e.substr( i , j - i ) );
   i = j;
   }
  else {
   ADS_PRE( std::string_view( "+-*/()" ).find( c ) !=
            std::string_view::npos );
   T.emplace_back( 1 , c );
   ++i;
   }
  }
 return( T );
 }

/// true for + - * /
inline bool isOperator( const std::string & t )
{
 return( ( t.size() == 1 ) &&
         ( std::string_view( "+-*/" ).find( t[ 0 ] ) !=
           std::string_view::npos ) );
 }

/// 2 for * and /, 1 for + and -, 0 for the open parenthesis
inline int precedence( const std::string & t )
{
 return( ( t == "*" ) || ( t == "/" ) ? 2 : ( t == "(" ? 0 : 1 ) );
 }

/// s op t fits a long, op one of + - * /, a division by 0 aside
inline bool fitsLong( long s , char op , long t )
{
 constexpr long M = std::numeric_limits< long >::max() ,
                m = std::numeric_limits< long >::min();
 switch( op ) {
  case( '+' ): return( t >= 0 ? s <= M - t : s >= m - t );
  case( '-' ): return( t >= 0 ? s >= m + t : s <= M + t );
  case( '*' ):
   if( ( s == 0 ) || ( t == 0 ) ) return( true );
   if( s > 0 ) return( t > 0 ? s <= M / t : t >= m / s );
   return( t > 0 ? s >= m / t : s >= M / t );
  default: return( ( s != m ) || ( t != -1 ) );
  }
 }

/// the tokens joined by one space
inline std::string join( const std::vector< std::string > & T )
{
 std::string s;
 for( const auto & t : T )
  s += ( s.empty() ? "" : " " ) + t;
 return( s );
 }

}  // end( namespace detail )

inline long evalPostfix( std::string_view e )
{
 const auto T = detail::tokens( e );
 ArrayStack< long > S( T.size() + 1 );
 for( const auto & t : T ) {
  ADS_PRE( ( t != "(" ) && ( t != ")" ) );
  if( ! detail::isOperator( t ) ) {
   S.push( std::stol( t ) );
   continue;
   }
  ADS_PRE( S.size() >= 2 );                // two operands for t
  const long top = S.pop() , suc = S.pop();   // suc t top
  ADS_PRE( detail::fitsLong( suc , t[ 0 ] , top ) );
  switch( t[ 0 ] ) {
   case( '+' ): S.push( suc + top ); break;
   case( '-' ): S.push( suc - top ); break;
   case( '*' ): S.push( suc * top ); break;
   default:
    ADS_PRE( top != 0 );
    S.push( suc / top );
   }
  }
 ADS_PRE( S.size() == 1 );                 // one value left
 return( S.pop() );
 }

inline std::string toPostfixParenthesized( std::string_view e )
{
 const auto T = detail::tokens( e );
 ArrayStack< std::string > S( T.size() + 1 );
 std::vector< std::string > out;
 for( const auto & t : T )
  if( t == "(" )
   continue;
  else if( t == ")" ) {
   ADS_PRE( ! S.empty() );                 // an operator for each pair
   out.push_back( S.pop() );
   }
  else if( detail::isOperator( t ) )
   S.push( t );
  else
   out.push_back( t );
 ADS_PRE( S.empty() );
 return( detail::join( out ) );
 }

inline std::string toPostfix( std::string_view e )
{
 const auto T = detail::tokens( e );
 ArrayStack< std::string > S( T.size() + 1 );
 std::vector< std::string > out;
 for( const auto & t : T )
  if( t == "(" )
   S.push( t );
  else if( t == ")" ) {
   while( ! S.empty() && ( S.top() != "(" ) )
    out.push_back( S.pop() );
   ADS_PRE( ! S.empty() );                 // the open parenthesis
   S.pop();
   }
  else if( detail::isOperator( t ) ) {
   while( ! S.empty() && ( S.top() != "(" ) &&
          ( detail::precedence( t ) <= detail::precedence( S.top() ) ) )
    out.push_back( S.pop() );
   S.push( t );
   }
  else
   out.push_back( t );
 while( ! S.empty() ) {
  ADS_PRE( S.top() != "(" );               // no open parenthesis left
  out.push_back( S.pop() );
  }
 return( detail::join( out ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File Expressions.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
