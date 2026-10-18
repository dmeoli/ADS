/*--------------------------------------------------------------------------*/
/*------------------------ File PigeonholeSort.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The definitions of the functions of PigeonholeSort.h [see PigeonholeSort.h].
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

template< class V >
std::vector< std::pair< int , V > > pigeonholeSort(
                    const std::vector< std::pair< int , V > > & A , int k )
{
 ADS_PRE( k >= 0 );
 std::vector< List< V > > B( k );
 for( const auto & [ key , v ] : A ) {
  ADS_PRE( ( key >= 0 ) && ( key < k ) );
  B[ key ].insert( B[ key ].next( B[ key ].tail() ) , v );
  }
 std::vector< std::pair< int , V > > out;
 out.reserve( A.size() );
 for( int i = 0 ; i < k ; ++i )
  for( auto p = B[ i ].head() ; ! B[ i ].finished( p ) ; p = B[ i ].next( p ) )
   out.emplace_back( i , B[ i ].read( p ) );
 ADS_POST( out.size() == A.size() );
 ADS_AUDIT( std::is_sorted( out.begin() , out.end() ,
                            []( const auto & x , const auto & y ) {
                             return( x.first < y.first ); } ) );
 return( out );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File PigeonholeSort.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
