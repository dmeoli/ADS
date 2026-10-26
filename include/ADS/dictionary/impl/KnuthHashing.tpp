/*--------------------------------------------------------------------------*/
/*------------------------- File KnuthHashing.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of CoalescedHashTable and BrentHashTable [see
 * KnuthHashing.h], with their contracts.
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

/*--------------------------------------------------------------------------*/
/*------------------------------- COALESCED --------------------------------*/
/*--------------------------------------------------------------------------*/

template< class K , class V , class H >
CoalescedHashTable< K , V , H >::CoalescedHashTable( std::size_t m ,
                                                     std::size_t cellar ,
                                                     H h )
 : f_T( m + cellar ) , f_m( m ) , f_free( long( m + cellar ) ) , f_h( h )
{
 ADS_PRE( m >= 1 );
 }

template< class K , class V , class H >
std::optional< V > CoalescedHashTable< K , V , H >::lookup( const K & k )
 const
{
 f_probes = 0;
 long i = long( f_h( k ) % f_m );
 if( ! f_T[ i ].used )
  return( std::nullopt );
 for( ; i >= 0 ; i = f_T[ i ].link ) {
  ++f_probes;
  if( f_T[ i ].key == k )
   return( f_T[ i ].value );
  }
 return( std::nullopt );
 }

template< class K , class V , class H >
void CoalescedHashTable< K , V , H >::insert( const K & k , const V & v )
{
 f_probes = 1;
 long i = long( f_h( k ) % f_m );
 if( f_T[ i ].used ) {
  for( ; ; i = f_T[ i ].link , ++f_probes ) {   // C2-C3: along the list
   if( f_T[ i ].key == k ) {
    f_T[ i ].value = v;
    ADS_POST( lookup( k ).has_value() );
    return;
    }
   if( f_T[ i ].link < 0 )
    break;
   }
  do --f_free;                              // C4: the empty place of
  while( ( f_free >= 0 ) && f_T[ f_free ].used );   // largest index
  ADS_PRE( f_free >= 0 );                   // the table is not full
  f_T[ i ].link = f_free;                   // C5: linked at the end
  i = f_free;
  }
 f_T[ i ].used = true;
 f_T[ i ].key = k;
 f_T[ i ].value = v;
 f_T[ i ].link = -1;
 ++f_size;
 ADS_POST( lookup( k ).has_value() );
 ADS_AUDIT( listed() );
 }

template< class K , class V , class H >
std::size_t CoalescedHashTable< K , V , H >::size( void ) const
{
 return( f_size );
 }

template< class K , class V , class H >
std::size_t CoalescedHashTable< K , V , H >::probes( void ) const
{
 return( f_probes );
 }

template< class K , class V , class H >
bool CoalescedHashTable< K , V , H >::listed( void ) const
{
 for( const auto & s : f_T )
  if( s.used ) {
   long i = long( f_h( s.key ) % f_m );
   while( ( i >= 0 ) && !( f_T[ i ].key == s.key ) ) i = f_T[ i ].link;
   if( i < 0 ) return( false );
   }
 return( true );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- BRENT ----------------------------------*/
/*--------------------------------------------------------------------------*/

template< class K , class V , class H >
BrentHashTable< K , V , H >::BrentHashTable( std::size_t m , H h )
 : f_T( m ) , f_h( h )
{
 ADS_PRE( m >= 2 );
 ADS_PRE( [ m ]( void ) {
  for( std::size_t d = 2 ; d * d <= m ; ++d ) if( m % d == 0 ) return( false );
  return( true ); }() );
 }

template< class K , class V , class H >
std::size_t BrentHashTable< K , V , H >::at( const K & k ,
                                             std::size_t i ) const
{
 const std::size_t m = f_T.size() , h = f_h( k );
 return( ( h % m + i * ( 1 + h % ( m - 1 ) ) ) % m );   // double hashing
 }

template< class K , class V , class H >
std::optional< V > BrentHashTable< K , V , H >::lookup( const K & k ) const
{
 f_probes = 0;
 for( std::size_t i = 0 ; i < f_T.size() ; ++i ) {
  const auto & s = f_T[ at( k , i ) ];
  ++f_probes;
  if( ! s )
   return( std::nullopt );
  if( s->first == k )
   return( s->second );
  }
 return( std::nullopt );
 }

template< class K , class V , class H >
void BrentHashTable< K , V , H >::insert( const K & k , const V & v )
{
 const std::size_t m = f_T.size();
 for( std::size_t i = 0 ; i < m ; ++i ) {   // already there: a new value
  auto & s = f_T[ at( k , i ) ];
  if( ! s ) break;
  if( s->first == k ) {
   s->second = v;
   return;
   }
  }
 ADS_PRE( f_size < m );
 for( std::size_t s = 0 ; s < m ; ++s )     // the least s , then j
  for( std::size_t j = 0 ; j <= s ; ++j ) {
   const std::size_t p = at( k , j );
   if( j == s ) {
    if( ! f_T[ p ] ) {                      // K at its s-th place
     f_T[ p ] = std::pair( k , v );
     ++f_size;
     ADS_POST( lookup( k ).has_value() );
     return;
     }
    continue;
    }
   const K & o = f_T[ p ]->first;           // the key in the j-th place
   std::size_t i0 = 0;                      // of K: its own index there
   while( at( o , i0 ) != p ) ++i0;
   const std::size_t q = at( o , i0 + ( s - j ) );
   if( ! f_T[ q ] ) {                       // o moves s - j places on
    f_T[ q ] = std::move( f_T[ p ] );
    f_T[ p ] = std::pair( k , v );
    ++f_size;
    ADS_POST( lookup( k ).has_value() );
    return;
    }
   }
 }

template< class K , class V , class H >
std::size_t BrentHashTable< K , V , H >::size( void ) const
{
 return( f_size );
 }

template< class K , class V , class H >
std::size_t BrentHashTable< K , V , H >::probes( void ) const
{
 return( f_probes );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File KnuthHashing.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
