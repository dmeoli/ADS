/*--------------------------------------------------------------------------*/
/*------------------------- File OpenHashTable.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of Probing and of OpenHashTable [see OpenHashTable.h],
 * with their contracts.
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

/*--------------------------------------------------------------------------*/
/*------------------------------- PROBING ----------------------------------*/
/*--------------------------------------------------------------------------*/

inline Probing::Probing( Kind k , long c1 , long c2 )
 : f_kind( k ) , f_c1( c1 ) , f_c2( c2 )
{
 }

inline Probing Probing::linear( long delta )
{
 return( Probing( Kind::LINEAR , delta , 0 ) );
 }

inline Probing Probing::quadratic( long c1 , long c2 )
{
 return( Probing( Kind::QUADRATIC , c1 , c2 ) );
 }

inline Probing Probing::double_hashing( void )
{
 return( Probing( Kind::DOUBLE , 0 , 0 ) );
 }

inline Probing Probing::double_hashing( Step g )
{
 Probing p( Kind::DOUBLE , 0 , 0 );
 p.f_g = std::move( g );
 return( p );
 }

inline Probing Probing::triangular( void )
{
 return( Probing( Kind::TRIANGULAR , 0 , 0 ) );
 }

inline Probing Probing::pseudo_random( unsigned long seed )
{
 Probing p( Kind::RANDOM , 0 , 0 );
 p.f_seed = seed;
 return( p );
 }

inline Probing::Kind Probing::kind( void ) const
{
 return( f_kind );
 }

inline std::size_t Probing::at( std::size_t h , std::size_t i ,
                                std::size_t m ) const
{
 ADS_PRE( m > 0 );
 const std::size_t h0 = h % m;
 std::size_t r;
 switch( f_kind ) {
  case( Kind::LINEAR ):
   r = ( h0 + mod( f_c1 , m ) * ( i % m ) ) % m;
   break;
  case( Kind::QUADRATIC ): {
   const std::size_t j = i % m;
   r = ( h0 + mod( f_c1 , m ) * j + mod( f_c2 , m ) * ( j * j % m ) ) % m;
   break;
   }
  case( Kind::TRIANGULAR ): {
   const std::size_t j = i % ( 2 * m );
   r = ( h0 + j * ( j + 1 ) / 2 % m ) % m;
   break;
   }
  case( Kind::RANDOM ):
   r = ( h0 + permutation( m )[ i % m ] ) % m;
   break;
  default:
   r = ( h0 + step( f_g ? f_g( h ) : h , m ) * ( i % m ) ) % m;
  }
 ADS_POST( r < m );
 return( r );
 }

inline std::size_t Probing::step( std::size_t h , std::size_t m )
{
 ADS_PRE( m > 0 );
 if( m == 1 )
  return( 1 );
 if( is_pow2( m ) )
  return( ( ( h / m ) % m ) | 1 );
 return( 1 + h % ( m - 1 ) );
 }

inline bool Probing::works( std::size_t m ) const
{
 if( m == 0 )
  return( false );
 switch( f_kind ) {
  case( Kind::LINEAR ): return( std::gcd( mod( f_c1 , m ) , m ) == 1 );
  case( Kind::QUADRATIC ): return( is_prime( m ) );
  case( Kind::TRIANGULAR ): return( is_pow2( m ) );
  case( Kind::RANDOM ): return( true );
  default: return( ( m == 1 ) || is_prime( m ) || is_pow2( m ) );
  }
 }

inline void Probing::check( std::size_t m ) const
{
 ADS_PRE( works( m ) );
 }

inline const std::vector< std::size_t > &
Probing::permutation( std::size_t m ) const
{
 if( f_m != m ) {                    // Fisher-Yates on 1 , ... , m - 1
  f_p.resize( m );
  for( std::size_t i = 0 ; i < m ; ++i )
   f_p[ i ] = i;
  std::mt19937_64 g( f_seed ^ ( 0x9e3779b97f4a7c15ULL * m ) );
  for( std::size_t i = m - 1 ; i > 1 ; --i )
   std::swap( f_p[ i ] , f_p[ 1 + g() % i ] );
  f_m = m;
  }
 return( f_p );
 }

