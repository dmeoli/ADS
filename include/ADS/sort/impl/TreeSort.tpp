/*--------------------------------------------------------------------------*/
/*--------------------------- File TreeSort.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sort of TreeSort.h, with its contracts.
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
void treeSort( std::vector< T > & A , bool balanced )
{
 const auto before = detail::auditCopy( A );
 auto run = [ & ]( auto & S ) {
  for( const auto & x : A ) {
   const auto c = S.lookup( x );
   S.insert( x , c ? *c + 1 : 1 );          // the copies of x
   }
  std::size_t i = 0;
  S.forEach( [ & ]( const T & k , std::size_t c ) {
   while( c-- > 0 ) A[ i++ ] = k; } );
  };
 if( balanced ) {
  RedBlackTree< T , std::size_t > S;
  run( S );
  }
 else {
  BinarySearchTree< T , std::size_t > S;
  run( S );
  }
 ADS_AUDIT( detail::sortedPermutation( A , before , std::less<>() ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File TreeSort.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
