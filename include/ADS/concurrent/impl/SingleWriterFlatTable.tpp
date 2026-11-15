/*--------------------------------------------------------------------------*/
/*--------------------- File SingleWriterFlatTable.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of SingleWriterFlatTable [see SingleWriterFlatTable.h],
 * with their contracts.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <algorithm>
#include <bit>
#include <cstring>

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*------------------------------- THE WORDS --------------------------------*/

template< class K , class V , class H , class Eq , class C >
template< class X >
std::uint64_t SingleWriterFlatTable< K , V , H , Eq , C >::bits( const X & x )
{
 std::uint64_t w = 0;
 std::memcpy( &w , &x , sizeof( X ) );
 return( w );
 }

template< class K , class V , class H , class Eq , class C >
K SingleWriterFlatTable< K , V , H , Eq , C >::keyOf( std::uint64_t w )
{
 K k;
 std::memcpy( &k , &w , sizeof( K ) );
 return( k );
 }

template< class K , class V , class H , class Eq , class C >
V SingleWriterFlatTable< K , V , H , Eq , C >::valueOf( std::uint64_t w )
{
 if constexpr( valueBytes() == 0 ) {
  ( void )w;
  return( V{} );
  }
 else {
  V v;
  std::memcpy( &v , &w , sizeof( V ) );
  return( v );
  }
 }

template< class K , class V , class H , class Eq , class C >
bool SingleWriterFlatTable< K , V , H , Eq , C >::zeroKey( const K & k )
{
 return( bits( k ) == 0 );
 }

template< class K , class V , class H , class Eq , class C >
std::size_t SingleWriterFlatTable< K , V , H , Eq ,
				   C >::home( std::uint64_t h ,
							   const Table & T )
{
 return( std::size_t( h >> ( 64 - T.lg ) ) );
 }

template< class K , class V , class H , class Eq , class C >
std::uint64_t
SingleWriterFlatTable< K , V , H , Eq , C >::hash( const K & k ) const
{
 if constexpr( requires { typename H::is_avalanching; } )
  return( std::uint64_t( f_h( k ) ) );
 else
  return( hashWord( std::uint64_t( f_h( k ) ) ) );
 }

/*------------------------------- THE VECTORS ------------------------------*/

template< class K , class V , class H , class Eq , class C >
SingleWriterFlatTable< K , V , H , Eq , C >::Table::Table( int l , Stamp e )
    : lg( l ) ,
      W( std::make_unique< std::atomic< std::uint64_t >[] >( kWords << l ) )
{
 if constexpr( epochs ) {  // every group in use
  E = std::make_unique< std::atomic< Stamp >[] >( groups() );
  for( std::size_t g = 0 ; g < groups() ; ++g )
   E[ g ].store( e , std::memory_order_relaxed );
  }
 else
  ( void )e;
 }

template< class K , class V , class H , class Eq , class C >
SingleWriterFlatTable< K , V , H , Eq ,
		       C >::SingleWriterFlatTable( std::size_t n ,
								H h , Eq eq )
    : f_h( h ) , f_eq( eq )
{
 f_zero[ 0 ].store( 0 , std::memory_order_relaxed );
 f_zero[ 1 ].store( 0 , std::memory_order_relaxed );
 const int lg = std::max(
   kLeast ,
   int( std::bit_width( std::max< std::size_t >( 2 * n , 1 ) - 1 ) ) );
 if constexpr( epochs )
  f_epoch = 1;
 f_cur = std::make_unique< Table >( lg , now() );
 f_table.store( f_cur.get() , std::memory_order_release );
 }

template< class K , class V , class H , class Eq , class C >
inline bool
SingleWriterFlatTable< K , V , H , Eq , C >::live( const Table & T ,
					       std::size_t g ) const
{
 if constexpr( epochs )
  return( T.E[ g ].load( std::memory_order_acquire ) == f_epoch );
 else {
  ( void )T;
  ( void )g;
  return( true );
  }
 }

template< class K , class V , class H , class Eq , class C >
inline std::uint64_t
SingleWriterFlatTable< K , V , H , Eq , C >::wordAt( const Table & T ,
						 std::size_t i ) const
{
 if( ! live( T , i >> 3 ) )
  return( 0 );
 return( T.W[ kWords * i ].load( std::memory_order_acquire ) );
 }

