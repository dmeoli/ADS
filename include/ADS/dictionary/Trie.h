/*--------------------------------------------------------------------------*/
/*------------------------------ File Trie.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Two digital search trees of strings, beyond the book:
 *
 * - `Trie`, the tree of Fredkin whose node at depth \f$ d \f$ stands for a
 *   prefix of \f$ d \f$ characters, the children of a node being indexed by
 *   the next character (here by a sorted map); a string is there if the
 *   node of its last character is marked. A search, an insertion and a
 *   removal take \f$ O( m \log \sigma ) \f$ time for a string of \f$ m \f$
 *   characters on an alphabet of \f$ \sigma \f$, independently of the number
 *   of strings, and the strings with a given prefix are those below the
 *   node of the prefix, visited in their order;
 * - `RadixTree`, the compact trie (the Patricia of Morrison, on characters
 *   rather than on bits), where every chain of nodes with a single child
 *   and no mark is merged into one edge labeled by a string, so that it
 *   has at most \f$ 2 n \f$ nodes for \f$ n \f$ strings, however long they
 *   are; an insertion splits an edge where the new string departs from it,
 *   and a removal merges back a node left with a single child.
 *
 * Both map each string to a value, i.e., they are a DICTIONARY [see
 * `ADT.h`] whose keys are strings, with `keysWithPrefix()` besides. The
 * contracts [see `Contract.h`]: `insert()` ensures that the key is there and
 * `remove()` that it is not; the invariant of `RadixTree` is that no node but
 * the root has a single child and no mark, and that the labels of the edges
 * out of a node begin with distinct characters. The operations are defined
 * in `Trie.tpp`.
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
 *         Università di Pisa \n
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
#include <iterator>
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

namespace detail {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS TrieIterator ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the keys of a trie of nodes `N` in lexicographic order, by a visit in
/// depth with a stack of the children still to see; the key is rebuilt
/// along the path, a character per edge, or the label of the child if
/// `Labeled`; `*it` gives the pair \f$ ( \mathit{key} , \mathit{value} ) \f$
/// by value
template< class N , class V , bool Labeled >
class TrieIterator {
 public:
 /// a reference to an item
 using reference = std::pair< std::string , const V & >;
 /// the type of the items, for the standard library
 using value_type = reference;
 /// the type of the distances between two iterators
 using difference_type = std::ptrdiff_t;
 /// the concept of the iterator, for C++20
 using iterator_concept = std::forward_iterator_tag;
 /// the category of the iterator
 using iterator_category = std::input_iterator_tag;

 /// the end
TrieIterator( void ) = default;

 /// on the first key below `root` (`root` included), or at the end if nil
 explicit TrieIterator( const N * root ) {
  if( ! root ) return;
  f_S.push_back( Frame{ root , root->next.begin() , 0 } );
  if( ! root->value ) advance();
  }

 /// the key and its value
 reference operator*( void ) const {
  return( reference( f_key , *f_S.back().node->value ) );
  }

 /// to the next key in lexicographic order
 TrieIterator & operator++( void ) {
  advance();
  return( *this );
  }
 /// to the next key, returning the iterator before the step
TrieIterator operator++( int ) { auto t = *this; ++*this; return( t ); }

 /// equal on the same node, or both at the end
 bool operator==( const TrieIterator & o ) const {
  return( f_S.empty() ? o.f_S.empty()
                      : ( ! o.f_S.empty() &&
                          ( f_S.back().node == o.f_S.back().node ) ) );
  }

 private:
 struct Frame {
  const N * node;                           ///< the node
  typename decltype( N::next )::const_iterator child;   ///< the next one
  std::size_t length;                       ///< of the key above the node
  };

 /// to the next node with a value in preorder, or to the end
 void advance( void ) {
  while( ! f_S.empty() ) {
   Frame & f = f_S.back();
   if( f.child == f.node->next.end() ) {    // its subtree is done
    f_key.resize( f.length );
    f_S.pop_back();
    continue;
    }
   const std::size_t len = f_key.size();
   const auto & [ c , x ] = *f.child++;
   if constexpr( Labeled ) f_key += x->label; else f_key += c;
   f_S.push_back( Frame{ x.get() , x->next.begin() , len } );
   if( x->value ) return;
   }
  }

 std::vector< Frame > f_S;                  ///< the path from the root
 std::string f_key;                         ///< the key of the path

 };  // end( class TrieIterator )

}  // end( namespace detail )

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

 /// the value of the key \f$ k \f$, or nothing, in
 /// \f$ O( |k| \log \sigma ) \f$ time
 ///
 /// In pseudocode (Fredkin, section 6.3 of TAOCP, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{lookup}( k ) \cr
 /// 2 & \quad x \gets \mathit{root} \cr
 /// 3 & \quad \textbf{foreach}\ \text{character } c \text{ of } k
 ///     \ \textbf{do} \cr
 /// 4 & \qquad x \gets \text{the child of } x \text{ by } c;\ \textbf{if}\ x
 ///     = \textbf{nil}\ \textbf{then return nil} \cr
 /// 5 & \quad \textbf{return}\ x.\mathit{value}
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::optional< V > lookup( std::string_view k ) const;

 /// associates the value \f$ v \f$ to the key \f$ k \f$, the path of
 /// \f$ k \f$ made by the nodes missing, in \f$ O( |k| \log \sigma ) \f$ time
 /// \post \f$ k \f$ has the value \f$ v \f$
 void insert( std::string_view k , const V & v );

 /// removes the key \f$ k \f$, if it is there, and the nodes of its path
 /// left with no value and no child, bottom up
 /// \post \f$ k \f$ is not there
 void remove( std::string_view k );

 /// true if the key \f$ k \f$ is there
 [[nodiscard]] bool contains( std::string_view k ) const;

 /// the number of keys
 [[nodiscard]] std::size_t size( void ) const;

 /// the keys that begin with \f$ p \f$, in increasing order: those below the
 /// node of \f$ p \f$
 [[nodiscard]] std::vector< std::string > keysWithPrefix(
                                                  std::string_view p ) const;

 /// the number of nodes, the root included
 [[nodiscard]] std::size_t nodes( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 struct Node {
  /// the children, by character
  std::map< char , std::unique_ptr< Node > > next;
  std::optional< V > value;      ///< set if a key ends here
  };

 public:

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( key , value ) in lexicographic order,
 // forward; the key is rebuilt along the path

 /// the type of the items, for the standard library
 using value_type = std::pair< std::string , const V & >;
 /// the type of the sizes
 using size_type = std::size_t;
 /// the type of the distances between two iterators
 using difference_type = std::ptrdiff_t;
 /// a reference to an item
 using reference = value_type;
 /// a constant reference to an item
 using const_reference = value_type;
 /// the iterator on the items, constant
 using const_iterator = detail::TrieIterator< Node , V , false >;
 /// the iterator on the items
 using iterator = const_iterator;

 /// the iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( &f_root ) );
  }
 /// the iterator past the last item
 [[nodiscard]] const_iterator end( void ) const { return( const_iterator() ); }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 private:

 /// the node of the prefix \f$ p \f$, nil if none
 [[nodiscard]] const Node * find( std::string_view p ) const;

 /// removes \f$ k \f$ below \f$ x \f$ from its character \f$ d \f$; true if
/// \f$ x \f$ became useless
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

 /// the value of the key \f$ k \f$, or nothing, along the edges whose labels
 /// are pieces of \f$ k \f$
 [[nodiscard]] std::optional< V > lookup( std::string_view k ) const;

 /// associates the value \f$ v \f$ to the key \f$ k \f$
 ///
 /// In pseudocode (the compact trie, section 6.3 of TAOCP, beyond the book),
 /// \f$ \lambda( y ) \f$ the label of the edge into \f$ y \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( k , v ) \cr
 /// 2 & \quad x \gets \mathit{root} \cr
 /// 3 & \quad \textbf{while}\ k \ne \varepsilon\ \textbf{do} \cr
 /// 4 & \qquad y \gets \text{the child of } x \text{ by } k[ 0 ] \cr
 /// 5 & \qquad \textbf{if}\ y = \textbf{nil}\ \textbf{then}\ \text{add a }
 ///     \text{leaf under } x \text{ labeled } k \text{ with } v;
 ///     \ \textbf{return}
 ///     \cr
 /// 6 & \qquad j \gets \text{the longest common prefix of } \lambda( y )
 ///     \text{ and } k \cr
 /// 7 & \qquad \textbf{if}\ j < |\lambda( y )|\ \textbf{then}\ \text{split }
 ///     \text{the edge into } y \text{ by a node } y' \text{ at } j;\ y \gets
 ///     y'
 ///     \cr
 /// 8 & \qquad k \gets k[ j .. ];\ x \gets y \cr
 /// 9 & \quad x.\mathit{value} \gets v
 /// \end{array}}
 /// \f]
 /// \post \f$ k \f$ has the value \f$ v \f$
 void insert( std::string_view k , const V & v );

 /// removes the key \f$ k \f$, if it is there, merging back a node left with
 /// a single child and no value
 /// \post \f$ k \f$ is not there
 void remove( std::string_view k );

 /// true if the key \f$ k \f$ is there
 [[nodiscard]] bool contains( std::string_view k ) const;

 /// the number of keys
 [[nodiscard]] std::size_t size( void ) const;

 /// the keys that begin with \f$ p \f$, in increasing order
 [[nodiscard]] std::vector< std::string > keysWithPrefix(
                                                  std::string_view p ) const;

 /// the number of nodes, the root included
 [[nodiscard]] std::size_t nodes( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 struct Node {
  std::string label;             ///< the label of the edge into it
  /// the children, by the first character of their labels
  std::map< char , std::unique_ptr< Node > > next;
  std::optional< V > value;      ///< set if a key ends here
  };

 public:

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( key , value ) in lexicographic order,
 // forward; the key is rebuilt along the path

 /// the type of the items, for the standard library
 using value_type = std::pair< std::string , const V & >;
 /// the type of the sizes
 using size_type = std::size_t;
 /// the type of the distances between two iterators
 using difference_type = std::ptrdiff_t;
 /// a reference to an item
 using reference = value_type;
 /// a constant reference to an item
 using const_reference = value_type;
 /// the iterator on the items, constant
 using const_iterator = detail::TrieIterator< Node , V , true >;
 /// the iterator on the items
 using iterator = const_iterator;

 /// the iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( &f_root ) );
  }
 /// the iterator past the last item
 [[nodiscard]] const_iterator end( void ) const { return( const_iterator() ); }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 private:

 /// removes \f$ k \f$ below \f$ x \f$; true if \f$ x \f$ is left with no
/// value and no child
 bool remove( Node * x , std::string_view k );

 /// merges \f$ x \f$ with its only child, if it has no value
 static void compress( Node * x );

 static void collect( const Node * x , std::string & s ,
                      std::vector< std::string > & out );

 static std::size_t count( const Node * x );

 /// no node but the root has one child and no value, \f$ O( n ) \f$
 static bool compact( const Node * x , bool root );

 Node f_root;              ///< the root, with the empty label
 std::size_t f_size = 0;   ///< the number of keys

 };  // end( class RadixTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/Trie.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Trie.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Trie.h -----------------------------*/
/*--------------------------------------------------------------------------*/
