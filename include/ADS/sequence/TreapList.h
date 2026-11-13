/*--------------------------------------------------------------------------*/
/*---------------------------- File TreapList.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The `TreapList` of exercise 7.11 of Morin, beyond the book: a sequence with
 * access, insertion and removal at any index in \f$ O( \log n ) \f$ expected
 * time, on a treap [see `Treap.h`] whose symmetric order is the order of the
 * sequence and whose nodes keep the size of their subtree, so that the index
 * of a node is the size of the left subtrees passed on the right along its
 * path (an implicit treap: the keys are the indices, never stored). An
 * insertion at \f$ i \f$ goes down by the sizes to the leaf between the items
 * \f$ i - 1 \f$ and \f$ i \f$ and rotates the new node up while its priority
 * beats its parent's; a removal rotates the node down until it has one child
 * at most. The shape is that of a random binary search tree on the indices,
 * whatever the order of the operations, as for the keys of a treap.
 *
 * `truncate( i )` splits the treap at the index \f$ i \f$ and `absorb( s )`
 * joins the treap of another sequence at the end, both along two spines in
 * \f$ O( \log n ) \f$ expected time, as `SkipListSequence` does on a skip
 * list. The priorities come from `SplitMix64` [see `RandomHashing.h`].
 *
 * Morin gives no code for it: the benchmarks compare it with
 * `SkipListSequence`, whose operations it has, and with his `SkiplistList`;
 * a node here holds the item, a priority, a size and two links, one block of
 * memory, and a search reads one node per level.
 *
 * The contracts [see `Contract.h`]: an index is below `size()` (at most
 * `size()` for an insertion and a truncation); the invariant, an
 * \f$ O( n ) \f$ check at the level 3, is the heap order of the priorities
 * and the sizes. The operations are defined in `TreapList.tpp`.
 *
 * \see R. Seidel, C. R. Aragon, Randomized search trees, Algorithmica
 *      16(4-5), 464-497, 1996, doi:10.1007/BF01940876
 * \see P. Morin, Open Data Structures (in C++), Athabasca University Press,
 *      2013, section 7.2 and exercise 7.11,
 *      doi:10.15215/aupress/9781927356388.01
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

#ifndef __ADS_TreapList
 #define __ADS_TreapList
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/dictionary/RandomHashing.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS TreapList -----------------------------*/
/*--------------------------------------------------------------------------*/

/// a sequence of items of type `T` on an implicit treap, the `TreapList`
/// of Morin
template< class T >
class TreapList {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;    ///< the type of the items

 /// the empty sequence, its priorities drawn from the seed
 explicit TreapList( std::uint64_t seed = 1 ) : f_g( seed ) {}

 /// the number \f$ n \f$ of items
 [[nodiscard]] std::size_t size( void ) const { return( sz( f_root ) ); }

 /// true if there is no item
 [[nodiscard]] bool empty( void ) const { return( ! f_root ); }

 /// the item of index \f$ i \f$, in \f$ O( \log n ) \f$ expected time
 ///
 /// In pseudocode (exercise 7.11 of Morin), \f$ s \f$ the sizes:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{get}( i ) \cr
 /// 2 & \quad t \gets \mathit{root} \cr
 /// 3 & \quad \textbf{while}\ i \ne s( t.\mathit{left} )\ \textbf{do} \cr
 /// 4 & \qquad \textbf{if}\ i < s( t.\mathit{left} )\ \textbf{then}\ t
 ///     \gets t.\mathit{left} \cr
 /// 5 & \qquad \textbf{else}\ i \gets i - s( t.\mathit{left} ) - 1;\ t
 ///     \gets t.\mathit{right} \cr
 /// 6 & \quad \textbf{return}\ t.\mathit{item}
 /// \end{array}}
 /// \f]
 /// \pre \f$ i < n \f$
 [[nodiscard]] const T & get( std::size_t i ) const;

 /// the item of index \f$ i \f$ replaced by \f$ v \f$, by the descent of
 /// `get()`
 /// \pre \f$ i < n \f$
 void set( std::size_t i , const T & v );

 /// \f$ v \f$ inserted at the index \f$ i \f$, in \f$ O( \log n ) \f$
 /// expected time
 ///
 /// In pseudocode (exercise 7.11 of Morin), \f$ p \f$ the priorities:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{add}( t , i , v ) \cr
 /// 2 & \quad \textbf{if}\ t = \textbf{nil}\ \textbf{then}\ t \gets
 ///     \text{a node of } v \text{ and a random priority};\ \textbf{return}
 ///     \cr
 /// 3 & \quad \textbf{if}\ i \le s( t.\mathit{left} )\ \textbf{then} \cr
 /// 4 & \qquad \mathrm{add}( t.\mathit{left} , i , v );\ \textbf{if}\ p(
 ///     t.\mathit{left} ) > p( t )\ \textbf{then}\ \mathrm{rotateRight}( t )
 ///     \cr
 /// 5 & \quad \textbf{else}\ \mathrm{add}( t.\mathit{right} , i - s(
 ///     t.\mathit{left} ) - 1 , v );\ \text{and the mirror of line 4} \cr
 /// 6 & \quad \text{the size of } t \text{ updated}
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \le n \f$
 void add( std::size_t i , const T & v );

