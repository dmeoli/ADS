/*--------------------------------------------------------------------------*/
/*----------------------- File KnapsackVariants.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of KnapsackVariants [see KnapsackVariants.h], with their
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

/// whether the integer n >= 0 is a square, without overflow
inline bool isPerfectSquare( std::int64_t n )
{
 std::int64_t r = 0 , step = std::int64_t( 1 ) << 31;
 for( ; step > 0 ; step >>= 1 )                 // the largest r, r^2 <= n
  if( ( r + step ) <= n / ( r + step ) )
   r += step;
 return( r * r == n );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

template< class T >
std::pair< T , std::vector< std::size_t > > rodCutting(
                               const std::vector< T > & price , std::size_t L )
{
 ADS_PRE( ( L == 0 ) || ( ! price.empty() ) );
 const std::size_t m = price.size();
 std::vector< T > R( L + 1 , T( 0 ) );
 std::vector< std::size_t > cut( L + 1 , 0 );
 for( std::size_t l = 1 ; l <= L ; ++l ) {
  cut[ l ] = 1;
  R[ l ] = price[ 0 ] + R[ l - 1 ];
  for( std::size_t t = 2 ; t <= std::min( l , m ) ; ++t )
   if( R[ l ] < price[ t - 1 ] + R[ l - t ] ) {
    R[ l ] = price[ t - 1 ] + R[ l - t ];
    cut[ l ] = t;
    }
  }
 std::vector< std::size_t > pieces;
 for( std::size_t l = L ; l > 0 ; l -= cut[ l ] )
  pieces.push_back( cut[ l ] );
 ADS_POST( [ & ] {
            std::size_t s = 0;
            T g = T( 0 );
            for( std::size_t t : pieces ) {
             s += t;
             g += price[ t - 1 ];
             }
            return( ( s == L ) && ( g == R[ L ] ) );
            }() );
 return( std::pair( R[ L ] , pieces ) );
 }

/*--------------------------------------------------------------------------*/

template< class T , class V >
std::pair< T , std::vector< std::size_t > > unboundedKnapsack(
                const std::vector< T > & p , const std::vector< V > & v , V C )
{
 ADS_PRE( ( p.size() == v.size() ) && ( C >= V( 0 ) ) );
 ADS_PRE( std::all_of( v.begin() , v.end() ,
                       []( const V & x ) { return( x > V( 0 ) ); } ) );
 const auto cap = static_cast< std::size_t >( C );
 const std::size_t n = p.size();
 std::vector< T > P( cap + 1 , T( 0 ) );
 std::vector< std::size_t > last( cap + 1 , n );   // n: one unit left empty
 for( std::size_t c = 1 ; c <= cap ; ++c ) {
  P[ c ] = P[ c - 1 ];
  for( std::size_t i = 0 ; i < n ; ++i ) {
   const auto vi = static_cast< std::size_t >( v[ i ] );
   if( ( vi <= c ) && ( P[ c ] < P[ c - vi ] + p[ i ] ) ) {
    P[ c ] = P[ c - vi ] + p[ i ];
    last[ c ] = i;
    }
   }
  }
 std::vector< std::size_t > copies( n , 0 );
 for( std::size_t c = cap ; c > 0 ; )
  if( last[ c ] == n )
   --c;
  else {
   ++copies[ last[ c ] ];
   c -= static_cast< std::size_t >( v[ last[ c ] ] );
   }
 ADS_POST( [ & ] {
            V vol = V( 0 );
            T gain = T( 0 );
            for( std::size_t i = 0 ; i < n ; ++i ) {
             vol += V( copies[ i ] ) * v[ i ];
             gain += T( copies[ i ] ) * p[ i ];
             }
            return( ( vol <= C ) && ( gain == P[ cap ] ) );
            }() );
 return( std::pair( P[ cap ] , copies ) );
 }

/*--------------------------------------------------------------------------*/

template< class T , class V >
std::optional< T > minValueExactWeight( const std::vector< V > & w ,
                                        const std::vector< T > & v , V W )
{
 ADS_PRE( ( w.size() == v.size() ) && ( W >= V( 0 ) ) );
 ADS_PRE( std::all_of( w.begin() , w.end() ,
                       []( const V & x ) { return( x > V( 0 ) ); } ) );
 const auto cap = static_cast< std::size_t >( W );
 std::vector< std::optional< T > > D( cap + 1 );
 D[ 0 ] = T( 0 );
 for( std::size_t x = 1 ; x <= cap ; ++x )
  for( std::size_t i = 0 ; i < w.size() ; ++i ) {
   const auto wi = static_cast< std::size_t >( w[ i ] );
   if( ( wi <= x ) && D[ x - wi ] &&
       ( ( ! D[ x ] ) || ( *D[ x - wi ] + v[ i ] < *D[ x ] ) ) )
    D[ x ] = *D[ x - wi ] + v[ i ];
   }
 return( D[ cap ] );
 }

/*--------------------------------------------------------------------------*/

template< class C , class V >
C coinChangeWays( const std::vector< V > & coins , V T )
{
 ADS_PRE( T >= V( 0 ) );
 ADS_PRE( std::all_of( coins.begin() , coins.end() ,
                       []( const V & d ) { return( d > V( 0 ) ); } ) );
 const auto t = static_cast< std::size_t >( T );
 std::vector< C > N( t + 1 , C( 0 ) );
 N[ 0 ] = C( 1 );
 for( const V & coin : coins ) {
  const auto d = static_cast< std::size_t >( coin );
  for( std::size_t x = d ; x <= t ; ++x )
   N[ x ] += N[ x - d ];
  }
 return( N[ t ] );
 }

/*--------------------------------------------------------------------------*/

template< class C , class V >
C subsetSumCount( const std::vector< V > & A , V k )
{
 ADS_PRE( k >= V( 0 ) );
 ADS_PRE( std::all_of( A.begin() , A.end() ,
                       []( const V & a ) { return( a >= V( 0 ) ); } ) );
 const auto K = static_cast< std::size_t >( k );
 std::vector< C > N( K + 1 , C( 0 ) );
 N[ 0 ] = C( 1 );
 for( const V & item : A ) {
  if( item > k )
   continue;
  const auto a = static_cast< std::size_t >( item );
  for( std::size_t x = K + 1 ; x-- > a ; )
   N[ x ] += N[ x - a ];
  }
 return( N[ K ] );
 }

/*--------------------------------------------------------------------------*/

inline int minimumSquares( std::int64_t n )
{
 ADS_PRE( n >= 0 );
 if( n == 0 )
  return( 0 );
 if( detail::isPerfectSquare( n ) )
  return( 1 );
 std::int64_t m = n;
 while( m % 4 == 0 )
  m /= 4;
 if( m % 8 == 7 )
  return( 4 );
 for( std::int64_t t = 1 ; t <= n / t ; ++t )
  if( detail::isPerfectSquare( n - t * t ) )
   return( 2 );
 return( 3 );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File KnapsackVariants.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
