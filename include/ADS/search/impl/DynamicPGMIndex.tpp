/*--------------------------------------------------------------------------*/
/*------------------------ File DynamicPGMIndex.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of DynamicPGMIndex [see DynamicPGMIndex.h], with their
 * contracts.
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

namespace ADS {

#define ADS_DPGM_T template< class K , class V , std::size_t E , \
                             std::size_t B , std::size_t I > \
                   requires std::is_arithmetic_v< K >
#define ADS_DPGM DynamicPGMIndex< K , V , E , B , I >

ADS_DPGM_T
inline std::size_t ADS_DPGM::capacity( std::size_t i )
{
 std::size_t c = kBuffer;
 for( std::size_t j = 0 ; j <= i ; ++j ) c *= B;
 return( c );
 }

ADS_DPGM_T
ADS_DPGM::DynamicPGMIndex( const std::vector< std::pair< K , V > > & P )
{
 ADS_AUDIT( std::adjacent_find( P.begin() , P.end() ,
                                []( const auto & a , const auto & b ) {
                                 return( !( a.first < b.first ) ); } ) ==
            P.end() );
 if( P.empty() )
  return;
 std::size_t t = 0;
 while( capacity( t ) < P.size() ) ++t;
 f_lev.resize( t + 1 );
 std::vector< K > k;
 k.reserve( P.size() );
 Level & L = f_lev[ t ];
 L.val.reserve( P.size() );
 for( const auto & p : P ) {
  k.push_back( p.first );
  L.val.push_back( p.second );
  }
 if( capacity( t ) >= kFiltered ) {
  L.room = std::min( capacity( t ) , 2 * P.size() + kBuffer );
  BlockedBloomFilter< std::uint64_t > F( L.room );
  for( const K & x : k ) F.insertHash( hashOf( x ) );
  L.filter = std::move( F );
  L.filtered = true;
  }
 if( capacity( t ) >= kIndexed ) {
  L.idx = PGMIndex< K , E , 4 >( std::move( k ) );
  L.indexed = true;
  }
 else
  L.key = std::move( k );
 }

ADS_DPGM_T
std::size_t ADS_DPGM::Level::lowerBound( const K & k ) const
{
 if( indexed )
  return( idx.lowerBound( k ) );
 std::size_t n = key.size();
 if( n == 0 )
  return( 0 );
 const K * base = key.data();
 while( n > 1 ) {
  const std::size_t h = n / 2;
#if defined( __GNUC__ ) || defined( __clang__ )
  __builtin_prefetch( base + ( n - h ) / 2 );   // the two next middles
  __builtin_prefetch( base + h + ( n - h ) / 2 );
#endif
  base = base[ h ] < k ? base + h : base;
  n -= h;
  }
 return( std::size_t( base - key.data() ) + ( *base < k ? 1 : 0 ) );
 }

ADS_DPGM_T
inline std::uint64_t ADS_DPGM::hashOf( const K & k )
{
 if constexpr( std::is_floating_point_v< K > )
  if( k == K( 0 ) )                         // -0 and +0 alike
   return( hashWord( detail::pgmOrd( K( 0 ) ) ) );
 return( hashWord( detail::pgmOrd( k ) ) );
 }

ADS_DPGM_T
inline std::size_t ADS_DPGM::bufferBound( const K & k ) const
{
 std::size_t n = f_buf.key.size();
 if( n == 0 )
  return( 0 );
 const K * base = f_buf.key.data();
 while( n > 1 ) {                           // a conditional move a step
  const std::size_t h = n / 2;
  base = base[ h ] < k ? base + h : base;
  n -= h;
  }
 return( std::size_t( base - f_buf.key.data() ) + ( *base < k ? 1 : 0 ) );
 }

ADS_DPGM_T
void ADS_DPGM::merge( const Run & M , const Level & L , Run & T )
{
 const std::size_t a = M.key.size() , b = L.size();
 T.key.resize( a + b );
 T.val.resize( a + b );
 T.dead.clear();
 // the arrays by their pointers, so that the compiler keeps them in
 // registers through the stores
 const K * mk = M.key.data() , * lk = L.keys();
 const V * mv = M.val.data() , * lv = L.val.data();
 K * tk = T.key.data();
 V * tv = T.val.data();
 std::size_t i = 0 , j = 0 , o = 0;
 // the older pair when its key is smaller, the newer one otherwise, and
 // the older one skipped when the keys are equal; with branches, which
 // the merges of a short run into a long one, the most, predict well
 while( ( i < a ) && ( j < b ) )
  if( lk[ j ] < mk[ i ] ) {
   tk[ o ] = lk[ j ];
   tv[ o++ ] = lv[ j++ ];
   }
  else {
   if( !( mk[ i ] < lk[ j ] ) ) ++j;
   tk[ o ] = mk[ i ];
   tv[ o++ ] = mv[ i++ ];
   }
 // the tombstones apart, as they are keys of their runs: those of M, and
 // those of L whose key M does not have (a key of M is newer)
 if( ! ( M.dead.empty() && L.dead.empty() ) ) {
  const std::size_t c = M.dead.size() , d = L.dead.size();
  const K * e = mk;                         // where the keys of M are left
  std::size_t p = 0 , q = 0;
  while( ( p < c ) || ( q < d ) )
   if( ( q == d ) || ( ( p < c ) && ( M.dead[ p ] < L.dead[ q ] ) ) )
    T.dead.push_back( M.dead[ p++ ] );
   else {
    const K y = L.dead[ q++ ];
    e = std::lower_bound( e , mk + a , y );
    if( ( e == mk + a ) || ( y < *e ) )
     T.dead.push_back( y );
    }
  }
 std::copy( mk + i , mk + a , tk + o );   // the tails, as blocks
 std::copy( mv + i , mv + a , tv + o );
 o += a - i;
 std::copy( lk + j , lk + b , tk + o );
 std::copy( lv + j , lv + b , tv + o );
 o += b - j;
 T.key.resize( o );
 T.val.resize( o );
 }

ADS_DPGM_T
void ADS_DPGM::put( const K & k , const V & v , bool dead )
{
 const std::size_t r = bufferBound( k );
 Run & U = f_buf;
 // the tombstones of the buffer, sorted, few: k joins them or leaves them
 auto mark = [ & ]( void ) {
  const auto d = std::lower_bound( U.dead.begin() , U.dead.end() , k );
  const bool there = ( d != U.dead.end() ) && !( k < *d );
  if( dead && ! there ) U.dead.insert( d , k );
  else if( ! dead && there ) U.dead.erase( d );
  };
 if( ( r < U.key.size() ) && !( k < U.key[ r ] ) ) {
  U.val[ r ] = v;                           // already in the buffer
  if( dead || ! U.dead.empty() ) mark();
  return;
  }
 U.key.insert( U.key.begin() + std::ptrdiff_t( r ) , k );
 U.val.insert( U.val.begin() + std::ptrdiff_t( r ) , v );
 if( dead || ! U.dead.empty() ) mark();
 if( U.key.size() <= kBuffer )
  return;

 // the first level with room for the buffer and the levels before it
 std::size_t s = f_buf.key.size() , t = 0;
 while( ( t < f_lev.size() ) && ( s > capacity( t ) - f_lev[ t ].size() ) ) {
  s += f_lev[ t ].size();
  ++t;
  }
 if( t == f_lev.size() )
  f_lev.emplace_back();
 // the buffer is the first run, and an empty run the next buffer
 Run * M = &f_run[ 0 ] , * T = &f_run[ 1 ];
 std::swap( *M , f_buf );
 f_buf.key.clear();
 f_buf.val.clear();
 f_buf.dead.clear();
 for( std::size_t i = 0 ; i < t ; ++i ) {
  if( f_lev[ i ].size() > 0 ) {
   merge( *M , f_lev[ i ] , *T );
   std::swap( M , T );
   }
  f_lev[ i ] = Level();
  }
 // the keys that come into L_t go into its filter, if it has room for
 // them (the keys that they replace stay there, a few false positives
 // more); else the filter is built again below, for twice the keys
 Level & L = f_lev[ t ];
 bool kept = false;
 if( L.filtered && ( L.size() > 0 ) &&
     ( L.filter.size() + M->key.size() <= L.room ) ) {
  for( const K & k : M->key ) L.filter.insertHash( hashOf( k ) );
  kept = true;
  }
 if( L.size() > 0 ) {
  merge( *M , L , *T );
  std::swap( M , T );
  }
 bool last = true;                          // no older pair to hide
 for( std::size_t i = t + 1 ; i < f_lev.size() ; ++i )
  if( f_lev[ i ].size() > 0 ) last = false;
 if( last && ! M->dead.empty() ) {          // the tombstones dropped
  std::size_t w = 0 , p = 0;
  for( std::size_t i = 0 ; i < M->key.size() ; ++i ) {
   while( ( p < M->dead.size() ) && ( M->dead[ p ] < M->key[ i ] ) ) ++p;
   if( ( p < M->dead.size() ) && !( M->key[ i ] < M->dead[ p ] ) )
    continue;
   M->key[ w ] = M->key[ i ];
   M->val[ w ] = std::move( M->val[ i ] );
   ++w;
   }
  M->key.resize( w );
  M->val.resize( w );
  M->dead.clear();
  }
 // the run becomes the level, whose old arrays are kept as a run
 if( ! kept && ( capacity( t ) >= kFiltered ) ) {
  L.room = std::min( capacity( t ) , 2 * M->key.size() + kBuffer );
  BlockedBloomFilter< std::uint64_t > F( L.room );
  for( const K & k : M->key ) F.insertHash( hashOf( k ) );
  L.filter = std::move( F );
  L.filtered = true;
  }
 std::swap( L.val , M->val );
 std::swap( L.dead , M->dead );
 if( capacity( t ) >= kIndexed ) {
  L.idx = PGMIndex< K , E , 4 >( std::move( M->key ) );
  L.indexed = true;
  L.key.clear();
  M->key.clear();
  }
 else {
  L.idx = PGMIndex< K , E , 4 >();
  L.indexed = false;
  std::swap( L.key , M->key );
  }
 }

ADS_DPGM_T
void ADS_DPGM::insert( const K & k , const V & v )
{
 put( k , v , false );
 }

ADS_DPGM_T
void ADS_DPGM::erase( const K & k )
{
 put( k , V() , true );
 }

ADS_DPGM_T
const V * ADS_DPGM::find( const K & k ) const
{
 const std::size_t r = bufferBound( k );
 if( ( r < f_buf.key.size() ) && !( k < f_buf.key[ r ] ) )
  return( ( ! f_buf.dead.empty() &&
            std::binary_search( f_buf.dead.begin() , f_buf.dead.end() ,
                                k ) ) ? nullptr : &f_buf.val[ r ] );
 const std::uint64_t h = hashOf( k );
 for( const Level & L : f_lev ) {
  if( ( L.size() == 0 ) || ( L.filtered && ! L.filter.maybeHash( h ) ) )
   continue;
  const std::size_t r = L.lowerBound( k );
  if( ( r < L.size() ) && !( k < L.at( r ) ) )
   return( L.isDead( k ) ? nullptr : &L.val[ r ] );
  }
 return( nullptr );
 }

ADS_DPGM_T
bool ADS_DPGM::contains( const K & k ) const
{
 return( find( k ) != nullptr );
 }

ADS_DPGM_T
template< class F >
void ADS_DPGM::scan( const K & lo , F && f ) const
{
 // the place of each run, the buffer as the run 0
 const std::size_t R = f_lev.size() + 1;
 std::vector< std::size_t > pos( R ) , end( R );
 pos[ 0 ] = bufferBound( lo );
 end[ 0 ] = f_buf.key.size();
 for( std::size_t i = 1 ; i < R ; ++i ) {
  pos[ i ] = f_lev[ i - 1 ].lowerBound( lo );
  end[ i ] = f_lev[ i - 1 ].size();
  }
 auto keyAt = [ & ]( std::size_t i ) -> const K & {
  return( i == 0 ? f_buf.key[ pos[ 0 ] ] : f_lev[ i - 1 ].at( pos[ i ] ) );
  };
 for( ; ; ) {
  std::size_t j = R;                        // the newest run of the least key
  for( std::size_t i = 0 ; i < R ; ++i )
   if( ( pos[ i ] < end[ i ] ) &&
       ( ( j == R ) || ( keyAt( i ) < keyAt( j ) ) ) )
    j = i;
  if( j == R )
   return;
  const K k = keyAt( j );
  const bool dead = j == 0 ? ( ! f_buf.dead.empty() &&
                               std::binary_search( f_buf.dead.begin() ,
                                                   f_buf.dead.end() , k ) )
                           : f_lev[ j - 1 ].isDead( k );
  if( ! dead ) {
   const V & v = j == 0 ? f_buf.val[ pos[ 0 ] ]
                        : f_lev[ j - 1 ].val[ pos[ j ] ];
   if( ! f( k , v ) )
    return;
   }
  for( std::size_t i = j ; i < R ; ++i )    // past k in every run
   if( ( pos[ i ] < end[ i ] ) && !( k < keyAt( i ) ) ) ++pos[ i ];
  }
 }

ADS_DPGM_T
auto ADS_DPGM::lowerBound( const K & k ) const
 -> std::optional< std::pair< K , V > >
{
 // the least key >= k of each run, the newest run winning a tie, with no
 // allocation; only if its pair is a tombstone the runs are scanned on
 const K * best = nullptr;
 const V * val = nullptr;
 bool dead = false;
 const std::size_t r = bufferBound( k );
 if( r < f_buf.key.size() ) {
  best = &f_buf.key[ r ];
  val = &f_buf.val[ r ];
  dead = ! f_buf.dead.empty() &&
         std::binary_search( f_buf.dead.begin() , f_buf.dead.end() , *best );
  }
 for( const Level & L : f_lev ) {
  const std::size_t p = L.size() > 0 ? L.lowerBound( k ) : 0;
  if( ( p < L.size() ) && ( ( best == nullptr ) || ( L.at( p ) < *best ) ) ) {
   best = &L.at( p );
   val = &L.val[ p ];
   dead = L.isDead( *best );
   }
  }
 std::optional< std::pair< K , V > > o;
 if( best == nullptr )
  return( o );
 if( ! dead ) {
  o.emplace( *best , *val );
  return( o );
  }
 scan( k , [ & ]( const K & a , const V & v ) {
  o.emplace( a , v );
  return( false ); } );
 return( o );
 }

ADS_DPGM_T
template< class F >
void ADS_DPGM::forEach( const K & lo , const K & hi , F && f ) const
{
 scan( lo , [ & ]( const K & a , const V & v ) {
  if( !( a < hi ) )
   return( false );
  f( a , v );
  return( true ); } );
 }

ADS_DPGM_T
std::size_t ADS_DPGM::entries( void ) const
{
 std::size_t c = f_buf.key.size();
 for( const Level & L : f_lev ) c += L.size();
 return( c );
 }

ADS_DPGM_T
std::size_t ADS_DPGM::levels( void ) const
{
 return( f_lev.size() );
 }

#undef ADS_DPGM
#undef ADS_DPGM_T

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File DynamicPGMIndex.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
