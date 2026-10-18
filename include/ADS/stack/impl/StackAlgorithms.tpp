/*--------------------------------------------------------------------------*/
/*------------------------ File StackAlgorithms.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The algorithms of StackAlgorithms.h, with their contracts.
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

/// the items of s from the top, read by popping them and pushing them
/// back, so that s is left as it was
template< Stack S >
std::vector< typename S::Item > stackItems( S & s )
{
 std::vector< typename S::Item > v;
 while( ! s.isEmpty() ) v.push_back( s.pop() );
 for( auto it = v.rbegin() ; it != v.rend() ; ++it ) s.push( *it );
 return( v );
 }

/// the items of s, counted by popping them and pushing them back
template< Stack S >
long stackSize( S & s )
{
 if( s.isEmpty() ) return( 0 );
 auto x = s.pop();
 const long n = 1 + stackSize( s );
 s.push( x );
 return( n );
 }

/// a pass: the least item rises to the top
template< Stack S >
void risePass( S & s )
{
 if( s.isEmpty() ) return;
 auto a = s.pop();
 if( s.isEmpty() ) {
  s.push( a );
  return;
  }
 auto b = s.pop();
 s.push( a < b ? b : a );
 risePass( s );
 s.push( a < b ? a : b );
 }

/// v from the top is sorted, the least first
template< class T >
bool risingFromTop( const std::vector< T > & v )
{
 for( std::size_t i = 1 ; i < v.size() ; ++i )
  if( v[ i ] < v[ i - 1 ] ) return( false );
 return( true );
 }

}  // end( namespace detail )

inline bool isPalindrome( const std::string & s )
{
 SequenceStack< List< char > > S;
 for( char c : s ) S.push( c );
 for( char c : s )
  if( S.pop() != c ) return( false );
 return( true );
 }

template< Stack S >
void sortStack( S & s )
{
 const long n = detail::stackSize( s );
 for( long k = n ; k > 0 ; --k ) detail::risePass( s );
 ADS_AUDIT( detail::risingFromTop( detail::stackItems( s ) ) &&
            ( detail::stackSize( s ) == n ) );
 }

template< Stack S >
void removeGreater( S & s , const typename S::Item & k )
{
 SequenceStack< List< typename S::Item > > aux;
 while( ! s.isEmpty() ) {
  auto x = s.pop();
  if( !( k < x ) ) aux.push( x );
  }
 while( ! aux.isEmpty() ) s.push( aux.pop() );
 ADS_AUDIT( [ & ] {
  for( const auto & x : detail::stackItems( s ) )
   if( k < x ) return( false );
  return( true ); }() );
 }

namespace detail {

template< Stack S , class F >
void bottomUp( S & s , F & f )
{
 if( s.isEmpty() )
  return;
 auto x = s.pop();
 bottomUp( s , f );
 f( x );
 s.push( x );
 }

}  // end( namespace detail )

template< Stack S , class F >
void forEachBottomUp( S & s , F && f )
{
 std::vector< typename S::Item > before;
 if constexpr( ADS_CONTRACTS >= 3 )
  before = detail::stackItems( s );
 detail::bottomUp( s , f );
 ADS_AUDIT( detail::stackItems( s ) == before );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File StackAlgorithms.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
