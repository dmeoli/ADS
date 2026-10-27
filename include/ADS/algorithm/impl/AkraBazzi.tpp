/*--------------------------------------------------------------------------*/
/*--------------------------- File AkraBazzi.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of AkraBazzi [see AkraBazzi.h], with their contracts.
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

inline double akraBazziExponent(
                       const std::vector< std::pair< double , double > > & ab )
{
 ADS_PRE( ! ab.empty() );
 for( const auto & [ a , b ] : ab )
  ADS_PRE( ( a > 0 ) && ( 0 < b ) && ( b < 1 ) );
 auto f = [ & ]( double p ) {                   // strictly decreasing
  double v = 0;
  for( const auto & [ a , b ] : ab )
   v += a * std::pow( b , p );
  return( v );
  };
 double lo = -1 , hi = 1;                       // f( lo ) >= 1 >= f( hi )
 while( f( lo ) < 1 )
  lo *= 2;
 while( f( hi ) > 1 )
  hi *= 2;
 for( int i = 0 ; i < 200 ; ++i ) {
  const double mid = ( lo + hi ) / 2;
  ( f( mid ) > 1 ? lo : hi ) = mid;
  }
 ADS_POST( std::abs( f( lo ) - 1 ) < 1e-9 );
 return( lo );
 }

inline Growth akraBazzi(
                  const std::vector< std::pair< double , double > > & ab ,
                  double beta , double k )
{
 const double p = akraBazziExponent( ab );
 const double eps = 1e-9;
 Growth g;
 if( beta < p - eps )
  g.p = p;
 else if( beta > p + eps ) {
  g.p = beta;
  g.q = k;
  }
 else {
  g.p = beta;
  if( k > -1 + eps )
   g.q = k + 1;
  else if( k > -1 - eps )
   g.r = 1;
  }
 return( g );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File AkraBazzi.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
