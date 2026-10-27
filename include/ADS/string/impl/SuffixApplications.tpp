/*--------------------------------------------------------------------------*/
/*---------------------- File SuffixApplications.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of SuffixApplications.h, with their contracts.
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

/// x is a substring of at least k of the strings S
inline bool inAtLeast( const std::vector< std::string > & S ,
                       const std::string & x , std::size_t k )
{
 return( std::size_t( std::count_if( S.begin() , S.end() ,
          [ & ]( const std::string & s ) {
           return( s.find( x ) != std::string::npos ); } ) ) >= k );
 }

}  // end( namespace detail )

inline std::pair< std::size_t , std::size_t > longestRepeatedSubstring(
                                                    const SuffixArray & SA )
{
 std::size_t pos = 0 , best = 0;
 for( std::size_t r = 0 ; r < SA.lcp().size() ; ++r )
  if( SA.lcp()[ r ] > best ) {
   best = SA.lcp()[ r ];
   pos = SA[ r ];
   }
 return( std::pair( pos , best ) );
 }

inline std::optional< std::size_t > repeatedSubstring(
                    const SuffixArray & SA , std::size_t L , std::size_t C )
{
 ADS_PRE( C >= 1 );
 const std::size_t n = SA.size();
 if( L == 0 )                     // the empty string, at n + 1 places
  return( C <= n + 1 ? std::optional< std::size_t >( 0 ) : std::nullopt );
 if( L > n ) return( std::nullopt );
 if( C == 1 ) return( 0 );
 std::size_t run = 0;             // the entries >= L ending at r
 for( std::size_t r = 0 ; r < SA.lcp().size() ; ++r )
  if( SA.lcp()[ r ] >= L ) {
   if( ++run >= C - 1 ) return( SA[ r ] );
   }
  else
   run = 0;
 return( std::nullopt );
 }

inline std::uint64_t distinctSubstrings( const SuffixArray & SA )
{
 const std::uint64_t n = SA.size();
 std::uint64_t d = n * ( n + 1 ) / 2;
 for( std::size_t h : SA.lcp() ) d -= h;
 return( d );
 }

inline std::string longestCommonSubstring(
                        const std::vector< std::string > & S , std::size_t k )
{
 const std::size_t m = S.size();
 ADS_PRE( ( k >= 1 ) && ( k <= m ) );
 if( k == 1 )
  return( *std::max_element( S.begin() , S.end() ,
           []( const std::string & a , const std::string & b ) {
            return( a.size() < b.size() ); } ) );

 // the join of the strings, each followed by its own separator s < m,
 // the bytes shifted to m , ... , m + 255
 Symbols X;
 std::vector< std::size_t > owner;
 for( std::size_t s = 0 ; s < m ; ++s ) {
  for( char c : S[ s ] ) {
   X.push_back( m + static_cast< unsigned char >( c ) );
   owner.push_back( s );
   }
  X.push_back( s );
  owner.push_back( s );
  }
 const SuffixArray A( X , m + 256 );

 // the windows of ranks with suffixes of k strings, by two pointers
 std::vector< std::size_t > cnt( m , 0 );
 std::size_t distinct = 0 , best = 0 , at = 0;
 for( std::size_t lo = 0 , hi = 0 ; hi < A.size() ; ++hi ) {
  if( cnt[ owner[ A[ hi ] ] ]++ == 0 ) ++distinct;
  while( distinct >= k ) {
   const std::size_t l = A.lce( A[ lo ] , A[ hi ] );
   if( l > best ) {
    best = l;
    at = A[ lo ];
    }
   if( --cnt[ owner[ A[ lo ] ] ] == 0 ) --distinct;
   ++lo;
   }
  }
 std::string x;
 for( std::size_t t = 0 ; t < best ; ++t )
  x.push_back( static_cast< char >( X[ at + t ] - m ) );
 ADS_AUDIT( detail::inAtLeast( S , x , k ) );
 return( x );
 }

inline std::vector< std::pair< std::size_t , std::size_t > > maximalRepeats(
                                                      const SuffixTree & ST )
{
 // the symbol before the occurrences below each node, or diverse; the
 // occurrence at 0 has the terminator 0, which precedes nothing else
 constexpr std::size_t diverse = SuffixTree::npos;
 const std::vector< std::size_t > pre = ST.preorder();
 std::vector< std::size_t > left( ST.size() );
 std::vector< std::pair< std::size_t , std::size_t > > out;
 for( std::size_t k = pre.size() ; k-- > 0 ; ) {
  const std::size_t u = pre[ k ];
  const SuffixTree::Node & x = ST.node( u );
  if( x.suffix != SuffixTree::npos ) {
   left[ u ] = x.suffix == 0 ? 0 : ST.symbol( x.suffix - 1 );
   continue;
   }
  left[ u ] = left[ x.child.begin()->second ];
  for( const auto & e : x.child )
   if( left[ e.second ] != left[ u ] ) left[ u ] = diverse;
  }
 for( std::size_t u : pre )
  if( ( u != ST.root() ) && ( ST.node( u ).suffix == SuffixTree::npos ) &&
      ( left[ u ] == diverse ) )
   out.emplace_back( ST.node( u ).pos , ST.node( u ).depth );
 return( out );
 }

inline bool proximity( const SuffixArray & SA , std::string_view P ,
                       std::string_view Q , std::size_t k )
{
 std::vector< std::pair< std::size_t , int > > O;
 for( std::size_t i : SA.locate( P ) ) O.emplace_back( i , 0 );
 for( std::size_t i : SA.locate( Q ) ) O.emplace_back( i , 1 );
 std::sort( O.begin() , O.end() );
 for( std::size_t t = 1 ; t < O.size() ; ++t )
  if( ( O[ t ].second != O[ t - 1 ].second ) &&
      ( O[ t ].first - O[ t - 1 ].first <= k ) )
   return( true );
 return( false );
 }

inline std::vector< std::size_t > kMismatches( std::string_view T ,
                                               std::string_view P ,
                                               std::size_t k )
{
 const std::size_t n = T.size() , p = P.size();
 std::vector< std::size_t > out;
 if( p > n ) return( out );
 if( p == 0 ) {
  for( std::size_t i = 0 ; i <= n ; ++i ) out.push_back( i );
  return( out );
  }

 // X = T # P, the bytes shifted by 1 and # = 0
 Symbols X( n + 1 + p , 0 );
 for( std::size_t i = 0 ; i < n ; ++i )
  X[ i ] = std::size_t( static_cast< unsigned char >( T[ i ] ) ) + 1;
 for( std::size_t j = 0 ; j < p ; ++j )
  X[ n + 1 + j ] = std::size_t( static_cast< unsigned char >( P[ j ] ) ) + 1;
 const SuffixArray A( X , 257 );

 for( std::size_t i = 0 ; i + p <= n ; ++i ) {
  std::size_t m = 0 , j = 0;
  while( ( m <= k ) && ( j < p ) ) {
   j += A.lce( i + j , n + 1 + j );
   if( j < p ) {
    ++m;
    ++j;
    }
   }
  if( m <= k ) out.push_back( i );
  }
 return( out );
 }

inline std::vector< CopyPhrase > lz77ByTree( std::string_view T )
{
 const std::size_t n = T.size();
 const SuffixTree ST( T );

 // the least leaf below each node, by a visit from the leaves up
 const std::vector< std::size_t > pre = ST.preorder();
 std::vector< std::size_t > least( ST.size() );
 for( std::size_t k = pre.size() ; k-- > 0 ; ) {
  const SuffixTree::Node & x = ST.node( pre[ k ] );
  least[ pre[ k ] ] = x.suffix;
  for( const auto & e : x.child )
   least[ pre[ k ] ] = std::min( least[ pre[ k ] ] , least[ e.second ] );
  }

 std::vector< CopyPhrase > Z;
 for( std::size_t i = 0 ; i < n ; ) {
  // down along the suffix i while the next node starts earlier too
  std::size_t u = ST.root();
  for( ; ; ) {
   const SuffixTree::Node & x = ST.node( u );
   const std::size_t c = x.child.at( ST.symbol( i + x.depth ) );
   if( least[ c ] >= i ) break;
   u = c;
   }
  CopyPhrase z;
  z.length = ST.node( u ).depth;
  z.distance = z.length > 0 ? i - least[ u ] : 0;
  if( i + z.length < n ) z.next = T[ i + z.length ];
  Z.push_back( z );
  i += z.length + 1;
  }
 ADS_AUDIT( decodeCopyPhrases( Z ) == T );
 return( Z );
 }

inline std::string decodeCopyPhrases( const std::vector< CopyPhrase > & Z )
{
 std::string out;
 for( const CopyPhrase & z : Z ) {
  ADS_PRE( z.distance <= out.size() );
  ADS_PRE( ( z.length == 0 ) || ( z.distance > 0 ) );
  const std::size_t start = out.size() - z.distance;
  for( std::size_t t = 0 ; t < z.length ; ++t )
   out.push_back( out[ start + t ] );
  if( z.next ) out.push_back( *z.next );
  }
 return( out );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------- End File SuffixApplications.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
