/*--------------------------------------------------------------------------*/
/*------------------------- File LZBicriteria.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of LZBicriteria.h, with their contracts.
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

namespace ADS
{

namespace detail
{

/// the class of the distance d >= 1: the bytes of the code of d - 1, less 1
inline unsigned lzbClass( std::uint64_t d )
{
 return( unsigned( vbyteLength( d - 1 ) - 1 ) );
 }

/// the bytes of the code of a phrase
inline std::uint64_t lzbSpace( const LZBicriteriaPhrase & f )
{
 if( f.distance == 0 )
  return( std::uint64_t( vbyteLength( 2 * ( f.length - 1 ) ) ) + f.length );
 return( std::uint64_t( vbyteLength( 2 * ( f.length - 1 ) + 1 ) +
			 vbyteLength( f.distance - 1 ) ) );
 }

/// the time of a phrase in the model m
inline std::uint64_t lzbTime( const LZBicriteriaPhrase & f ,
			      const LZBicriteriaModel & m )
{
 return( m.phrase + m.byte * lzbSpace( f ) +
	  ( f.distance == 0 ? 0 : m.distance[ lzbClass( f.distance ) ] ) );
 }

/// a copy known at a place: the longest copy of its class, and its distance
struct LZBSource
{
 std::uint32_t length , distance;
 };

/// an arc of a path: from, to, and whether it is a run of literals
struct LZBArc
{
 std::uint32_t from , to;
 bool literal;
 };

/// a path from 0 to n with its weights
struct LZBPath
{
 std::vector< LZBArc > arcs;
 std::uint64_t s = 0 , t = 0;
 };

/// the graph of the parses: at each place the copies of the classes of
/// distances where the length grows, the classes increasing with the
/// length, so that the weights of a copy do not decrease with its length
struct LZBGraph
{
 std::size_t n = 0;
 LZBicriteriaModel m;
 std::vector< std::uint64_t > first;  // n + 1 offsets into sources
 std::vector< LZBSource > sources;

 /// the phrase of the copy of l symbols from the place i, from the
 /// closest source that has them
 LZBicriteriaPhrase copy( std::size_t i , std::size_t l ) const {
  std::uint64_t k = first[ i ];
  while( ( k < first[ i + 1 ] ) && ( sources[ k ].length < l ) )
   ++k;
  ADS_INVARIANT( k < first[ i + 1 ] );
  return( LZBicriteriaPhrase{ sources[ k ].distance , l } );
  }

 /// the weights ( space , time ) of the arc from i to j
 std::pair< std::uint64_t , std::uint64_t >
 weights( std::size_t i , std::size_t j , bool literal ) const {
  const LZBicriteriaPhrase f =
    literal ? LZBicriteriaPhrase{ 0 , j - i } : copy( i , j - i );
  return( std::pair( lzbSpace( f ) , lzbTime( f , m ) ) );
  }
 };

/// the graph of the parses of s [see LZBicriteria.h]
inline LZBGraph lzbGraph( std::string_view s , const LZBicriteriaModel & m )
{
 constexpr std::uint32_t none = ~std::uint32_t( 0 );
 constexpr unsigned C = 5;  // the classes of the distances
 LZBGraph G;
 const std::size_t n = G.n = s.size();
 G.m = m;
 G.first.assign( n + 1 , 0 );
 if( n == 0 )
  return( G );
 const auto sa = suffixArraySais( s );
 const auto lcp = lcpArray( s , sa );  // lcp[ r ] of the ranks r , r + 1
 // the longest previous factor of each place and its source (Crochemore
 // and Ilie): a scan of SA with a stack of ranks of increasing places, each
 // with its lcp with the one below it; a place popped by a smaller one has
 // the one below it as its previous smaller value and the popping one as
 // its next smaller value, and the longer of the two lcp is its factor
 std::vector< std::uint32_t > lpf( n , 0 ) , src( n , none );
 {
  struct Item
  {
   std::size_t r , h;
   };
  std::vector< Item > st;
  st.reserve( 1024 );
  for( std::size_t r = 0 ; r <= n ; ++r ) {
   std::size_t h = ( ( r > 0 ) && ( r < n ) ) ? lcp[ r - 1 ] : 0;
   while( ! st.empty() && ( ( r == n ) || ( sa[ st.back().r ] > sa[ r ] ) ) ) {
    const Item top = st.back();
    st.pop_back();
    const std::size_t y = sa[ top.r ];
    if( top.h >= h ) {
     if( top.h > 0 ) {  // then a place is below it
      lpf[ y ] = std::uint32_t( top.h );
      src[ y ] = std::uint32_t( sa[ st.back().r ] );
      }
     }
    else {
     lpf[ y ] = std::uint32_t( h );
     src[ y ] = std::uint32_t( sa[ r ] );
     }
    h = std::min( h , top.h );
    }
   if( r < n )
    st.push_back( Item{ r , st.empty() ? 0 : h } );
   }
  }
 const auto rank = inversePermutation( sa );
 std::array< std::uint32_t , C > L{} , D{};  // of the place before
 G.sources.reserve( n + n / 2 );
 for( std::size_t i = 0 ; i < n ; ++i ) {
  // the copies of i - 1, one symbol shorter, from the same distances
  for( unsigned c = 0 ; c < C ; ++c )
   if( L[ c ] > 1 )
    --L[ c ];
   else
    L[ c ] = D[ c ] = 0;
  // the copy of length h from the distance d, for the classes from that of
  // d up to the first one that has a copy as long
  auto raise = [ & ]( std::size_t h , std::size_t d ) {
   for( unsigned c = lzbClass( d ) ; ( c < C ) && ( L[ c ] < h ) ; ++c ) {
    L[ c ] = std::uint32_t( h );
    D[ c ] = std::uint32_t( d );
    }
   };
  if( lpf[ i ] > 0 )
   raise( lpf[ i ] , i - src[ i ] );
  // the closer sources, by the decreasing lcp around the rank of i
  const std::size_t r = rank[ i ];
  std::size_t a = r , b = r;
  std::size_t hl = r > 0 ? lcp[ r - 1 ] : 0 , hr = r + 1 < n ? lcp[ r ] : 0;
  for( std::size_t k = 0 ; k < m.candidates ; ++k ) {
   const std::size_t h = std::max( hl , hr );
   if( h <= L[ 0 ] )
    break;  // no class can grow
   std::size_t j;
   if( hl >= hr ) {
    j = sa[ --a ];
    hl = a > 0 ? std::min( hl , lcp[ a - 1 ] ) : 0;
    }
   else {
    j = sa[ ++b ];
    hr = b + 1 < n ? std::min( hr , lcp[ b ] ) : 0;
    }
   if( j < i )
    raise( h , i - j );
   }
  // the copies of the classes where the length grows, those of a longer
  // copy from a distance of no greater class dropped
  std::array< LZBSource , C > kept;
  unsigned q = 0;
  for( unsigned c = C ; c-- > 0 ; )
   if( ( L[ c ] > 0 ) && ( ( c == 0 ) || ( L[ c - 1 ] < L[ c ] ) ) &&
       ( ( q == 0 ) ||
	 ( lzbClass( D[ c ] ) < lzbClass( kept[ q - 1 ].distance ) ) ) )
    kept[ q++ ] = LZBSource{ L[ c ] , D[ c ] };
  while( q > 0 )
   G.sources.push_back( kept[ --q ] );
  G.first[ i + 1 ] = G.sources.size();
  }
 return( G );
 }

/// a weight ordered lexicographically, ( a , b )
struct LZBLex
{
 std::int64_t a , b;
 friend LZBLex operator+( LZBLex x , LZBLex y ) {
  return( LZBLex{ x.a + y.a , x.b + y.b } );
  }
 friend LZBLex operator-( LZBLex x , LZBLex y ) {
  return( LZBLex{ x.a - y.a , x.b - y.b } );
  }
 friend LZBLex operator*( LZBLex x , std::int64_t k ) {
  return( LZBLex{ x.a * k , x.b * k } );
  }
 friend bool operator<( LZBLex x , LZBLex y ) {
  return( ( x.a < y.a ) || ( ( x.a == y.a ) && ( x.b < y.b ) ) );
  }
 };

/// the shortest path of G from 0 to n for the weight w( space , time ),
/// linear in both, of which inf is above every path: the copies relaxed
/// forward on the maximal arcs, the runs of literals by the least
/// W[ i ] - B i over the last 64 places i, B the weight of a literal
template< class W , class F >
LZBPath lzbShortest( const LZBGraph & G , F w , W inf )
{
 const std::size_t n = G.n , K = lzBicriteriaMaxLiterals;
 const auto & m = G.m;
 std::vector< W > dist( n + 1 , inf );
 std::vector< std::uint32_t > pred( n + 1 , 0 );
 std::vector< std::uint8_t > lit( n + 1 , 0 );
 // the run of k literals weighs A + B k, its token of one byte
 const W A = w( 1 , m.phrase + m.byte ) , B = w( 1 , m.byte );
 constexpr std::size_t Q = 128;  // a ring of the window, more than K + 1
 std::array< std::uint32_t , Q > ring;
 std::array< W , Q > key;
 std::size_t head = 0 , tail = 0;
 dist[ 0 ] = w( 0 , 0 );
 // the lengths where the token of a copy grows
 constexpr std::uint64_t steps[] = { 64 , 8192 , std::uint64_t( 1 ) << 20 ,
				     std::uint64_t( 1 ) << 27 };
 for( std::size_t j = 0 ; j <= n ; ++j ) {
  if( j > 0 ) {
   const std::size_t i = j - 1;
   const W x = dist[ i ] - B * std::int64_t( i );
   while( ( tail != head ) && ! ( key[ ( tail - 1 ) % Q ] < x ) )
    --tail;
   ring[ tail % Q ] = std::uint32_t( i );
   key[ tail % Q ] = x;
   ++tail;
   while( ring[ head % Q ] + K < j )
    ++head;
   const W v = key[ head % Q ] + A + B * std::int64_t( j );
   if( v < dist[ j ] ) {
    dist[ j ] = v;
    pred[ j ] = ring[ head % Q ];
    lit[ j ] = 1;
    }
   }
  if( j == n )
   break;
  std::size_t prev = 0;
  for( std::uint64_t k = G.first[ j ] ; k < G.first[ j + 1 ] ; ++k ) {
   // the fields named, not bound: a lambda that captures a structured
   // binding is rejected by clang under -fopenmp
   const std::uint32_t l = G.sources[ k ].length;
   const std::uint32_t d = G.sources[ k ].distance;
   auto relax = [ & ]( std::size_t len ) {
    const LZBicriteriaPhrase f{ d , len };
    const W v = dist[ j ] + w( lzbSpace( f ) , lzbTime( f , m ) );
    if( v < dist[ j + len ] ) {
     dist[ j + len ] = v;
     pred[ j + len ] = std::uint32_t( j );
     lit[ j + len ] = 0;
     }
    };
   for( const std::uint64_t b : steps )
    if( ( b > prev ) && ( b < l ) )
     relax( std::size_t( b ) );
   relax( l );
   prev = l;
   }
  }
 LZBPath P;
 for( std::size_t j = n ; j > 0 ; j = pred[ j ] )
  P.arcs.push_back( LZBArc{ pred[ j ] , std::uint32_t( j ) , lit[ j ] != 0 } );
 std::reverse( P.arcs.begin() , P.arcs.end() );
 for( const auto & e : P.arcs ) {
  const auto [ sp , tm ] = G.weights( e.from , e.to , e.literal );
  P.s += sp;
  P.t += tm;
  }
 return( P );
 }

/// the best path swap of P1 and P2 within limit on the resource r( s , t ),
/// least in the cost c( s , t ): the prefix of P1 up to each place v of
/// either path and the suffix of P2 from v, the arcs across v cut there
template< class Cost , class Res >
LZBPath lzbSwap( const LZBGraph & G , const LZBPath & P1 , const LZBPath & P2 ,
		 std::uint64_t limit , Cost cost , Res res )
{
 const auto &A1 = P1.arcs , &A2 = P2.arcs;
 // the weights of the first k arcs of P1 and of the arcs of P2 from k
 std::vector< std::uint64_t > s1( A1.size() + 1 , 0 ) ,
   t1( A1.size() + 1 , 0 ) ,
   s2( A2.size() + 1 , 0 ) , t2( A2.size() + 1 , 0 );
 for( std::size_t k = 0 ; k < A1.size() ; ++k ) {
  const auto [ sp , tm ] =
    G.weights( A1[ k ].from , A1[ k ].to , A1[ k ].literal );
  s1[ k + 1 ] = s1[ k ] + sp;
  t1[ k + 1 ] = t1[ k ] + tm;
  }
 for( std::size_t k = A2.size() ; k-- > 0 ; ) {
  const auto [ sp , tm ] =
    G.weights( A2[ k ].from , A2[ k ].to , A2[ k ].literal );
  s2[ k ] = s2[ k + 1 ] + sp;
  t2[ k ] = t2[ k + 1 ] + tm;
  }
 // the places of the paths in order, P1 at its k1-th arc and P2 at its
 // k2-th: the last node of P1 at most v and the first of P2 at least v
 std::size_t k1 = 0 , k2 = 0 , bestV = 0;
 std::uint64_t bestC = std::numeric_limits< std::uint64_t >::max() , bestR = 0;
 auto evaluate = [ & ]( std::size_t v , std::size_t i1 , std::size_t i2 ,
			std::uint64_t & sp , std::uint64_t & tm ) {
  // i1 arcs of P1 end at most v, P2 from its arc i2 starts at least v
  sp = s1[ i1 ] + s2[ i2 ];
  tm = t1[ i1 ] + t2[ i2 ];
  const std::size_t u = i1 == 0 ? 0 : A1[ i1 - 1 ].to;
  if( u < v ) {
   const auto [ a , b ] = G.weights( u , v , A1[ i1 ].literal );
   sp += a;
   tm += b;
   }
  const std::size_t x = i2 == A2.size() ? G.n : A2[ i2 ].from;
  if( v < x ) {
   const auto [ a , b ] = G.weights( v , x , A2[ i2 - 1 ].literal );
   sp += a;
   tm += b;
   }
  };
 for( std::size_t v = 0 ; ; ) {
  while( ( k1 < A1.size() ) && ( A1[ k1 ].to <= v ) )
   ++k1;
  while( ( k2 < A2.size() ) && ( A2[ k2 ].from < v ) )
   ++k2;
  std::uint64_t sp , tm;
  evaluate( v , k1 , k2 , sp , tm );
  const std::uint64_t c = cost( sp , tm ) , r = res( sp , tm );
  if( ( r <= limit ) &&
      ( ( c < bestC ) || ( ( c == bestC ) && ( r < bestR ) ) ) ) {
   bestC = c;
   bestR = r;
   bestV = v;
   }
  if( v == G.n )
   break;
  // the next place of either path
  std::size_t next = G.n;
  if( k1 < A1.size() )
   next = std::min< std::size_t >( next , A1[ k1 ].to );
  if( k2 < A2.size() )
   next = std::min< std::size_t >( next , A2[ k2 ].from > v ? A2[ k2 ].from
							   : A2[ k2 ].to );
  v = next;
  }
 // the path of the best place
 const std::size_t v = bestV;
 std::size_t i1 = 0 , i2 = 0;
 while( ( i1 < A1.size() ) && ( A1[ i1 ].to <= v ) )
  ++i1;
 while( ( i2 < A2.size() ) && ( A2[ i2 ].from < v ) )
  ++i2;
 LZBPath P;
 P.arcs.assign( A1.begin() , A1.begin() + std::ptrdiff_t( i1 ) );
 const std::size_t u = i1 == 0 ? 0 : A1[ i1 - 1 ].to;
 if( u < v )
  P.arcs.push_back(
    LZBArc{ std::uint32_t( u ) , std::uint32_t( v ) , A1[ i1 ].literal } );
 const std::size_t x = i2 == A2.size() ? G.n : A2[ i2 ].from;
 if( v < x )
  P.arcs.push_back(
    LZBArc{ std::uint32_t( v ) , std::uint32_t( x ) , A2[ i2 - 1 ].literal } );
 P.arcs.insert( P.arcs.end() , A2.begin() + std::ptrdiff_t( i2 ) , A2.end() );
 evaluate( v , i1 , i2 , P.s , P.t );
 return( P );
 }

}  // end( namespace detail )

inline std::uint64_t
lzBicriteriaTime( const std::vector< LZBicriteriaPhrase > & P ,
		  const LZBicriteriaModel & m )
{
 std::uint64_t t = 0;
 for( const auto & f : P )
  t += detail::lzbTime( f , m );
 return( t );
 }

inline LZBicriteriaParse lzBicriteriaParse( std::string_view s ,
					    LZBicriteriaGoal goal ,
					    std::uint64_t bound ,
					    const LZBicriteriaModel & m ,
					    std::uint64_t slack )
{
 using namespace detail;
 ADS_PRE( std::uint64_t( s.size() ) < ( std::uint64_t( 1 ) << 32 ) - 1 );
 const std::size_t n = s.size();
 const std::uint64_t header = std::uint64_t( vbyteLength( n ) );
 const bool onTime = goal == LZBicriteriaGoal::time;
 constexpr double infinity = std::numeric_limits< double >::infinity();
 // the minimized and the bounded weight of a path, and the bound on the
 // phrases (the header is the same for every parse)
 auto cost = [ & ]( std::uint64_t sp , std::uint64_t tm ) {
  return( onTime ? tm : sp );
  };
 auto res = [ & ]( std::uint64_t sp , std::uint64_t tm ) {
  return( onTime ? sp : tm );
  };
 const bool headerFits = ! onTime || ( bound >= header );
 const std::uint64_t R = onTime ? ( headerFits ? bound - header : 0 ) : bound;
 const LZBGraph G = lzbGraph( s , m );
 auto finish = [ & ]( const LZBPath & P , double lb , double lambda ,
		      double gap ,
		      bool feasible ) {
  LZBicriteriaParse out;
  for( const auto & e : P.arcs )
   out.phrases.push_back( e.literal ? LZBicriteriaPhrase{ 0 , e.to - e.from }
				    : G.copy( e.from , e.to - e.from ) );
  out.space = P.s + header;
  out.time = P.t;
  out.lowerBound = lb + ( onTime ? 0.0 : double( header ) );
  out.multiplier = lambda;
  out.gapBound = gap;
  out.feasible = feasible;
  ADS_POST( out.time == lzBicriteriaTime( out.phrases , m ) );
  ADS_AUDIT( lzBicriteriaDecode( lzBicriteriaEncode( s ,
						     out.phrases ) ) == s );
  ADS_AUDIT( lzBicriteriaEncode( s , out.phrases ).size() == out.space );
  return( out );
  };
 const LZBLex top{ std::numeric_limits< std::int64_t >::max() / 4 ,
		   std::numeric_limits< std::int64_t >::max() / 4 };
 const LZBPath piC = lzbShortest(
   G ,
   [ & ]( std::uint64_t sp , std::uint64_t tm ) {
    return( LZBLex{ std::int64_t( cost( sp , tm ) ) ,
		     std::int64_t( res( sp , tm ) ) } );
    } ,
   top );
 const LZBPath piR = lzbShortest(
   G ,
   [ & ]( std::uint64_t sp , std::uint64_t tm ) {
    return( LZBLex{ std::int64_t( res( sp , tm ) ) ,
		     std::int64_t( cost( sp , tm ) ) } );
    } ,
   top );
 if( ! headerFits || ( res( piR.s , piR.t ) > R ) )
  return( finish( piR , infinity , 0 , infinity , false ) );
 if( res( piC.s , piC.t ) <= R )
  return( finish( piC , double( cost( piC.s , piC.t ) ) , 0 , 0 , true ) );
 // the cutting planes on the basis ( piL , piR ), above and within R
 LZBPath L = piC , Rp = piR;
 auto lagrangian = [ & ]( const LZBPath & P , double lambda ) {
  return( double( cost( P.s , P.t ) ) +
	   lambda * ( double( res( P.s , P.t ) ) - double( R ) ) );
  };
 double lambda = 0 , lb = -infinity;
 bool converged = false;
 for( std::size_t it = 0 ; it < m.iterations ; ++it ) {
  lambda = std::max(
    0.0 , ( double( cost( Rp.s , Rp.t ) ) - double( cost( L.s , L.t ) ) ) /
	   ( double( res( L.s , L.t ) ) - double( res( Rp.s , Rp.t ) ) ) );
  const double phiB = lagrangian( L , lambda );
  const LZBPath P = lzbShortest(
    G ,
    [ & ]( std::uint64_t sp , std::uint64_t tm ) {
     return( double( cost( sp , tm ) ) + lambda * double( res( sp , tm ) ) );
     } ,
    infinity );
  const double phi = lagrangian( P , lambda );
  lb = std::max( lb , phi );
  if( phi >= phiB - 1e-9 * ( 1 + std::abs( phiB ) ) ) {
   converged = true;
   break;
   }
  if( res( P.s , P.t ) == R )  // feasible and of cost phi: optimal
   return( finish( P , double( cost( P.s , P.t ) ) , lambda , 0 , true ) );
  if( res( P.s , P.t ) > R )
   L = P;
  else
   Rp = P;
  }
 // the largest weights of an arc: a run of 64 literals, or a copy of the
 // longest length from the farthest distance
 const LZBicriteriaPhrase run{ 0 , lzBicriteriaMaxLiterals } ,
   far{ std::max< std::size_t >( n , 1 ) , std::max< std::size_t >( n , 1 ) };
 const std::uint64_t sMax = std::max( lzbSpace( run ) , lzbSpace( far ) ) ,
		     tMax = std::max( lzbTime( run , m ) ,
				      lzbTime( far , m ) );
 const double cMax = double( onTime ? tMax : sMax ) ,
	      rMax = double( onTime ? sMax : tMax );
 const LZBPath P = lzbSwap( G , L , Rp , R + slack , cost , res );
 const double gap =
   converged ? cMax + lambda * std::max( 0.0 , 2 * rMax - double( slack ) )
	     : infinity;
 return( finish( P , lb , lambda , gap , true ) );
 }

inline std::vector< std::uint8_t >
lzBicriteriaEncode( std::string_view s ,
		    const std::vector< LZBicriteriaPhrase > & P )
{
 std::vector< std::uint8_t > C;
 encodeVByte( C , s.size() );
 std::size_t p = 0;
 for( const auto & f : P ) {
  ADS_PRE( ( f.length > 0 ) && ( f.length <= s.size() - p ) &&
	   ( f.distance <= p ) );
  if( f.distance == 0 ) {
   encodeVByte( C , 2 * ( f.length - 1 ) );
   C.insert( C.end() , s.begin() + std::ptrdiff_t( p ) ,
	     s.begin() + std::ptrdiff_t( p + f.length ) );
   }
  else {
   encodeVByte( C , 2 * ( f.length - 1 ) + 1 );
   encodeVByte( C , f.distance - 1 );
   }
  p += f.length;
  }
 ADS_PRE( p == s.size() );
 ADS_AUDIT( lzBicriteriaDecode( C ) == s );
 return( C );
 }

inline std::size_t lzBicriteriaSize( const std::vector< std::uint8_t > & C )
{
 std::size_t p = 0;
 return( std::size_t( decodeVByte( C , p ) ) );
 }

inline std::size_t lzBicriteriaDecode( const std::vector< std::uint8_t > & C ,
				       std::span< char > out )
{
 std::size_t p = 0;
 const std::size_t n = std::size_t( decodeVByte( C , p ) );
 ADS_PRE( out.size() >= n );
 char * const o0 = out.data();
 char * o = o0;
 char *const oe = o0 + n , *const room = o0 + out.size();
 const std::uint8_t * const c0 = C.data();
 const std::size_t cn = C.size();
 while( o < oe ) {
  const std::uint64_t x = decodeVByte( C , p );
  const std::size_t l = std::size_t( x >> 1 ) + 1;
  ADS_PRE( l <= std::size_t( oe - o ) );
  if( ( x & 1 ) == 0 ) {  // a run of l literals
   ADS_PRE( l <= cn - p );
   if( ( cn - p >= l + 16 ) && ( std::size_t( room - o ) >= l + 16 ) )
    for( std::size_t k = 0 ; k < l ; k += 16 )
     std::memcpy( o + k , c0 + p + k , 16 );
   else
    std::memcpy( o , c0 + p , l );
   p += l;
   }
  else {
   const std::size_t d = std::size_t( decodeVByte( C , p ) ) + 1;
   ADS_PRE( d <= std::size_t( o - o0 ) );
   const char * const from = o - d;
   if( d >= 16 ) {
    // a block of 16 bytes ends at least d >= 16 bytes before it is
    // written, so that the blocks never overlap the bytes they write
    if( std::size_t( room - o ) >= l + 16 )
     for( std::size_t k = 0 ; k < l ; k += 16 )
      std::memcpy( o + k , from + k , 16 );
    else {
     std::size_t k = 0;
     for( ; k + 16 <= l ; k += 16 )
      std::memcpy( o + k , from + k , 16 );
     std::memcpy( o + k , from + k , l - k );  // fewer than d bytes
     }
    }
   else if( d == 1 )
    std::memset( o , *from , l );
   else
    for( std::size_t k = 0 ; k < l ; ++k )  // the overlap included
     o[ k ] = from[ k ];
   }
  o += l;
  }
 ADS_PRE( p == cn );
 return( n );
 }

inline std::string
lzBicriteriaDecode( const std::vector< std::uint8_t > & C )
{
 // 32 bytes of room past the text, so that the last phrases are copied by
 // blocks too
 std::string s( lzBicriteriaSize( C ) + 32 , '\0' );
 const std::size_t n = lzBicriteriaDecode( C , std::span< char >( s ) );
 s.resize( n );
 return( s );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File LZBicriteria.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
