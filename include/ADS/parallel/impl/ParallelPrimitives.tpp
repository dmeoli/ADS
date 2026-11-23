/*--------------------------------------------------------------------------*/
/*----------------------- File ParallelPrimitives.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The parallel primitives on sequences [see ParallelPrimitives.h].
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

#include <algorithm>
#include <atomic>
#include <bit>
#include <iterator>

namespace ADS
{

namespace detail
{

/// the items from which the primitives use more than one thread
inline constexpr std::size_t kPrimitiveSmall = std::size_t( 1 ) << 14;

/// the pack by blocks: the \f$ c \f$ indices \f$ i < n \f$ with `keep( i )`
/// give `emit( i )`, in their order, into the \f$ c \f$ places from the
/// iterator that `room( c )` returns; their number \f$ c \f$
template< class E , class Keep , class Emit , class Room >
std::size_t packInto( E & ex , std::size_t n , Keep && keep , Emit && emit ,
		      Room && room )
{
 const auto cut = blocksOf( 0 , n , 2048 );
 const std::size_t B = cut.first , s = cut.second;
 std::vector< std::size_t > at( B + 1 , 0 );
 parallelFor(
   ex , 0 , B ,
   [ & ]( std::size_t b ) {
    std::size_t c = 0;
    for( std::size_t i = b * s ; i < std::min( n , ( b + 1 ) * s ) ; ++i )
     c += keep( i ) ? 1 : 0;
    at[ b + 1 ] = c;
    } ,
   1 );
 for( std::size_t b = 0 ; b < B ; ++b )
  at[ b + 1 ] += at[ b ];
 auto out = room( at[ B ] );
 parallelFor(
   ex , 0 , B ,
   [ & ]( std::size_t b ) {
    auto o = out + std::ptrdiff_t( at[ b ] );
    for( std::size_t i = b * s ; i < std::min( n , ( b + 1 ) * s ) ; ++i )
     if( keep( i ) )
      *o++ = emit( i );
    } ,
   1 );
 return( at[ B ] );
 }

/// the pack by blocks into a vector of the items `emit( i )` kept
template< class T , class E , class Keep , class Emit >
std::vector< T > packBy( E & ex , std::size_t n , Keep && keep , Emit && emit )
{
 std::vector< T > out;
 packInto( ex , n , keep , emit , [ & ]( std::size_t c ) {
  out.resize( c );
  return( out.begin() );
  } );
 return( out );
 }

/// the stable distribution of \f$ X[ 0 .. n ) \f$ into \f$ Y \f$ by the
/// buckets \f$ \mathit{digit}( i ) < m \f$ of the places \f$ i \f$ of
/// \f$ X \f$, and the \f$ m + 1 \f$ starts of the buckets
template< class E , class T , class D >
std::vector< std::size_t > distribute( E & ex , T * X , T * Y , std::size_t n ,
				       D && digit , std::size_t m )
{
 const auto cut = blocksOf( 0 , n ,
			    std::max< std::size_t >( kGrain , 2 * m ) );
 const std::size_t B = std::max< std::size_t >( cut.first , 1 ) ,
		   s = std::max< std::size_t >( cut.second , 1 );
 // the counts in the order (bucket, block), whose scan gives the offsets
 std::vector< std::size_t > C( m * B + 1 , 0 );
 parallelFor(
   ex , 0 , B ,
   [ & ]( std::size_t b ) {
    std::vector< std::size_t > c( m , 0 );
    for( std::size_t i = b * s ; i < std::min( n , ( b + 1 ) * s ) ; ++i ) {
     const std::size_t k = std::size_t( digit( i ) );
     ADS_PRE( k < m );
     ++c[ k ];
     }
    for( std::size_t k = 0 ; k < m ; ++k )
     C[ k * B + b ] = c[ k ];
    } ,
   1 );
 parallelExclusiveScan( ex , C.begin() , m * B + 1 , C.begin() ,
			std::size_t( 0 ) );
 std::vector< std::size_t > start( m + 1 );
 for( std::size_t k = 0 ; k < m ; ++k )
  start[ k ] = C[ k * B ];
 start[ m ] = n;
 parallelFor(
   ex , 0 , B ,
   [ & ]( std::size_t b ) {
    std::vector< std::size_t > o( m );
    for( std::size_t k = 0 ; k < m ; ++k )
     o[ k ] = C[ k * B + b ];
    for( std::size_t i = b * s ; i < std::min( n , ( b + 1 ) * s ) ; ++i )
     Y[ o[ std::size_t( digit( i ) ) ]++ ] = std::move( X[ i ] );
    } ,
   1 );
 return( start );
 }

/// the moves of \f$ X[ 0 .. n ) \f$ into \f$ Y \f$, in parallel when many
template< class E , class T >
void moveAll( E & ex , T * X , T * Y , std::size_t n )
{
 if( ( n < kPrimitiveSmall ) || ( ex.concurrency() == 1 ) ) {
  std::move( X , X + n , Y );
  return;
  }
 parallelForBlocks( ex , 0 , n , [ & ]( std::size_t lo , std::size_t hi ) {
  std::move( X + lo , X + hi , Y + lo );
  } );
 }

/// the stable sort of \f$ X[ 0 .. n ) \f$ on one thread by the low
/// \f$ b \f$ bits of the keys
template< class T , class K >
void sortLowBits( T * X , std::size_t n , K & key , int b )
{
 if( ( n < 2 ) || ( b == 0 ) )
  return;
 const std::uint64_t mask =
   b >= 64 ? ~std::uint64_t( 0 ) : ( std::uint64_t( 1 ) << b ) - 1;
 auto low = [ & ]( const T & x ) {
  return( std::uint64_t( key( x ) ) & mask );
  };
 if( n <= 32 ) {  // insertion, stable
  for( std::size_t i = 1 ; i < n ; ++i ) {
   T x = std::move( X[ i ] );
   const std::uint64_t k = low( x );
   std::size_t j = i;
   for( ; ( j > 0 ) && ( low( X[ j - 1 ] ) > k ) ; --j )
    X[ j ] = std::move( X[ j - 1 ] );
   X[ j ] = std::move( x );
   }
  return;
  }
 radixSortBy( std::span< T >( X , n ) , low );
 }

/// the radix sort from the most significant digit of \f$ X[ 0 .. n ) \f$
/// by the low \f$ b \f$ bits of the keys, \f$ Y \f$ a buffer of \f$ n \f$
/// items: the result in \f$ X \f$ if `here`, in \f$ Y \f$ otherwise
template< class E , class T , class K >
void msdSort( E & ex , T * X , T * Y , std::size_t n , K & key , int b ,
	      bool here )
{
 if( ( b == 0 ) || ( n < kPrimitiveSmall ) || ( ex.concurrency() == 1 ) ) {
  sortLowBits( X , n , key , b );
  if( ! here )
   moveAll( ex , X , Y , n );
  return;
  }
 const int r = std::min( b , 8 );
 const std::size_t m = std::size_t( 1 ) << r;
 const std::uint64_t mask = m - 1;
 const auto start = distribute(
   ex , X , Y , n ,
   [ & ]( std::size_t i ) {
    return( ( std::uint64_t( key( X[ i ] ) ) >> ( b - r ) ) & mask );
    } ,
   m );
 const std::size_t big = std::max(
   kPrimitiveSmall , n / std::max< std::size_t >( ex.concurrency() , 1 ) );
 Sequential seq;
 parallelFor(
   ex , 0 , m ,
   [ & ]( std::size_t k ) {
    const std::size_t l = start[ k ] , h = start[ k + 1 ];
    if( h - l < big )
     msdSort( seq , Y + l , X + l , h - l , key , b - r , ! here );
    } ,
   1 , m );
 for( std::size_t k = 0 ; k < m ; ++k ) {
  const std::size_t l = start[ k ] , h = start[ k + 1 ];
  if( h - l >= big )
   msdSort( ex , Y + l , X + l , h - l , key , b - r , ! here );
  }
 }

/// a SplitMix64 of the seed \f$ s \f$, the random numbers of a bucket of
/// the shuffle
class SplitMix
{
 public:
 /// the generator of the seed \f$ s \f$
 explicit SplitMix( std::uint64_t s ) : f_x( s ) { }
 /// the next random word
 std::uint64_t operator()( void ) {
  std::uint64_t z = ( f_x += 0x9e3779b97f4a7c15ULL );
  z = ( z ^ ( z >> 30 ) ) * 0xbf58476d1ce4e5b9ULL;
  z = ( z ^ ( z >> 27 ) ) * 0x94d049bb133111ebULL;
  return( z ^ ( z >> 31 ) );
  }

 private:
 std::uint64_t f_x;  ///< the state
 };

/// \f$ \lfloor m x / 2^{64} \rfloor \f$, a random word mapped to
/// \f$ [ 0 , m ) \f$
inline std::size_t scaleTo( std::uint64_t x , std::size_t m )
{
 std::uint64_t hi;
 ( void )wideProduct( x , std::uint64_t( m ) , hi );
 return( std::size_t( hi ) );
 }

/// the starts of the groups of equal keys of a semisorted \f$ A \f$
template< class E , class T , class K >
std::vector< std::size_t > groupStarts( E & ex , const std::vector< T > & A ,
					K && key )
{
 return( packIndex( ex , A.size() , [ & ]( std::size_t i ) {
  return( ( i == 0 ) || ! ( key( A[ i ] ) == key( A[ i - 1 ] ) ) );
  } ) );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

template< Executor E , class F >
auto tabulate( E & ex , std::size_t n , F && f )
  -> std::vector< std::decay_t< std::invoke_result_t< F & , std::size_t > > >
{
 std::vector< std::decay_t< std::invoke_result_t< F & , std::size_t > > > R(
   n );
 parallelForBlocks( ex , 0 , n , [ & ]( std::size_t lo , std::size_t hi ) {
  for( std::size_t i = lo ; i < hi ; ++i )
   R[ i ] = f( i );
  } );
 return( R );
 }

/*--------------------------------------------------------------------------*/