template< class K , class V , class H , class Eq , class C >
void SingleWriterFlatTable< K , V , H , Eq , C >::grow( int lg )
{
 // the new vector filled while no reader sees it, then published with
 // release; the old one no longer written, kept for the readers on it
 auto N = std::make_unique< Table >( lg , now() );
 const Table & T = *f_cur;
 const std::size_t m = N->places() - 1;
 for( std::size_t i = 0 ; i < T.places() ; ++i ) {
  const std::uint64_t w = wordAt( T , i );
  if( w == 0 )
   continue;
  std::size_t j = home( hash( keyOf( w ) ) , *N );
  while( N->W[ kWords * j ].load( std::memory_order_relaxed ) != 0 )
   j = ( j + 1 ) & m;
  if constexpr( kWords == 2 )
   N->W[ 2 * j + 1 ].store( T.W[ 2 * i + 1 ].load( std::memory_order_relaxed ) ,
			    std::memory_order_relaxed );
  N->W[ kWords * j ].store( w , std::memory_order_relaxed );
  }
 f_table.store( N.get() , std::memory_order_release );
 f_old.push_back( std::move( f_cur ) );
 f_cur = std::move( N );
 f_rehashes.store( f_rehashes.load( std::memory_order_relaxed ) + 1 ,
		   std::memory_order_relaxed );
 }

template< class K , class V , class H , class Eq , class C >
void SingleWriterFlatTable< K , V , H , Eq , C >::reserve( std::size_t n )
{
 const int lg =
   int( std::bit_width( std::max< std::size_t >( 2 * n , 1 ) - 1 ) );
 if( lg > f_cur->lg )
  grow( lg );
 }

/*------------------------------ THE LOOKUPS -------------------------------*/

template< class K , class V , class H , class Eq , class C >
auto SingleWriterFlatTable< K , V , H , Eq , C >::locate( const K & k ,
						      std::uint64_t h ) const
  -> const std::atomic< std::uint64_t > *
{
 ADS_AUDIT( hash( k ) == h );
 if( zeroKey( k ) )
  return( f_zero[ 0 ].load( std::memory_order_acquire ) ? f_zero : nullptr );
 for( ; ; ) {
  const Table * T = f_table.load( std::memory_order_acquire );
  const std::size_t m = T->places() - 1;
  for( std::size_t i = home( h , *T ) , r = 0 ; r <= m ; i = ( i + 1 ) & m ,
       ++r ) {
   const std::uint64_t w = wordAt( *T , i );
   if( w == 0 )
    break;
   if( f_eq( keyOf( w ) , k ) )
    return( T->W.get() + kWords * i );
   }
  // absent at the free place read, unless the table grew meanwhile
  if( f_table.load( std::memory_order_acquire ) == T )
   return( nullptr );
  }
 }

template< class K , class V , class H , class Eq , class C >
std::optional< V >
SingleWriterFlatTable< K , V , H , Eq , C >::find( const K & k ,
					       std::uint64_t h ) const
{
 const auto * p = locate( k , h );
 if( ! p )
  return( std::nullopt );
 if constexpr( kWords == 2 )
  return( std::optional< V >(
    valueOf( p[ 1 ].load( std::memory_order_acquire ) ) ) );
 else
  return( std::optional< V >( V{} ) );
 }

template< class K , class V , class H , class Eq , class C >
std::optional< V >
SingleWriterFlatTable< K , V , H , Eq , C >::find( const K & k ) const
{
 return( find( k , hash( k ) ) );
 }

template< class K , class V , class H , class Eq , class C >
std::optional< K >
SingleWriterFlatTable< K , V , H , Eq , C >::findKey( const K & k ,
						  std::uint64_t h ) const
{
 const auto * p = locate( k , h );
 if( ! p )
  return( std::nullopt );
 if( zeroKey( k ) )  // kept apart, the bytes of k
  return( std::optional< K >( k ) );
 return(
   std::optional< K >( keyOf( p->load( std::memory_order_acquire ) ) ) );
 }

template< class K , class V , class H , class Eq , class C >
std::optional< K >
SingleWriterFlatTable< K , V , H , Eq , C >::findKey( const K & k ) const
{
 return( findKey( k , hash( k ) ) );
 }

