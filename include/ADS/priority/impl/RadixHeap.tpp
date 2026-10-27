/*--------------------------------------------------------------------------*/
/*--------------------------- File RadixHeap.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of RadixHeap [see RadixHeap.h], with their contracts. The
 * hot ones are inline, so that the compiler expands them in the loop of
 * the caller.
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

template< class T , std::integral K , unsigned B >
inline void RadixHeap< T , K , B >::push( const T & x , K k )
{
 const U u = encode( k );
 ADS_PRE( u >= f_last );
 place( bucket( u ) , Entry{ u , x } );
 ++f_size;
 ADS_INVARIANT( bucketed() );
 }

template< class T , std::integral K , unsigned B >
inline void RadixHeap< T , K , B >::pull( void )
{
 ADS_PRE( f_size > 0 );
 if( ! f_B[ f_cur ].empty() && ( f_cur < R ) )
  return;                                   // still the top
 unsigned i = first();
 if( i >= R ) {                             // its least key is the new
  std::vector< Entry > & b = f_B[ i ];      // last, and its items go
  f_last = f_min[ i ];                      // down to the rows below
  f_min[ i ] = std::numeric_limits< U >::max();
  f_mask[ i >> 6 ] &= ~( std::uint64_t( 1 ) << ( i & 63 ) );
  for( Entry & e : b )
   place( bucket( e.key ) , std::move( e ) );
  b.clear();
  i = bucket( f_last );
  }
 else                                       // the keys of a bucket of the
  f_last = ( f_last & ~U( R - 1 ) ) | U( i );   // row 0 are equal
 f_cur = i;
 }

template< class T , std::integral K , unsigned B >
inline const T & RadixHeap< T , K , B >::top( void )
{
 pull();
 return( f_B[ f_cur ].back().item );
 }

template< class T , std::integral K , unsigned B >
inline K RadixHeap< T , K , B >::topKey( void )
{
 pull();
 return( decode( f_last ) );
 }

template< class T , std::integral K , unsigned B >
inline K RadixHeap< T , K , B >::peekKey( void ) const
{
 ADS_PRE( f_size > 0 );
 const unsigned i = first();
 return( decode( i < R ? ( f_last & ~U( R - 1 ) ) | U( i ) : f_min[ i ] ) );
 }

template< class T , std::integral K , unsigned B >
inline T RadixHeap< T , K , B >::pop( void )
{
 pull();
 std::vector< Entry > & b = f_B[ f_cur ];
 T x = std::move( b.back().item );
 b.pop_back();
 if( b.empty() ) {
  f_min[ f_cur ] = std::numeric_limits< U >::max();
  f_mask[ f_cur >> 6 ] &= ~( std::uint64_t( 1 ) << ( f_cur & 63 ) );
  }
 --f_size;
 ADS_INVARIANT( bucketed() );
 return( x );
 }

template< class T , std::integral K , unsigned B >
template< class F >
void RadixHeap< T , K , B >::extractBelow( K limit , F && f )
{
 const U u = encode( limit );
 while( f_size > 0 ) {
  const unsigned i = first();
  std::vector< Entry > & b = f_B[ i ];
  if( f_min[ i ] >= u )                     // nothing below the limit
   break;
  if( ceiling( i ) < u ) {                  // all of them, as they are
   for( Entry & e : b )
    f( std::move( e.item ) , decode( e.key ) );
   f_size -= b.size();
   b.clear();
   f_min[ i ] = std::numeric_limits< U >::max();
   f_mask[ i >> 6 ] &= ~( std::uint64_t( 1 ) << ( i & 63 ) );
   continue;
   }
  if( i >= R ) {                            // a bucket across the limit,
   f_last = f_min[ i ];                     // above the row 0: down to the
   f_min[ i ] = std::numeric_limits< U >::max();   // rows below, for its
   f_mask[ i >> 6 ] &= ~( std::uint64_t( 1 ) << ( i & 63 ) );   // least
   for( Entry & e : b )                     // key (removed next)
    place( bucket( e.key ) , std::move( e ) );
   b.clear();
   continue;
   }
  std::size_t k = 0;                        // those below, and the rest
  U m = std::numeric_limits< U >::max();    // kept in the bucket
  for( std::size_t j = 0 ; j < b.size() ; ++j )
   if( b[ j ].key < u )
    f( std::move( b[ j ].item ) , decode( b[ j ].key ) );
   else {
    m = b[ j ].key < m ? b[ j ].key : m;
    if( k != j )
     b[ k ] = std::move( b[ j ] );
    ++k;
    }
  f_size -= b.size() - k;
  b.erase( b.begin() + std::ptrdiff_t( k ) , b.end() );
  f_min[ i ] = m;
  if( k == 0 ) {                            // all below after all: on to
   f_mask[ i >> 6 ] &= ~( std::uint64_t( 1 ) << ( i & 63 ) );   // the next
   continue;
   }
  break;
  }
 ADS_INVARIANT( bucketed() );
 }

template< class T , std::integral K , unsigned B >
void RadixHeap< T , K , B >::clear( void )
{
 for( auto & b : f_B )
  b.clear();
 f_min = fill_max();
 f_mask = {};
 f_last = 0;
 f_cur = 0;
 f_size = 0;
 }

template< class T , std::integral K , unsigned B >
bool RadixHeap< T , K , B >::bucketed( void ) const
{
 if( f_size > 1024 )
  return( true );
 std::size_t n = 0;
 for( unsigned i = 0 ; i < NB ; ++i ) {
  const bool used = ( f_mask[ i >> 6 ] >> ( i & 63 ) ) & 1;
  if( used == f_B[ i ].empty() )
   return( false );
  U m = std::numeric_limits< U >::max();
  for( const Entry & e : f_B[ i ] ) {
   if( ( e.key < f_last ) || ( bucket( e.key ) != i ) )
    return( false );
   if( e.key < m ) m = e.key;
   }
  if( m != f_min[ i ] )
   return( false );
  n += f_B[ i ].size();
  }
 return( n == f_size );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File RadixHeap.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
