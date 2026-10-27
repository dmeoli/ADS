/*--------------------------------------------------------------------------*/
/*------------------------------ File BTree.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The B-tree of Bayer and McCreight, beyond the book [see `BPlusTree.h` for
 * the B+-tree of chapter 6, which has no removal]: a search tree whose
 * nodes hold from \f$ t - 1 \f$ to \f$ 2 t - 1 \f$ keys (the root from 1),
 * \f$ t \f$ the minimum degree, a node of \f$ j \f$ keys having
 * \f$ j + 1 \f$ children, and all the leaves at the same depth, so that the
 * height is at most \f$ \log_t( ( n + 1 ) / 2 ) \f$ and a node fills a
 * block of the disk. `insert()` goes down from the root splitting each full
 * node it meets, so that the leaf reached has room; `remove()` goes down
 * making sure that each node it enters has at least \f$ t \f$ keys, by
 * borrowing a key from a sibling or by merging with it, so that a key can
 * always be taken out of a leaf (a key of an internal node is replaced by
 * its predecessor or successor, or the two children around it are merged);
 * both in one pass, \f$ O( t \log_t n ) \f$ time. It is a DICTIONARY [see
 * `ADT.h`], and `forEach()` visits the pairs by increasing keys.
 *
 * The contracts [see `Contract.h`]: \f$ t \ge 2 \f$; `insert()` ensures that
 * the key is there and `remove()` that it is not; the invariant is the order
 * of the keys, their number in each node and the depth of the leaves, an
 * \f$ O( n ) \f$ check at the level 3. The operations are defined in
 * `BTree.tpp`.
 *
 * \see R. Bayer, E. M. McCreight, Organization and maintenance of large
 *      ordered indexes, Acta Informatica 1(3), 173-189, 1972,
 *      doi:10.1007/BF00288683
 * \see T. H. Cormen, C. E. Leiserson, R. L. Rivest, C. Stein, Introduction
 *      to Algorithms, 3rd ed., MIT Press, 2009, chapter 18
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, 2nd ed.,
 *      Addison-Wesley, 1998, section 6.2.4
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

#ifndef __ADS_BTree
 #define __ADS_BTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS BTree --------------------------------*/
/*--------------------------------------------------------------------------*/

/// a DICTIONARY on a B-tree of minimum degree \f$ t \f$
template< std::totally_ordered K , class V >
class BTree {

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 /// the empty B-tree of minimum degree \f$ t \f$
 /// \pre \f$ t \ge 2 \f$
 explicit BTree( int t = 3 );

