/*--------------------------------------------------------------------------*/
/*------------------------- File CuckooHashing.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of CuckooHashTable [see CuckooHashing.h], with their
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

template< class K , class V , class H >
CuckooHashTable< K , V , H >::CuckooHashTable( std::size_t m ,
                                               double maxLoad ,
                                               std::size_t s ,
                                               std::uint64_t seed , H h )
 : f_T( m + s ) , f_m( m ) , f_m0( m ) , f_s( s ) , f_maxLoad( maxLoad ) ,
   f_seeds( seed ) , f_g1( f_seeds() ) , f_g2( f_seeds() ) , f_h( h )
{
 ADS_PRE( ( m >= 2 ) && ( maxLoad > 0 ) && ( maxLoad < 0.5 ) );
 }

template< class K , class V , class H >
void CuckooHashTable< K , V , H >::draw( void )
{
 f_g1 = TabulationHash( f_seeds() );
 f_g2 = TabulationHash( f_seeds() );
 }

template< class K , class V , class H >
std::size_t CuckooHashTable< K , V , H >::place( const K & k , int i ) const
{
 const auto x = std::uint64_t( f_h( k ) );
 return( std::size_t( ( i ? f_g2( x ) : f_g1( x ) ) % f_m ) );
 }

template< class K , class V , class H >
std::optional< std::size_t > CuckooHashTable< K , V , H >::find(
                                                       const K & k ) const
{
 for( int i = 0 ; i < 2 ; ++i ) {
  const std::size_t p = place( k , i );
  if( f_T[ p ].full && ( f_T[ p ].key == k ) )
   return( p );
  }
 for( std::size_t p = f_m ; p < f_T.size() ; ++p )   // the stash
  if( f_T[ p ].full && ( f_T[ p ].key == k ) )
   return( p );
 return( std::nullopt );
 }

template< class K , class V , class H >
bool CuckooHashTable< K , V , H >::walk( Cell & c )
{
 std::size_t p = place( c.key , 0 );
 if( ! f_T[ p ].full ) {
  f_T[ p ] = std::move( c );
  return( true );
  }
 const std::size_t q = place( c.key , 1 );
 if( ! f_T[ q ].full ) {
  f_T[ q ] = std::move( c );
  return( true );
  }
 const int maxLoop = 8 * int( std::bit_width( f_m - 1 ) ) + 16;
 for( int t = 0 ; t < maxLoop ; ++t ) {
  std::swap( c , f_T[ p ] );                // c is the key evicted from p
  ++f_evictions;
  const std::size_t a = place( c.key , 0 );
  p = a == p ? place( c.key , 1 ) : a;      // to its other place
  if( ! f_T[ p ].full ) {
   f_T[ p ] = std::move( c );
   return( true );
   }
  }
 return( false );
 }

template< class K , class V , class H >
bool CuckooHashTable< K , V , H >::settle( Cell & c )
{
 if( walk( c ) )
  return( true );
 for( std::size_t p = f_m ; p < f_T.size() ; ++p )   // the stash
  if( ! f_T[ p ].full ) {
   f_T[ p ] = std::move( c );
   return( true );
   }
 return( false );
 }

template< class K , class V , class H >
void CuckooHashTable< K , V , H >::rebuild( std::size_t m ,
                                            std::optional< Cell > extra )
{
 std::vector< Cell > items;
 for( auto & c : f_T )
  if( c.full ) items.push_back( std::move( c ) );
 if( extra ) items.push_back( std::move( *extra ) );
 for( ; ; ) {
  f_m = m;
  f_T.assign( m + f_s , Cell() );
  draw();
  bool ok = true;
  for( std::size_t i = 0 ; ok && ( i < items.size() ) ; ++i ) {
   Cell c = items[ i ];
   ok = settle( c );
   }
  if( ok ) break;
  ++f_rehashes;
  }
 }

template< class K , class V , class H >
std::optional< V > CuckooHashTable< K , V , H >::lookup( const K & k ) const
{
 const auto p = find( k );
 return( p ? std::optional< V >( f_T[ *p ].value ) : std::nullopt );
 }

template< class K , class V , class H >
void CuckooHashTable< K , V , H >::insert( const K & k , const V & v )
{
 if( const auto p = find( k ) )
  f_T[ *p ].value = v;
 else {
  if( double( f_n + 1 ) > f_maxLoad * double( f_m ) )
   rebuild( 2 * f_m , std::nullopt );      // the global rebuilding
  Cell c{ true , k , v };
  if( ! settle( c ) ) {                     // c holds the homeless key
   ++f_rehashes;
   rebuild( f_m , std::move( c ) );
   }
  ++f_n;
  }
 ADS_POST( contains( k ) );
 ADS_INVARIANT( placed( k ) );
 }

template< class K , class V , class H >
void CuckooHashTable< K , V , H >::remove( const K & k )
{
 if( const auto p = find( k ) ) {
  f_T[ *p ] = Cell();
  --f_n;
  if( ( f_m > f_m0 ) &&
      ( double( f_n ) < f_maxLoad * double( f_m ) / 4 ) )
   rebuild( std::max( f_m0 , f_m / 2 ) , std::nullopt );
  }
 ADS_POST( ! contains( k ) );
 ADS_INVARIANT( placed( k ) );
 }

template< class K , class V , class H >
bool CuckooHashTable< K , V , H >::contains( const K & k ) const
{
 return( find( k ).has_value() );
 }

template< class K , class V , class H >
std::size_t CuckooHashTable< K , V , H >::size( void ) const
{
 return( f_n );
 }

template< class K , class V , class H >
std::size_t CuckooHashTable< K , V , H >::capacity( void ) const
{
 return( f_m );
 }

template< class K , class V , class H >
std::size_t CuckooHashTable< K , V , H >::rehashes( void ) const
{
 return( f_rehashes );
 }

template< class K , class V , class H >
std::size_t CuckooHashTable< K , V , H >::evictions( void ) const
{
 return( f_evictions );
 }

template< class K , class V , class H >
std::size_t CuckooHashTable< K , V , H >::stashed( void ) const
{
 std::size_t c = 0;
 for( std::size_t p = f_m ; p < f_T.size() ; ++p )
  c += f_T[ p ].full;
 return( c );
 }

template< class K , class V , class H >
bool CuckooHashTable< K , V , H >::placed( const K & k ) const
{
 std::size_t c = 0;                         // k at most once
 const std::size_t p1 = place( k , 0 ) , p2 = place( k , 1 );
 c += f_T[ p1 ].full && ( f_T[ p1 ].key == k );
 if( p2 != p1 ) c += f_T[ p2 ].full && ( f_T[ p2 ].key == k );
 for( std::size_t p = f_m ; p < f_T.size() ; ++p )
  c += f_T[ p ].full && ( f_T[ p ].key == k );
 if( c > 1 )
  return( false );
 if( f_T.size() > 1024 )                    // a long sequence stays linear
  return( true );
 std::size_t n = 0;
 for( std::size_t p = 0 ; p < f_T.size() ; ++p )
  if( f_T[ p ].full ) {
   const auto q = find( f_T[ p ].key );
   if( ! q || ( *q != p ) )
    return( false );
   ++n;
   }
 return( n == f_n );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File CuckooHashing.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
