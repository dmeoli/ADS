/*--------------------------------------------------------------------------*/
/*-------------------------- File Allocators.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of BoundaryTagAllocator and BuddyAllocator [see
 * Allocators.h], with their contracts.
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
/*----------------------------- BOUNDARY TAGS ------------------------------*/
/*--------------------------------------------------------------------------*/

inline BoundaryTagAllocator::BoundaryTagAllocator( long n , Fit fit )
 : M( n + 2 , 0 ) , N( n ) , f_fit( fit ) , f_rover( n )
{
 ADS_PRE( n >= 4 );
 M[ N ] = M[ N + 1 ] = N;                   // the empty list: head alone
 M[ 0 ] = M[ N - 1 ] = N;                   // one free block of N words
 link( 0 );
 ADS_AUDIT( tiled() );
 }

inline void BoundaryTagAllocator::link( long p )
{
 const long head = N;                       // after the head: next at
 M[ p + 1 ] = M[ head ];                    // M[ head ] , prev at
 M[ p + 2 ] = head;                         // M[ head + 1 ]
 if( M[ head ] == head ) M[ head + 1 ] = p;
 else M[ M[ head ] + 2 ] = p;
 M[ head ] = p;
 }

inline void BoundaryTagAllocator::unlink( long p )
{
 const long nx = M[ p + 1 ] , pv = M[ p + 2 ];
 if( pv == N ) M[ N ] = nx; else M[ pv + 1 ] = nx;
 if( nx == N ) M[ N + 1 ] = pv; else M[ nx + 2 ] = pv;
 if( f_rover == p ) f_rover = nx;
 }

inline long BoundaryTagAllocator::allocate( long n )
{
 ADS_PRE( n >= 1 );
 if( n > N ) return( -1 );                  // no n + 2 past a long
 const long need = n + 2;                   // the two tags
 const long s0 = need < 4 ? 4 : need;       // room for the links when free
 long best = -1;
 auto fits = [ & ]( long p ) { return( M[ p ] >= s0 ); };
 if( f_fit == Fit::NEXT ) {                 // from the rover to the end,
  const long start = f_rover == N ? M[ N ] : f_rover;   // then from the
  for( long p = start ; p != N ; p = M[ p + 1 ] )       // head
   if( fits( p ) ) { best = p; break; }
  if( best < 0 )
   for( long p = M[ N ] ; ( p != start ) && ( p != N ) ; p = M[ p + 1 ] )
    if( fits( p ) ) { best = p; break; }
  }
 else
  for( long p = M[ N ] ; p != N ; p = M[ p + 1 ] )
   if( fits( p ) && ( ( best < 0 ) || ( ( f_fit == Fit::BEST ) &&
                                        ( M[ p ] < M[ best ] ) ) ) ) {
    best = p;
    if( f_fit == Fit::FIRST ) break;
    }
 if( best < 0 )
  return( -1 );
 const long s = M[ best ];
 long a;
 if( s - s0 >= 4 ) {                        // split: the upper part goes
  M[ best ] = M[ best + s - s0 - 1 ] = s - s0;   // to the request
  a = best + s - s0;
  M[ a ] = M[ a + s0 - 1 ] = -s0;
  f_rover = M[ best + 1 ];
  }
 else {                                     // the whole block
  unlink( best );
  a = best;
  M[ a ] = M[ a + s - 1 ] = -s;
  }
 f_used.insert( a );
 ADS_AUDIT( tiled() );
 return( a + 1 );                           // the first word of the user
 }

inline void BoundaryTagAllocator::free( long a )
{
 long p = a - 1;
 ADS_PRE( f_used.count( p ) == 1 );
 f_used.erase( p );
 long s = -M[ p ];
 if( ( p > 0 ) && ( M[ p - 1 ] > 0 ) ) {    // the block below is free
  const long q = p - M[ p - 1 ];
  unlink( q );
  s += M[ q ];
  p = q;
  }
 if( ( p + s < N ) && ( M[ p + s ] > 0 ) ) {   // the block above is free
  const long q = p + s;
  unlink( q );
  s += M[ q ];
  }
 M[ p ] = M[ p + s - 1 ] = s;
 link( p );
 ADS_AUDIT( tiled() );
 }

inline long BoundaryTagAllocator::freeWords( void ) const
{
 long w = 0;
 for( long p = M[ N ] ; p != N ; p = M[ p + 1 ] ) w += M[ p ];
 return( w );
 }

