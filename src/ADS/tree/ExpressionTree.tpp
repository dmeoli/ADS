/*--------------------------------------------------------------------------*/
/*------------------------ File ExpressionTree.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of ExpressionTree.h, with their contracts.
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

/// true for + - * / ^
inline bool isBinaryOperator( const std::string & t )
{
 return( ( t == "+" ) || ( t == "-" ) || ( t == "*" ) || ( t == "/" ) ||
         ( t == "^" ) );
 }

}  // end( namespace detail )

inline BinaryTree< std::string > * expressionTree( std::string_view e )
{
 std::vector< std::string > T;
 for( std::size_t i = 0 ; i < e.size() ; ) {
  if( e[ i ] == ' ' ) {
   ++i;
   continue;
   }
  std::size_t j = i;
  while( ( j < e.size() ) && ( e[ j ] != ' ' ) )
   ++j;
  T.emplace_back( e.substr( i , j - i ) );
  i = j;
  }
 using B = BinaryTree< std::string >;
 ArrayStack< B * > S( T.size() + 1 );
 auto clear = [ & ]( void ) { while( ! S.isEmpty() ) delete S.pop(); };
 for( const auto & t : T ) {
  auto * u = new B( t );
  if( detail::isBinaryOperator( t ) ) {
   if( S.size() < 2 ) {                     // not well formed
    delete u;
    clear();
    ADS_PRE( false );
    return( nullptr );
    }
   B * r = S.pop() , * l = S.pop();
   u->insertLeft( l );
   u->insertRight( r );
   }
  S.push( u );
  }
 if( S.size() != 1 ) {                       // not well formed
  clear();
  ADS_PRE( false );
  return( nullptr );
  }
 return( S.pop() );
 }

inline double evaluate( const BinaryTree< std::string > * t ,
                        const std::map< std::string , double > & env )
{
 ADS_PRE( t != nullptr );
 const std::string & v = t->read();
 if( ! detail::isBinaryOperator( v ) ) {     // a leaf
  if( ( ( v[ 0 ] >= '0' ) && ( v[ 0 ] <= '9' ) ) || ( v[ 0 ] == '.' ) )
   return( std::stod( v ) );
  const auto it = env.find( v );
  ADS_PRE( it != env.end() );               // a value for the variable
  return( it->second );
  }
 const double a = evaluate( t->left() , env ) ,
              b = evaluate( t->right() , env );
 switch( v[ 0 ] ) {
  case( '+' ): return( a + b );
  case( '-' ): return( a - b );
  case( '*' ): return( a * b );
  case( '^' ): return( std::pow( a , b ) );
  default:
   ADS_PRE( b != 0 );
   return( a / b );
  }
 }

inline std::string prefixForm( const BinaryTree< std::string > * t )
{
 ADS_PRE( t != nullptr );
 std::string s = "(" + t->read();
 if( t->left() ) s += prefixForm( t->left() );
 if( t->right() ) s += prefixForm( t->right() );
 return( s + ")" );
 }

inline std::string infixForm( const BinaryTree< std::string > * t )
{
 ADS_PRE( t != nullptr );
 if( ! t->left() )
  return( t->read() );
 return( "( " + infixForm( t->left() ) + " " + t->read() + " " +
         infixForm( t->right() ) + " )" );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File ExpressionTree.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
