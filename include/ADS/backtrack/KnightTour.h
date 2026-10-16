/*--------------------------------------------------------------------------*/
/*--------------------------- File KnightTour.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The tour of the knight (exercise 16.6): a path of the knight that visits
 * every square of an n x n board once. cavallo() of the book tries the 8
 * moves in a fixed order and backtracks from the dead ends, which on an
 * 8 x 8 board may take very long; the rule of Warnsdorff tries first the
 * moves to the squares with the fewest onward moves, and with it the
 * backtrack seldom happens, a tour of a board of hundreds of squares in
 * linear time. Both return the board with the step of each square, from 1,
 * or nothing if there is no tour from the given square.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, exercise 16.6,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap16.pdf
 * \see H. C. von Warnsdorf, Des Roesselsprunges einfachste und
 *      allgemeinste Loesung, Schmalkalden, 1823
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

#ifndef __ADS_KnightTour
 #define __ADS_KnightTour
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <algorithm>
#include <array>
#include <optional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// a tour of the knight from ( x , y ) on an n x n board, by backtrack,
/// the moves in the order of the book or by the rule of Warnsdorff
inline std::optional< std::vector< std::vector< int > > > knightTour(
                                    int n , int x , int y ,
                                    bool warnsdorff = true )
{
 static constexpr std::array< int , 8 > X = { -1 , 1 , 2 , 2 , 1 , -1 , -2 ,
                                              -2 };
 static constexpr std::array< int , 8 > Y = { -2 , -2 , -1 , 1 , 2 , 2 , 1 ,
                                              -1 };
 std::vector< std::vector< int > > A( n , std::vector< int >( n , 0 ) );
 auto ok = [ & ]( int a , int b ) {
  return( a >= 0 && a < n && b >= 0 && b < n && A[ a ][ b ] == 0 ); };
 auto degree = [ & ]( int a , int b ) {
  int d = 0;
  for( int i = 0 ; i < 8 ; ++i ) d += ok( a + X[ i ] , b + Y[ i ] );
  return( d );
  };
 auto rec = [ & ]( auto & self , int k , int a , int b ) -> bool {
  A[ a ][ b ] = k + 1;
  if( k == n * n - 1 )
   return( true );
  std::vector< std::pair< int , int > > moves;      // ( degree , move )
  for( int i = 0 ; i < 8 ; ++i )
   if( ok( a + X[ i ] , b + Y[ i ] ) )
    moves.emplace_back( warnsdorff ? degree( a + X[ i ] , b + Y[ i ] ) : 0 ,
                        i );
  std::stable_sort( moves.begin() , moves.end() , []( auto & p , auto & q ) {
   return( p.first < q.first ); } );
  for( auto [ d , i ] : moves )
   if( self( self , k + 1 , a + X[ i ] , b + Y[ i ] ) )
    return( true );
  A[ a ][ b ] = 0;
  return( false );
  };
 if( ( n < 1 ) || ! rec( rec , 0 , x , y ) )
  return( std::nullopt );
 return( A );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* KnightTour.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File KnightTour.h --------------------------*/
/*--------------------------------------------------------------------------*/
