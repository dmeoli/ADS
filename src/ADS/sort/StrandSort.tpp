/*--------------------------------------------------------------------------*/
/*-------------------------- File StrandSort.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sort of StrandSort.h, with its contracts.
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

template< class T , class C >
void strandSort( List< T > & L , C less , std::size_t * strands )
{
 List< T > R;                               // the result, sorted
 std::size_t k = 0;
 while( ! L.isEmpty() ) {
  List< T > S;                              // pull a strand out of L
  S.insert( S.next( S.tail() ) , L.read( L.head() ) );
  L.remove( L.head() );
  for( auto p = L.head() ; ! L.finished( p ) ; )
   if( ! less( L.read( p ) , S.read( S.tail() ) ) ) {
    S.insert( S.next( S.tail() ) , L.read( p ) );
    p = L.remove( p );
    }
   else
    p = L.next( p );
  ++k;
  List< T > M;                              // merge R and S, R first
  auto r = R.head() , s = S.head();         // among equals: stable
  while( ! R.finished( r ) || ! S.finished( s ) )
   if( S.finished( s ) || ( ! R.finished( r ) &&
                            ! less( S.read( s ) , R.read( r ) ) ) ) {
    M.insert( M.next( M.tail() ) , R.read( r ) );
    r = R.next( r );
    }
   else {
    M.insert( M.next( M.tail() ) , S.read( s ) );
    s = S.next( s );
    }
  R = std::move( M );
  }
 L = std::move( R );
 if( strands ) *strands = k;
 ADS_AUDIT( detail::sequenceSorted( L , less ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File StrandSort.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