template< Executor E , class T , class F >
std::vector< T > pack( E & ex , const std::vector< T > & A ,
		       const std::vector< F > & flags )
{
 ADS_PRE( flags.size() == A.size() );
 return( detail::packBy< T >(
   ex , A.size() , [ & ]( std::size_t i ) { return( bool( flags[ i ] ) ); } ,
   [ & ]( std::size_t i ) { return( A[ i ] ); } ) );
 }

template< Executor E , class P >
std::vector< std::size_t > packIndex( E & ex , std::size_t n , P && pred )
{
 std::vector< unsigned char > f( n );
 parallelFor( ex , 0 , n ,
	      [ & ]( std::size_t i ) { f[ i ] = pred( i ) ? 1 : 0; } );
 return( detail::packBy< std::size_t >(
   ex , n , [ & ]( std::size_t i ) { return( f[ i ] != 0 ); } ,
   []( std::size_t i ) { return( i ); } ) );
 }

template< Executor E , class T , class P >
std::vector< T > filter( E & ex , const std::vector< T > & A , P && pred )
{
 std::vector< unsigned char > f( A.size() );
 parallelFor( ex , 0 , A.size() ,
	      [ & ]( std::size_t i ) { f[ i ] = pred( A[ i ] ) ? 1 : 0; } );
 return( pack( ex , A , f ) );
 }

