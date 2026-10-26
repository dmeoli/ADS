/*--------------------------------------------------------------------------*/
/*---------------------------- File module.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Test of the C++20 module ads, built when ADS_BUILD_MODULE is ON [see
 * CMakeLists.txt] or by make module [see makefile].
 *
 * The program includes no header of ADS: it imports the module, and with
 * what that exports it runs an algorithm of each of several areas (a List
 * sorted by mergeSort(), a ChainedHashTable, Dijkstra's algorithm on a
 * ListGraph, kmp(), the product of two polynomials by convolve() and of two
 * BigInteger) and checks their results. Each names a different part of the
 * library, so a name that the module fails to export stops the build, and
 * a wrong result the run.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <cmath>
#include <cstddef>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

import ads;

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

 // sequence and sort: a List sorted by merging
 List< int > L{ 5 , 3 , 8 , 1 , 4 };
 mergeSort( L );
 std::vector< int > s( L.begin() , L.end() );
 check( s == std::vector< int >( { 1 , 3 , 4 , 5 , 8 } ) , "mergeSort" );

 // dictionary: insert, lookup, remove
 ChainedHashTable< std::string , int > D( 16 );
 D.insert( "one" , 1 );
 D.insert( "two" , 2 );
 D.remove( "one" );
 check( ! D.lookup( "one" ) && D.lookup( "two" ) == 2 ,
        "ChainedHashTable" );

 // graph: the shortest paths of the example of the book
 ListGraph< int > g( 7 , true );
 const struct { char u , v; int w; } E[] = { { 'A' , 'B' , 2 } ,
  { 'A' , 'D' , 1 } , { 'B' , 'C' , 3 } , { 'B' , 'D' , 3 } ,
  { 'D' , 'F' , 1 } , { 'F' , 'C' , 2 } , { 'C' , 'G' , 2 } ,
  { 'F' , 'G' , 5 } };
 for( const auto & e : E )
  g.insertEdge( e.u - 'A' , e.v - 'A' , e.w );
 const auto R = dijkstra( g , 0 );
 check( R.d == std::vector< int >( { 0 , 2 , 4 , 1 , infinity< int >() ,
                                     2 , 6 } ) , "dijkstra" );

 // string: the occurrences of a pattern
 check( kmp( "abracadabra" , "abra" ) ==
        std::vector< std::size_t >( { 0 , 7 } ) , "kmp" );

 // numeric: ( 1 + 2 x ) ( 3 + x ) = 3 + 7 x + 2 x^2 by the FFT
 const auto c = convolve( std::vector< double >( { 1 , 2 } ) ,
                          std::vector< double >( { 3 , 1 } ) );
 check( c.size() == 3 && std::abs( c[ 0 ] - 3 ) < 1e-9 &&
        std::abs( c[ 1 ] - 7 ) < 1e-9 && std::abs( c[ 2 ] - 2 ) < 1e-9 ,
        "convolve" );

 // numeric: a product of BigInteger beyond 64 bits
 const BigInteger a( "123456789012345678901234567890" );
 check( ( a * a ).toString() ==
        "15241578753238836750495351562536198787501905199875019052100" ,
        "BigInteger" );

 std::cout << ( failed ? "FAILED" : "PASSED" ) << " module\n";
 return failed ? 1 : 0;
}

/*--------------------------------------------------------------------------*/
/*-------------------------- End File module.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