inline std::size_t Probing::mod( long c , std::size_t m )
{
 const long r = c % long( m );
 return( r < 0 ? r + m : r );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- THE DICTIONARY ------------------------------*/
/*--------------------------------------------------------------------------*/

template< class K , class V , class H >
OpenHashTable< K , V , H >::OpenHashTable( std::size_t m , Probing probe ,
                                           bool restructure , H h )
 : f_T( m ) , f_probe( probe ) , f_m0( m ) , f_restructure( restructure ) ,
   f_h( h )
{
 ADS_PRE( ( m > 0 ) && f_probe.works( m ) );
 }

template< class K , class V , class H >
std::optional< V > OpenHashTable< K , V , H >::lookup( const K & k ) const
{
 const auto i = scan( k , false );
 if( is( i , k ) )
  return( f_T[ i ].value );
 return( std::nullopt );
 }

template< class K , class V , class H >
void OpenHashTable< K , V , H >::insert( const K & k , const V & v )
{
 const auto i = scan( k , true );
 auto & c = f_T[ i ];
 if( c.slot == Slot::FULL && ! ( c.key == k ) )
  throw( std::overflow_error( "ADS::OpenHashTable::insert: no place" ) );
 if( c.slot != Slot::FULL ) {
  if( c.slot == Slot::DELETED )
   --f_deleted;
  ++f_size;
  }
 c = { Slot::FULL , k , v };
 if( f_restructure && ( 2 * ( f_size + f_deleted ) > f_T.size() ) )
  rebuild( resized( f_T.size() , true , f_m0 ) );
 ADS_POST( where( k ) < f_T.size() );
 ADS_INVARIANT( consistent( k ) );
 }

template< class K , class V , class H >
void OpenHashTable< K , V , H >::remove( const K & k )
{
 const auto i = scan( k , false );
 if( ! is( i , k ) )
  return;
 f_T[ i ].slot = Slot::DELETED;
 --f_size;
 ++f_deleted;
 if( f_restructure && ( f_T.size() > f_m0 ) && ( 8 * f_size < f_T.size() ) )
  rebuild( resized( f_T.size() , false , f_m0 ) );
 ADS_POST( where( k ) == f_T.size() );
 ADS_INVARIANT( consistent( k ) );
 }

template< class K , class V , class H >
bool OpenHashTable< K , V , H >::contains( const K & k ) const
{
 return( is( scan( k , false ) , k ) );
 }

template< class K , class V , class H >
std::size_t OpenHashTable< K , V , H >::size( void ) const
{
 return( f_size );
 }

template< class K , class V , class H >
bool OpenHashTable< K , V , H >::isEmpty( void ) const
{
 return( f_size == 0 );
 }

template< class K , class V , class H >
std::size_t OpenHashTable< K , V , H >::capacity( void ) const
{
 return( f_T.size() );
 }

template< class K , class V , class H >
double OpenHashTable< K , V , H >::load( void ) const
{
 return( double( f_size ) / f_T.size() );
 }

template< class K , class V , class H >
template< class F >
void OpenHashTable< K , V , H >::forEach( F && f ) const
{
 for( const auto & c : f_T )
  if( c.slot == Slot::FULL )
   f( c.key , c.value );
 }

template< class K , class V , class H >
typename OpenHashTable< K , V , H >::Slot
OpenHashTable< K , V , H >::slot( std::size_t i ) const
{
 ADS_PRE( i < f_T.size() );
 return( f_T[ i ].slot );
 }

template< class K , class V , class H >
const K & OpenHashTable< K , V , H >::keyAt( std::size_t i ) const
{
 ADS_PRE( ( i < f_T.size() ) && ( f_T[ i ].slot == Slot::FULL ) );
 return( f_T[ i ].key );
 }

template< class K , class V , class H >
std::size_t OpenHashTable< K , V , H >::probes( void ) const
{
 return( f_probes );
 }

template< class K , class V , class H >
std::size_t OpenHashTable< K , V , H >::restructures( void ) const
{
 return( f_rebuilds );
 }

template< class K , class V , class H >
std::size_t OpenHashTable< K , V , H >::moved( void ) const
{
 return( f_moved );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE PLACES --------------------------------*/
/*--------------------------------------------------------------------------*/

template< class K , class V , class H >
bool OpenHashTable< K , V , H >::is( std::size_t i , const K & k ) const
{
 return( ( f_T[ i ].slot == Slot::FULL ) && ( f_T[ i ].key == k ) );
 }

template< class K , class V , class H >
std::size_t OpenHashTable< K , V , H >::scan( const K & k , bool insert ) const
{
 const std::size_t m = f_T.size();
 const std::size_t h = f_h( k );
 long firstDeleted = -1;
 std::size_t i = 0;
 std::size_t j = f_probe.at( h , 0 , m );
 while( ! is( j , k ) && ( f_T[ j ].slot != Slot::NIL ) && ( i < m ) ) {
  if( ( f_T[ j ].slot == Slot::DELETED ) && ( firstDeleted == -1 ) )
   firstDeleted = j;
  j = f_probe.at( h , ++i , m );
  }
 f_probes = std::min( i + 1 , m );
 if( insert && ! is( j , k ) && ( firstDeleted != -1 ) )
  return( firstDeleted );
 return( j );
 }

template< class K , class V , class H >
std::size_t OpenHashTable< K , V , H >::where( const K & k ) const
{
 const std::size_t m = f_T.size() , h = f_h( k );
 for( std::size_t i = 0 ; i < m ; ++i ) {
  const std::size_t j = f_probe.at( h , i , m );
  if( is( j , k ) )
   return( j );
  if( f_T[ j ].slot == Slot::NIL )
   break;
  }
 return( m );
 }

template< class K , class V , class H >
bool OpenHashTable< K , V , H >::consistent( const K & k ) const
{
 const std::size_t m = f_T.size();
 if( m > 1024 ) {
  const std::size_t h = f_h( k );
  for( std::size_t i = 0 ; i < m ; ++i ) {
   const std::size_t j = f_probe.at( h , i , m );
   if( f_T[ j ].slot == Slot::NIL )
    break;
   if( ( f_T[ j ].slot == Slot::FULL ) && ( where( f_T[ j ].key ) != j ) )
    return( false );
   }
  return( true );
  }
 std::size_t full = 0 , deleted = 0;
 for( std::size_t i = 0 ; i < m ; ++i )
  if( f_T[ i ].slot == Slot::FULL ) {
   ++full;
   if( where( f_T[ i ].key ) != i )
    return( false );
   }
  else
   deleted += f_T[ i ].slot == Slot::DELETED;
 return( ( full == f_size ) && ( deleted == f_deleted ) );
 }

template< class K , class V , class H >
void OpenHashTable< K , V , H >::rebuild( std::size_t m )
{
 f_probe.check( m );
 std::vector< Cell > T( m );
 std::swap( f_T , T );
 f_size = f_deleted = 0;
 for( auto & c : T )
  if( c.slot == Slot::FULL ) {
   f_T[ scan( c.key , true ) ] = c;
   ++f_size;
   }
 ++f_rebuilds;
 f_moved += f_size;
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File OpenHashTable.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