inline long BoundaryTagAllocator::freeBlocks( void ) const
{
 long b = 0;
 for( long p = M[ N ] ; p != N ; p = M[ p + 1 ] ) ++b;
 return( b );
 }

inline bool BoundaryTagAllocator::tiled( void ) const
{
 long free = 0 , onList = 0;
 bool prevFree = false;
 for( long p = 0 ; p < N ; ) {              // the blocks tile [ 0 , N )
  const long s = M[ p ] > 0 ? M[ p ] : -M[ p ];
  if( ( s < 1 ) || ( p + s > N ) || ( M[ p + s - 1 ] != M[ p ] ) )
   return( false );
  if( M[ p ] > 0 ) {
   if( prevFree ) return( false );          // two adjacent free blocks
   free += s;
   prevFree = true;
   }
  else
   prevFree = false;
  p += s;
  }
 long blocks = 0;
 for( long p = 0 ; p < N ; p += M[ p ] > 0 ? M[ p ] : -M[ p ] )
  blocks += M[ p ] > 0;
 for( long p = M[ N ] ; p != N ; p = M[ p + 1 ] )
  if( ( M[ p ] <= 0 ) || ( ++onList > N ) ) return( false );
 return( ( free == freeWords() ) && ( onList == blocks ) );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- BUDDY ----------------------------------*/
/*--------------------------------------------------------------------------*/

inline BuddyAllocator::BuddyAllocator( int m ) : f_m( m ) , f_free( m + 1 )
{
 ADS_PRE( ( m >= 0 ) && ( m <= 40 ) &&
          ( m < std::numeric_limits< long >::digits ) );
 f_free[ m ].insert( 0 );
 }

inline long BuddyAllocator::allocate( long n )
{
 ADS_PRE( n >= 1 );
 if( n > ( 1LL << f_m ) ) return( -1 );     // no 2^k past a long
 int k = 0;                                 // the least 2^k >= n
 while( ( 1LL << k ) < n ) ++k;
 int j = k;                                 // R1: the least free block
 while( ( j <= f_m ) && f_free[ j ].empty() ) ++j;   // of 2^j >= 2^k
 if( j > f_m )
  return( -1 );
 const long a = *f_free[ j ].begin();
 f_free[ j ].erase( f_free[ j ].begin() );
 while( j > k ) {                           // R3: split, the upper halves
  --j;                                      // free
  f_free[ j ].insert( a + ( 1L << j ) );
  }
 f_used[ a ] = k;
 ADS_AUDIT( merged() );
 return( a );
 }

inline void BuddyAllocator::free( long a )
{
 const auto it = f_used.find( a );
 ADS_PRE( it != f_used.end() );
 int k = it->second;
 f_used.erase( it );
 while( k < f_m ) {                         // S1: merge with the buddy
  const long b = a ^ ( 1L << k );           // while it is free
  const auto f = f_free[ k ].find( b );
  if( f == f_free[ k ].end() ) break;
  f_free[ k ].erase( f );
  a = a < b ? a : b;
  ++k;
  }
 f_free[ k ].insert( a );
 ADS_AUDIT( merged() );
 }

inline long BuddyAllocator::freeWords( void ) const
{
 long w = 0;
 for( int k = 0 ; k <= f_m ; ++k ) w += long( f_free[ k ].size() ) << k;
 return( w );
 }

inline long BuddyAllocator::blockSize( long a ) const
{
 const auto it = f_used.find( a );
 ADS_PRE( it != f_used.end() );
 return( 1L << it->second );
 }

inline bool BuddyAllocator::merged( void ) const
{
 std::map< long , long > blocks;            // address -> end
 for( int k = 0 ; k <= f_m ; ++k )
  for( long a : f_free[ k ] ) {
   if( ( a % ( 1L << k ) ) != 0 ) return( false );
   if( ( k < f_m ) && f_free[ k ].count( a ^ ( 1L << k ) ) )
    return( false );                        // two free buddies
   blocks[ a ] = a + ( 1L << k );
   }
 for( const auto & [ a , k ] : f_used ) blocks[ a ] = a + ( 1L << k );
 long end = 0;                              // all disjoint, covering
 for( const auto & [ a , e ] : blocks ) {
  if( a != end ) return( false );
  end = e;
  }
 return( end == ( 1L << f_m ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File Allocators.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
