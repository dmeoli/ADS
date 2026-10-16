/*--------------------------------------------------------------------------*/
/*----------------------------- File Huffman.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The code of Huffman (reality check of section 14.4): a prefix code, no
 * code being a prefix of another one, so that a text can be decoded, of
 * least length for the frequencies of the characters. The characters go
 * into a min-heap by frequency; the two least frequent ones, c_1 and c_2,
 * are replaced by a new virtual character of frequency f_1 + f_2, whose
 * children in a binary tree are c_1 and c_2; after n - 1 steps the root is
 * left, and the code of a character is the path from the root to its leaf,
 * 0 to the left and 1 to the right; O( n log n ) time. The least frequent
 * characters get the longest codes.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 14.4,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap14.pdf
 * \see D. A. Huffman, A method for the construction of minimum-redundancy
 *      codes, Proceedings of the IRE 40(9), 1098-1101, 1952,
 *      doi:10.1109/JRPROC.1952.273898
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

#ifndef __ADS_Huffman
 #define __ADS_Huffman
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/priority/HeapPriorityQueue.h"

#include <cstddef>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the codes of Huffman of the characters with the given frequencies, at
/// least one; a single character gets the code "0"
template< class C , class F >
std::map< C , std::string > huffman( const std::map< C , F > & freq )
{
 if( freq.empty() )
  throw( std::invalid_argument( "ADS::huffman: no character" ) );
 struct Node { int left , right; C c; };
 std::vector< Node > T;                         // the nodes of the tree
 HeapPriorityQueue< int , F > Q( int( 2 * freq.size() ) );
 for( const auto & [ c , f ] : freq ) {
  T.push_back( { -1 , -1 , c } );
  Q.insert( int( T.size() ) - 1 , f );
  }
 while( Q.size() > 1 ) {
  const F f1 = Q.minPriority();
  const int c1 = Q.deleteMin();
  const F f2 = Q.minPriority();
  const int c2 = Q.deleteMin();
  T.push_back( { c1 , c2 , C() } );              // a virtual character
  Q.insert( int( T.size() ) - 1 , f1 + f2 );
  }
 std::map< C , std::string > code;
 auto walk = [ & ]( auto & self , int x , const std::string & s ) -> void {
  if( T[ x ].left < 0 ) {
   code[ T[ x ].c ] = s.empty() ? "0" : s;
   return;
   }
  self( self , T[ x ].left , s + '0' );
  self( self , T[ x ].right , s + '1' );
  };
 walk( walk , Q.deleteMin() , "" );
 return( code );
 }

/// the text encoded by the codes
template< class C >
std::string encode( const std::basic_string< C > & text ,
                    const std::map< C , std::string > & code )
{
 std::string bits;
 for( C c : text )
  bits += code.at( c );
 return( bits );
 }

/// the bits decoded by the codes, a prefix code
template< class C >
std::basic_string< C > decode( const std::string & bits ,
                               const std::map< C , std::string > & code )
{
 std::map< std::string , C > back;
 for( const auto & [ c , s ] : code )
  back[ s ] = c;
 std::basic_string< C > text;
 std::string s;
 for( char b : bits ) {
  s += b;
  if( auto it = back.find( s ) ; it != back.end() ) {
   text += it->second;
   s.clear();
   }
  }
 if( ! s.empty() )
  throw( std::invalid_argument( "ADS::decode: bits left over" ) );
 return( text );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* Huffman.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Huffman.h ---------------------------*/
/*--------------------------------------------------------------------------*/