/*--------------------------------------------------------------------------*/

template< Executor E , class T >
std::vector< T > flatten( E & ex , const std::vector< std::vector< T > > & L ,
			  std::vector< T > head )
{
 const std::size_t m = L.size() , h = head.size();
 std::vector< std::size_t > at( m + 1 );
 const auto size = [ & ]( std::size_t k ) {
  return( k < m ? L[ k ].size() : 0 );
  };
 if( m < detail::kPrimitiveSmall ) {
  at[ 0 ] = h;
  for( std::size_t k = 0 ; k < m ; ++k )
   at[ k + 1 ] = at[ k ] + size( k );
  }
 else {
  parallelFor( ex , 0 , m + 1 ,
	       [ & ]( std::size_t k ) { at[ k ] = size( k ); } );
  parallelExclusiveScan( ex , at.begin() , m + 1 , at.begin() , h );
  }
 head.resize( at[ m ] );
 auto copy = [ & ]( std::size_t k ) {
  std::copy( L[ k ].begin() , L[ k ].end() ,
	     head.begin() + std::ptrdiff_t( at[ k ] ) );
  };
 // the copies in parallel only when they are many, and a long vector by
 // blocks of its own
 if( ( at[ m ] - h < ( std::size_t( 1 ) << 16 ) ) ||
     ( ex.concurrency() == 1 ) ) {
  for( std::size_t k = 0 ; k < m ; ++k )
   copy( k );
  return( head );
  }
 const std::size_t big = std::size_t( 1 ) << 16;
 parallelFor(
   ex , 0 , m ,
   [ & ]( std::size_t k ) {
    if( L[ k ].size() < big )
     copy( k );
    } ,
   1 , std::min< std::size_t >( m , 1024 ) );
 for( std::size_t k = 0 ; k < m ; ++k )
  if( L[ k ].size() >= big )
   parallelForBlocks(
     ex , 0 , L[ k ].size() , [ & ]( std::size_t lo , std::size_t hi ) {
      std::copy( L[ k ].begin() + std::ptrdiff_t( lo ) ,
		 L[ k ].begin() + std::ptrdiff_t( hi ) ,
		 head.begin() + std::ptrdiff_t( at[ k ] + lo ) );
      } );
 return( head );
 }

