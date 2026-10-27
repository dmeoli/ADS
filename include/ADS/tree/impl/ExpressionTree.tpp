/*--------------------------------------------------------------------------*/
/*------------------------ File ExpressionTree.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of ExpressionTree.h, with their contracts.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
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
 auto clear = [ & ]( void ) { while( ! S.empty() ) delete S.pop(); };
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
 // a postvisit with a stack of its own, as the height may be n: each node
 // with whether its subtrees are done, whose values are then on V
 using B = BinaryTree< std::string >;
 std::vector< std::pair< const B * , bool > > S = { { t , false } };
 std::vector< double > V;
 while( ! S.empty() ) {
  const auto [ u , done ] = S.back();
  S.pop_back();
  ADS_PRE( u != nullptr );                  // an operator has two children
  const std::string & v = u->read();
  if( ! detail::isBinaryOperator( v ) ) {   // a leaf
   const std::size_t s = ( ( v[ 0 ] == '-' ) || ( v[ 0 ] == '+' ) ) &&
                         ( v.size() > 1 ) ? 1 : 0;  // a sign, then a number
   if( ( ( v[ s ] >= '0' ) && ( v[ s ] <= '9' ) ) || ( v[ s ] == '.' ) )
    V.push_back( std::stod( v ) );
   else {
    const auto it = env.find( v );
    ADS_PRE( it != env.end() );             // a value for the variable
    V.push_back( it->second );
    }
   continue;
   }
  if( ! done ) {                            // the left subtree first
   S.emplace_back( u , true );
   S.emplace_back( u->right() , false );
   S.emplace_back( u->left() , false );
   continue;
   }
  const double b = V.back();
  V.pop_back();
  const double a = V.back();
  V.pop_back();
  switch( v[ 0 ] ) {
   case( '+' ): V.push_back( a + b ); break;
   case( '-' ): V.push_back( a - b ); break;
   case( '*' ): V.push_back( a * b ); break;
   case( '^' ): V.push_back( std::pow( a , b ) ); break;
   default:
    ADS_PRE( b != 0 );
    V.push_back( a / b );
   }
  }
 return( V.back() );
 }

inline std::string prefixForm( const BinaryTree< std::string > * t )
{
 ADS_PRE( t != nullptr );
 // a previsit with a stack of its own, as the height may be n: a node, or
 // nil for the ")" that closes the subtree below it
 using B = BinaryTree< std::string >;
 std::string s;
 std::vector< const B * > S = { t };
 while( ! S.empty() ) {
  const B * u = S.back();
  S.pop_back();
  if( ! u ) {
   s += ")";
   continue;
   }
  s += "(" + u->read();
  S.push_back( nullptr );
  if( u->right() ) S.push_back( u->right() );
  if( u->left() ) S.push_back( u->left() );
  }
 return( s );
 }

inline std::string infixForm( const BinaryTree< std::string > * t )
{
 ADS_PRE( t != nullptr );
 // an invisit with a stack of its own, as the height may be n: each node
 // with the number of its parts written, "( " and the left subtree, the
 // operator and the right subtree, " )"
 using B = BinaryTree< std::string >;
 std::string s;
 std::vector< std::pair< const B * , int > > S = { { t , 0 } };
 while( ! S.empty() ) {
  const B * u = S.back().first;
  const int k = S.back().second++;
  if( k == 0 ) {
   ADS_PRE( u != nullptr );
   if( ! u->left() ) {                      // a leaf
    s += u->read();
    S.pop_back();
    }
   else {
    s += "( ";
    S.emplace_back( u->left() , 0 );
    }
   }
  else if( k == 1 ) {
   s += " " + u->read() + " ";
   S.emplace_back( u->right() , 0 );
   }
  else {
   s += " )";
   S.pop_back();
   }
  }
 return( s );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File ExpressionTree.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
