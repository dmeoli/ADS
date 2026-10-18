/*--------------------------------------------------------------------------*/
/*------------------------- File HouseProblems.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of HouseProblems [see HouseProblems.h], with their
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
T maxNonAdjacentSum( const std::vector< T > & A )
{
 T prev = T( 0 ) , cur = T( 0 );                // D[ i - 2 ], D[ i - 1 ]
 for( const T & x : A ) {
  const T next = std::max( cur , prev + x );
  prev = cur;
  cur = next;
  }
 ADS_POST( ! ( cur < T( 0 ) ) );
 return( cur );
 }

/*--------------------------------------------------------------------------*/

template< class T >
std::pair< T , std::vector< std::size_t > > nonAdjacentSelection(
                                                   const std::vector< T > & A )
{
 const std::size_t n = A.size();
 std::vector< T > D( n + 1 , T( 0 ) );          // D[ i ]: the first i items
 for( std::size_t i = 1 ; i <= n ; ++i )
  D[ i ] = std::max( D[ i - 1 ] ,
                     ( i >= 2 ? D[ i - 2 ] : T( 0 ) ) + A[ i - 1 ] );
 std::vector< std::size_t > out;
 for( std::size_t i = n ; i > 0 ; )
  if( D[ i - 1 ] < D[ i ] ) {                   // A[ i - 1 ] taken
   out.push_back( i - 1 );
   i = ( i >= 2 ) ? i - 2 : 0;
   }
  else
   --i;
 std::reverse( out.begin() , out.end() );
 ADS_POST( std::adjacent_find( out.begin() , out.end() ,
            []( std::size_t a , std::size_t b ) { return( b <= a + 1 ); } )
           == out.end() );
 ADS_POST( [ & ] {
            T s = T( 0 );
            for( std::size_t i : out )
             s += A[ i ];
            return( s == D[ n ] );
            }() );
 return( std::pair( D[ n ] , out ) );
 }

/*--------------------------------------------------------------------------*/

template< class T >
std::pair< T , std::vector< std::size_t > > paintHouses(
                                 const std::vector< std::vector< T > > & cost )
{
 const std::size_t n = cost.size();
 if( n == 0 )
  return( std::pair( T( 0 ) , std::vector< std::size_t >() ) );
 const std::size_t k = cost.front().size();
 ADS_PRE( ( k >= 2 ) || ( ( k == 1 ) && ( n == 1 ) ) );
 ADS_PRE( std::all_of( cost.begin() , cost.end() ,
                       [ k ]( const std::vector< T > & r ) {
                        return( r.size() == k ); } ) );
 std::vector< std::vector< T > > D( n , std::vector< T >( k ) );
 D[ 0 ] = cost[ 0 ];
 for( std::size_t h = 1 ; h < n ; ++h ) {
  std::size_t best = 0;                         // the two least of D[ h - 1 ]
  for( std::size_t c = 1 ; c < k ; ++c )
   if( D[ h - 1 ][ c ] < D[ h - 1 ][ best ] )
    best = c;
  std::size_t second = ( best == 0 ) ? 1 : 0;
  for( std::size_t c = 0 ; c < k ; ++c )
   if( ( c != best ) && ( D[ h - 1 ][ c ] < D[ h - 1 ][ second ] ) )
    second = c;
  for( std::size_t c = 0 ; c < k ; ++c )
   D[ h ][ c ] = cost[ h ][ c ] + D[ h - 1 ][ c == best ? second : best ];
  }
 std::vector< std::size_t > color( n );
 color[ n - 1 ] = std::size_t( std::min_element( D[ n - 1 ].begin() ,
                                                D[ n - 1 ].end() )
                               - D[ n - 1 ].begin() );
 for( std::size_t h = n - 1 ; h > 0 ; --h ) {   // a color of h - 1 that fits
  const std::size_t next = color[ h ];
  std::size_t c = ( next == 0 ) ? 1 : 0;
  for( std::size_t p = 0 ; p < k ; ++p )
   if( ( p != next ) && ( D[ h - 1 ][ p ] < D[ h - 1 ][ c ] ) )
    c = p;
  color[ h - 1 ] = c;
  }
 const T total = D[ n - 1 ][ color[ n - 1 ] ];
 ADS_POST( [ & ] {
            T s = T( 0 );
            for( std::size_t h = 0 ; h < n ; ++h ) {
             if( ( h > 0 ) && ( color[ h ] == color[ h - 1 ] ) )
              return( false );
             s += cost[ h ][ color[ h ] ];
             }
            return( s == total );
            }() );
 return( std::pair( total , color ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File HouseProblems.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
