/*--------------------------------------------------------------------------*/
/*-------------------- File SpaceEfficientSortedSet.tpp --------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of SpaceEfficientSortedSet [see
 * `SpaceEfficientSortedSet.h`], with their contracts.
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

template< std::totally_ordered T >
 requires std::semiregular< T >
SpaceEfficientSortedSet< T >::SpaceEfficientSortedSet(
 std::size_t b , std::uint64_t seed )
 : f_b( b ) , f_index( seed )
{
 ADS_PRE( b >= 3 );
 }

template< std::totally_ordered T >
 requires std::semiregular< T >
SpaceEfficientSortedSet< T >::~SpaceEfficientSortedSet()
{
 for( Block * B = f_first ; B ; ) {
  Block * N = B->next;
  delete B;
  B = N;
  }
 }

template< std::totally_ordered T >
 requires std::semiregular< T >
auto SpaceEfficientSortedSet< T >::blockOf( const T & x ) const -> Block *
{
 const auto it = f_index.lowerBound( Key{ this , nullptr , &x } );
 return( it == f_index.end() ? nullptr : ( *it ).second );
 }

template< std::totally_ordered T >
 requires std::semiregular< T >
std::size_t SpaceEfficientSortedSet< T >::place( const Block * B ,
                                               const T & x ) const
{
 std::size_t lo = 0 , hi = B->s;            // binary search
 while( lo < hi ) {
  const std::size_t m = lo + ( hi - lo ) / 2;
  if( at( B , m ) < x ) lo = m + 1;
  else hi = m;
  }
 return( lo );
 }

template< std::totally_ordered T >
 requires std::semiregular< T >
bool SpaceEfficientSortedSet< T >::contains( const T & x ) const
{
 const Block * B = blockOf( x );
 return( B && ( at( B , place( B , x ) ) == x ) );
 }

template< std::totally_ordered T >
 requires std::semiregular< T >
std::optional< T > SpaceEfficientSortedSet< T >::lowerBound(
 const T & x ) const
{
//! [find]
 const Block * B = blockOf( x );
 if( ! B )
  return( std::nullopt );
 return( at( B , place( B , x ) ) );
//! [find]
 }

template< std::totally_ordered T >
 requires std::semiregular< T >
auto SpaceEfficientSortedSet< T >::newBlock( Block * B ) -> Block *
{
 Block * N = new Block;
 N->a = std::make_unique< T[] >( f_b + 1 );
 N->prev = B;
 N->next = B ? B->next : f_first;
 ( N->next ? N->next->prev : f_last ) = N;
 ( B ? B->next : f_first ) = N;
 ++f_blocks;
 return( N );
 }

template< std::totally_ordered T >
 requires std::semiregular< T >
void SpaceEfficientSortedSet< T >::freeBlock( Block * B )
{
 if( B->s > 0 )                             // indexed by its largest item
  f_index.remove( Key{ this , B , nullptr } );
 ( B->prev ? B->prev->next : f_first ) = B->next;
 ( B->next ? B->next->prev : f_last ) = B->prev;
 delete B;
 --f_blocks;
 }

template< std::totally_ordered T >
 requires std::semiregular< T >
void SpaceEfficientSortedSet< T >::shiftForward( Block * B , Block * C )
{
 C->h = ( C->h + f_b ) % ( f_b + 1 );       // one place back
 C->a[ C->h ] = std::move( at( B , B->s - 1 ) );
 ++C->s;
 --B->s;
 }

template< std::totally_ordered T >
 requires std::semiregular< T >
void SpaceEfficientSortedSet< T >::shiftBack( Block * B , Block * C )
{
 at( B , B->s ) = std::move( C->a[ C->h ] );
 ++B->s;
 C->h = ( C->h + 1 ) % ( f_b + 1 );
 --C->s;
 }

template< std::totally_ordered T >
 requires std::semiregular< T >
void SpaceEfficientSortedSet< T >::insert( const T & x )
{
//! [add]
 for( ; ; ) {
  Block * B = blockOf( x );
  if( ! B ) B = f_last;
  if( ! B ) {                               // the first item
   B = newBlock( nullptr );
   B->a[ 0 ] = x;
   B->s = 1;
   f_index.insert( Key{ this , B , nullptr } , B );
   ++f_size;
   break;
   }
  std::size_t i = place( B , x );
  if( ( i < B->s ) && ( at( B , i ) == x ) )
   break;
  if( B->s <= f_b ) {                       // room: x in its place
   if( i == B->s ) {                        // a new largest item of the
    f_index.remove( Key{ this , B , nullptr } );   // last block: indexed
    at( B , i ) = x;                        // again
    ++B->s;
    f_index.insert( Key{ this , B , nullptr } , B );
    }
   else {
    if( i < B->s / 2 ) {                    // the shorter side moves
     B->h = ( B->h + f_b ) % ( f_b + 1 );
     for( std::size_t j = 0 ; j < i ; ++j )
      at( B , j ) = std::move( at( B , j + 1 ) );
     }
    else
     for( std::size_t j = B->s ; j > i ; --j )
      at( B , j ) = std::move( at( B , j - 1 ) );
    at( B , i ) = x;
    ++B->s;
    }
   ++f_size;
   break;
   }
  // B full: room from the next b blocks, or a spread over a new one
  std::size_t r = 1;
  Block * C = B->next;
  for( ; C && ( r < f_b ) && ( C->s == f_b + 1 ) ; ++r ) C = C->next;
  if( r < f_b ) {
   if( ! C ) {                              // a new last block takes the
    C = newBlock( f_last );                 // last item of the old one
    shiftForward( C->prev , C );
    f_index.insert( Key{ this , C , nullptr } , C );
    }
   for( ; C != B ; C = C->prev )
    if( C->prev->s == f_b + 1 ) shiftForward( C->prev , C );
   }
  else {                                    // b full blocks: spread
   Block * N = newBlock( C ? C->prev : f_last );
   while( N->s < f_b ) shiftForward( N->prev , N );
   f_index.insert( Key{ this , N , nullptr } , N );
   for( C = N->prev ; C != B ; C = C->prev )
    while( C->s < f_b ) shiftForward( C->prev , C );
   }
  }
//! [add]
 ADS_POST( contains( x ) );
 ADS_INVARIANT( valid() );
 }

template< std::totally_ordered T >
 requires std::semiregular< T >
void SpaceEfficientSortedSet< T >::remove( const T & x )
{
//! [remove]
 Block * B = blockOf( x );
 if( B ) {
  const std::size_t i = place( B , x );
  if( at( B , i ) == x ) {
   if( B->s == 1 ) {                        // the last block, now empty
    freeBlock( B );
    --f_size;
    ADS_INVARIANT( valid() );
    return;
    }
   if( i == B->s - 1 )                      // the largest item goes: the
    f_index.remove( Key{ this , B , nullptr } );   // block indexed again
   if( i < B->s / 2 ) {
    for( std::size_t j = i ; j > 0 ; --j )
     at( B , j ) = std::move( at( B , j - 1 ) );
    B->h = ( B->h + 1 ) % ( f_b + 1 );
    }
   else
    for( std::size_t j = i ; j + 1 < B->s ; ++j )
     at( B , j ) = std::move( at( B , j + 1 ) );
   --B->s;
   if( i == B->s )
    f_index.insert( Key{ this , B , nullptr } , B );
   --f_size;
   if( B->next && ( B->s + 2 == f_b ) ) {   // too few: borrow or gather
    std::size_t r = 1;
    Block * C = B->next;
    for( ; ( r < f_b ) && C->next && ( C->s + 1 == f_b ) ; ++r )
     C = C->next;
    if( r < f_b ) {
     if( ! C->next && ( C->s == 1 ) ) {     // the last block is emptied
      f_index.remove( Key{ this , C , nullptr } );
      }
     for( Block * D = B ; D != C ; D = D->next ) shiftBack( D , D->next );
     if( C->s == 0 ) {
      ( C->prev ? C->prev->next : f_first ) = C->next;
      f_last = C->prev;
      delete C;
      --f_blocks;
      }
     }
    else {                                  // b blocks: gather into b - 1
     f_index.remove( Key{ this , C->prev , nullptr } );
     Block * E = C->prev;                   // the one that empties
     for( Block * D = B ; D != E ; D = D->next )
      while( ( D->s < f_b ) && ( D->next->s > 0 ) ) {
       shiftBack( D , D->next );
       if( ( D->next == E ) && ( E->s == 0 ) ) break;
       }
     freeBlock( E );
     }
    }
   }
  }
//! [remove]
 ADS_POST( ! contains( x ) );
 ADS_INVARIANT( valid() );
 }

template< std::totally_ordered T >
 requires std::semiregular< T >
template< class F >
void SpaceEfficientSortedSet< T >::forEach( F && f ) const
{
 for( const Block * B = f_first ; B ; B = B->next )
  for( std::size_t i = 0 ; i < B->s ; ++i ) f( at( B , i ) );
 }

template< std::totally_ordered T >
 requires std::semiregular< T >
bool SpaceEfficientSortedSet< T >::valid( void ) const
{
 std::size_t n = 0 , m = 0;
 const T * last = nullptr;
 for( const Block * B = f_first ; B ; B = B->next , ++m ) {
  if( ( B->s == 0 ) || ( B->s > f_b + 1 ) ||
      ( B->next && ( B->s + 1 < f_b ) ) )
   return( false );
  if( ( B->next ? B->next->prev : f_last ) != B ) return( false );
  for( std::size_t i = 0 ; i < B->s ; ++i , ++n ) {
   if( last && !( *last < at( B , i ) ) ) return( false );
   last = &at( B , i );
   }
  if( blockOf( at( B , B->s - 1 ) ) != B ) return( false );
  }
 return( ( n == f_size ) && ( m == f_blocks ) &&
         ( f_index.size() == f_blocks ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------ End File SpaceEfficientSortedSet.tpp ------------------*/
/*--------------------------------------------------------------------------*/
