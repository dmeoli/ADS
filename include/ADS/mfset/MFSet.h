/*--------------------------------------------------------------------------*/
/*------------------------------ File MFSet.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The MFSET of section 10.2, the union of disjoint sets: Mfset( n ) makes
 * the n components {0} , ... , {n - 1}, find( x ) gives the representative
 * of the component of x, merge( x , y ) joins the components of x and y,
 * if they are distinct.
 *
 * MFSet keeps the components as a forest, a vector of parents p, the roots
 * with p[ x ] = x being the representatives (section 10.2.2): find() goes
 * up to the root, merge() makes a root a child of the other. Two heuristics
 * may be switched off, to study them:
 *
 * - the rank (section 10.2.3): rank[ r ] is the height of the tree of the
 *   root r, the lower tree goes under the higher one and the rank grows by
 *   one only when they are equal; by the Teorema 10.1 a tree of root r has
 *   at least \f$ 2^{rank[ r ]} \f$ nodes, so find() and merge() take
 *   O( log n );
 * - the compression of the paths (section 10.2.4): find() makes each node
 *   on its way a child of the root. With both, a sequence of m operations,
 *   \f$ m \geq n \f$, takes \f$ O( n + m \alpha( m , n ) ) \f$, where
 *   \f$ \alpha \f$, the inverse of the function of Ackermann, is at most 4
 *   for every n and m that fit in a computer. Then rank[ r ] is an upper
 *   bound to the height (exercise 10.7).
 *
 * find() is the iterative version in two passes, the root first and then
 * the compression, with no stack (exercise 10.8). ListMFSet.h has
 * the realization on lists of section 10.2.1. components() gives
 * the MFSET of the connected components of an unoriented graph, which can
 * then follow new edges (section 10.2.5).
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 10.2,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap10.pdf
 * \see B. A. Galler, M. J. Fischer, An improved equivalence algorithm,
 *      Communications of the ACM 7(5), 301-303, 1964,
 *      doi:10.1145/364099.364331
 * \see R. E. Tarjan, Efficiency of a good but not linear set union
 *      algorithm, Journal of the ACM 22(2), 215-225, 1975,
 *      doi:10.1145/321879.321884
 * \see R. E. Tarjan, J. van Leeuwen, Worst-case analysis of set union
 *      algorithms, Journal of the ACM 31(2), 245-281, 1984,
 *      doi:10.1145/62.2160

 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_MFSet
 #define __ADS_MFSet
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/Graph.h"

#include <stdexcept>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------------ CLASS MFSet -------------------------------*/
/*--------------------------------------------------------------------------*/

/// the union of disjoint sets on a forest
class MFSet {

/*--------------------------------------------------------------------------*/
 public:

 /// Mfset( n ): the n components {0} , ... , {n - 1}, with the rank and
 /// the compression of the paths or not
 explicit MFSet( int n , bool rank = true , bool compress = true )
  : f_p( n ) , f_rank( n , 0 ) , f_byrank( rank ) , f_compress( compress ) ,
    f_count( n ) {
  if( n < 0 )
   throw( std::invalid_argument( "ADS::MFSet: n < 0" ) );
  for( int i = 0 ; i < n ; ++i )
   f_p[ i ] = i;
  }

 [[nodiscard]] int n( void ) const { return( int( f_p.size() ) ); }

 /// the number of components
 [[nodiscard]] int count( void ) const { return( f_count ); }

 /// the representative of the component of x, the root of its tree
 int find( int x ) {
  check( x );
  int r = x;
  while( f_p[ r ] != r ) {
   r = f_p[ r ];
   ++f_steps;
   }
  if( f_compress )
   while( f_p[ x ] != r ) {
    const int t = f_p[ x ];
    f_p[ x ] = r;
    x = t;
    }
  return( r );
  }

 /// joins the components of x and y; true if they were distinct
 bool merge( int x , int y ) {
  const int rx = find( x ) , ry = find( y );
  if( rx == ry )
   return( false );
  --f_count;
  if( ! f_byrank )
   f_p[ ry ] = rx;
  else if( f_rank[ rx ] > f_rank[ ry ] )
   f_p[ ry ] = rx;
  else if( f_rank[ ry ] > f_rank[ rx ] )
   f_p[ rx ] = ry;
  else {
   f_p[ rx ] = ry;
   ++f_rank[ ry ];
   }
  return( true );
  }

 /// the parent of x, x itself if a root
 [[nodiscard]] int parent( int x ) const { return( f_p[ check( x ) ] ); }

 /// the rank of x
 [[nodiscard]] int rank( int x ) const { return( f_rank[ check( x ) ] ); }

 /// the steps up the trees made by find() so far
 [[nodiscard]] long steps( void ) const { return( f_steps ); }

/*--------------------------------------------------------------------------*/
 private:

 int check( int x ) const {
  if( ( x < 0 ) || ( x >= n() ) )
   throw( std::out_of_range( "ADS::MFSet: element out of [0, n)" ) );
  return( x );
  }

 std::vector< int > f_p;      ///< the parents
 std::vector< int > f_rank;   ///< the ranks
 bool f_byrank;               ///< the heuristic of the rank
 bool f_compress;             ///< the compression of the paths
 int f_count;                 ///< the number of components
 long f_steps = 0;            ///< see steps()

 };  // end( class MFSet )

/*--------------------------------------------------------------------------*/

/// the MFSET of the connected components of an unoriented graph, built by
/// merging the ends of each edge (the cc() of section 10.2.5)
template< Graph G >
MFSet components( const G & g )
{
 MFSet M( g.bound() );
 for( int u : g.V() )
  g.forEachAdj( u , [ & ]( int v ) { M.merge( u , v ); } );
 return( M );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* MFSet.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File MFSet.h ----------------------------*/
/*--------------------------------------------------------------------------*/
