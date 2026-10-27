/*--------------------------------------------------------------------------*/
/*-------------------------- File SuffixArray.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of SuffixArray.h, with their contracts.
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

namespace detail {

/// the empty place of the arrays of the constructions
inline constexpr std::size_t saEmpty =
                                     std::numeric_limits< std::size_t >::max();

/// every symbol of T is less than K
inline bool symbolsBelow( const Symbols & T , std::size_t K )
{
 return( std::all_of( T.begin() , T.end() ,
                      [ K ]( std::size_t c ) { return( c < K ); } ) );
 }

/// the suffix a is smaller than the suffix b
inline bool suffixLess( const Symbols & T , std::size_t a , std::size_t b )
{
 return( std::lexicographical_compare( T.begin() + std::ptrdiff_t( a ) ,
                                       T.end() ,
                                       T.begin() + std::ptrdiff_t( b ) ,
                                       T.end() ) );
 }

/// SA is the suffix array of T: a permutation whose adjacent suffixes a
/// and b have T[ a ] < T[ b ], or T[ a ] = T[ b ] and the suffix a + 1
/// before the suffix b + 1, O( n )
inline bool isSuffixArray( const Symbols & T ,
                           const std::vector< std::size_t > & SA )
{
 const std::size_t n = T.size();
 if( SA.size() != n ) return( false );
 std::vector< std::size_t > R( n , saEmpty );
 for( std::size_t r = 0 ; r < n ; ++r ) {
  if( ( SA[ r ] >= n ) || ( R[ SA[ r ] ] != saEmpty ) ) return( false );
  R[ SA[ r ] ] = r;
  }
 auto rk = [ & ]( std::size_t i ) { return( i == n ? 0 : R[ i ] + 1 ); };
 for( std::size_t r = 1 ; r < n ; ++r ) {
  const std::size_t a = SA[ r - 1 ] , b = SA[ r ];
  if( T[ a ] != T[ b ] ) {
   if( T[ a ] > T[ b ] ) return( false );
   }
  else
   if( rk( a + 1 ) >= rk( b + 1 ) ) return( false );
  }
 return( true );
 }

/// LCP[ r ] is the lcp of the suffixes SA[ r ] and SA[ r + 1 ]
inline bool isLcpArray( const Symbols & T ,
                        const std::vector< std::size_t > & SA ,
                        const std::vector< std::size_t > & LCP )
{
 const std::size_t n = T.size();
 if( LCP.size() != ( n < 2 ? 0 : n - 1 ) ) return( false );
 for( std::size_t r = 0 ; r + 1 < n ; ++r ) {
  const std::size_t a = SA[ r ] , b = SA[ r + 1 ] , h = LCP[ r ];
  if( ( a + h > n ) || ( b + h > n ) ) return( false );
  if( ! std::equal( T.begin() + std::ptrdiff_t( a ) ,
                    T.begin() + std::ptrdiff_t( a + h ) ,
                    T.begin() + std::ptrdiff_t( b ) ) )
   return( false );
  if( ( a + h < n ) && ( b + h < n ) && ( T[ a + h ] == T[ b + h ] ) )
   return( false );
  }
 return( true );
 }

/// a pass of radix sort: b gets a sorted by the keys r[ a[ i ] ] in
/// [ 0 , K ], stably
inline void skewPass( const std::size_t * a , std::size_t * b ,
                      const std::size_t * r , std::size_t n , std::size_t K )
{
 std::vector< std::size_t > c( K + 1 , 0 );
 for( std::size_t i = 0 ; i < n ; ++i ) ++c[ r[ a[ i ] ] ];
 for( std::size_t i = 0 , sum = 0 ; i <= K ; ++i ) {
  const std::size_t t = c[ i ];
  c[ i ] = sum;
  sum += t;
  }
 for( std::size_t i = 0 ; i < n ; ++i ) b[ c[ r[ a[ i ] ] ]++ ] = a[ i ];
 }

inline bool skewLeq( std::size_t a1 , std::size_t a2 , std::size_t b1 ,
                     std::size_t b2 )
{
 return( ( a1 < b1 ) || ( ( a1 == b1 ) && ( a2 <= b2 ) ) );
 }

inline bool skewLeq( std::size_t a1 , std::size_t a2 , std::size_t a3 ,
                     std::size_t b1 , std::size_t b2 , std::size_t b3 )
{
 return( ( a1 < b1 ) || ( ( a1 == b1 ) && skewLeq( a2 , a3 , b2 , b3 ) ) );
 }

/// the Skew algorithm on T[ 0 .. n - 1 ], symbols in [ 1 , K ] followed by
/// three 0, n >= 2; the sample is the places i mod 3 != 0, the ranks of
/// those i mod 3 = 1 first in R and those i mod 3 = 2 after them
inline void skew( const std::size_t * T , std::size_t * SA , std::size_t n ,
                  std::size_t K )
{
 const std::size_t n0 = ( n + 2 ) / 3 , n1 = ( n + 1 ) / 3 , n2 = n / 3 ,
                   n02 = n0 + n2;
 std::vector< std::size_t > R( n02 + 3 , 0 ) , SA12( n02 + 3 , 0 ) ,
                            R0( n0 ) , SA0( n0 );

 // the triples of the sample, a dummy one at n if n mod 3 = 1
 for( std::size_t i = 0 , j = 0 ; i < n + ( n0 - n1 ) ; ++i )
  if( i % 3 != 0 ) R[ j++ ] = i;
 skewPass( R.data() , SA12.data() , T + 2 , n02 , K );
 skewPass( SA12.data() , R.data() , T + 1 , n02 , K );
 skewPass( R.data() , SA12.data() , T , n02 , K );

 // the lexicographic names of the triples
 std::size_t name = 0 , c0 = saEmpty , c1 = saEmpty , c2 = saEmpty;
 for( std::size_t i = 0 ; i < n02 ; ++i ) {
  const std::size_t s = SA12[ i ];
  if( ( T[ s ] != c0 ) || ( T[ s + 1 ] != c1 ) || ( T[ s + 2 ] != c2 ) ) {
   ++name;
   c0 = T[ s ];
   c1 = T[ s + 1 ];
   c2 = T[ s + 2 ];
   }
  if( s % 3 == 1 ) R[ s / 3 ] = name;
  else R[ s / 3 + n0 ] = name;
  }

 // the sample sorted by recursion, unless all the names differ
 if( name < n02 ) {
  skew( R.data() , SA12.data() , n02 , name );
  for( std::size_t i = 0 ; i < n02 ; ++i ) R[ SA12[ i ] ] = i + 1;
  }
 else
  for( std::size_t i = 0 ; i < n02 ; ++i ) SA12[ R[ i ] - 1 ] = i;

 // the places i mod 3 = 0, by ( T[ i ] , rank of i + 1 )
 for( std::size_t i = 0 , j = 0 ; i < n02 ; ++i )
  if( SA12[ i ] < n0 ) R0[ j++ ] = 3 * SA12[ i ];
 skewPass( R0.data() , SA0.data() , T , n0 , K );

 // the merge, by a pair or a triple ending with a rank of the sample
 auto place = [ & ]( std::size_t t ) {
  return( SA12[ t ] < n0 ? SA12[ t ] * 3 + 1 : ( SA12[ t ] - n0 ) * 3 + 2 );
  };
 for( std::size_t p = 0 , t = n0 - n1 , k = 0 ; k < n ; ++k ) {
  const std::size_t i = place( t ) , j = SA0[ p ];
  const bool sampleFirst = SA12[ t ] < n0 ?
   skewLeq( T[ i ] , R[ SA12[ t ] + n0 ] , T[ j ] , R[ j / 3 ] ) :
   skewLeq( T[ i ] , T[ i + 1 ] , R[ SA12[ t ] - n0 + 1 ] ,
            T[ j ] , T[ j + 1 ] , R[ j / 3 + n0 ] );
  if( sampleFirst ) {
   SA[ k ] = i;
   if( ++t == n02 )
    for( ++k ; p < n0 ; ++p , ++k ) SA[ k ] = SA0[ p ];
   }
  else {
   SA[ k ] = j;
   if( ++p == n0 )
    for( ++k ; t < n02 ; ++t , ++k ) SA[ k ] = place( t );
   }
  }
 }

/// SA-IS on s, whose last symbol is a 0 that occurs nowhere else, symbols
/// in [ 0 , K )
inline void sais( const Symbols & s , std::size_t K ,
                  std::vector< std::size_t > & SA )
{
 const std::size_t n = s.size();
 SA.assign( n , saEmpty );
 if( n == 1 ) {
  SA[ 0 ] = 0;
  return;
  }

 // the types: S if smaller than the next suffix, L if larger
 std::vector< bool > S( n );
 S[ n - 1 ] = true;
 for( std::size_t i = n - 1 ; i-- > 0 ; )
  S[ i ] = ( s[ i ] < s[ i + 1 ] ) ||
           ( ( s[ i ] == s[ i + 1 ] ) && S[ i + 1 ] );
 auto lms = [ & ]( std::size_t i ) {
  return( ( i > 0 ) && S[ i ] && ! S[ i - 1 ] );
  };

 // the buckets of the first symbols
 std::vector< std::size_t > count( K , 0 ) , head( K ) , tail( K );
 for( std::size_t c : s ) ++count[ c ];
 auto buckets = [ & ]( void ) {
  for( std::size_t c = 0 , sum = 0 ; c < K ; ++c ) {
   head[ c ] = sum;
   sum += count[ c ];
   tail[ c ] = sum;
   }
  };

 // the LMS-suffixes at the ends of their buckets, in the order of L, then
 // the L-suffixes from left to right and the S-suffixes from right to left
 auto induce = [ & ]( const std::vector< std::size_t > & L ) {
  std::fill( SA.begin() , SA.end() , saEmpty );
  buckets();
  for( std::size_t k = L.size() ; k-- > 0 ; )
   SA[ --tail[ s[ L[ k ] ] ] ] = L[ k ];
  buckets();
  for( std::size_t i = 0 ; i < n ; ++i )
   if( ( SA[ i ] != saEmpty ) && ( SA[ i ] > 0 ) && ! S[ SA[ i ] - 1 ] ) {
    const std::size_t v = SA[ i ] - 1;
    SA[ head[ s[ v ] ]++ ] = v;
    }
  buckets();
  for( std::size_t i = n ; i-- > 0 ; )
   if( ( SA[ i ] != saEmpty ) && ( SA[ i ] > 0 ) && S[ SA[ i ] - 1 ] ) {
    const std::size_t v = SA[ i ] - 1;
    SA[ --tail[ s[ v ] ] ] = v;
    }
  };

 // the LMS-substrings sorted by one induction
 std::vector< std::size_t > L1;
 for( std::size_t i = 1 ; i < n ; ++i )
  if( lms( i ) ) L1.push_back( i );
 induce( L1 );

 // their names, equal for equal LMS-substrings
 auto same = [ & ]( std::size_t a , std::size_t b ) {
  if( ( a == n - 1 ) || ( b == n - 1 ) ) return( a == b );
  for( std::size_t k = 0 ; ; ++k ) {
   if( ( s[ a + k ] != s[ b + k ] ) || ( S[ a + k ] != S[ b + k ] ) )
    return( false );
   if( ( k > 0 ) && ( lms( a + k ) || lms( b + k ) ) )
    return( lms( a + k ) && lms( b + k ) );
   }
  };
 std::vector< std::size_t > name( n , saEmpty );
 std::size_t names = 0 , prev = saEmpty;
 for( std::size_t i = 0 ; i < n ; ++i )
  if( lms( SA[ i ] ) ) {
   if( ( prev != saEmpty ) && ! same( prev , SA[ i ] ) ) ++names;
   name[ SA[ i ] ] = names;
   prev = SA[ i ];
   }
 ++names;

 // the LMS-suffixes sorted by the suffix array of the names
 Symbols s1;
 s1.reserve( L1.size() );
 for( std::size_t i : L1 ) s1.push_back( name[ i ] );
 std::vector< std::size_t > SA1( s1.size() );
 if( names < s1.size() )
  sais( s1 , names , SA1 );
 else
  for( std::size_t k = 0 ; k < s1.size() ; ++k ) SA1[ s1[ k ] ] = k;
 std::vector< std::size_t > L2( L1.size() );
 for( std::size_t k = 0 ; k < L1.size() ; ++k ) L2[ k ] = L1[ SA1[ k ] ];
 induce( L2 );
 }

}  // end( namespace detail )

inline Symbols toSymbols( std::string_view T )
{
 Symbols S( T.size() );
 for( std::size_t i = 0 ; i < T.size() ; ++i )
  S[ i ] = static_cast< unsigned char >( T[ i ] );
 return( S );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- CONSTRUCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

inline std::vector< std::size_t > suffixArraySorting( const Symbols & T )
{
 std::vector< std::size_t > SA( T.size() );
 std::iota( SA.begin() , SA.end() , std::size_t( 0 ) );
 std::sort( SA.begin() , SA.end() , [ & ]( std::size_t a , std::size_t b ) {
  return( detail::suffixLess( T , a , b ) ); } );
 ADS_AUDIT( detail::isSuffixArray( T , SA ) );
 return( SA );
 }

inline std::vector< std::size_t > suffixArraySorting( std::string_view T )
{
 return( suffixArraySorting( toSymbols( T ) ) );
 }

inline std::vector< std::size_t > suffixArrayDoubling( const Symbols & T )
{
 const std::size_t n = T.size();
 ADS_PRE( std::uint64_t( n ) < ( std::uint64_t( 1 ) << 32 ) );
 std::vector< std::size_t > SA( n ) , R( n ) , tmp( n );
 if( n == 0 ) return( SA );
 std::iota( SA.begin() , SA.end() , std::size_t( 0 ) );
 radixSortBy( SA , [ & ]( std::size_t i ) {
  return( std::uint64_t( T[ i ] ) ); } );
 R[ SA[ 0 ] ] = 0;
 for( std::size_t r = 1 ; r < n ; ++r )
  R[ SA[ r ] ] = R[ SA[ r - 1 ] ] + ( T[ SA[ r ] ] != T[ SA[ r - 1 ] ] );
 for( std::size_t k = 1 ; ( R[ SA[ n - 1 ] ] < n - 1 ) && ( k < n ) ;
      k *= 2 ) {
  // the rank of the prefix of 2 k symbols, as the pair of two of k
  auto key = [ & ]( std::size_t i ) {
   return( ( std::uint64_t( R[ i ] ) << 32 ) |
           std::uint64_t( i + k < n ? R[ i + k ] + 1 : 0 ) );
   };
  radixSortBy( SA , key );
  tmp[ SA[ 0 ] ] = 0;
  for( std::size_t r = 1 ; r < n ; ++r )
   tmp[ SA[ r ] ] = tmp[ SA[ r - 1 ] ] +
                    ( key( SA[ r ] ) != key( SA[ r - 1 ] ) );
  R.swap( tmp );
  }
 ADS_AUDIT( detail::isSuffixArray( T , SA ) );
 return( SA );
 }

inline std::vector< std::size_t > suffixArrayDoubling( std::string_view T )
{
 return( suffixArrayDoubling( toSymbols( T ) ) );
 }

inline std::vector< std::size_t > suffixArrayLarssonSadakane(
                                                          const Symbols & T )
{
 const std::size_t n = T.size();
 std::vector< std::size_t > I( n ) , V( n );
 if( n == 0 ) return( I );
 std::iota( I.begin() , I.end() , std::size_t( 0 ) );
 std::stable_sort( I.begin() , I.end() , [ & ]( std::size_t a ,
                                                std::size_t b ) {
  return( T[ a ] < T[ b ] ); } );

 // len[ i ] at the first place of a group is its size if it is unsorted,
 // and minus the length of the run of sorted groups that starts there
 std::vector< long > len( n , 0 );
 auto combine = [ & ]( void ) {
  for( std::size_t i = 0 ; i < n ; )
   if( len[ i ] < 0 ) {
    const std::size_t s = i;
    long run = 0;
    while( ( i < n ) && ( len[ i ] < 0 ) ) {
     run -= len[ i ];
     i += std::size_t( -len[ i ] );
     }
    len[ s ] = -run;
    }
   else
    i += std::size_t( len[ i ] );
  };

 // the groups of h = 1, V the last place of the group of each suffix
 for( std::size_t i = 0 ; i < n ; ) {
  std::size_t j = i;
  while( ( j + 1 < n ) && ( T[ I[ j + 1 ] ] == T[ I[ i ] ] ) ) ++j;
  for( std::size_t k = i ; k <= j ; ++k ) V[ I[ k ] ] = j;
  len[ i ] = j == i ? -1 : long( j - i + 1 );
  i = j + 1;
  }
 combine();

 std::vector< std::pair< long , std::size_t > > G;
 for( std::size_t h = 1 ; len[ 0 ] != -long( n ) ; h *= 2 ) {
  for( std::size_t i = 0 ; i < n ; ) {
   if( len[ i ] < 0 ) {
    i += std::size_t( -len[ i ] );
    continue;
    }
   // the group I[ i .. i + g - 1 ] sorted by the group h places ahead
   const std::size_t g = std::size_t( len[ i ] );
   G.clear();
   for( std::size_t k = i ; k < i + g ; ++k )
    G.emplace_back( I[ k ] + h < n ? long( V[ I[ k ] + h ] ) : -1 , I[ k ] );
   std::sort( G.begin() , G.end() );
   for( std::size_t a = 0 ; a < g ; ) {
    std::size_t b = a;
    while( ( b + 1 < g ) && ( G[ b + 1 ].first == G[ a ].first ) ) ++b;
    for( std::size_t k = a ; k <= b ; ++k ) {
     I[ i + k ] = G[ k ].second;
     V[ G[ k ].second ] = i + b;
     }
    len[ i + a ] = a == b ? -1 : long( b - a + 1 );
    a = b + 1;
    }
   i += g;
   }
  combine();
  }
 ADS_AUDIT( detail::isSuffixArray( T , I ) );
 return( I );
 }

inline std::vector< std::size_t > suffixArrayLarssonSadakane(
                                                         std::string_view T )
{
 return( suffixArrayLarssonSadakane( toSymbols( T ) ) );
 }

inline std::vector< std::size_t > suffixArraySkew( const Symbols & T ,
                                                   std::size_t K )
{
 ADS_PRE( detail::symbolsBelow( T , K ) );
 const std::size_t n = T.size();
 std::vector< std::size_t > SA( n );
 if( n < 2 ) {
  std::iota( SA.begin() , SA.end() , std::size_t( 0 ) );
  return( SA );
  }
 Symbols s( n + 3 , 0 );
 for( std::size_t i = 0 ; i < n ; ++i ) s[ i ] = T[ i ] + 1;
 detail::skew( s.data() , SA.data() , n , K );
 ADS_AUDIT( detail::isSuffixArray( T , SA ) );
 return( SA );
 }

inline std::vector< std::size_t > suffixArraySkew( std::string_view T )
{
 return( suffixArraySkew( toSymbols( T ) , 256 ) );
 }

inline std::vector< std::size_t > suffixArraySais( const Symbols & T ,
                                                   std::size_t K )
{
 ADS_PRE( detail::symbolsBelow( T , K ) );
 const std::size_t n = T.size();
 Symbols s( n + 1 , 0 );
 for( std::size_t i = 0 ; i < n ; ++i ) s[ i ] = T[ i ] + 1;
 std::vector< std::size_t > SA;
 detail::sais( s , K + 1 , SA );
 SA.erase( SA.begin() );         // the terminator, the smallest suffix
 ADS_AUDIT( detail::isSuffixArray( T , SA ) );
 return( SA );
 }

inline std::vector< std::size_t > suffixArraySais( std::string_view T )
{
 return( suffixArraySais( toSymbols( T ) , 256 ) );
 }

inline std::vector< std::size_t > suffixArrayScan( const Symbols & T ,
                                                   std::size_t m )
{
 ADS_PRE( m > 0 );
 const std::size_t n = T.size();
 auto less = [ & ]( std::size_t a , std::size_t b ) {
  return( detail::suffixLess( T , a , b ) );
  };
 std::vector< std::size_t > SA , B , C , out;
 for( std::size_t start = 0 ; start < n ; start += m ) {
  // the suffixes of the block, sorted
  const std::size_t end = std::min( n , start + m );
  B.resize( end - start );
  std::iota( B.begin() , B.end() , start );
  std::sort( B.begin() , B.end() , less );

  // C[ j ]: the earlier suffixes between B[ j - 1 ] and B[ j ]
  C.assign( B.size() + 1 , 0 );
  for( std::size_t j = 0 ; j < start ; ++j )
   ++C[ std::size_t( std::upper_bound( B.begin() , B.end() , j , less ) -
                     B.begin() ) ];

  // the merge, by one scan of the older array
  out.clear();
  for( std::size_t q = 0 , idx = 0 ; q <= B.size() ; ++q ) {
   out.insert( out.end() , SA.begin() + std::ptrdiff_t( idx ) ,
               SA.begin() + std::ptrdiff_t( idx + C[ q ] ) );
   idx += C[ q ];
   if( q < B.size() ) out.push_back( B[ q ] );
   }
  SA.swap( out );
  }
 ADS_AUDIT( detail::isSuffixArray( T , SA ) );
 return( SA );
 }

inline std::vector< std::size_t > suffixArrayScan( std::string_view T ,
                                                   std::size_t m )
{
 return( suffixArrayScan( toSymbols( T ) , m ) );
 }

inline std::vector< std::size_t > inversePermutation(
                                        const std::vector< std::size_t > & SA )
{
 std::vector< std::size_t > R( SA.size() );
 for( std::size_t r = 0 ; r < SA.size() ; ++r ) R[ SA[ r ] ] = r;
 return( R );
 }

inline std::vector< std::size_t > lcpArray(
                    const Symbols & T , const std::vector< std::size_t > & SA )
{
 ADS_AUDIT( detail::isSuffixArray( T , SA ) );
 const std::size_t n = T.size();
 std::vector< std::size_t > LCP( n < 2 ? 0 : n - 1 );
 const std::vector< std::size_t > R = inversePermutation( SA );
 for( std::size_t i = 0 , h = 0 ; i < n ; ++i ) {
  const std::size_t q = R[ i ];
  if( q == 0 ) {
   h = 0;
   continue;
   }
  const std::size_t k = SA[ q - 1 ];
  while( ( i + h < n ) && ( k + h < n ) && ( T[ i + h ] == T[ k + h ] ) )
   ++h;
  LCP[ q - 1 ] = h;
  if( h > 0 ) --h;
  }
 ADS_AUDIT( detail::isLcpArray( T , SA , LCP ) );
 return( LCP );
 }

inline std::vector< std::size_t > lcpArray(
                   std::string_view T , const std::vector< std::size_t > & SA )
{
 return( lcpArray( toSymbols( T ) , SA ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- SuffixArray ---------------------------------*/
