/*--------------------------------------------------------------------------*/
/*------------------------ File PerfectHashing.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of PerfectHashTable and OrderedMinimalPerfectHash [see
 * PerfectHashing.h], with their contracts.
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

inline PerfectHashTable::PerfectHashTable(
                                      const std::vector< std::uint64_t > & S ,
                                      std::uint64_t seed )
 : f_h( 1 )
{
 ADS_PRE( std::all_of( S.begin() , S.end() , []( std::uint64_t k ) {
  return( k < kMersenne61 ); } ) );
 ADS_AUDIT( [ & ]( void ) {
  std::vector< std::uint64_t > s( S );
  std::sort( s.begin() , s.end() );
  return( std::adjacent_find( s.begin() , s.end() ) == s.end() ); }() );
 const std::size_t n = S.size() , m = std::max< std::size_t >( 1 , n );
 SplitMix64 g( seed );
 std::vector< std::size_t > c( m );        // n_j, then the starts
 for( ; ; ) {                               // the first level
  if( f_trials == kMaxTrials )
   throw( std::runtime_error( "PerfectHashTable: no first level found, "
                              "are the keys distinct?" ) );
  f_h = CarterWegmanHash( m , g() );
  ++f_trials;
  std::fill( c.begin() , c.end() , 0 );
  for( const auto k : S ) ++c[ f_h( k ) ];
  std::size_t L = 0;
  for( const auto x : c ) L += x * x;
  if( ( n == 0 ) || ( L < 4 * n ) ) break;   // Markov: p > 2 / 3
  }
 std::vector< std::uint64_t > keys( n );   // the keys by list
 std::vector< std::size_t > start( m + 1 , 0 );
 for( std::size_t j = 0 ; j < m ; ++j ) start[ j + 1 ] = start[ j ] + c[ j ];
 for( std::size_t j = 0 ; j < m ; ++j ) c[ j ] = start[ j ];
 for( const auto k : S ) keys[ c[ f_h( k ) ]++ ] = k;
 f_B.resize( m );
 std::size_t offset = 0;
 for( std::size_t j = 0 ; j < m ; ++j ) {
  const std::size_t nj = start[ j + 1 ] - start[ j ];
  f_B[ j ].offset = offset;
  f_B[ j ].size = nj * nj;
  offset += nj * nj;
  }
 f_T.assign( offset , kEmpty );
 for( std::size_t j = 0 ; j < m ; ++j ) {   // the second level
  Bucket & B = f_B[ j ];
  if( B.size == 0 ) continue;
  for( bool ok = false ; ! ok ; ) {
   B.h = CarterWegmanHash( B.size , g() );
   ++f_trials;
   ok = true;
   for( std::size_t i = start[ j ] ; ok && ( i < start[ j + 1 ] ) ; ++i ) {
    std::uint64_t & t = f_T[ B.offset + B.h( keys[ i ] ) ];
    ok = t == kEmpty;
    if( ok ) t = keys[ i ];
    }
   if( ! ok )                               // a collision: draw again
    std::fill( f_T.begin() + std::ptrdiff_t( B.offset ) ,
               f_T.begin() + std::ptrdiff_t( B.offset + B.size ) , kEmpty );
   }
  }
 f_n = n;
 ADS_POST( ( n == 0 ) || ( f_T.size() < 4 * n ) );
 ADS_AUDIT( std::all_of( S.begin() , S.end() , [ this ]( std::uint64_t k ) {
  return( contains( k ) ); } ) );
 }

inline bool PerfectHashTable::contains( std::uint64_t k ) const
{
 if( k >= kMersenne61 )                     // out of the universe
  return( false );
 const Bucket & B = f_B[ f_h( k ) ];
 return( ( B.size > 0 ) && ( f_T[ B.offset + B.h( k ) ] == k ) );
 }

inline std::size_t PerfectHashTable::size( void ) const
{
 return( f_n );
 }

inline std::size_t PerfectHashTable::space( void ) const
{
 return( f_T.size() );
 }

inline std::size_t PerfectHashTable::trials( void ) const
{
 return( f_trials );
 }

/*--------------------------------------------------------------------------*/

template< class K >
 requires( std::integral< K > ||
           std::convertible_to< const K & , std::string_view > )