/*--------------------------------------------------------------------------*/

template< Executor E , class K >
std::vector< std::size_t > histogram( E & ex , std::size_t n , K && key ,
				      std::size_t m )
{
 std::vector< std::size_t > H( m , 0 );
 if( n == 0 )
  return( H );
 if( 4 * m > n ) {  // few items per key: atomic increments
  parallelFor( ex , 0 , n , [ & ]( std::size_t i ) {
   const std::size_t k = std::size_t( key( i ) );
   ADS_PRE( k < m );
   std::atomic_ref< std::size_t >( H[ k ] ).fetch_add(
     1 , std::memory_order_relaxed );
   } );
  return( H );
  }
 const auto cut = blocksOf( 0 , n , std::max< std::size_t >( kGrain , m ) );
 const std::size_t B = cut.first , s = cut.second;
 std::vector< std::size_t > C( B * m , 0 );
 parallelFor(
   ex , 0 , B ,
   [ & ]( std::size_t b ) {
    std::size_t * c = C.data() + b * m;
    for( std::size_t i = b * s ; i < std::min( n , ( b + 1 ) * s ) ; ++i ) {
     const std::size_t k = std::size_t( key( i ) );
     ADS_PRE( k < m );
     ++c[ k ];
     }
    } ,
   1 );
 parallelFor( ex , 0 , m , [ & ]( std::size_t k ) {
  std::size_t t = 0;
  for( std::size_t b = 0 ; b < B ; ++b )
   t += C[ b * m + k ];
  H[ k ] = t;
  } );
 return( H );
 }

template< Executor E , std::unsigned_integral U >
std::vector< std::size_t > histogram( E & ex , const std::vector< U > & A ,
				      std::size_t m )
{
 return( histogram(
   ex , A.size() ,
   [ & ]( std::size_t i ) { return( std::size_t( A[ i ] ) ); } ,
   m ) );
 }

/*--------------------------------------------------------------------------*/

template< Executor E , class T , class K >
std::vector< std::size_t > countingSort( E & ex , std::vector< T > & A ,
					 K && key , std::size_t m )
{
 const std::size_t n = A.size();
 std::vector< T > X( n );
 auto start = detail::distribute(
   ex , A.data() , X.data() , n ,
   [ & ]( std::size_t i ) { return( std::size_t( key( A[ i ] ) ) ); } , m );
 A.swap( X );
 return( start );
 }

/*--------------------------------------------------------------------------*/

template< Executor E , class T , class K >
void integerSort( E & ex , std::span< T > A , K && key , int b )
{
 ADS_PRE( ( b >= 0 ) && ( b <= 64 ) );
 const std::size_t n = A.size();
 if( n < 2 )
  return;
 if( ( n < detail::kPrimitiveSmall ) || ( ex.concurrency() == 1 ) )
  detail::sortLowBits( A.data() , n , key , b );
 else {
  std::vector< T > Y( n );
  detail::msdSort( ex , A.data() , Y.data() , n , key , b , true );
  }
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  for( std::size_t i = 1 ; i < n ; ++i )
   if( std::uint64_t( key( A[ i ] ) ) < std::uint64_t( key( A[ i - 1 ] ) ) )
    return( false );
  return( true );
  } ) );
 }

template< Executor E , class T , class K >
void integerSort( E & ex , std::vector< T > & A , K && key , int b )
{
 integerSort( ex , std::span< T >( A ) , std::forward< K >( key ) , b );
 }

/*--------------------------------------------------------------------------*/