/*--------------------------------------------------------------------------*/

inline SuffixArray::SuffixArray( std::string_view T )
 : f_T( toSymbols( T ) )
{
 f_SA = suffixArraySais( f_T , 256 );
 index();
 }

inline SuffixArray::SuffixArray( Symbols T , std::size_t K )
 : f_T( std::move( T ) )
{
 ADS_PRE( detail::symbolsBelow( f_T , K ) );
 f_SA = suffixArraySais( f_T , K );
 index();
 }

inline void SuffixArray::index( void )
{
 const std::size_t n = f_T.size();
 f_rank = inversePermutation( f_SA );
 f_lcp = lcpArray( f_T , f_SA );
 f_Llcp.assign( n , 0 );
 f_Rlcp.assign( n , 0 );
 if( n > 0 ) build( -1 , long( n ) );
 f_rmq = RangeMinimum< std::size_t >( f_lcp );
 }

inline std::size_t SuffixArray::build( long L , long R )
{
 if( R - L == 1 )
  return( ( L >= 0 ) && ( R < long( f_SA.size() ) ) ?
          f_lcp[ std::size_t( L ) ] : 0 );
 const long M = ( L + R ) / 2;
 f_Llcp[ std::size_t( M ) ] = build( L , M );
 f_Rlcp[ std::size_t( M ) ] = build( M , R );
 return( std::min( f_Llcp[ std::size_t( M ) ] , f_Rlcp[ std::size_t( M ) ] ) );
 }

