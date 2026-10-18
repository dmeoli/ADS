/*--------------------------------------------------------------------------*/
/*------------------------ File LinearPartition.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of LinearPartition [see LinearPartition.h], with their
 * contracts.
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

template< class T >
std::pair< T , std::vector< std::size_t > > linearPartition(
                                   const std::vector< T > & A , std::size_t k )
{
 const std::size_t n = A.size();
 ADS_PRE( ( k >= 1 ) && ( k <= n ) );
 std::vector< T > S( n + 1 , T( 0 ) );
 for( std::size_t i = 0 ; i < n ; ++i )
  S[ i + 1 ] = S[ i ] + A[ i ];
 // D[ t ][ i ]: the first i items in t parts, defined for i >= t
 std::vector< std::vector< T > > D( k + 1 , std::vector< T >( n + 1 ,
                                                              T( 0 ) ) );
 std::vector< std::vector< std::size_t > > cut( k + 1 ,
                                     std::vector< std::size_t >( n + 1 , 0 ) );
 for( std::size_t i = 1 ; i <= n ; ++i )
  D[ 1 ][ i ] = S[ i ];
 for( std::size_t t = 2 ; t <= k ; ++t )
  for( std::size_t i = t ; i <= n ; ++i ) {
   D[ t ][ i ] = std::max( D[ t - 1 ][ t - 1 ] , S[ i ] - S[ t - 1 ] );
   cut[ t ][ i ] = t - 1;
   for( std::size_t j = t ; j < i ; ++j ) {
    const T c = std::max( D[ t - 1 ][ j ] , S[ i ] - S[ j ] );
    if( c < D[ t ][ i ] ) {
     D[ t ][ i ] = c;
     cut[ t ][ i ] = j;
     }
    }
   }
 std::vector< std::size_t > starts( k - 1 );
 for( std::size_t t = k , i = n ; t > 1 ; --t ) {
  i = cut[ t ][ i ];
  starts[ t - 2 ] = i;
  }
 ADS_POST( std::is_sorted( starts.begin() , starts.end() ) &&
           ( starts.empty() || ( ( starts.front() >= 1 ) &&
                                 ( starts.back() < n ) ) ) );
 ADS_AUDIT( [ & ] {
             T worst = S[ starts.empty() ? n : starts.front() ];
             for( std::size_t t = 0 ; t < starts.size() ; ++t ) {
              const std::size_t e = ( t + 1 < starts.size() )
                                    ? starts[ t + 1 ] : n;
              if( e <= starts[ t ] )
               return( false );
              worst = std::max( worst , S[ e ] - S[ starts[ t ] ] );
              }
             return( worst == D[ k ][ n ] );
             }() );
 return( std::pair( D[ k ][ n ] , starts ) );
 }

/*--------------------------------------------------------------------------*/

template< std::integral T >
T painterPartition( const std::vector< T > & A , std::size_t k )
{
 ADS_PRE( k >= 1 );
 ADS_PRE( std::all_of( A.begin() , A.end() ,
                       []( T a ) { return( a >= T( 0 ) ); } ) );
 if( A.empty() )
  return( T( 0 ) );
 T lo = *std::max_element( A.begin() , A.end() ) , hi = T( 0 );
 for( T a : A )
  hi += a;
 auto fits = [ & ]( T x ) {                     // the greedy, at most k parts
  std::size_t parts = 1;
  T sum = T( 0 );
  for( T a : A )
   if( sum + a <= x )
    sum += a;
   else {
    if( ++parts > k )
     return( false );
    sum = a;
    }
  return( true );
  };
 while( lo < hi ) {                             // fits( hi ), lo the least
  const T mid = lo + ( hi - lo ) / 2;
  if( fits( mid ) )
   hi = mid;
  else
   lo = mid + 1;
  }
 return( lo );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File LinearPartition.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
