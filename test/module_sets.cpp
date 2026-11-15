/*--------------------------------------------------------------------------*/
/*-------------------------- File module_sets.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Test of the C++20 modules of the sets of ADS [see cmake/ads_sets.txt],
 * built when ADS_BUILD_MODULE is ON [see CMakeLists.txt] or by make
 * ADSMOD=1 [see makefile], once for each module: with ADS_MODULE_SET 1 it
 * imports ads.core, with 2 ads.base and with 3 ads.advanced, and it is
 * linked only with that module and ads.core, so that a module of a set
 * that needs one of another set does not link.
 *
 * Each program includes no header of ADS, and runs a structure of its set
 * and SplitMix64 of the core, which ads.base and ads.advanced export in
 * turn; a name that the module fails to export stops the build, and a
 * wrong result the run.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <iostream>

#if ADS_MODULE_SET == 1
import ads.core;
#elif ADS_MODULE_SET == 2
import ads.base;
#elif ADS_MODULE_SET == 3
import ads.advanced;
#endif

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main()
{
 int failed = 0;
 auto check = [ & ]( bool ok , const char * what ) {
  if( ! ok ) {
   std::cerr << "FAILED " << what << '\n';
   ++failed;
   }
  };

 // core: two generators of the same seed give the same draws
 SplitMix64 g( 7 ) , h( 7 );
 check( ( g() == h() ) && ( g() == h() ) , "SplitMix64" );

#if ADS_MODULE_SET == 2
 // base: the merge-find set of the book and a stack
 MFSet S( 4 );
 S.merge( 0 , 1 );
 S.merge( 2 , 3 );
 check( ( S.find( 0 ) == S.find( 1 ) ) && ( S.find( 1 ) != S.find( 2 ) ) ,
	"MFSet" );
 ArrayStack< int > A( 4 );
 A.push( 1 );
 A.push( 2 );
 check( ( A.top() == 2 ) && ( A.size() == 2 ) , "ArrayStack" );
#elif ADS_MODULE_SET == 3
 // advanced: the engineered union-find and a 4-ary heap
 UnionFind U( 4 );
 U.merge( 0 , 1 );
 U.merge( 2 , 3 );
 check( ( U.find( 0 ) == U.find( 1 ) ) && ( U.find( 1 ) != U.find( 2 ) ) ,
	"UnionFind" );
 DaryHeap< int > H;
 for( int x : { 5 , 3 , 8 , 1 } )
  H.push( x );
 check( ( H.pop() == 1 ) && ( H.pop() == 3 ) , "DaryHeap" );
#endif

 const char * const name[] = { "" , "core" , "base" , "advanced" };
 std::cout << ( failed ? "FAILED" : "PASSED" ) << " module."
	   << name[ ADS_MODULE_SET ] << '\n';
 return( failed ? 1 : 0 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ End File module_sets.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