 /// the value of the key \f$ k \f$, or nothing
 ///
 /// In pseudocode (Bayer and McCreight, Cormen et al., chapter 18, beyond
 /// the book), \f$ x.k[ 0 .. j - 1 ] \f$ the keys of a node \f$ x \f$, in
 /// increasing order, and \f$ x.c[ 0 .. j ] \f$ its children:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{lookup}( k ) \cr
 /// 2 & \quad x \gets \mathit{root} \cr
 /// 3 & \quad \textbf{while}\ x \ne \textbf{nil}\ \textbf{do} \cr
 /// 4 & \qquad i \gets \text{the first place with } x.k[ i ] \ge k \cr
 /// 5 & \qquad \textbf{if}\ i < j\ \textbf{and}\ x.k[ i ] = k
 ///     \ \textbf{then return}\ x.v[ i ] \cr
 /// 6 & \qquad \textbf{if}\ x \text{ is a leaf}\ \textbf{then return nil}
 ///     \cr
 /// 7 & \qquad x \gets x.c[ i ] \cr
 /// 8 & \quad \textbf{return nil}
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// gives the key \f$ k \f$ the value \f$ v \f$, in one pass down from the
 /// root
 ///
 /// In pseudocode (Bayer and McCreight, Cormen et al., chapter 18, beyond
 /// the book), each full node met split before it is entered, so that its
 /// parent has room for the median:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( k , v ) \cr
 /// 2 & \quad \textbf{if}\ \mathit{root} \text{ has } 2 t - 1 \text{ keys}
 ///     \ \textbf{then} \cr
 /// 3 & \qquad \mathit{root} \gets \text{a new node with the old root as }
 ///     \text{its child};\ \mathrm{split}( \mathit{root} , 0 ) \cr
 /// 4 & \quad x \gets \mathit{root} \cr
 /// 5 & \quad \textbf{loop} \cr
 /// 6 & \qquad i \gets \text{the first place with } x.k[ i ] \ge k \cr
 /// 7 & \qquad \textbf{if}\ x.k[ i ] = k\ \textbf{then}\ x.v[ i ] \gets v;
 ///     \ \textbf{return} \cr
 /// 8 & \qquad \textbf{if}\ x \text{ is a leaf}\ \textbf{then}
 ///     \ \text{put } ( k , v ) \text{ at the place } i \text{ of } x;
 ///     \ \textbf{return} \cr
 /// 9 & \qquad \textbf{if}\ x.c[ i ] \text{ has } 2 t - 1 \text{ keys}
 ///     \ \textbf{then} \cr
 /// 10 & \qquad\quad \mathrm{split}( x , i ) \cr
 /// 11 & \qquad\quad \textbf{if}\ x.k[ i ] < k\ \textbf{then}\ i \gets i +
 ///     1 \cr
 /// 12 & \qquad\quad \textbf{else if}\ x.k[ i ] = k\ \textbf{then}\ x.v[ i
 ///     ] \gets v;\ \textbf{return} \cr
 /// 13 & \qquad x \gets x.c[ i ] \cr
 /// 14 & \textbf{procedure}\ \mathrm{split}( x , i )\quad \triangleright
 ///     \ \text{the child } y = x.c[ i ] \text{ has } 2 t - 1 \text{ keys} \cr
 /// 15 & \quad z \gets \text{a new node with the keys } y.k[ t .. 2 t - 2 ]
 ///     \text{ and their children} \cr
 /// 16 & \quad \text{put } y.k[ t - 1 ] \text{ at the place } i \text{ of }
 ///     x \text{, and } z \text{ as the child } i + 1 \cr
 /// 17 & \quad \text{keep in } y \text{ the keys } y.k[ 0 .. t - 2 ]
 ///     \text{ and their children}
 /// \end{array}}
 /// \f]
 /// \post \f$ k \f$ has the value \f$ v \f$
 void insert( const K & k , const V & v );

 /// removes the key \f$ k \f$, if there, in one pass down from the root
 ///
 /// In pseudocode (Cormen et al., chapter 18, beyond the book), each node
 /// entered below the root having at least \f$ t \f$ keys, so that a key
 /// can leave it:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{remove}( x , k ) \cr
 /// 2 & \quad \textbf{loop} \cr
 /// 3 & \qquad i \gets \text{the first place with } x.k[ i ] \ge k \cr
 /// 4 & \qquad \textbf{if}\ x \text{ is a leaf}\ \textbf{then}
 ///     \ \text{take } k \text{ out of } x \text{, if there};
 ///     \ \textbf{return} \cr
 /// 5 & \qquad \textbf{if}\ x.k[ i ] = k\ \textbf{then} \cr
 /// 6 & \qquad\quad \textbf{if}\ x.c[ i ] \text{ has } t \text{ keys or }
 ///     \text{more}\ \textbf{then} \cr
 /// 7 & \qquad\qquad k' \gets \text{the predecessor of } k;\ x.k[ i ]
 ///     \gets k';\ \mathrm{remove}( x.c[ i ] , k' );\ \textbf{return} \cr
 /// 8 & \qquad\quad \textbf{else if}\ x.c[ i + 1 ] \text{ has } t
 ///     \text{ keys or more}\ \textbf{then} \cr
 /// 9 & \qquad\qquad k' \gets \text{the successor of } k;\ x.k[ i ]
 ///     \gets k';\ \mathrm{remove}( x.c[ i + 1 ] , k' );
 ///     \ \textbf{return} \cr
 /// 10 & \qquad\quad \mathrm{merge}( x , i );\ x \gets x.c[ i ];
 ///     \ \textbf{continue} \cr
 /// 11 & \qquad x \gets x.c[ \mathrm{fill}( x , i ) ] \cr
 /// 12 & \textbf{function}\ \mathrm{fill}( x , i )\quad \triangleright
 ///     \ \text{the child to go down, with } t \text{ keys at least} \cr
 /// 13 & \quad \textbf{if}\ x.c[ i ] \text{ has } t \text{ keys or more}
 ///     \ \textbf{then return}\ i \cr
 /// 14 & \quad \textbf{if}\ x.c[ i - 1 ] \text{ has } t \text{ keys or }
 ///     \text{more}\ \textbf{then} \cr
 /// 15 & \qquad \text{move } x.k[ i - 1 ] \text{ down to the front of }
 ///     x.c[ i ] \text{, the last key of } x.c[ i - 1 ] \text{ up to its }
 ///     \text{place};\ \textbf{return}\ i \cr
 /// 16 & \quad \textbf{if}\ x.c[ i + 1 ] \text{ has } t \text{ keys or }
 ///     \text{more}\ \textbf{then} \cr
 /// 17 & \qquad \text{move } x.k[ i ] \text{ down to the back of } x.c[ i ]
 ///     \text{, the first key of } x.c[ i + 1 ] \text{ up to its place};
 ///     \ \textbf{return}\ i \cr
 /// 18 & \quad \textbf{if}\ i \text{ is not the last child}
 ///     \ \textbf{then}\ \mathrm{merge}( x , i );\ \textbf{return}\ i \cr
 /// 19 & \quad \mathrm{merge}( x , i - 1 );\ \textbf{return}\ i - 1 \cr
 /// 20 & \textbf{procedure}\ \mathrm{merge}( x , j ) \cr
 /// 21 & \quad \text{append } x.k[ j ] \text{ and the keys and the }
 ///     \text{children of } x.c[ j + 1 ] \text{ to } x.c[ j ] \cr
 /// 22 & \quad \text{take } x.k[ j ] \text{ and } x.c[ j + 1 ] \text{ out }
 ///     \text{of } x
 /// \end{array}}
 /// \f]
 /// and a root left with no key gives way to its only child.
 /// \post \f$ k \f$ is not there
 void remove( const K & k );

