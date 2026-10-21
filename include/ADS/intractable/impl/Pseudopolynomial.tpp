/*--------------------------------------------------------------------------*/
/*----------------------- File Pseudopolynomial.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of Pseudopolynomial [see Pseudopolynomial.h], with their
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

/*--------------------------------------------------------------------------*/
/*---------------------------- THE CONTRACTS -------------------------------*/
/*--------------------------------------------------------------------------*/

namespace detail {

/// the pieces P run each program for its time t[ i ] within [ 0 , d ], on
/// its processors, never two pieces of a program at the same time
inline bool preemptive( const std::vector< double > & t , double d ,
                        const std::vector< std::vector< Piece > > & P )
{
 const double eps = 1e-9 * std::max( d , 1.0 );
 std::vector< double > done( t.size() , 0 );
 std::vector< std::vector< std::pair< double , double > > > at( t.size() );
 for( const auto & q : P )
  for( const Piece & p : q ) {
   if( ( p.program < 0 ) || ( std::size_t( p.program ) >= t.size() ) ||
       ( p.start < -eps ) || ( p.end > d + eps ) || ( p.end < p.start ) )
    return( false );
   done[ p.program ] += p.end - p.start;
   at[ p.program ].emplace_back( p.start , p.end );
   }
 for( std::size_t i = 0 ; i < t.size() ; ++i ) {
  if( std::abs( done[ i ] - t[ i ] ) > eps ) return( false );
  std::sort( at[ i ].begin() , at[ i ].end() );
  for( std::size_t j = 1 ; j < at[ i ].size() ; ++j )
   if( at[ i ][ j ].first < at[ i ][ j - 1 ].second - eps ) return( false );
  }
 return( true );
 }

/// a gives each jewel to one of three thieves, a third of the value each
inline bool fairShares( const std::vector< long > & P ,
                        const std::vector< int > & a )
{
 if( a.size() != P.size() ) return( false );
 long s[ 3 ] = { 0 , 0 , 0 };
 for( std::size_t i = 0 ; i < P.size() ; ++i ) {
  if( ( a[ i ] < 0 ) || ( a[ i ] > 2 ) ) return( false );
  s[ a[ i ] ] += P[ i ];
  }
 return( ( s[ 0 ] == s[ 1 ] ) && ( s[ 1 ] == s[ 2 ] ) );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*---------------------------- SUBSET SUM ----------------------------------*/
/*--------------------------------------------------------------------------*/

namespace detail {

/// the bit j of M set to b: set() of the bit vectors of ADS, the
/// references of std and Boost
template< class Bits >
void setBit( Bits & M , std::size_t j , bool b )
{
 if constexpr( requires { M.set( j , b ); } ) M.set( j , b );
 else M[ j ] = b;
 }

/// the bits of M as a std::vector< bool >
template< class Bits >
std::vector< bool > bitsOf( const Bits & M , std::size_t n )
{
 std::vector< bool > r( n );
 for( std::size_t j = 0 ; j < n ; ++j ) r[ j ] = M[ j ];
 return( r );
 }

}  // end( namespace detail )

template< class Bits >
bool subsetSumDP( const std::vector< long > & A , long k ,
                  std::vector< std::vector< bool > > * rows )
{
 ADS_PRE( ( k >= 0 ) && detail::nonnegative( A ) );
 const std::size_t K = std::size_t( k ) + 1;
 Bits M( K );                           // all false
 detail::setBit( M , 0 , true );
 if( rows ) rows->push_back( detail::bitsOf( M , K ) );
 for( long a : A ) {
  for( long j = k ; j >= a ; --j )      // downwards, each a used once
   detail::setBit( M , std::size_t( j ) ,
                   M[ std::size_t( j - a ) ] || M[ std::size_t( j ) ] );
  if( rows ) rows->push_back( detail::bitsOf( M , K ) );
  }
 ADS_POST( ! rows || ( rows->size() == A.size() + 1 ) );
 return( M[ std::size_t( k ) ] );
 }

inline std::optional< Nodes > subsetSumSet( const std::vector< long > & A ,
                                            long k )
{
 ADS_PRE( detail::nonnegative( A ) );
 const auto result = [ & ]( void ) -> std::optional< Nodes > {
  if( k < 0 ) return( std::nullopt );
  const std::size_t n = A.size();
  std::vector< std::vector< bool > > M( n + 1 ,
                                        std::vector< bool >( k + 1 , false ) );
  M[ 0 ][ 0 ] = true;
  for( std::size_t i = 0 ; i < n ; ++i )
   for( long j = 0 ; j <= k ; ++j )
    M[ i + 1 ][ j ] = M[ i ][ j ] ||
                      ( ( j >= A[ i ] ) && M[ i ][ j - A[ i ] ] );
  if( ! M[ n ][ k ] ) return( std::nullopt );
  Nodes S;
  for( std::size_t i = n , j = std::size_t( k ) ; i > 0 ; --i )
   if( ! M[ i - 1 ][ j ] ) {             // A[ i - 1 ] is needed
    S.push_back( int( i - 1 ) );
    j -= std::size_t( A[ i - 1 ] );
    }
  std::reverse( S.begin() , S.end() );
  return( S );
  }();
 ADS_POST( ! result || detail::places( *result , A.size() ) );
 ADS_POST( ! result || ( detail::sumOver( A , *result ) == k ) );
 return( result );
 }

inline std::optional< Nodes > partitionDP( const std::vector< long > & A )
{
 ADS_PRE( detail::nonnegative( A ) );
 const auto result = [ & ]( void ) -> std::optional< Nodes > {
  const long s = std::accumulate( A.begin() , A.end() , 0L );
  if( s % 2 ) return( std::nullopt );
  return( subsetSumSet( A , s / 2 ) );
  }();
 ADS_POST( ! result || ( 2 * detail::sumOver( A , *result ) ==
                          std::accumulate( A.begin() , A.end() , 0L ) ) );
 return( result );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- SCHEDULING ----------------------------------*/
/*--------------------------------------------------------------------------*/

inline std::optional< std::vector< int > >
schedulingDP( const std::vector< long > & t , int m , long d )
{
 ADS_PRE( ( m >= 1 ) && detail::nonnegative( t ) );
 const auto result = [ & ]( void ) -> std::optional< std::vector< int > > {
  if( d < 0 ) return( std::nullopt );          // no load is below 0
  using Loads = std::vector< long >;
  // the loads reached after i programs, each with the loads before the
  // last one and its processor
  using Step = std::map< Loads , std::pair< Loads , int > >;
  std::vector< Step > L( t.size() + 1 );
  L[ 0 ][ Loads( m , 0 ) ] = { Loads() , -1 };
  for( std::size_t i = 0 ; i < t.size() ; ++i )
   for( const auto & [ x , from ] : L[ i ] )
    for( int q = 0 ; q < m ; ++q )
     if( x[ q ] + t[ i ] <= d ) {
      Loads y = x;
      y[ q ] += t[ i ];
      L[ i + 1 ].try_emplace( y , x , q );
      }
  if( L.back().empty() ) return( std::nullopt );
  std::vector< int > a( t.size() );
  Loads x = L.back().begin()->first;
  for( std::size_t i = t.size() ; i > 0 ; --i ) {
   const auto & [ y , q ] = L[ i ].at( x );
   a[ i - 1 ] = q;
   x = y;
   }
  return( a );
  }();
 ADS_POST( ! result || detail::withinLoads( t , m , d , *result ) );
 return( result );
 }

inline std::pair< double , std::vector< std::vector< Piece > > >
mcNaughton( const std::vector< double > & t , int m )
{
 ADS_PRE( ( m >= 1 ) && std::all_of( t.begin() , t.end() , []( double x ) {
  return( x >= 0 ); } ) );
 using Result = std::pair< double , std::vector< std::vector< Piece > > >;
 const auto result = [ & ]( void ) -> Result {
  double d = std::accumulate( t.begin() , t.end() , 0.0 ) / m;
  for( double x : t ) d = std::max( d , x );
  std::vector< std::vector< Piece > > P( m );
  const double eps = 1e-12 * std::max( d , 1.0 );
  int q = 0;
  double now = 0;
  for( std::size_t i = 0 ; i < t.size() ; ++i )
   for( double r = t[ i ] ; ( r > eps ) && ( q < m ) ; ) {
    const double s = std::min( r , d - now );
    P[ q ].push_back( { int( i ) , now , now + s } );
    now += s;
    r -= s;
    if( d - now <= eps ) {               // the processor is full
     ++q;
     now = 0;
     }
    }
  return( std::pair( d , P ) );
  }();
 ADS_POST( detail::preemptive( t , result.first , result.second ) );
 return( result );
 }

inline std::optional< std::vector< std::vector< Piece > > >
realTimeScheduling( const std::vector< double > & C ,
                    const std::vector< double > & T , int m )
{
 ADS_PRE( ( m >= 1 ) && ( C.size() == T.size() ) );
 ADS_PRE( std::all_of( T.begin() , T.end() , []( double x ) {
  return( x > 0 ); } ) );
 ADS_PRE( std::all_of( C.begin() , C.end() , []( double x ) {
  return( x >= 0 ); } ) );
 using Result = std::optional< std::vector< std::vector< Piece > > >;
 const auto result = [ & ]( void ) -> Result {
  std::vector< double > u( C.size() );
  for( std::size_t i = 0 ; i < C.size() ; ++i ) u[ i ] = C[ i ] / T[ i ];
  auto [ d , P ] = mcNaughton( u , m );
  if( d > 1 + 1e-12 ) return( std::nullopt );
  return( P );
  }();
 ADS_POST( ! result || ( int( result->size() ) == m ) );
 return( result );
 }

inline std::optional< std::vector< int > >
bottino( const std::vector< long > & P )
{
 ADS_PRE( detail::nonnegative( P ) );
 const auto result = [ & ]( void ) -> std::optional< std::vector< int > > {
  const long m = std::accumulate( P.begin() , P.end() , 0L );
  if( m % 3 ) return( std::nullopt );
  const long t = m / 3;
  const std::size_t n = P.size();
  using Table = std::vector< std::vector< bool > >;
  const Table empty( t + 1 , std::vector< bool >( t + 1 , false ) );
  std::vector< Table > S( n + 1 , empty );
  S[ 0 ][ 0 ][ 0 ] = true;
  for( std::size_t i = 0 ; i < n ; ++i )
   for( long j = 0 ; j <= t ; ++j )
    for( long h = 0 ; h <= t ; ++h )
     S[ i + 1 ][ j ][ h ] = S[ i ][ j ][ h ] ||
                            ( ( j >= P[ i ] ) && S[ i ][ j - P[ i ] ][ h ] ) ||
                            ( ( h >= P[ i ] ) && S[ i ][ j ][ h - P[ i ] ] );
  if( ! S[ n ][ t ][ t ] ) return( std::nullopt );
  std::vector< int > a( n );
  for( long i = long( n ) , j = t , h = t ; i > 0 ; --i ) {   // backwards
   const long p = P[ i - 1 ];
   if( S[ i - 1 ][ j ][ h ] ) a[ i - 1 ] = 2;
   else if( ( j >= p ) && S[ i - 1 ][ j - p ][ h ] ) {
    a[ i - 1 ] = 0;
    j -= p;
    }
   else {
    a[ i - 1 ] = 1;
    h -= p;
    }
   }
  return( a );
  }();
 ADS_POST( ! result || detail::fairShares( P , *result ) );
 return( result );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File Pseudopolynomial.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