OrderedMinimalPerfectHash< K >::OrderedMinimalPerfectHash(
                     const std::vector< K > & keys , int r , double c ,
                     std::uint64_t seed )
 : f_r( r ) , f_n( keys.size() )
{
 if( c == 0 ) c = r == 2 ? 3 : 1.25;
 ADS_PRE( ( ( r == 2 ) && ( c > 2 ) ) || ( ( r == 3 ) && ( c > 1.23 ) ) );
 ADS_PRE( keys.size() < ( std::size_t( 1 ) << 32 ) );
 ADS_AUDIT( [ & ]( void ) {
  std::vector< K > s( keys );
  std::sort( s.begin() , s.end() );
  return( std::adjacent_find( s.begin() , s.end() ) == s.end() ); }() );
 if( f_n == 0 ) return;
 const auto cn = std::size_t( std::ceil( c * double( f_n ) ) );
 if( r == 2 )                               // one range for both
  f_block = f_m = std::max< std::size_t >( cn , 3 );
 else {                                     // a block for each function
  f_block = std::max< std::size_t >( ( cn + 2 ) / 3 , 2 );
  f_m = 3 * f_block;
  }
 std::vector< std::size_t > h( f_n ) , E( f_n * std::size_t( r ) );
 for( std::size_t i = 0 ; i < f_n ; ++i ) h[ i ] = i;
 SplitMix64 g( seed );
 for( ; ; ) {
  if( f_trials == kMaxTrials )
   throw( std::runtime_error( "OrderedMinimalPerfectHash: no graph found, "
                              "are the keys distinct?" ) );
  ++f_trials;
  f_seed.clear();
  f_h.clear();
  for( int i = 0 ; i < r ; ++i ) {
   f_seed.push_back( g() );
   f_h.emplace_back( f_block , g() );
   }
  for( std::size_t e = 0 ; e < f_n ; ++e )
   for( int i = 0 ; i < r ; ++i )
    E[ e * std::size_t( r ) + std::size_t( i ) ] = vertex( keys[ e ] , i );
  auto G = r == 2 ? labelAcyclicGraph( f_m , E , h , f_n )
                  : labelByPeeling( f_m , E , r , h , f_n );
  if( G ) {
   f_g = std::move( *G );
   break;
   }
  }
 ADS_AUDIT( [ & ]( void ) {
  for( std::size_t i = 0 ; i < f_n ; ++i )
   if( ( *this )( keys[ i ] ) != i ) return( false );
  return( true ); }() );
 }

template< class K >
 requires( std::integral< K > ||
           std::convertible_to< const K & , std::string_view > )
std::uint64_t OrderedMinimalPerfectHash< K >::fingerprint( const K & k ,
                                                           int i ) const
{
 const std::uint64_t s = f_seed[ std::size_t( i ) ];
 if constexpr( std::integral< K > )
  return( splitmix64( std::uint64_t( k ) ^ s ) % kMersenne61 );
 else
  return( StringHash( s )( std::string_view( k ) ) );
 }

template< class K >
 requires( std::integral< K > ||
           std::convertible_to< const K & , std::string_view > )
std::size_t OrderedMinimalPerfectHash< K >::vertex( const K & k ,
                                                    int i ) const
{
 const auto v = std::size_t( f_h[ std::size_t( i ) ]( fingerprint( k , i ) ) );
 return( f_r == 2 ? v : std::size_t( i ) * f_block + v );
 }

template< class K >
 requires( std::integral< K > ||
           std::convertible_to< const K & , std::string_view > )
std::size_t OrderedMinimalPerfectHash< K >::operator()( const K & k ) const
{
 ADS_PRE( f_n >= 1 );
 std::size_t s = 0;
 for( int i = 0 ; i < f_r ; ++i )
  s += f_g[ vertex( k , i ) ];
 return( s % f_n );
 }

template< class K >
 requires( std::integral< K > ||
           std::convertible_to< const K & , std::string_view > )
std::size_t OrderedMinimalPerfectHash< K >::size( void ) const
{
 return( f_n );
 }

template< class K >
 requires( std::integral< K > ||
           std::convertible_to< const K & , std::string_view > )
std::size_t OrderedMinimalPerfectHash< K >::vertices( void ) const
{
 return( f_m );
 }

template< class K >
 requires( std::integral< K > ||
           std::convertible_to< const K & , std::string_view > )
std::size_t OrderedMinimalPerfectHash< K >::trials( void ) const
{
 return( f_trials );
 }

template< class K >
 requires( std::integral< K > ||
           std::convertible_to< const K & , std::string_view > )
