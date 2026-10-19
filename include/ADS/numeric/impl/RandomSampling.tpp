/*--------------------------------------------------------------------------*/
/*------------------------ File RandomSampling.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of RandomSampling.h, with their contracts.
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

/// a real uniform in ( 0 , 1 ), from any generator
template< class R >
double uniformOpen( R & g )
{
 const double span = double( R::max() - R::min() ) + 1.0;
 return( ( double( g() - R::min() ) + 0.5 ) / span );
 }

/// an integer uniform in [ 0 , k )
template< class R >
std::size_t uniformBelow( R & g , std::size_t k )
{
 return( std::uniform_int_distribution< std::size_t >( 0 , k - 1 )( g ) );
 }

/// P is a sample of n distinct positions below N, sorted
inline bool sampleOf( const std::vector< std::size_t > & P , std::size_t n ,
                      std::size_t N )
{
 return( ( P.size() == n ) &&
         std::adjacent_find( P.begin() , P.end() ,
                             std::greater_equal<>() ) == P.end() &&
         ( P.empty() || ( P.back() < N ) ) );
 }

}  // end( namespace detail )

template< class R >
std::vector< std::size_t > sampleBySwaps( std::size_t n , std::size_t N ,
                                          R & g )
{
 ADS_PRE( n <= N );
 std::vector< std::size_t > S( N );         // the pointers to the items
 std::iota( S.begin() , S.end() , std::size_t( 0 ) );
 for( std::size_t s = 0 ; s < n ; ++s )     // S[ N - s .. N ) are drawn
  std::swap( S[ detail::uniformBelow( g , N - s ) ] , S[ N - 1 - s ] );
 std::vector< std::size_t > out( S.end() - std::ptrdiff_t( n ) , S.end() );
 std::sort( out.begin() , out.end() );
 ADS_AUDIT( detail::sampleOf( out , n , N ) );
 return( out );
 }

template< class R >
std::vector< std::size_t > sampleByDictionary( std::size_t n ,
                                               std::size_t N , R & g )
{
 ADS_PRE( n <= N );
 const bool complement = 2 * n > N;        // draw the positions left out
 const std::size_t m = complement ? N - n : n;
 std::unordered_set< std::size_t > D;
 D.reserve( 2 * m );
 while( D.size() < m ) D.insert( detail::uniformBelow( g , N ) );
 std::vector< std::size_t > out;
 if( complement ) {
  for( std::size_t p = 0 ; p < N ; ++p )
   if( ! D.count( p ) ) out.push_back( p );
  }
 else {
  out.assign( D.begin() , D.end() );
  std::sort( out.begin() , out.end() );
  }
 ADS_AUDIT( detail::sampleOf( out , n , N ) );
 return( out );
 }

template< class R >
std::vector< std::size_t > sampleBySorting( std::size_t n , std::size_t N ,
                                            R & g , std::size_t * rounds )
{
 ADS_PRE( n <= N );
 const bool complement = 2 * n > N;        // draw the positions left out
 const std::size_t m = complement ? N - n : n;
 std::vector< std::size_t > D;              // distinct and sorted
 std::size_t r = 0;
 for( ; D.size() < m ; ++r ) {
  const std::size_t k = m - D.size() , w = ( N + k - 1 ) / k;
  std::vector< std::size_t > X( k ) , count( k + 1 , 0 ) , Y( k );
  for( auto & x : X ) {
   x = detail::uniformBelow( g , N );
   ++count[ x / w + 1 ];
   }
  for( std::size_t b = 1 ; b <= k ; ++b ) count[ b ] += count[ b - 1 ];
  std::vector< std::size_t > start( count.begin() , count.end() );
  for( const auto x : X ) Y[ count[ x / w ]++ ] = x;
  for( std::size_t b = 0 ; b < k ; ++b )   // the buckets, O( 1 ) each
   insertionSort( Y , start[ b ] , start[ b + 1 ] );
  std::vector< std::size_t > Z;
  std::merge( D.begin() , D.end() , Y.begin() , Y.end() ,
              std::back_inserter( Z ) );
  Z.erase( std::unique( Z.begin() , Z.end() ) , Z.end() );
  D.swap( Z );
  }
 if( rounds ) *rounds = r;
 if( complement ) {
  std::vector< std::size_t > out;
  for( std::size_t p = 0 , i = 0 ; p < N ; ++p )
   if( ( i < D.size() ) && ( D[ i ] == p ) ) ++i;
   else out.push_back( p );
  D.swap( out );
  }
 ADS_AUDIT( detail::sampleOf( D , n , N ) );
 return( D );
 }

template< class R >
std::vector< std::size_t > sampleBySkips( std::size_t n , std::size_t N ,
                                          R & g , std::size_t * randoms )
{
 ADS_PRE( n <= N );
 std::vector< std::size_t > out;
 out.reserve( n );
 std::size_t drawn = 0 , cur = 0;           // cur: the next position
 auto U = [ & ]( void ) {
  ++drawn;
  return( detail::uniformOpen( g ) ); };
 auto select = [ & ]( std::size_t S ) {     // skip S, take the next
  cur += S;
  out.push_back( cur++ );
  };
 auto methodA = [ & ]( std::size_t v , std::size_t V ) {
  double top = double( V - v ) , Vr = double( V );
  for( ; v >= 2 ; --v ) {
   const double u = U();
   std::size_t S = 0;
   double quot = top / Vr;
   while( quot > u ) {                      // the gap, sequentially
    ++S;
    top -= 1;
    Vr -= 1;
    quot = quot * top / Vr;
    }
   select( S );
   Vr -= 1;
   }
  if( v == 1 ) select( std::size_t( std::floor( Vr * U() ) ) );
  };
 const double alphainv = 13;
 if( double( n ) * alphainv >= double( N ) ) {
  methodA( n , N );
  if( randoms ) *randoms = drawn;
  ADS_AUDIT( detail::sampleOf( out , n , N ) );
  return( out );
  }
 std::size_t v = n , V = N;                 // to select, still to see
 double vr = double( v ) , Vr = double( V ) , ninv = 1 / vr;
 double Vprime = std::exp( std::log( U() ) * ninv );
 std::size_t qu1 = V - v + 1;
 double qu1r = double( qu1 ) , threshold = alphainv * vr;
 while( ( v > 1 ) && ( threshold < Vr ) ) {
  const double nmin1inv = 1 / ( vr - 1 );
  std::size_t S;
  double X;
  for( ; ; ) {
   for( ; ; ) {                             // D2: X from the envelope
    X = Vr * ( 1 - Vprime );
    S = std::size_t( X );
    if( S < qu1 ) break;
    Vprime = std::exp( std::log( U() ) * ninv );
    }
   const double u = U() , negS = -double( S );
   const double y1 = std::exp( std::log( u * Vr / qu1r ) * nmin1inv );
   Vprime = y1 * ( 1 - X / Vr ) * ( qu1r / ( negS + qu1r ) );
   if( Vprime <= 1 ) break;                 // D3: accepted at once
   double y2 = 1 , top = Vr - 1 , bottom;
   std::size_t limit;
   if( v - 1 > S ) {
    bottom = Vr - vr;
    limit = V - S;
    }
   else {
    bottom = Vr + negS - 1;
    limit = qu1;
    }
   for( std::size_t t = V - 1 ; t >= limit ; --t ) {
    y2 = ( y2 * top ) / bottom;
    top -= 1;
    bottom -= 1;
    }
   if( Vr / ( Vr - X ) >= y1 * std::exp( std::log( y2 ) * nmin1inv ) ) {
    Vprime = std::exp( std::log( U() ) * nmin1inv );
    break;                                  // D4: accepted by the test
    }
   Vprime = std::exp( std::log( U() ) * ninv );
   }
  select( S );                              // D5
  V = V - S - 1;
  Vr = double( V );
  --v;
  vr -= 1;
  ninv = nmin1inv;
  qu1 -= S;
  qu1r = double( qu1 );
  threshold -= alphainv;
  }
 if( v > 1 )
  methodA( v , V );
 else if( v == 1 )
  select( std::size_t( Vr * Vprime ) );
 if( randoms ) *randoms = drawn;
 ADS_AUDIT( detail::sampleOf( out , n , N ) );
 return( out );
 }

template< class T , class R >
std::vector< T > heapSampling( const std::vector< T > & S , std::size_t n ,
                               R & g )
{
 ADS_PRE( n <= S.size() );
 std::vector< T > out;
 if( n == 0 ) return( out );
 struct Keyed {
  double key;
  std::size_t pos;
  };
 auto smaller = []( const Keyed & a , const Keyed & b ) {
  return( a.key < b.key ); };
 DaryHeap< Keyed , decltype( smaller ) > H( smaller );   // the min on top
 for( std::size_t j = 0 ; j < S.size() ; ++j ) {
  const double r = detail::uniformOpen( g );
  if( H.size() < n )
   H.push( Keyed{ r , j } );
  else if( r > H.top().key ) {              // among the n largest keys
   H.pop();
   H.push( Keyed{ r , j } );
   }
  }
 std::vector< std::size_t > pos;
 for( const Keyed & k : H ) pos.push_back( k.pos );
 std::sort( pos.begin() , pos.end() );
 for( const std::size_t p : pos ) out.push_back( S[ p ] );
 return( out );
 }

/*--------------------------------------------------------------------------*/