 /// whether the key \f$ k \f$ is there, `lookup()` above
 [[nodiscard]] bool contains( const K & k ) const;

 /// the number \f$ n \f$ of keys
 [[nodiscard]] std::size_t size( void ) const;

 /// true if there is no key
 [[nodiscard]] bool empty( void ) const;

 /// the height, 0 for a single leaf, -1 if empty
 [[nodiscard]] int height( void ) const;

 /// `f( k , v )` on the pairs by increasing keys
 ///
 /// In pseudocode (the symmetric visit of section 6.1 of the book, on the
 /// nodes of many keys):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{visit}( x , f ) \cr
 /// 2 & \quad \textbf{if}\ x = \textbf{nil}\ \textbf{then return} \cr
 /// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ j - 1\ \textbf{do} \cr
 /// 4 & \qquad \textbf{if}\ x \text{ is not a leaf}\ \textbf{then}
 ///     \ \mathrm{visit}( x.c[ i ] , f ) \cr
 /// 5 & \qquad f( x.k[ i ] , x.v[ i ] ) \cr
 /// 6 & \quad \textbf{if}\ x \text{ is not a leaf}\ \textbf{then}
 ///     \ \mathrm{visit}( x.c[ j ] , f )
 /// \end{array}}
 /// \f]
 template< class F >
 void forEach( F && f ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// a node: its keys, their values and its children
 struct Node {
  std::vector< K > k;                         ///< the keys, increasing
  std::vector< V > v;                         ///< the values of the keys
  std::vector< std::unique_ptr< Node > > c;   ///< empty in a leaf
  /// true if the node has no child
  [[nodiscard]] bool leaf( void ) const { return( c.empty() ); }
  };

 public:

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( key , value ) by increasing key,
 // forward, with the path of ( node , index ) from the root: in a node
 // above the last, the index is the child the path went into, and the key
 // of that index comes once the child is done

 /// the iterator on the pairs by increasing keys, with the path from the
 /// root
 class const_iterator {
  public:
  /// a pair of constant references, the key and its value
  using reference = std::pair< const K & , const V & >;
  using value_type = reference;             ///< the pairs
  using difference_type = std::ptrdiff_t;   ///< the type of the distances
  /// the category of the iterator for the concepts of C++20, forward
  using iterator_concept = std::forward_iterator_tag;
  /// input for the older algorithms, since `*it` is a value
  using iterator_category = std::input_iterator_tag;

  /// the end
  const_iterator( void ) = default;

  /// on the first key below \f$ x \f$, or at the end if \f$ x \f$ is nil
  /// or empty
  explicit const_iterator( const Node * x ) {
   if( x && ! x->k.empty() ) leftmost( x );
   }

  /// the pair of the key at the end of the path
  reference operator*( void ) const {
   const auto & [ x , i ] = f_path.back();
   return( reference( x->k[ i ] , x->v[ i ] ) );
   }

  /// to the next key, \f$ O( 1 ) \f$ amortized and \f$ O( \log_t n ) \f$
  /// at most
  ///
  /// In pseudocode (the step of `PathIterator` of `Iterators.h` on the
  /// nodes of many keys, beyond the book), \f$ ( x , i ) \f$ the last place
  /// of the path \f$ \pi \f$:
  /// \f[
  /// \class{pseudocode}{\begin{array}{r@{\quad}l}
  /// 1 & \textbf{procedure}\ \mathrm{next}() \cr
  /// 2 & \quad \textbf{if}\ x \text{ is not a leaf}\ \textbf{then}\ i
  ///     \gets i + 1;\ \text{append the leftmost path from } x.c[ i ]
  ///     \text{ to } \pi;\ \textbf{return} \cr
  /// 3 & \quad i \gets i + 1;\ \textbf{if}\ i < j\ \textbf{then return} \cr
  /// 4 & \quad \text{remove the places of } \pi \text{ from the end while }
  ///     \text{their node has no key left at their index}
  /// \end{array}}
  /// \f]
  const_iterator & operator++( void ) {
   auto & [ x , i ] = f_path.back();
   if( ! x->c.empty() ) {                   // into the child after the key
    ++i;
    leftmost( x->c[ i ].get() );
    return( *this );
    }
   if( ++i < x->k.size() ) return( *this );
   f_path.pop_back();                       // up to the first ancestor
   while( ! f_path.empty() &&               // with a key left
          ( f_path.back().second >= f_path.back().first->k.size() ) )
    f_path.pop_back();
   return( *this );
   }

  /// to the next key, returning the iterator before the step
  const_iterator operator++( int ) { auto t = *this; ++*this; return( t ); }

  /// equal on the same place of the same node, or both at the end
  bool operator==( const const_iterator & o ) const {
   return( f_path.empty() ? o.f_path.empty()
                          : ( ! o.f_path.empty() &&
                              ( f_path.back() == o.f_path.back() ) ) );
   }

  private:
  /// down from \f$ x \f$ to its first key, the places appended to the path
  void leftmost( const Node * x ) {
   for( ; ! x->c.empty() ; x = x->c[ 0 ].get() ) f_path.emplace_back( x , 0 );
   f_path.emplace_back( x , 0 );
   }

  /// the places \f$ ( x , i ) \f$ from the root
  std::vector< std::pair< const Node * , std::size_t > > f_path;
  };

 /// the pairs of constant references ( key , value )
 using value_type = std::pair< const K & , const V & >;
 using size_type = std::size_t;            ///< the type of the sizes
 using difference_type = std::ptrdiff_t;   ///< the type of the distances
 using reference = value_type;             ///< a pair, by value
 using const_reference = value_type;       ///< a pair, by value
 using iterator = const_iterator;          ///< the constant iterator

 /// the iterator on the smallest key
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( f_root.get() ) );
  }

 /// the iterator past the largest key
 [[nodiscard]] const_iterator end( void ) const { return( const_iterator() ); }

 /// as `begin()`
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }

 /// as `end()`
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 private:

 /// the first place in \f$ x \f$ of a key not smaller than `key`
 static std::size_t place( const Node * x , const K & key );

 /// splits the full child \f$ i \f$ of \f$ x \f$
 void split( Node * x , std::size_t i );

 /// merges the children \f$ j \f$ and \f$ j + 1 \f$ of \f$ x \f$ with the
 /// key \f$ j \f$ between them
 static void merge( Node * x , std::size_t j );

 /// makes the child \f$ i \f$ of \f$ x \f$ have at least \f$ t \f$ keys;
 /// the child to go down
 std::size_t fill( Node * x , std::size_t i );

 /// removes `key` from the subtree of \f$ x \f$; true if it was there
 bool remove( Node * x , const K & key );

 /// `f( k , v )` on the pairs of the subtree of \f$ x \f$, by increasing
 /// keys
 template< class F >
 static void visit( const Node * x , F & f );

 /// the order, the sizes and the depth of the leaves, \f$ O( n ) \f$
 bool valid( const Node * x , const K * lo , const K * hi , int d ,
             int & leafDepth , bool root ) const;

 int f_t;                          ///< the minimum degree
 std::unique_ptr< Node > f_root;   ///< the root
 std::size_t f_size = 0;           ///< the number of keys

 };  // end( class BTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/BTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* BTree.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File BTree.h ----------------------------*/
/*--------------------------------------------------------------------------*/
