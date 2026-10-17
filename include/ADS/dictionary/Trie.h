/*--------------------------------------------------------------------------*/
/*------------------------------ File Trie.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Two digital search trees of strings, beyond the book:
 *
 * - Trie, the tree of Fredkin whose node at depth d stands for a prefix of
 *   d characters, the children of a node being indexed by the next
 *   character (here by a sorted map); a string is there if the node of its
 *   last character is marked. A search, an insertion and a removal take
 *   \f$ O( m \log \sigma ) \f$ time for a string of m characters on an
 *   alphabet of \sigma, independently of the number of strings, and the
 *   strings with a given prefix are those below the node of the prefix,
 *   visited in their order;
 * - RadixTree, the compact trie (the Patricia of Morrison, on characters
 *   rather than on bits), where every chain of nodes with a single child
 *   and no mark is merged into one edge labelled by a string, so that it
 *   has at most 2 n nodes for n strings, however long they are; an
 *   insertion splits an edge where the new string departs from it, and a
 *   removal merges back a node left with a single child.
 *
 * Both map each string to a value, i.e., they are a DICTIONARY [see ADT.h]
 * whose keys are strings, with keysWithPrefix() besides. The contracts [see
 * Contract.h]: insert() ensures that the key is there and remove() that it
 * is not; the invariant of RadixTree is that no node but the root has a
 * single child and no mark, and that the labels of the edges out of a node
 * begin with distinct characters. The operations are defined in Trie.tpp.
 *
 * \see E. Fredkin, Trie memory, Communications of the ACM 3(9), 490-499,
 *      1960, doi:10.1145/367390.367400
 * \see D. R. Morrison, PATRICIA: practical algorithm to retrieve
 *      information coded in alphanumeric, Journal of the ACM 15(4),
 *      514-534, 1968, doi:10.1145/321479.321481
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, 2nd ed.,
 *      Addison-Wesley, 1998, section 6.3
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

#ifndef __ADS_Trie
 #define __ADS_Trie
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------------ CLASS Trie --------------------------------*/
/*--------------------------------------------------------------------------*/

/// a DICTIONARY of strings on a trie
template< class V >
class Trie {

/*--------------------------------------------------------------------------*/
 public:

 using Key = std::string;   ///< the type of the keys
 using Value = V;           ///< the type of the values

 [[nodiscard]] std::optional< V > lookup( std::string_view k ) const;

 /// \post k has the value v
 void insert( std::string_view k , const V & v );

 /// \post k is not there
 void remove( std::string_view k );

 [[nodiscard]] bool contains( std::string_view k ) const;

 [[nodiscard]] std::size_t size( void ) const;

 /// the keys that begin with p, in increasing order
 [[nodiscard]] std::vector< std::string > keysWithPrefix(
                                                  std::string_view p ) const;

 /// the number of nodes, the root included
 [[nodiscard]] std::size_t nodes( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 struct Node {
  std::map< char , std::unique_ptr< Node > > next;
  std::optional< V > value;      ///< set if a key ends here
  };

 /// the node of the prefix p, nil if none
 [[nodiscard]] const Node * find( std::string_view p ) const;

 /// removes k below x from its character d; true if x became useless
 bool remove( Node * x , std::string_view k , std::size_t d );

 static void collect( const Node * x , std::string & s ,
                      std::vector< std::string > & out );

 static std::size_t count( const Node * x );

 Node f_root;              ///< the root, the empty prefix
 std::size_t f_size = 0;   ///< the number of keys

 };  // end( class Trie )

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS RadixTree -----------------------------*/
/*--------------------------------------------------------------------------*/

/// a DICTIONARY of strings on a compact trie
template< class V >
class RadixTree {

/*--------------------------------------------------------------------------*/
 public:

 using Key = std::string;   ///< the type of the keys
 using Value = V;           ///< the type of the values

 [[nodiscard]] std::optional< V > lookup( std::string_view k ) const;

 /// \post k has the value v
 void insert( std::string_view k , const V & v );

 /// \post k is not there
 void remove( std::string_view k );

 [[nodiscard]] bool contains( std::string_view k ) const;

 [[nodiscard]] std::size_t size( void ) const;

 /// the keys that begin with p, in increasing order
 [[nodiscard]] std::vector< std::string > keysWithPrefix(
                                                  std::string_view p ) const;

 /// the number of nodes, the root included
 [[nodiscard]] std::size_t nodes( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 struct Node {
  std::string label;             ///< the label of the edge into it
  std::map< char , std::unique_ptr< Node > > next;
  std::optional< V > value;      ///< set if a key ends here
  };

 /// removes k below x; true if x is left with no value and no child
 bool remove( Node * x , std::string_view k );

 /// merges x with its only child, if it has no value
 static void compress( Node * x );

 static void collect( const Node * x , std::string & s ,
                      std::vector< std::string > & out );

 static std::size_t count( const Node * x );

 /// no node but the root has one child and no value, O( n )
 static bool compact( const Node * x , bool root );

 Node f_root;              ///< the root, with the empty label
 std::size_t f_size = 0;   ///< the number of keys

 };  // end( class RadixTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/Trie.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Trie.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Trie.h -----------------------------*/
/*--------------------------------------------------------------------------*/