template< std::input_iterator I , std::sentinel_for< I > S , class R >
std::vector< std::iter_value_t< I > > reservoirSampleL( I first , S last ,
                                                        std::size_t n ,
                                                        R & g )
{
 std::vector< std::iter_value_t< I > > res;
 if( n == 0 )
  return( res );
 res.reserve( n );
 for( ; ( first != last ) && ( res.size() < n ) ; ++first )
  res.push_back( *first );                  // the first n fill it
 if( first == last )
  return( res );
 const double ninv = 1 / double( n );
 double W = std::exp( std::log( detail::uniformOpen( g ) ) * ninv );
 for( ; ; ) {
  // the items skipped before the next one in the reservoir, geometric
  const double skip = std::floor( std::log( detail::uniformOpen( g ) ) /
                                  std::log1p( - W ) );
  if constexpr( std::sized_sentinel_for< S , I > ) {
   if( !( skip < double( last - first ) ) )
    break;
   first += std::iter_difference_t< I >( skip );
   }
  else {
   double k = 0;
   for( ; ( k < skip ) && ( first != last ) ; k += 1 )
    ++first;
   if( first == last )
    break;
   }
  res[ detail::uniformBelow( g , n ) ] = *first;
  ++first;
  W *= std::exp( std::log( detail::uniformOpen( g ) ) * ninv );
  }
 return( res );
 }

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File RandomSampling.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
