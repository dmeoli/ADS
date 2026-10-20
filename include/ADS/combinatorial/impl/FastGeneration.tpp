/*--------------------------------------------------------------------------*/
/*------------------------ File FastGeneration.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The generators of FastGeneration.h, with their contracts.
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

/// visit( x ), true if it asks to stop; never if it returns void
template< class F , class X >
inline bool visitStops( F & visit , const X & x )
{
 if constexpr( std::is_void_v< std::invoke_result_t< F & , const X & > > ) {
  visit( x );
  return( false );
  }
 else
  return( bool( visit( x ) ) );
 }

}  // end( namespace detail )

template< class T , class F >
long forEachPermutation( std::vector< T > & a , F && visit )
{
 const int n = int( a.size() );
 long count = 0;
 auto V = [ & ]( void ) {
  ++count;
  return( detail::visitStops( visit , std::as_const( a ) ) );
  };
 if( n < 3 ) {                              // 1 or 2 arrangements
  if( V() || ( n < 2 ) ) return( count );
  std::swap( a[ 0 ] , a[ 1 ] );
  V();
  return( count );
  }
 std::vector< int > c( n , 0 );
 for( ; ; ) {                               // the 6 of the first 3 items
  if( V() ) return( count );
  std::swap( a[ 0 ] , a[ 1 ] );
  if( V() ) return( count );
  std::swap( a[ 0 ] , a[ 2 ] );
  if( V() ) return( count );
  std::swap( a[ 0 ] , a[ 1 ] );
  if( V() ) return( count );
  std::swap( a[ 0 ] , a[ 2 ] );
  if( V() ) return( count );
  std::swap( a[ 0 ] , a[ 1 ] );
  if( V() ) return( count );
  int i = 3;                                // the counters of Heap
  while( ( i < n ) && ( c[ i ] >= i ) ) c[ i++ ] = 0;
  if( i >= n ) return( count );
  std::swap( a[ ( i & 1 ) ? c[ i ] : 0 ] , a[ i ] );
  ++c[ i ];
  }
 }

template< class T , class F >
long forEachPermutationLex( std::vector< T > & a , F && visit )
{
 std::sort( a.begin() , a.end() );
 const int n = int( a.size() );
 long count = 0;
 auto V = [ & ]( void ) {
  ++count;
  return( detail::visitStops( visit , std::as_const( a ) ) );
  };
 auto step = [ & ]( int from ) {            // L2-L4 from the place from
  int j = from;
  while( ( j >= 0 ) && ! ( a[ j ] < a[ j + 1 ] ) ) --j;
  if( j < 0 ) {
   std::reverse( a.begin() , a.end() );     // back to sorted
   return( false );
   }
  int l = n - 1;
  while( ! ( a[ j ] < a[ l ] ) ) --l;
  std::swap( a[ j ] , a[ l ] );
  std::reverse( a.begin() + j + 1 , a.end() );
  return( true );
  };
 if( n < 4 ) {                              // Algorithm L alone
  do
   if( V() ) return( count );
  while( step( n - 2 ) );
  return( count );
  }
 T * s = a.data() + ( n - 3 );              // the last 3, increasing
 for( ; ; ) {
  const T x = s[ 0 ] , y = s[ 1 ] , z = s[ 2 ];
  const bool xy = x < y , yz = y < z;
  if( V() ) return( count );                // x y z
  if( xy && yz ) {                          // 6 distinct orders
   s[ 1 ] = z; s[ 2 ] = y;
   if( V() ) return( count );               // x z y
   s[ 0 ] = y; s[ 1 ] = x; s[ 2 ] = z;
   if( V() ) return( count );               // y x z
   s[ 1 ] = z; s[ 2 ] = x;
   if( V() ) return( count );               // y z x
   s[ 0 ] = z; s[ 1 ] = x; s[ 2 ] = y;
   if( V() ) return( count );               // z x y
   s[ 1 ] = y; s[ 2 ] = x;
   if( V() ) return( count );               // z y x
   }
  else if( yz ) {                           // x x z
   s[ 1 ] = z; s[ 2 ] = x;
   if( V() ) return( count );               // x z x
   s[ 0 ] = z; s[ 1 ] = x;
   if( V() ) return( count );               // z x x
   }
  else if( xy ) {                           // x y y
   s[ 0 ] = y; s[ 1 ] = x;
   if( V() ) return( count );               // y x y
   s[ 1 ] = y; s[ 2 ] = x;
   if( V() ) return( count );               // y y x
   }
  if( ! step( n - 4 ) ) return( count );    // the last 3 now decreasing
  }
 }

template< class F >
long forEachCombination( int n , int t , F && visit )
{
 ADS_PRE( ( t >= 0 ) && ( t <= n ) );
 std::vector< int > c( t );
 for( int j = 0 ; j < t ; ++j ) c[ j ] = j;
 long count = 0;
 if( t == 0 ) {
  ++count;
  detail::visitStops( visit , std::as_const( c ) );
  return( count );
  }
 int & last = c[ t - 1 ];
 for( ; ; ) {
  for( ; last < n ; ++last ) {              // the last place alone
   ++count;
   if( detail::visitStops( visit , std::as_const( c ) ) ) return( count );
   }
  int j = t - 2;                            // the last other that can grow
  while( ( j >= 0 ) && ( c[ j ] == n - t + j ) ) --j;
  if( j < 0 ) {
   last = n - 1;
   return( count );
   }
  ++c[ j ];
  for( int k = j + 1 ; k < t ; ++k ) c[ k ] = c[ k - 1 ] + 1;
  }
 }

constexpr Word nextCombination( Word x )
{
 ADS_PRE( x != 0 );
 const Word u = x | ( x - 1 );              // the trailing 0s set
 ADS_PRE( u + 1 != 0 );                     // no overflow
 return( ( u + 1 ) | ( ( ( ~u & ( u + 1 ) ) - 1 ) >>
                       ( std::countr_zero( x ) + 1 ) ) );
 }

template< class F >
long forEachCombinationMask( int n , int t , F && visit )
{
 ADS_PRE( ( t >= 0 ) && ( t <= n ) && ( n <= 64 ) );
 long count = 1;
 if( t == 0 ) {
  detail::visitStops( visit , Word( 0 ) );
  return( count );
  }
 Word x = t == 64 ? ~Word( 0 ) : ( Word( 1 ) << t ) - 1;
 const Word last = x << ( n - t ) , top = Word( 1 ) << ( n - 1 );
 for( ; ; ++count ) {
  if( detail::visitStops( visit , x ) ) return( count );
  Word low = x & ( ~x + 1 );                // the lowest 1 moves up alone
  while( ( low < top ) && ! ( x & ( low << 1 ) ) ) {
   x += low;
   low <<= 1;
   ++count;
   if( detail::visitStops( visit , x ) ) return( count );
   }
  if( x == last ) return( count );
  x = nextCombination( x );
  }
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File FastGeneration.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