template< class K , class V , class H , class Eq , class C >
bool SingleWriterFlatTable< K , V , H , Eq , C >::contains(
  const K & k , std::uint64_t h ) const
{
 return( locate( k , h ) != nullptr );
 }

template< class K , class V , class H , class Eq , class C >
bool SingleWriterFlatTable< K , V , H , Eq , C >::contains( const K & k ) const
{
 return( locate( k , hash( k ) ) != nullptr );
 }

/*------------------------------ THE INSERTION -----------------------------*/

template< class K , class V , class H , class Eq , class C >
auto SingleWriterFlatTable< K , V , H , Eq , C >::place( Table & T ,
							 const K & k ,
						     std::uint64_t h )
  -> std::atomic< std::uint64_t > *
{
 const std::size_t m = T.places() - 1;
 for( std::size_t i = home( h , T ) ; ; i = ( i + 1 ) & m ) {
  if constexpr( epochs )
   if( T.E[ i >> 3 ].load( std::memory_order_relaxed ) != f_epoch ) {
    // renewed: its words 0, then its stamp with release, which a reader
    // reads with acquire before the words of the group
    const std::size_t g = i >> 3;
    for( std::size_t j = 8 * kWords * g ; j < 8 * kWords * ( g + 1 ) ; ++j )
     T.W[ j ].store( 0 , std::memory_order_relaxed );
    T.E[ g ].store( f_epoch , std::memory_order_release );
    }
  const std::uint64_t w = T.W[ kWords * i ].load( std::memory_order_relaxed );
  if( ( w == 0 ) || f_eq( keyOf( w ) , k ) )
   return( T.W.get() + kWords * i );
  }
 }

template< class K , class V , class H , class Eq , class C >
auto SingleWriterFlatTable< K , V , H , Eq , C >::findOrInsert( const K & k ,
							    std::uint64_t h ,
							    const V & v )
  -> std::pair< V , bool >
{
 ADS_AUDIT( hash( k ) == h );
 const std::size_t n = f_size.load( std::memory_order_relaxed );
 if( zeroKey( k ) ) {  // kept apart
  if( f_zero[ 0 ].load( std::memory_order_relaxed ) )
   return( std::pair< V , bool >(
     valueOf( f_zero[ 1 ].load( std::memory_order_relaxed ) ) , false ) );
  f_zero[ 1 ].store( bits( v ) , std::memory_order_relaxed );
  f_zero[ 0 ].store( 1 , std::memory_order_release );
  f_size.store( n + 1 , std::memory_order_relaxed );
  return( std::pair< V , bool >( v , true ) );
  }
 if( 2 * ( n + 1 ) > f_cur->places() )
  grow( f_cur->lg + 1 );
 std::atomic< std::uint64_t > * p = place( *f_cur , k , h );
 if( const std::uint64_t w = p->load( std::memory_order_relaxed ) ) {
  ( void )w;
  if constexpr( kWords == 2 )
   return( std::pair< V , bool >(
     valueOf( p[ 1 ].load( std::memory_order_relaxed ) ) , false ) );
  else
   return( std::pair< V , bool >( V{} , false ) );
  }
 if constexpr( kWords == 2 )
  p[ 1 ].store( bits( v ) , std::memory_order_relaxed );
 p->store( bits( k ) , std::memory_order_release );  // the pair published
 f_size.store( n + 1 , std::memory_order_relaxed );
 return( std::pair< V , bool >( v , true ) );
 }

template< class K , class V , class H , class Eq , class C >
auto SingleWriterFlatTable< K , V , H , Eq , C >::findOrInsert( const K & k ,
							    const V & v )
  -> std::pair< V , bool >
{
 return( findOrInsert( k , hash( k ) , v ) );
 }

template< class K , class V , class H , class Eq , class C >
std::pair< K , bool >
SingleWriterFlatTable< K , V , H , Eq , C >::findOrInsertKey( const K & k ,
							  std::uint64_t h )
 requires( valueBytes() == 0 )
{
 const bool in = findOrInsert( k , h , V{} ).second;
 if( in || zeroKey( k ) )
  return( std::pair< K , bool >( k , in ) );
 return( std::pair< K , bool >(
   keyOf( place( *f_cur , k , h )->load( std::memory_order_relaxed ) ) ,
   false ) );
 }

