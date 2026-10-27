/*--------------------------------------------------------------------------*/
/*------------------------- File GraphBuilder.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of GraphBuilder [see GraphBuilder.h], with their
 * contracts.
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

#include <climits>

namespace ADS {

/*--------------------------------------------------------------------------*/
/*----------------------------- THE OPERATIONS -----------------------------*/
/*--------------------------------------------------------------------------*/

template< class W >
GraphBuilder< W >::GraphBuilder( int n , bool directed )
 : f_directed( directed ) , f_n( n )
{
 ADS_PRE( n >= 0 );
 }

template< class W >
void GraphBuilder< W >::addEdge( int u , int v , const W & w )
{
 ADS_PRE( ( u >= 0 ) && ( u < f_n ) && ( v >= 0 ) && ( v < f_n ) &&
          ( u != v ) );
 ADS_PRE( f_E.size() < std::size_t( INT_MAX ) );
 f_E.emplace_back( u , v , w );
 }

template< class W >
StaticGraph< W > GraphBuilder< W >::toStaticGraph( void ) const
{
 return( StaticGraph< W >( f_n , f_E , f_directed ) );
 }

template< class W >
ArrayGraph< W > GraphBuilder< W >::toArrayGraph( void ) const
{
 return( ArrayGraph< W >( f_n , f_E , f_directed ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File GraphBuilder.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