template< Executor E , class T , class K , class H >
void semisort( E & ex , std::vector< T > & A , K && key , H hash )
{
 const std::size_t n = A.size();
 if( n < 2 )
  return;
 struct Pair
 {
  std::uint64_t h;  // the hash of the key
  std::size_t i;    // the place of the item
  };
 const int t = std::min( 64 , int( std::bit_width( n ) ) + 8 );
 auto top = [ t ]( const Pair & q ) { return( q.h >> ( 64 - t ) ); };
 auto P = tabulate( ex , n , [ & ]( std::size_t i ) {
  return( Pair{ detail::hash64( hash , key( A[ i ] ) ) , i } );
  } );
 integerSort( ex , P , top , t );
 // the places where a key differs from the one before in its run of equal
 // top bits: the runs to regroup, rare
 const auto D = packIndex( ex , n , [ & ]( std::size_t j ) {
  return( ( j > 0 ) && ( top( P[ j ] ) == top( P[ j - 1 ] ) ) &&
	   ! ( key( A[ P[ j ].i ] ) == key( A[ P[ j - 1 ].i ] ) ) );
  } );
 if( ! D.empty() ) {
  std::vector< std::size_t > S;
  for( std::size_t d : D ) {
   std::size_t s = d;
   while( ( s > 0 ) && ( top( P[ s - 1 ] ) == top( P[ d ] ) ) )
    --s;
   if( S.empty() || ( S.back() != s ) )
    S.push_back( s );
   }
  parallelFor(
    ex , 0 , S.size() ,
    [ & ]( std::size_t r ) {
     const std::size_t s = S[ r ];
     std::size_t e = s;
     while( ( e < n ) && ( top( P[ e ] ) == top( P[ s ] ) ) )
      ++e;
     std::vector< Pair > G;
     std::vector< char > done( e - s , 0 );
     for( std::size_t a = s ; a < e ; ++a )
      if( ! done[ a - s ] )
       for( std::size_t c = a ; c < e ; ++c )
	if( ! done[ c - s ] &&
	    ( key( A[ P[ c ].i ] ) == key( A[ P[ a ].i ] ) ) ) {
	 done[ c - s ] = 1;
	 G.push_back( P[ c ] );
	 }
     std::copy( G.begin() , G.end() , P.begin() + std::ptrdiff_t( s ) );
     } ,
    1 );
  }
 std::vector< T > R( n );
 parallelFor(
   ex , 0 , n ,
   [ & ]( std::size_t j ) { R[ j ] = std::move( A[ P[ j ].i ] ); } );
 A.swap( R );
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  // the first item of each group, by hash: two groups of equal keys would
  // meet in a run of equal hashes
  std::vector< std::pair< std::uint64_t , std::size_t > > F;
  for( std::size_t j = 0 ; j < n ; ++j )
   if( ( j == 0 ) || ! ( key( A[ j ] ) == key( A[ j - 1 ] ) ) )
    F.emplace_back( detail::hash64( hash , key( A[ j ] ) ) , j );
  std::sort( F.begin() , F.end() );
  for( std::size_t a = 0 ; a < F.size() ; ++a )
   for( std::size_t c = a + 1 ;
	( c < F.size() ) && ( F[ c ].first == F[ a ].first ) ; ++c )
    if( key( A[ F[ c ].second ] ) == key( A[ F[ a ].second ] ) )
     return( false );
  return( true );
  } ) );
 }

/*--------------------------------------------------------------------------*/

template< Executor E , class K , class V , class H >
std::vector< std::pair< K , std::vector< V > > >
groupByKey( E & ex , const std::vector< std::pair< K , V > > & A , H hash )
{
 auto S = A;
 auto first = []( const std::pair< K , V > & x ) -> const K & {
  return( x.first );
  };
 semisort( ex , S , first , hash );
 const auto at = detail::groupStarts( ex , S , first );
 return( tabulate( ex , at.size() , [ & ]( std::size_t g ) {
  const std::size_t l = at[ g ] ,
		    h = g + 1 < at.size() ? at[ g + 1 ] : S.size();
  std::vector< V > v;
  v.reserve( h - l );
  for( std::size_t i = l ; i < h ; ++i )
   v.push_back( S[ i ].second );
  return( std::pair< K , std::vector< V > >( S[ l ].first , std::move( v ) ) );
  } ) );
 }

template< Executor E , class K , class H >
std::vector< std::pair< K , std::size_t > >
histogramByKey( E & ex , const std::vector< K > & A , H hash )
{
 auto S = A;
 auto self = []( const K & x ) -> const K & { return( x ); };
 semisort( ex , S , self , hash );
 const auto at = detail::groupStarts( ex , S , self );
 return( tabulate( ex , at.size() , [ & ]( std::size_t g ) {
  const std::size_t h = g + 1 < at.size() ? at[ g + 1 ] : S.size();
  return( std::pair< K , std::size_t >( S[ at[ g ] ] , h - at[ g ] ) );
  } ) );
 }

template< Executor E , class K , class H >
std::vector< K > removeDuplicates( E & ex , const std::vector< K > & A ,
				   H hash )
{
 auto S = A;
 auto self = []( const K & x ) -> const K & { return( x ); };
 semisort( ex , S , self , hash );
 const auto at = detail::groupStarts( ex , S , self );
 return( tabulate( ex , at.size() ,
		    [ & ]( std::size_t g ) { return( S[ at[ g ] ] ); } ) );
 }

