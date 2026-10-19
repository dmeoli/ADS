/*--------------------------------------------------------------------------*/
/*------------------------- File PatienceSort.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sort of PatienceSort.h, with its contracts.
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
void patienceSort( std::vector< T > & A , C less , std::size_t * piles )
{
 const auto before = detail::auditCopy( A );
 std::vector< std::vector< T > > P;         // the piles, tops at the back
 for( auto & x : A ) {
  std::size_t lo = 0 , hi = P.size();       // the first top not below x
  while( lo < hi ) {
   const std::size_t m = lo + ( hi - lo ) / 2;
   if( less( P[ m ].back() , x ) ) lo = m + 1; else hi = m;
   }
  if( lo == P.size() )
   P.emplace_back();
  P[ lo ].push_back( std::move( x ) );
  }
 if( piles ) *piles = P.size();
 struct Top { std::size_t pile; };
 auto better = [ & ]( const Top & a , const Top & b ) {
  return( less( P[ a.pile ].back() , P[ b.pile ].back() ) ); };
 DaryHeap< Top , decltype( better ) > H( better );
 for( std::size_t k = 0 ; k < P.size() ; ++k )
  H.push( Top{ k } );
 for( std::size_t i = 0 ; ! H.empty() ; ++i ) {     // the k-way merge
  const Top t = H.pop();
  A[ i ] = std::move( P[ t.pile ].back() );
  P[ t.pile ].pop_back();
  if( ! P[ t.pile ].empty() )
   H.push( t );
  }
 ADS_AUDIT( detail::sortedPermutation( A , before , less ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File PatienceSort.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
