/*--------------------------------------------------------------------------*/
/*------------------------------ File Texts.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The texts of the benchmarks on strings: random bytes over alphabets of
 * 4, 26 and 256 symbols, a text like natural language (words of a Zipf
 * vocabulary, with the frequencies of the English letters, between blanks,
 * stops and newlines), the Fibonacci word, a highly repetitive text (a
 * block of 1000 random letters repeated with one mutation in ten thousand),
 * and the adversarial text of the naive search (one repeated byte), each of
 * n bytes from a seed.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */

#ifndef __TEXTS_H
 #define __TEXTS_H

/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#include <cmath>
#include <cstddef>
#include <random>
#include <string>
#include <vector>

namespace ADS::bench {

/// the kinds of text
inline const char * textKinds[] = { "sigma4" , "sigma26" , "sigma256" ,
                                    "english" , "fibonacci" , "repeats" ,
                                    "one-byte" };

/// the text of n bytes of the given kind, from the seed
inline std::string makeText( const std::string & kind , std::size_t n ,
                             unsigned seed )
{
 std::mt19937_64 g( seed );
 std::string T;
 T.reserve( n + 64 );
 if( kind == "sigma4" )
  while( T.size() < n ) T += "ACGT"[ g() % 4 ];
 else if( kind == "sigma26" )
  while( T.size() < n ) T += char( 'a' + g() % 26 );
 else if( kind == "sigma256" )
  while( T.size() < n ) T += char( g() % 256 );
 else if( kind == "english" ) {
  // the letters by their frequency in English, per 1000
  static const double f[ 26 ] = { 82 , 15 , 28 , 43 , 127 , 22 , 20 , 61 ,
                                   70 , 2 , 8 , 40 , 24 , 67 , 75 , 19 , 1 ,
                                   60 , 63 , 91 , 28 , 10 , 24 , 2 , 20 ,
                                   1 };
  std::discrete_distribution< int > letter( f , f + 26 );
  std::geometric_distribution< int > len( 0.22 );
  std::vector< std::string > V( 20000 );
  for( auto & w : V ) {
   const int l = 1 + std::min( len( g ) , 14 );
   for( int i = 0 ; i < l ; ++i ) w += char( 'a' + letter( g ) );
   }
  // Zipf of exponent 1 on the ranks of the vocabulary
  std::vector< double > z( V.size() );
  for( std::size_t r = 0 ; r < z.size() ; ++r ) z[ r ] = 1.0 / double( r + 1 );
  std::discrete_distribution< std::size_t > word( z.begin() , z.end() );
  while( T.size() < n ) {
   std::string w = V[ word( g ) ];
   if( g() % 12 == 0 ) w[ 0 ] = char( w[ 0 ] - 'a' + 'A' );
   T += w;
   const auto e = g() % 100;
   T += e < 6 ? ". " : e < 10 ? ", " : e < 11 ? ".\n" : " ";
   }
  }
 else if( kind == "fibonacci" ) {
  std::string a = "a" , b = "ab";
  while( b.size() < n ) {
   std::string c = b + a;
   a = std::move( b );
   b = std::move( c );
   }
  T = std::move( b );
  }
 else if( kind == "repeats" ) {
  std::string B;
  for( int i = 0 ; i < 1000 ; ++i ) B += char( 'a' + g() % 26 );
  while( T.size() < n ) T += B;
  T.resize( n );
  for( std::size_t i = 0 ; i < n / 10000 ; ++i )
   T[ g() % n ] = char( 'a' + g() % 26 );
  }
 else
  T.assign( n , 'a' );
 T.resize( n );
 return( T );
 }

}  // end( namespace ADS::bench )

#endif  /* Texts.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Texts.h ----------------------------*/
/*--------------------------------------------------------------------------*/