template< class K , class V , class H , class Eq , class C >
bool SingleWriterFlatTable< K , V , H , Eq , C >::insert( const K & k ,
						      const V & v )
{
 return( findOrInsert( k , hash( k ) , v ).second );
 }

template< class K , class V , class H , class Eq , class C >
template< class F >
bool SingleWriterFlatTable< K , V , H , Eq , C >::update( const K & k ,
							  F && f )
 requires( valueBytes() > 0 )
{
 std::atomic< std::uint64_t > * p;
 if( zeroKey( k ) ) {
  if( ! f_zero[ 0 ].load( std::memory_order_relaxed ) )
   return( false );
  p = f_zero;
  }
 else {
  p = place( *f_cur , k , hash( k ) );
  if( p->load( std::memory_order_relaxed ) == 0 )
   return( false );
  }
 const V x = valueOf( p[ 1 ].load( std::memory_order_relaxed ) );
 p[ 1 ].store( bits( V( f( x ) ) ) , std::memory_order_release );
 return( true );
 }

/*-------------------------------- THE REST --------------------------------*/

template< class K , class V , class H , class Eq , class C >
void SingleWriterFlatTable< K , V , H , Eq , C >::clear( void )
{
 Table & T = *f_cur;
 bool all = true;
 if constexpr( epochs ) {
  // every group out of use at once; at the largest stamp, the whole
  // vector written and the epochs again from 1
  all = Stamp( f_epoch + 1 ) == kLast;
  f_epoch = all ? Stamp( 1 ) : Stamp( f_epoch + 1 );
  if( all )
   for( std::size_t g = 0 ; g < T.groups() ; ++g )
    T.E[ g ].store( f_epoch , std::memory_order_relaxed );
  }
 if( all )
  for( std::size_t i = 0 ; i < kWords * T.places() ; ++i )
   T.W[ i ].store( 0 , std::memory_order_relaxed );
 f_zero[ 0 ].store( 0 , std::memory_order_relaxed );
 f_zero[ 1 ].store( 0 , std::memory_order_relaxed );
 f_size.store( 0 , std::memory_order_relaxed );
 f_old.clear();
 }

template< class K , class V , class H , class Eq , class C >
template< class F >
void SingleWriterFlatTable< K , V , H , Eq , C >::forEach( F && f ) const
{
 if( f_zero[ 0 ].load( std::memory_order_acquire ) )
  f( keyOf( 0 ) , valueOf( f_zero[ 1 ].load( std::memory_order_acquire ) ) );
 const Table & T = *f_table.load( std::memory_order_acquire );
 for( std::size_t i = 0 ; i < T.places() ; ++i )
  if( const std::uint64_t w = wordAt( T , i ) ) {
   if constexpr( kWords == 2 )
    f( keyOf( w ) ,
       valueOf( T.W[ 2 * i + 1 ].load( std::memory_order_acquire ) ) );
   else
    f( keyOf( w ) , V{} );
   }
 }

template< class K , class V , class H , class Eq , class C >
bool SingleWriterFlatTable< K , V , H , Eq , C >::consistent( void ) const
{
 const Table & T = *f_table.load( std::memory_order_acquire );
 if( &T != f_cur.get() )
  return( false );
 const std::size_t m = T.places() - 1;
 std::size_t n = f_zero[ 0 ].load( std::memory_order_acquire ) ? 1 : 0;
 for( std::size_t i = 0 ; i < T.places() ; ++i ) {
  const std::uint64_t w = wordAt( T , i );
  if( w == 0 )
   continue;
  ++n;
  for( std::size_t j = home( hash( keyOf( w ) ) , T ) ; j != i ;
       j = ( j + 1 ) & m ) {
   const std::uint64_t x = wordAt( T , j );
   if( ( x == 0 ) || f_eq( keyOf( x ) , keyOf( w ) ) )
    return( false );  // a free place in between, or the key twice
   }
  }
 return( ( n == size() ) && ( 2 * n <= T.places() ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------- End File SingleWriterFlatTable.tpp -------------------*/
/*--------------------------------------------------------------------------*/
