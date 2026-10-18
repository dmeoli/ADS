/*--------------------------------------------------------------------------*/
/*----------------------- File OptimalSearchTree.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of OptimalSearchTree [see OptimalSearchTree.h], with their
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

namespace detail {

/// the subtree of the keys i .. j - 1 from the roots R, its root returned
template< class T >
int buildOptimalSearchTree( const std::vector< std::vector< int > > & R ,
                            int i , int j , OptimalSearchTree< T > & B )
{
 if( i >= j )
  return( -1 );
 const int r = R[ i ][ j ];
 B.left[ r ] = buildOptimalSearchTree( R , i , r , B );
 B.right[ r ] = buildOptimalSearchTree( R , r + 1 , j , B );
 return( r );
 }

/// the weighted path length of the subtree of u at depth d, and the keys
/// of its symmetric visit appended to order
template< class T >
T searchTreeCost( const OptimalSearchTree< T > & B ,
                  const std::vector< T > & p , const std::vector< T > & q ,
                  int u , int lo , T d , std::vector< int > & order )
{
 if( u < 0 )                                    // the external node lo
  return( q[ lo ] * d );
 const T a = searchTreeCost( B , p , q , B.left[ u ] , lo , d + T( 1 ) ,
                             order );
 order.push_back( u );
 const T b = searchTreeCost( B , p , q , B.right[ u ] , u + 1 , d + T( 1 ) ,
                             order );
 return( a + b + p[ u ] * ( d + T( 1 ) ) );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

template< class T >
OptimalSearchTree< T > optimalSearchTree( const std::vector< T > & p ,
                                          const std::vector< T > & q )
{
 const int n = static_cast< int >( p.size() );
 ADS_PRE( q.size() == p.size() + 1 );
 ADS_PRE( std::all_of( p.begin() , p.end() ,
                       []( const T & x ) { return( ! ( x < T( 0 ) ) ); } ) &&
          std::all_of( q.begin() , q.end() ,
                       []( const T & x ) { return( ! ( x < T( 0 ) ) ); } ) );
 std::vector< T > P( n + 1 , T( 0 ) ) , Q( n + 2 , T( 0 ) );
 for( int i = 0 ; i < n ; ++i )
  P[ i + 1 ] = P[ i ] + p[ i ];
 for( int i = 0 ; i <= n ; ++i )
  Q[ i + 1 ] = Q[ i ] + q[ i ];
 std::vector< std::vector< T > > C( n + 1 , std::vector< T >( n + 1 ,
                                                              T( 0 ) ) );
 std::vector< std::vector< int > > R( n + 1 , std::vector< int >( n + 1 ,
                                                                  -1 ) );
 for( int h = 1 ; h <= n ; ++h )
  for( int i = 0 ; i + h <= n ; ++i ) {
   const int j = i + h;
   const int lo = ( h == 1 ) ? i : R[ i ][ j - 1 ];
   const int hi = ( h == 1 ) ? i : R[ i + 1 ][ j ];
   int best = lo;
   for( int r = lo + 1 ; r <= hi ; ++r )        // the largest best root
    if( ! ( C[ i ][ best ] + C[ best + 1 ][ j ] <
            C[ i ][ r ] + C[ r + 1 ][ j ] ) )
     best = r;
   R[ i ][ j ] = best;
   C[ i ][ j ] = C[ i ][ best ] + C[ best + 1 ][ j ] + ( P[ j ] - P[ i ] )
                 + ( Q[ j + 1 ] - Q[ i ] );
   }
 OptimalSearchTree< T > B{ C[ 0 ][ n ] , -1 , std::vector< int >( n , -1 ) ,
                           std::vector< int >( n , -1 ) };
 B.root = detail::buildOptimalSearchTree( R , 0 , n , B );
 ADS_AUDIT( [ & ] {
             std::vector< int > order;
             const T c = detail::searchTreeCost( B , p , q , B.root , 0 ,
                                                 T( 0 ) , order );
             for( int i = 0 ; i < n ; ++i )
              if( order[ i ] != i )
               return( false );
             return( ( int( order.size() ) == n ) && ( c == B.cost ) );
             }() );
 return( B );
 }

/*--------------------------------------------------------------------------*/

template< class T >
OptimalSearchTree< T > optimalSearchTree( const std::vector< T > & p )
{
 return( optimalSearchTree( p , std::vector< T >( p.size() + 1 , T( 0 ) ) ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File OptimalSearchTree.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
