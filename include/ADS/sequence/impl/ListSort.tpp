/*--------------------------------------------------------------------------*/
/*---------------------------- File ListSort.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sort of a List by relinking its cells [see ListSort.h].
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

namespace detail {

/// the chains a and b, sorted and ended by nil along succ, merged into one,
/// a first on equal items
template< class Cell , class C >
Cell * mergeChains( Cell * a , Cell * b , C & less )
{
 Cell * r = nullptr;
 Cell ** t = &r;                            // where the next cell goes
 while( a && b )
  if( less( b->value , a->value ) ) {
   *t = b;
   t = &b->succ;
   b = b->succ;
   }
  else {
   *t = a;
   t = &a->succ;
   a = a->succ;
   }
 *t = a ? a : b;
 return( r );
 }

/// the cells of a chain by their successors, relinked with their
/// predecessors between the sentinel s and itself
template< class Cell >
void relink( Cell * s , Cell * x )
{
 Cell * p = s;
 for( ; x ; p = x , x = x->succ ) {
  p->succ = x;
  x->pred = p;
  }
 p->succ = s;
 s->pred = p;
 }

/// the numbers in their natural order: an arithmetic type sorted by
/// std::less
template< class T , class C >
inline constexpr bool kNaturalNumbers = RadixKey< T > &&
 ( std::is_same_v< C , std::less<> > || std::is_same_v< C , std::less< T > > );

}  // end( namespace detail )

template< class T , bool P , class C >
void listSort( List< T , P > & L , C less )
{
 using Cell = typename List< T , P >::Cell;
 if( L.empty() || ( L.head() == L.tail() ) )
  return;
 Cell * const s = L.next( L.tail() );       // the sentinel
 [[maybe_unused]] std::size_t n = 0;
 if constexpr( ADS_CONTRACTS >= 3 )
  for( Cell * x = s->succ ; x != s ; x = x->succ ) ++n;
 if constexpr( detail::kNaturalNumbers< T , C > ) {
  using K = decltype( radixKey( std::declval< T >() ) );
  std::vector< std::pair< K , Cell * > > A;
  for( Cell * x = s->succ ; x != s ; x = x->succ )
   A.emplace_back( radixKey( x->value ) , x );
  radixSortBy( A , []( const std::pair< K , Cell * > & e ) {
   return( e.first ); } );
  Cell * p = s;
  for( const auto & e : A ) {
   p->succ = e.second;
   e.second->pred = p;
   p = e.second;
   }
  p->succ = s;
  s->pred = p;
  }
 else {
  s->pred->succ = nullptr;                  // a chain ended by nil
  Cell * x = s->succ;
  Cell * R[ 64 ] = {};                      // R[ i ]: about 2^i runs
  int top = 0;
  while( x ) {
   Cell * r = x;                            // the next run
   Cell * y = x->succ;
   if( y && less( y->value , x->value ) ) { // strictly decreasing: reversed
    Cell * rev = x;
    rev->succ = nullptr;
    while( y && less( y->value , rev->value ) ) {
     Cell * z = y->succ;
     y->succ = rev;
     rev = y;
     y = z;
     }
    r = rev;
    x = y;
    }
   else {                                   // not decreasing
    Cell * last = x;
    while( y && ! less( y->value , last->value ) ) {
     last = y;
     y = y->succ;
     }
    last->succ = nullptr;
    x = y;
    }
   int i = 0;                               // the carries of the counter
   for( ; R[ i ] ; ++i ) {
    r = detail::mergeChains( R[ i ] , r , less );
    R[ i ] = nullptr;
    }
   R[ i ] = r;
   if( i >= top ) top = i + 1;
   }
  Cell * r = nullptr;                       // the earlier runs are higher
  for( int i = 0 ; i < top ; ++i )
   if( R[ i ] )
    r = r ? detail::mergeChains( R[ i ] , r , less ) : R[ i ];
  detail::relink( s , r );
  }
 ADS_AUDIT( [ & ]( void ) {
  std::size_t m = 0;
  for( Cell * x = s->succ ; x != s ; x = x->succ , ++m )
   if( ( x->succ != s ) && less( x->succ->value , x->value ) )
    return( false );
  return( m == n );
  }() );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File ListSort.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
