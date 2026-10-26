/*--------------------------------------------------------------------------*/
/*------------------------- File BinarySearch.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The definitions of the functions of BinarySearch.h [see BinarySearch.h],
 * with their contracts.
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

template< class T , class F >
long binarySearch( const std::vector< T > & A , const T & v , F && trace )
{
 ADS_AUDIT( std::is_sorted( A.begin() , A.end() ) );
 long i = 0 , j = long( A.size() ) - 1;
 while( i <= j ) {                          // the tail recursion of bsRec()
  const long m = i + ( j - i ) / 2;
  trace( i , j , m );
  if( A[ m ] == v )
   return( m );
  if( A[ m ] < v )
   i = m + 1;                               // the right part
  else
   j = m - 1;                               // the left part
  }
 ADS_AUDIT( std::find( A.begin() , A.end() , v ) == A.end() );
 return( -1 );                              // an empty part
 }

template< class T >
long binarySearch( const std::vector< T > & A , const T & v )
{
 return( binarySearch( A , v , []( long , long , long ) {} ) );
 }

namespace detail {

/// the decision tree of A[ i .. j ], nil if i > j
template< class T >
BinaryTree< T > * decision( const std::vector< T > & A , long i , long j )
{
 if( i > j )
  return( nullptr );
 const long m = i + ( j - i ) / 2;          // the middle of binarySearch()
 auto * t = new BinaryTree< T >( A[ m ] );
 if( auto * l = decision( A , i , m - 1 ) )
  t->insertLeft( l );
 if( auto * r = decision( A , m + 1 , j ) )
  t->insertRight( r );
 return( t );
 }

}  // end( namespace detail )

template< class T >
BinaryTree< T > * decisionTree( const std::vector< T > & A )
{
 ADS_AUDIT( std::is_sorted( A.begin() , A.end() ) );
 auto * t = detail::decision( A , 0 , long( A.size() ) - 1 );
 ADS_AUDIT( [ & ]( void ) {
  std::vector< T > v;
  in_visit( t , [ & ]( BinaryTree< T > * u ) { v.push_back( u->read() ); } );
  return( v == A );
  }() );
 return( t );
 }

template< class T , class C >
std::size_t lowerBound( const std::vector< T > & A , const T & v ,
                        std::size_t lo , std::size_t hi , C less )
{
 ADS_PRE( ( lo <= hi ) && ( hi <= A.size() ) );
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  return( std::is_sorted( A.begin() + lo , A.begin() + hi , less ) ); } ) );
 const std::size_t lo0 = lo , hi0 = hi;
 while( lo < hi ) {
  const std::size_t m = lo + ( hi - lo ) / 2;
  if( less( A[ m ] , v ) )
   lo = m + 1;
  else
   hi = m;
  }
 ADS_POST( ( lo >= lo0 ) && ( lo <= hi0 ) &&
           detail::contract( [ & ]( void ) {
            return( ( ( lo == hi0 ) || ! less( A[ lo ] , v ) ) &&
                    ( ( lo == lo0 ) || less( A[ lo - 1 ] , v ) ) ); } ) );
 return( lo );
 }

template< class T , class C >
std::size_t lowerBound( const std::vector< T > & A , const T & v ,
                        C less )
{
 return( lowerBound( A , v , 0 , A.size() , less ) );
 }

template< class T , class C >
std::size_t upperBound( const std::vector< T > & A , const T & v ,
                        C less )
{
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  return( std::is_sorted( A.begin() , A.end() , less ) ); } ) );
 std::size_t lo = 0 , hi = A.size();
 while( lo < hi ) {
  const std::size_t m = lo + ( hi - lo ) / 2;
  if( less( v , A[ m ] ) )
   hi = m;
  else
   lo = m + 1;
  }
 ADS_POST( detail::contract( [ & ]( void ) {
  return( ( ( lo == A.size() ) || less( v , A[ lo ] ) ) &&
          ( ( lo == 0 ) || ! less( v , A[ lo - 1 ] ) ) ); } ) );
 return( lo );
 }

template< class T , class C >
std::size_t gallopSearch( const std::vector< T > & A , const T & v ,
                          C less )
{
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  return( std::is_sorted( A.begin() , A.end() , less ) ); } ) );
 std::size_t lo = 0 , step = 1 , hi = 0;
 while( ( hi < A.size() ) && less( A[ hi ] , v ) ) {
  lo = hi + 1;
  hi += step;
  step *= 2;
  }
 return( lowerBound( A , v , lo , std::min( hi + 1 , A.size() ) , less ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File BinarySearch.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