inline std::pair< std::size_t , bool > SuffixArray::compare(
                                  const Symbols & P , std::size_t M ,
                                  std::size_t h , bool upper ,
                                  long * comparisons ) const
{
 const std::size_t n = f_T.size() , s = f_SA[ M ];
 std::size_t k = h;
 while( ( k < P.size() ) && ( s + k < n ) ) {
  if( comparisons ) ++*comparisons;
  if( P[ k ] != f_T[ s + k ] ) break;
  ++k;
  }
 if( k == P.size() ) return( std::pair( k , upper ) );
 if( s + k == n ) return( std::pair( k , true ) );
 return( std::pair( k , P[ k ] > f_T[ s + k ] ) );
 }

inline std::size_t SuffixArray::bound( const Symbols & P , bool upper ,
                                       long * c ) const
{
 std::size_t L = 0 , R = f_SA.size();
 while( L < R ) {
  const std::size_t M = ( L + R ) / 2;
  if( compare( P , M , 0 , upper , c ).second ) L = M + 1;
  else R = M;
  }
 return( L );
 }

inline std::size_t SuffixArray::boundLcp( const Symbols & P , bool upper ,
                                          long * c ) const
{
 // P is greater than the suffix L and not greater than the suffix R, and
 // shares l and r symbols with them; -1 and n are virtual ends
 long L = -1 , R = long( f_SA.size() );
 std::size_t l = 0 , r = 0;
 while( R - L > 1 ) {
  const std::size_t M = std::size_t( ( L + R ) / 2 );
  std::size_t m;
  bool greater;
  if( l >= r ) {
   if( f_Llcp[ M ] > l ) {          // as against the suffix L
    m = l;
    greater = true;
    }
   else if( f_Llcp[ M ] < l ) {     // smaller where P agrees with L
    m = f_Llcp[ M ];
    greater = false;
    }
   else
    std::tie( m , greater ) = compare( P , M , l , upper , c );
   }
  else {
   if( f_Rlcp[ M ] > r ) {          // as against the suffix R
    m = r;
    greater = false;
    }
   else if( f_Rlcp[ M ] < r ) {     // larger where P agrees with R
    m = f_Rlcp[ M ];
    greater = true;
    }
   else
    std::tie( m , greater ) = compare( P , M , r , upper , c );
   }
  if( greater ) {
   L = long( M );
   l = m;
   }
  else {
   R = long( M );
   r = m;
   }
  }
 return( std::size_t( R ) );
 }

