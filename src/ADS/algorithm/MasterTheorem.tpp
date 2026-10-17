/*--------------------------------------------------------------------------*/
/*------------------------- File MasterTheorem.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The definitions of the functions of MasterTheorem.h [see MasterTheorem.h].
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

inline Growth master( double a , double b , double beta , double k )
{
 ADS_PRE( ( a >= 1 ) && ( b > 1 ) );
 const double alpha = std::log( a ) / std::log( b );
 const double eps = 1e-12;
 Growth g;
 if( beta < alpha - eps )
  g.p = alpha;
 else if( beta > alpha + eps ) {
  g.p = beta;
  g.q = k;
  }
 else {
  g.p = alpha;
  if( k > -1 + eps )
   g.q = k + 1;
  else if( k > -1 - eps )
   g.r = 1;
  }
 return( g );
 }

inline Growth linear( double a , double beta )
{
 ADS_PRE( a >= 1 );
 Growth g;
 if( a < 2 )
  g.p = beta + 1;
 else {
  g.c = a;
  g.p = beta;
  g.upper = true;
  }
 return( g );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File MasterTheorem.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
