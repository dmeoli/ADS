/*--------------------------------------------------------------------------*/
/*------------------------------ File fft.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the fast Fourier transform: fft() of ADS against the
 * transform by its definition, dft(), for the smallest sizes, and, if FFTW
 * is found (ADS_HAS_FFTW), against fftw_execute() on a plan made once with
 * FFTW_ESTIMATE and with FFTW_MEASURE, whose planning is not timed. The
 * sizes are powers of 2 of complex numbers in double precision, the
 * transform forward and in place.
 *
 * Each cell is the median of r runs of one transform, in nanoseconds per
 * point; the contracts are off (ADS_CONTRACTS=0).
 *
 *   fft [ -r runs ] [ -s seed ] n1 n2 ... *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef ADS_CONTRACTS
 #define ADS_CONTRACTS 0
#endif

#include "Bench.h"

#include "ADS/numeric/FFT.h"

#ifdef ADS_HAS_FFTW
 #include <fftw3.h>
#endif

#include <complex>

using namespace ADS::bench;

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 1024 , 16384 , 262144 , 4194304 } );
 for( std::size_t n : o.sizes ) {
  std::mt19937_64 g( o.seed );
  std::normal_distribution< double > N;
  std::vector< ADS::Complex > A( n );
  for( auto & x : A ) x = ADS::Complex( N( g ) , N( g ) );
  const int r = o.runs;
  for( const char * who : { "ADS::fft" , "ADS::fft A/A" } )
   print( "fft" , "forward" , n , who , median( [ & ](){ return( A ); } ,
           [ & ]( auto & B ) { ADS::fft( B , false );
                               sink = sink + ( B[ 0 ].real() > 0 ); } ,
           r , n ) );
  if( n <= 4096 )
   print( "fft" , "forward" , n , "ADS::dft (definition)" , median(
           [](){ return( 0 ); } , [ & ]( int ) {
            const auto B = ADS::dft( A , false );
            sink = sink + ( B[ 0 ].real() > 0 ); } , r , n ) );
#ifdef ADS_HAS_FFTW
  for( unsigned flag : { unsigned( FFTW_ESTIMATE ) ,
                         unsigned( FFTW_MEASURE ) } ) {
   auto * in = fftw_alloc_complex( n );
   fftw_plan p = fftw_plan_dft_1d( int( n ) , in , in , FFTW_FORWARD , flag );
   print( "fft" , "forward" , n , flag == FFTW_ESTIMATE ? "FFTW estimate"
                                                         : "FFTW measure" ,
          median( [ & ](){
           for( std::size_t i = 0 ; i < n ; ++i ) {
            in[ i ][ 0 ] = A[ i ].real();
            in[ i ][ 1 ] = A[ i ].imag();
            }
           return( 0 ); } , [ & ]( int ) {
            fftw_execute( p );
            sink = sink + ( in[ 0 ][ 0 ] > 0 ); } , r , n ) );
   fftw_destroy_plan( p );
   fftw_free( in );
   }
#endif
  }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- End File fft.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
