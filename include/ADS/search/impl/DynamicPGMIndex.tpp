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
 L.dead.assign( P.size() , 0 );
 if( P.size() >= kIndexed ) {
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
void ADS_DPGM::merge( const Run & M , const Level & L , Run & T )
{
 const std::size_t a = M.key.size() , b = L.size();
 T.key.resize( a + b );
 T.val.resize( a + b );
 T.dead.resize( a + b );
 const K * mk = M.key.data() , * lk = L.keys();
 std::size_t i = 0 , j = 0 , o = 0;
 while( ( i < a ) && ( j < b ) ) {
  if( lk[ j ] < mk[ i ] ) {                 // the older pair
   T.key[ o ] = lk[ j ];
   T.val[ o ] = L.val[ j ];
   T.dead[ o++ ] = L.dead[ j++ ];
   continue;
   }
  if( !( mk[ i ] < lk[ j ] ) ) ++j;         // the same key: the newer
  T.key[ o ] = mk[ i ];
  T.val[ o ] = M.val[ i ];
  T.dead[ o++ ] = M.dead[ i++ ];
  }
 for( ; i < a ; ++i , ++o ) {
  T.key[ o ] = mk[ i ];
  T.val[ o ] = M.val[ i ];
  T.dead[ o ] = M.dead[ i ];
  }
 for( ; j < b ; ++j , ++o ) {
  T.key[ o ] = lk[ j ];
  T.val[ o ] = L.val[ j ];
  T.dead[ o ] = L.dead[ j ];
  }
 T.key.resize( o );
 T.val.resize( o );
 T.dead.resize( o );
 }

ADS_DPGM_T
void ADS_DPGM::put( const Entry & e )
{
 auto it = std::lower_bound( f_buf.begin() , f_buf.end() , e.key ,
                             []( const Entry & a , const K & k ) {
                              return( a.key < k ); } );
 if( ( it != f_buf.end() ) && !( e.key < it->key ) ) {
  *it = e;                                  // already in the buffer
  return;
  }
 f_buf.insert( it , e );
 if( f_buf.size() <= kBuffer )
  return;

 // the first level with room for the buffer and the levels before it
 std::size_t s = f_buf.size() , t = 0;
 while( ( t < f_lev.size() ) && ( s > capacity( t ) - f_lev[ t ].size() ) ) {
  s += f_lev[ t ].size();
  ++t;
  }
 if( t == f_lev.size() )
  f_lev.emplace_back();
 Run M , T;
 M.key.reserve( s );
 M.val.reserve( s );
 M.dead.reserve( s );
 for( Entry & a : f_buf ) {
  M.key.push_back( a.key );
  M.val.push_back( std::move( a.val ) );
  M.dead.push_back( a.dead ? 1 : 0 );
  }
 f_buf.clear();
 for( std::size_t i = 0 ; i <= t ; ++i ) {
  if( f_lev[ i ].size() > 0 ) {
   merge( M , f_lev[ i ] , T );
   std::swap( M , T );
   }
  f_lev[ i ] = Level();
  }
 bool last = true;                          // no older pair to hide
 for( std::size_t i = t + 1 ; i < f_lev.size() ; ++i )
  if( f_lev[ i ].size() > 0 ) last = false;
 if( last ) {
  std::size_t w = 0;
  for( std::size_t i = 0 ; i < M.key.size() ; ++i )
   if( ! M.dead[ i ] ) {
    M.key[ w ] = M.key[ i ];
    M.val[ w ] = std::move( M.val[ i ] );
    M.dead[ w ] = 0;
    ++w;
    }
  M.key.resize( w );
  M.val.resize( w );
  M.dead.resize( w );
  }
 Level & L = f_lev[ t ];
 L.val = std::move( M.val );
 L.dead = std::move( M.dead );
 if( M.key.size() >= kIndexed ) {
  L.idx = PGMIndex< K , E , 4 >( std::move( M.key ) );
  L.indexed = true;
  }
 else
  L.key = std::move( M.key );
 }

ADS_DPGM_T
void ADS_DPGM::insert( const K & k , const V & v )
{
 put( Entry{ k , v , false } );
 }

ADS_DPGM_T
void ADS_DPGM::erase( const K & k )
{
 put( Entry{ k , V() , true } );
 }

ADS_DPGM_T
const V * ADS_DPGM::find( const K & k ) const
{
 const auto it = std::lower_bound( f_buf.begin() , f_buf.end() , k ,
                                   []( const Entry & a , const K & q ) {
                                    return( a.key < q ); } );
 if( ( it != f_buf.end() ) && !( k < it->key ) )
  return( it->dead ? nullptr : &it->val );
 for( const Level & L : f_lev ) {
  const std::size_t r = L.lowerBound( k );
  if( ( r < L.size() ) && !( k < L.at( r ) ) )
   return( L.dead[ r ] ? nullptr : &L.val[ r ] );
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
 pos[ 0 ] = std::size_t( std::lower_bound( f_buf.begin() , f_buf.end() , lo ,
                          []( const Entry & a , const K & q ) {
                           return( a.key < q ); } ) - f_buf.begin() );
 end[ 0 ] = f_buf.size();
 for( std::size_t i = 1 ; i < R ; ++i ) {
  pos[ i ] = f_lev[ i - 1 ].lowerBound( lo );
  end[ i ] = f_lev[ i - 1 ].size();
  }
 auto keyAt = [ & ]( std::size_t i ) -> const K & {
  return( i == 0 ? f_buf[ pos[ 0 ] ].key : f_lev[ i - 1 ].at( pos[ i ] ) );
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
  const bool dead = j == 0 ? f_buf[ pos[ 0 ] ].dead
                           : bool( f_lev[ j - 1 ].dead[ pos[ j ] ] );
  if( ! dead ) {
   const V & v = j == 0 ? f_buf[ pos[ 0 ] ].val
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
 std::optional< std::pair< K , V > > r;
 scan( k , [ & ]( const K & a , const V & v ) {
  r.emplace( a , v );
  return( false ); } );
 return( r );
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
 std::size_t c = f_buf.size();
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