inline std::pair< std::size_t , std::size_t > SuffixArray::range(
                              const Symbols & P , long * comparisons ) const
{
 return( std::pair( bound( P , false , comparisons ) ,
                    bound( P , true , comparisons ) ) );
 }

inline std::pair< std::size_t , std::size_t > SuffixArray::range(
                           std::string_view P , long * comparisons ) const
{
 return( range( toSymbols( P ) , comparisons ) );
 }

inline std::pair< std::size_t , std::size_t > SuffixArray::rangeLcp(
                              const Symbols & P , long * comparisons ) const
{
 const auto res = std::pair( boundLcp( P , false , comparisons ) ,
                             boundLcp( P , true , comparisons ) );
 ADS_AUDIT( res == range( P ) );
 return( res );
 }

inline std::pair< std::size_t , std::size_t > SuffixArray::rangeLcp(
                           std::string_view P , long * comparisons ) const
{
 return( rangeLcp( toSymbols( P ) , comparisons ) );
 }

inline std::size_t SuffixArray::count( std::string_view P ) const
{
 const auto [ first , last ] = rangeLcp( P );
 return( last - first );
 }

inline std::vector< std::size_t > SuffixArray::locate(
                                                   const Symbols & P ) const
{
 const auto [ first , last ] = rangeLcp( P );
 return( std::vector< std::size_t >(
                                f_SA.begin() + std::ptrdiff_t( first ) ,
                                f_SA.begin() + std::ptrdiff_t( last ) ) );
 }

inline std::vector< std::size_t > SuffixArray::locate(
                                                std::string_view P ) const
{
 return( locate( toSymbols( P ) ) );
 }

inline std::size_t SuffixArray::lce( std::size_t i , std::size_t j ) const
{
 const std::size_t n = f_T.size();
 ADS_PRE( ( i <= n ) && ( j <= n ) );
 if( ( i == n ) || ( j == n ) ) return( 0 );
 if( i == j ) return( n - i );
 std::size_t a = f_rank[ i ] , b = f_rank[ j ];
 if( a > b ) std::swap( a , b );
 return( f_lcp[ f_rmq.query( a , b - 1 ) ] );
 }

inline std::size_t SuffixArray::rank( std::size_t i ) const
{
 ADS_PRE( i < f_rank.size() );
 return( f_rank[ i ] );
 }

inline std::size_t SuffixArray::operator[]( std::size_t r ) const
{
 ADS_PRE( r < f_SA.size() );
 return( f_SA[ r ] );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File SuffixArray.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