 /// \f$ v \f$ appended, `add( size() , v )`
 void push_back( const T & v ) { add( size() , v ); }

 /// the item of index \f$ i \f$ removed and returned, in
 /// \f$ O( \log n ) \f$ expected time, the node rotated down as in
 /// `Treap::remove()`
 /// \pre \f$ i < n \f$
 T remove( std::size_t i );

 /// the items of index \f$ i \f$ to \f$ n - 1 \f$ taken out and returned as
 /// a sequence, by the split of the treap at \f$ i \f$, in
 /// \f$ O( \log n ) \f$ expected time
 ///
 /// In pseudocode (Seidel and Aragon, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{split}( t , i ) \cr
 /// 2 & \quad \textbf{if}\ t = \textbf{nil}\ \textbf{then return}
 ///     \ ( \textbf{nil} , \textbf{nil} ) \cr
 /// 3 & \quad \textbf{if}\ i \le s( t.\mathit{left} )\ \textbf{then} \cr
 /// 4 & \qquad ( a , b ) \gets \mathrm{split}( t.\mathit{left} , i );
 ///     \ t.\mathit{left} \gets b;\ \textbf{return}\ ( a , t ) \cr
 /// 5 & \quad ( a , b ) \gets \mathrm{split}( t.\mathit{right} , i - s(
 ///     t.\mathit{left} ) - 1 );\ t.\mathit{right} \gets a;\ \textbf{return}
 ///     \ ( t , b )
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \le n \f$
 [[nodiscard]] TreapList truncate( std::size_t i );

 /// the items of \f$ s \f$ appended, in order, and \f$ s \f$ left empty, by
 /// the join of the two treaps, in \f$ O( \log n ) \f$ expected time
 ///
 /// In pseudocode (Seidel and Aragon, beyond the book), the two right and
 /// left spines merged by the priorities:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{join}( a , b ) \cr
 /// 2 & \quad \textbf{if}\ a = \textbf{nil}\ \textbf{or}\ b = \textbf{nil}
 ///     \ \textbf{then return}\ \text{the other} \cr
 /// 3 & \quad \textbf{if}\ p( a ) > p( b )\ \textbf{then}\ a.\mathit{right}
 ///     \gets \mathrm{join}( a.\mathit{right} , b );\ \textbf{return}\ a \cr
 /// 4 & \quad b.\mathit{left} \gets \mathrm{join}( a , b.\mathit{left} );
 ///     \ \textbf{return}\ b
 /// \end{array}}
 /// \f]
 void absorb( TreapList & s );

/*--------------------------------------------------------------------------*/
 private:

 /// a node: the item, its priority, the size of its subtree
 struct Node {
  T item;                          ///< the item
  std::uint64_t priority;          ///< the random priority
  std::size_t s = 1;               ///< the nodes of the subtree
  std::unique_ptr< Node > left ,   ///< the left child
                          right;   ///< the right child
  };

 public:

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 /// the shape of the tree, for `PathIterator`
 struct Shape {
  /// the root of the tree `o`
  static auto root( const auto & o ) { return( o.f_root.get() ); }
  /// the left child of the node `n`
  static auto left( const auto * n ) { return( n->left.get() ); }
  /// the right child of the node `n`
  static auto right( const auto * n ) { return( n->right.get() ); }
  /// the item of the node `n`
  static const T & read( const auto * n ) { return( n->item ); }
  };

 using value_type = T;                     ///< the items
 using size_type = std::size_t;            ///< the type of the sizes
 using difference_type = std::ptrdiff_t;   ///< the type of the distances
 using reference = const T &;              ///< a constant reference
 using const_reference = const T &;        ///< a constant reference
 /// the constant iterator in the order of the sequence
 using const_iterator = PathIterator< TreapList , Shape >;
 using iterator = const_iterator;          ///< the constant iterator

 /// the iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , true ) );
  }
 /// the iterator past the last item
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , false ) );
  }
 /// as `begin()`
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// as `end()`
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 private:
 using Ptr = std::unique_ptr< Node >;

 static std::size_t sz( const Ptr & t ) { return( t ? t->s : 0 ); }
 static void update( Ptr & t ) { t->s = 1 + sz( t->left ) + sz( t->right ); }
 static void rotateRight( Ptr & t );
 static void rotateLeft( Ptr & t );
 Node * nodeAt( std::size_t i ) const;
 void add( Ptr & t , std::size_t i , const T & v );
 static T remove( Ptr & t , std::size_t i );
 static std::pair< Ptr , Ptr > split( Ptr t , std::size_t i );
 static Ptr join( Ptr a , Ptr b );
 static bool valid( const Ptr & t );

 Ptr f_root;                       ///< the root
 SplitMix64 f_g;                   ///< the generator of the priorities

 };  // end( class TreapList )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/impl/TreapList.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* TreapList.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File TreapList.h --------------------------*/
/*--------------------------------------------------------------------------*/