auto OrderedMinimalPerfectHash< K >::labelAcyclicGraph(
                    std::size_t m , const std::vector< std::size_t > & E ,
                    const std::vector< std::size_t > & h , std::size_t n )
 -> std::optional< std::vector< std::uint32_t > >
{
 const std::size_t ne = h.size();
 ADS_PRE( ( n >= 1 ) && ( E.size() == 2 * ne ) &&
          std::all_of( E.begin() , E.end() ,
                       [ m ]( std::size_t v ) { return( v < m ); } ) );
 std::vector< std::size_t > start( m + 1 , 0 ) , adj( 2 * ne );
 for( const auto v : E ) ++start[ v + 1 ];  // the lists of adjacency
 for( std::size_t v = 0 ; v < m ; ++v ) start[ v + 1 ] += start[ v ];
 std::vector< std::size_t > fill( start.begin() , start.end() - 1 );
 for( std::size_t i = 0 ; i < 2 * ne ; ++i )
  adj[ fill[ E[ i ] ]++ ] = i;              // the end i of the edge i / 2
 constexpr std::uint32_t undef = ~std::uint32_t( 0 );
 std::vector< std::uint32_t > g( m , undef );
 std::vector< std::size_t > stack;
 for( std::size_t v0 = 0 ; v0 < m ; ++v0 ) {
  if( g[ v0 ] != undef ) continue;
  g[ v0 ] = 0;                              // LabelFrom( v0 , 0 )
  stack.push_back( v0 );
  while( ! stack.empty() ) {
   const std::size_t v = stack.back();
   stack.pop_back();
   for( std::size_t a = start[ v ] ; a < start[ v + 1 ] ; ++a ) {
    const std::size_t i = adj[ a ] , e = i / 2 , u = E[ i ^ 1 ];
    const auto c = std::uint32_t( ( h[ e ] % n + n - g[ v ] ) % n );
    if( g[ u ] == undef ) {
     g[ u ] = c;
     stack.push_back( u );
     }
    else if( g[ u ] != c )                  // a cycle that does not add up
     return( std::nullopt );
    }
   }
  }
 ADS_POST( [ & ]( void ) {
  for( std::size_t e = 0 ; e < ne ; ++e )
   if( ( g[ E[ 2 * e ] ] + g[ E[ 2 * e + 1 ] ] ) % n != h[ e ] % n )
    return( false );
  return( true ); }() );
 return( g );
 }

template< class K >
 requires( std::integral< K > ||
           std::convertible_to< const K & , std::string_view > )
auto OrderedMinimalPerfectHash< K >::labelByPeeling(
                    std::size_t m , const std::vector< std::size_t > & E ,
                    int r , const std::vector< std::size_t > & h ,
                    std::size_t n )
 -> std::optional< std::vector< std::uint32_t > >
{
 const std::size_t ne = h.size() , R = std::size_t( r );
 ADS_PRE( ( n >= 1 ) && ( r >= 2 ) && ( E.size() == R * ne ) &&
          std::all_of( E.begin() , E.end() ,
                       [ m ]( std::size_t v ) { return( v < m ); } ) );
 ADS_PRE( [ & ]( void ) {
  for( std::size_t e = 0 ; e < ne ; ++e )
   for( std::size_t i = 0 ; i < R ; ++i )
    for( std::size_t j = i + 1 ; j < R ; ++j )
     if( E[ e * R + i ] == E[ e * R + j ] ) return( false );
  return( true ); }() );
 // the degree of each vertex and the XOR of its edges: a vertex of degree
 // 1 knows its edge with no list
 std::vector< std::size_t > deg( m , 0 ) , ex( m , 0 );
 for( std::size_t e = 0 ; e < ne ; ++e )
  for( std::size_t i = 0 ; i < R ; ++i ) {
   ++deg[ E[ e * R + i ] ];
   ex[ E[ e * R + i ] ] ^= e;
   }
 std::vector< std::size_t > queue , order;  // order: pairs ( e , v )
 for( std::size_t v = 0 ; v < m ; ++v )
  if( deg[ v ] == 1 ) queue.push_back( v );
 for( std::size_t q = 0 ; q < queue.size() ; ++q ) {
  const std::size_t v = queue[ q ];
  if( deg[ v ] != 1 ) continue;
  const std::size_t e = ex[ v ];
  order.push_back( e );
  order.push_back( v );
  for( std::size_t i = 0 ; i < R ; ++i ) {
   const std::size_t w = E[ e * R + i ];
   --deg[ w ];
   ex[ w ] ^= e;
   if( deg[ w ] == 1 ) queue.push_back( w );
   }
  }
 if( order.size() < 2 * ne )                // a 2-core is left
  return( std::nullopt );
 std::vector< std::uint32_t > g( m , 0 );
 for( std::size_t k = ne ; k-- > 0 ; ) {    // the last one peeled first
  const std::size_t e = order[ 2 * k ] , v = order[ 2 * k + 1 ];
  std::size_t s = 0;
  for( std::size_t i = 0 ; i < R ; ++i )
   if( E[ e * R + i ] != v ) s += g[ E[ e * R + i ] ];
  g[ v ] = std::uint32_t( ( h[ e ] % n + n - s % n ) % n );
  }
 ADS_POST( [ & ]( void ) {
  for( std::size_t e = 0 ; e < ne ; ++e ) {
   std::size_t s = 0;
   for( std::size_t i = 0 ; i < R ; ++i ) s += g[ E[ e * R + i ] ];
   if( s % n != h[ e ] % n ) return( false );
   }
  return( true ); }() );
 return( g );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File PerfectHashing.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