/*--------------------------------------------------------------------------*/

template< Executor E , class T >
void randomShuffle( E & ex , std::vector< T > & A , std::uint64_t seed )
{
 const std::size_t n = A.size();
 if( n < 2 )
  return;
 const std::size_t m = std::max< std::size_t >( 1 , n / 4096 );
 std::vector< std::size_t > start = { 0 , n };
 if( m > 1 ) {
  std::vector< T > X( n );
  start = detail::distribute(
    ex , A.data() , X.data() , n ,
    [ & ]( std::size_t i ) {
     return( detail::scaleTo( hashWord( std::uint64_t( i ) , seed ) , m ) );
     } ,
    m );
  A.swap( X );
  }
 parallelFor(
   ex , 0 , m ,
   [ & ]( std::size_t c ) {
    detail::SplitMix g(
      hashWord( std::uint64_t( c ) , seed ^ 0x5851f42d4c957f2dULL ) );
    for( std::size_t i = start[ c + 1 ] - 1 ; i > start[ c ] ; --i ) {
     const std::size_t j =
       start[ c ] + detail::scaleTo( g() , i - start[ c ] + 1 );
     std::swap( A[ i ] , A[ j ] );
     }
    } ,
   1 );
 }

template< Executor E >
std::vector< std::size_t > randomPermutation( E & ex , std::size_t n ,
					      std::uint64_t seed )
{
 auto P = tabulate( ex , n , []( std::size_t i ) { return( i ); } );
 randomShuffle( ex , P , seed );
 return( P );
 }

/*--------------------------------------------------------------------------*/

template< Executor E , class P >
std::vector< std::string_view > tokens( E & ex , std::string_view s ,
					P && isSpace )
{
 const std::size_t n = s.size();
 auto space = [ & ]( std::size_t i ) {  // i = n + 1 stands for -1
  return( ( i >= n ) || isSpace( s[ i ] ) );
  };
 const auto L = packIndex( ex , n , [ & ]( std::size_t i ) {
  return( ! space( i ) && ( ( i == 0 ) || space( i - 1 ) ) );
  } );
 const auto R = packIndex( ex , n , [ & ]( std::size_t i ) {
  return( ! space( i ) && space( i + 1 ) );
  } );
 return( tabulate( ex , L.size() , [ & ]( std::size_t j ) {
  return( s.substr( L[ j ] , R[ j ] - L[ j ] + 1 ) );
  } ) );
 }

template< Executor E , class P >
std::vector< std::string_view > split( E & ex , std::string_view s ,
				       P && isSep )
{
 const auto S = packIndex( ex , s.size() , [ & ]( std::size_t i ) {
  return( bool( isSep( s[ i ] ) ) );
  } );
 return( tabulate( ex , S.size() + 1 , [ & ]( std::size_t j ) {
  const std::size_t l = j == 0 ? 0 : S[ j - 1 ] + 1 ,
		    h = j == S.size() ? s.size() : S[ j ];
  return( s.substr( l , h - l ) );
  } ) );
 }

/*--------------------------------------------------------------------------*/

template< Monoid G , Executor E >
typename G::Value reduceOf( E & ex ,
			    const std::vector< typename G::Value > & A )
{
 using V = typename G::Value;
 return( parallelReduce(
   ex , 0 , A.size() , V( G::identity() ) ,
   [ & ]( std::size_t lo , std::size_t hi ) {
    V r = G::identity();
    for( std::size_t i = lo ; i < hi ; ++i )
     r = G::combine( r , A[ i ] );
    return( r );
    } ,
   []( const V & a , const V & b ) { return( V( G::combine( a , b ) ) ); } ) );
 }

template< Monoid G , Executor E >
std::pair< std::vector< typename G::Value > , typename G::Value >
scanOf( E & ex , const std::vector< typename G::Value > & A )
{
 using V = typename G::Value;
 std::vector< V > R( A.size() );
 V t = parallelExclusiveScan(
   ex , A.begin() , A.size() , R.begin() , V( G::identity() ) ,
   []( const V & a , const V & b ) { return( V( G::combine( a , b ) ) ); } );
 return( std::pair< std::vector< V > , V >( std::move( R ) ,
					    std::move( t ) ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File ParallelPrimitives.tpp --------------------*/
/*--------------------------------------------------------------------------*/
