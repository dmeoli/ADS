/*--------------------------------------------------------------------------*/
/*---------------------------- File BTreeMap.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * An ordered map on a B+-tree in main memory, beyond the book: the
 * engineered ordered DICTIONARY of ADS [see `ADT.h`], the one that the
 * benchmark of the ordered dictionaries compares with `std::map`,
 * `absl::btree_map` and `tlx::btree_map`, while `BPlusTree` [see
 * `BPlusTree.h`] stays the tree of section 6.3 and `BTree` [see `BTree.h`]
 * that of Bayer and McCreight.
 *
 * The pairs are in the leaves, which hold up to \f$ L \f$ of them, the keys
 * in an array and the values in another one, so that a search reads the
 * keys alone, and which are linked in a list in both directions; an
 * internal node holds up to \f$ I \f$ keys and \f$ I + 1 \f$ children, the
 * child \f$ c[ i ] \f$ having the keys in \f$ ( k[ i - 1 ] , k[ i ] ] \f$,
 * i.e., \f$ k[ i ] \f$ is not smaller than the keys below \f$ c[ i ] \f$ and
 * smaller than those below \f$ c[ i + 1 ] \f$. \f$ L \f$ and \f$ I \f$ come
 * from the bytes \f$ B \f$ of a node (512 by default, 8 lines of cache),
 * \f$ L = B / ( \mathrm{sizeof}( K ) + \mathrm{sizeof}( V ) ) \f$ and
 * \f$ I = B / ( \mathrm{sizeof}( K ) + \mathrm{sizeof}( \mathit{void}\,* ) )
 * \f$, each between 4 and 255. The nodes keep no parent and no height: the
 * height is that of the tree, and an operation that climbs back keeps the
 * path of its descent.
 *
 * The place of a key in a node is the number of the keys smaller than it.
 * On the arithmetic keys under `std::less`, the unused places of a node hold
 * the largest value of the type (its infinity for the floating types), so
 * that the count runs on all the places at once, with no branch and no
 * dependence on the count of the node, as a few comparisons of vectors [see
 * `CountLess.h`] (four of AVX-512 for 32 keys of 64 bits), which beats a
 * binary search on a few lines of cache, whose branches are mispredicted
 * half of the times. On the other keys the place is a binary search. On the
 * way down, the lines of the keys of the next node are all prefetched at
 * once, so that their misses overlap instead of following one another, as
 * in the prefetching B+-trees of Chen, Gibbons and Mowry; a search of a
 * value prefetches the values of the leaf too, whose line is read right
 * after the keys. The nodes come from two pools [see `ObjectPool.h`] on huge
 * pages, so that a descent misses the TLB once per 2 MB rather than once
 * per node, and the internal nodes of a level are linked in a list, as the
 * leaves are.
 *
 * An insertion into a full node splits it in two halves, the largest key of
 * the left one going up into the parent, which may split in turn, up to the
 * root. An insertion past the largest key of the tree, or before the
 * smallest one, splits the node so that the old one stays full and the new
 * one gets the new key alone (the trick of the B-trees of Abseil and of the
 * bulk loaders): the keys inserted in increasing order fill all the nodes,
 * instead of half of them; and a key larger than all the others goes into
 * the last leaf, if it has room, with no descent, since no bound of an
 * internal node lies above the largest key. A removal that leaves a node
 * with fewer than half of its places full either merges it with a sibling,
 * if the two fit in one node, or takes from the sibling half of the
 * difference, so that the two end up balanced; a merge may leave the parent
 * short in turn, and a root with a single child gives way to it. Hence every
 * node but those of the leftmost and of the rightmost paths, and those the
 * removals have not touched since a split, is at least half full, so that,
 * but for the two nodes of each level on those paths, a level of internal
 * nodes has at least \f$ \lceil I / 2 \rceil + 1 \f$ times the nodes of the
 * level above and the leaves hold at least \f$ \lceil L / 2 \rceil \f$ pairs
 * each: the height of a tree of \f$ n \f$ pairs is about
 * \f$ 1 + \log_{\lceil I / 2 \rceil + 1}( n / \lceil L / 2 \rceil ) \f$ at
 * worst (5 levels for a million pairs of 64 bits, \f$ L = I = 32 \f$), and
 * all the operations take \f$ O( ( L + I ) \log n ) \f$ time, a few lines of
 * cache per level. `build()` loads a sorted sequence bottom up in
 * \f$ O( n ) \f$, the nodes as full and as even as they can be.
 *
 * With `R`, each internal node keeps the number of the pairs below each of
 * its children, \f$ \mathit{cnt}[ j ] = | \{ \text{pairs below } c[ j ] \} |
 * \f$, which an insertion or a removal adds or takes one to along its way
 * and a split, a merge or a loan counts again on the nodes it touches
 * (\f$ O( I ) \f$ children each); then `rank( k )`, the number of the keys
 * smaller than \f$ k \f$, is the sum of the counts of the children before
 * the way down plus the place in the leaf, and `select( i )`, the pair of
 * rank \f$ i \f$, goes down to the child whose counts pass \f$ i \f$, both
 * in \f$ O( ( L + I ) \log n ) \f$ time (the order-statistic tree of Cormen
 * et al. on a B-tree).
 *
 * The iterators are a leaf, a place in it and the parent of the leaf,
 * bidirectional, on constant pairs of references ( key , value ); `find()`
 * gives a value to change in place. An iterator steps to the next leaf
 * through the children of the parent (and through the list of the parents),
 * so that it prefetches the leaf 4 places ahead with no chase of the links
 * between the leaves, whose misses would otherwise follow one another in a
 * scan of a tree larger than the cache. An insertion or a removal
 * invalidates all the iterators. Keys and values must be default
 * constructible, the places of a node being built with the node, and a tree
 * that shrinks keeps the memory of its pools until it is cleared.
 *
 * The contracts [see `Contract.h`]: after `insert()` the key is there and
 * after `remove()` it is not; `build()` needs strictly increasing keys, an
 * audit; the invariant is the order of the keys in the tree and their
 * bounds, the depth of the leaves, the list of the leaves, the count and the
 * padding of the unused places, an \f$ O( n ) \f$ check at the level 3 on
 * the trees of up to 4096 pairs. The operations are defined in
 * `BTreeMap.tpp`.
 *
 * \see R. Bayer, E. M. McCreight, Organization and maintenance of large
 *      ordered indexes, Acta Informatica 1(3), 173-189, 1972,
 *      doi:10.1007/BF00288683
 * \see D. Comer, The ubiquitous B-tree, ACM Computing Surveys 11(2),
 *      121-137, 1979, doi:10.1145/356770.356776
 * \see S. Chen, P. B. Gibbons, T. C. Mowry, Improving index performance
 *      through prefetching, Proceedings of SIGMOD 2001, 235-246, 2001,
 *      doi:10.1145/375663.375688
 * \see T. H. Cormen, C. E. Leiserson, R. L. Rivest, C. Stein, Introduction
 *      to Algorithms, 4th ed., MIT Press, 2022, chapter 17 (augmenting
 *      data structures)
 * \see G. Graefe, Modern B-tree techniques, Foundations and Trends in
 *      Databases 3(4), 203-402, 2011, doi:10.1561/1900000028
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

#ifndef __ADS_BTreeMap
 #define __ADS_BTreeMap
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/memory/ObjectPool.h"
#include "ADS/search/CountLess.h"

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <new>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS BTreeMap ------------------------------*/
/*--------------------------------------------------------------------------*/

/// an ordered map on a B+-tree of nodes of \f$ B \f$ bytes, by the order
/// `less()`, with the counts of the pairs below each child if `R` (the
/// ranks)
template< class K , class V , class Less = std::less< K > ,
          std::size_t B = 512 , bool R = false >
 requires std::default_initializable< K > &&
          std::default_initializable< V >
class BTreeMap {

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 /// the pairs of a leaf
 static constexpr std::size_t L =
  std::clamp< std::size_t >( B / ( sizeof( K ) + sizeof( V ) ) , 4 , 255 );

 /// the keys of an internal node
 static constexpr std::size_t I =
  std::clamp< std::size_t >( B / ( sizeof( K ) + sizeof( void * ) ) , 4 ,
                             255 );

 /// the empty map, by the order `less`
 explicit BTreeMap( Less less = Less() );

 /// a copy of `o`, node by node
 BTreeMap( const BTreeMap & o );

 /// the nodes of `o`, which is left empty
 BTreeMap( BTreeMap && o ) noexcept;

 /// the pairs of `o`, by copy and swap
 BTreeMap & operator=( BTreeMap o ) noexcept;

 /// frees the nodes
 ~BTreeMap();

/*--------------------------------------------------------------------------*/

 /// the value of \f$ k \f$, if there
 ///
 /// In pseudocode (the search of a B+-tree, Comer, with the prefetching of
 /// Chen, Gibbons and Mowry, beyond the book), \f$ h \f$ the height:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{lookup}( k ) \cr
 /// 2 & \quad u \gets \mathit{root} \cr
 /// 3 & \quad \textbf{for}\ h' \gets h\ \textbf{downto}\ 2\ \textbf{do} \cr
 /// 4 & \qquad u \gets u.c[ \mathrm{place}( u , k ) ];\ \text{prefetch the }
 ///     \text{keys of } u \cr
 /// 5 & \quad p \gets \mathrm{place}( u , k ) \cr
 /// 6 & \quad \textbf{if}\ p < u.n\ \textbf{and}\ u.k[ p ] = k
 ///     \ \textbf{then return}\ u.v[ p ]\ \textbf{else return nil}
 /// \end{array}}
 /// \f]
 /// where \f$ \mathrm{place}( u , k ) \f$ is the number of the keys of
 /// \f$ u \f$ smaller than \f$ k \f$, counted on all the padded places at
 /// once on the arithmetic keys, by a binary search on the others.
 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// the value of \f$ k \f$, to change in place, or `nullptr`, by the
 /// descent of `lookup()`
 [[nodiscard]] const V * find( const K & k ) const;

 /// the value of \f$ k \f$, to change in place, or `nullptr`, by the
 /// descent of `lookup()`
 [[nodiscard]] V * find( const K & k );

 /// gives \f$ k \f$ the value \f$ v \f$; true if \f$ k \f$ was not there
 ///
 /// In pseudocode (the insertion of a B+-tree, Comer, beyond the book),
 /// \f$ \pi \f$ the path of the descent, the places \f$ ( u , i ) \f$ of
 /// the internal nodes:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insert}( k , v ) \cr
 /// 2 & \quad \textbf{if}\ \text{the tree is empty}\ \textbf{then}
 ///     \ \mathit{root} \gets \text{a leaf with } ( k , v );
 ///     \ \textbf{return true} \cr
 /// 3 & \quad \textbf{if}\ k > \text{the largest key}\ \textbf{and}
 ///     \ \text{the last leaf has room}\ \textbf{then} \cr
 /// 4 & \qquad \text{append } ( k , v ) \text{ to the last leaf};
 ///     \ \textbf{return true}\quad \triangleright\ \text{no descent} \cr
 /// 5 & \quad x \gets \text{the leaf reached from the root, }
 ///     \text{recording the path } \pi;\ p \gets \mathrm{place}( x , k ) \cr
 /// 6 & \quad \textbf{if}\ x.k[ p ] = k\ \textbf{then}\ x.v[ p ] \gets
 ///     v;\ \textbf{return false} \cr
 /// 7 & \quad \textbf{if}\ x.n < L\ \textbf{then}\ \text{put } ( k , v )
 ///     \text{ at the place } p \text{ of } x \cr
 /// 8 & \quad \textbf{else if}\ x \text{ is the last leaf and } p = L
 ///     \ \textbf{then}\ r \gets \text{a new leaf with } ( k , v ) \text{ }
 ///     \text{alone};\ \mathrm{climb}( x.k[ L - 1 ] , r , \pi ) \cr
 /// 9 & \quad \textbf{else if}\ x \text{ is the first leaf and } p = 0
 ///     \ \textbf{then}\ l \gets \text{a new leaf with } ( k , v ) \text{ }
 ///     \text{alone, before } x;\ \mathrm{climb}( k , l , \pi ) \cr
 /// 10 & \quad \textbf{else} \cr
 /// 11 & \qquad r \gets \text{a new leaf after } x \text{ with the last }
 ///     \lfloor ( L + 1 ) / 2 \rfloor \text{ of the } L + 1 \text{ pairs} \cr
 /// 12 & \qquad \mathrm{climb}( \text{the largest key left in } x , r , \pi
 ///     ) \cr
 /// 13 & \quad \textbf{return true} \cr
 /// 14 & \textbf{procedure}\ \mathrm{climb}( s , r , \pi )\quad
 ///     \triangleright\ \text{a child split: } s \text{ bounds its left }
 ///     \text{part, } r \text{ is new} \cr
 /// 15 & \quad \textbf{foreach}\ ( u , i ) \text{ of } \pi \text{, from }
 ///     \text{the leaf up}\ \textbf{do} \cr
 /// 16 & \qquad \textbf{if}\ u.n < I\ \textbf{then}\ \text{put } s
 ///     \text{ and } r \text{ after the child } i \text{ of } u;
 ///     \ \textbf{return} \cr
 /// 17 & \qquad \text{split the } I + 1 \text{ keys and } I + 2 \text{ }
 ///     \text{children into } u \text{ and a new } w \text{ after it,} \cr
 /// & \qquad\quad \text{almost all in } u \text{ on the right edge, in }
 ///     w \text{ on the left edge, half each otherwise} \cr
 /// 18 & \qquad s \gets \text{the key between } u \text{ and } w;\ r \gets
 ///     w \cr
 /// 19 & \quad \mathit{root} \gets \text{a new node with the key } s
 ///     \text{ and the children the old root and } r
 /// \end{array}}
 /// \f]
 /// and with `R` the counts along \f$ \pi \f$ grow by 1, and those of the
 /// nodes that split are counted again.
 bool insert( const K & k , const V & v );

 /// removes \f$ k \f$; true if it was there
 ///
 /// In pseudocode (the removal of a B+-tree, Comer, beyond the book),
 /// \f$ \pi \f$ the path of the descent:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{remove}( k ) \cr
 /// 2 & \quad x \gets \text{the leaf reached from the root, }
 ///     \text{recording the path } \pi;\ p \gets \mathrm{place}( x , k ) \cr
 /// 3 & \quad \textbf{if}\ x.k[ p ] \ne k\ \textbf{then return false} \cr
 /// 4 & \quad \text{take the pair of place } p \text{ out of } x \cr
 /// 5 & \quad \textbf{if}\ x \text{ is the root and empty}
 ///     \ \textbf{then}\ \text{the tree is empty} \cr
 /// 6 & \quad \textbf{else if}\ x.n < L / 2\ \textbf{then}
 ///     \ \mathrm{shrink}( x , \pi ) \cr
 /// 7 & \quad \textbf{return true} \cr
 /// 8 & \textbf{procedure}\ \mathrm{shrink}( x , \pi )\quad
 ///     \triangleright\ x \text{ is less than half full} \cr
 /// 9 & \quad ( u , i ) \gets \text{the last place of } \pi;\ l , r \gets
 ///     \text{the children of } u \text{ around } x \cr
 /// 10 & \quad \textbf{if}\ l \text{ and } r \text{ fit in one node}
 ///     \ \textbf{then} \cr
 /// 11 & \qquad \text{move } r \text{ (and, if internal, the bound of } u
 ///     \text{ between them) into } l;\ \text{take } r \text{ out of } u \cr
 /// 12 & \qquad \textbf{if}\ u \text{ is the root with no key}
 ///     \ \textbf{then}\ \mathit{root} \gets l \cr
 /// 13 & \qquad \textbf{else if}\ u \text{ is less than half full}
 ///     \ \textbf{then}\ \mathrm{shrink}( u , \pi \text{ without its last }
 ///     \text{place} ) \cr
 /// 14 & \quad \textbf{else}\ \text{move items between } l \text{ and } r
 ///     \text{ until they hold half each, and set the bound of } u
 /// \end{array}}
 /// \f]
 /// and with `R` the counts along \f$ \pi \f$ fall by 1, and those of the
 /// nodes that change are counted again.
 bool remove( const K & k );

 /// whether \f$ k \f$ is there, by the descent of `lookup()`
 [[nodiscard]] bool contains( const K & k ) const;

 /// the number \f$ n \f$ of pairs
 [[nodiscard]] std::size_t size( void ) const;

 /// true if there is no pair
 [[nodiscard]] bool empty( void ) const;

 /// the levels of the tree, 0 if empty, 1 if a single leaf
 [[nodiscard]] int height( void ) const;

 /// removes the pairs, the pools keeping their memory
 void clear( void );

 /// replaces the pairs with those of `[ first , last )`, pairs ( key ,
 /// value ) by strictly increasing keys, the nodes as full and as even as
 /// they can be, \f$ O( n ) \f$
 ///
 /// In pseudocode (the bulk loading of a B+-tree, Graefe, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{build}( P )\quad \triangleright
 ///     \ P \text{ the } n \text{ pairs, by increasing keys} \cr
 /// 2 & \quad \mathit{level} \gets \lceil n / L \rceil \text{ leaves, each }
 ///     \text{with } \lfloor n / \lceil n / L \rceil \rfloor \text{ or one }
 ///     \text{more pairs of } P \text{ in order, linked} \cr
 /// 3 & \quad \textbf{while}\ \mathit{level} \text{ has more than one }
 ///     \text{node}\ \textbf{do} \cr
 /// 4 & \qquad g \gets \lceil |\mathit{level}| / ( I + 1 ) \rceil \cr
 /// 5 & \qquad \mathit{level} \gets g \text{ internal nodes, each with }
 ///     \text{the next nodes of } \mathit{level} \text{ as children,} \cr
 /// 6 & \qquad\quad \text{as many to each as evenly as possible, and the }
 ///     \text{largest key below each child but the last as a bound} \cr
 /// 7 & \quad \mathit{root} \gets \text{the node of } \mathit{level}
 /// \end{array}}
 /// \f]
 /// \pre the keys are strictly increasing (an audit)
 template< std::input_iterator It >
 void build( It first , It last );

 /// `f( key , value )` on each pair, by increasing keys, along the
 /// iterators
 template< class F >
 void forEach( F && f ) const;

 /// `f( key , value )` on each pair of key in
 /// \f$ [ \mathit{lo} , \mathit{hi} ) \f$, by increasing keys,
 /// \f$ O( ( L + I ) \log n + k ) \f$ for \f$ k \f$ pairs
 ///
 /// In pseudocode (the scan of the leaves of a B+-tree, Comer, beyond the
 /// book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{forRange}( \mathit{lo} , \mathit{hi} ,
 ///     f ) \cr
 /// 2 & \quad \textbf{foreach}\ ( k , v ) \text{ from }
 ///     \mathrm{lowerBound}( \mathit{lo} ) \text{ on}\ \textbf{do} \cr
 /// 3 & \qquad \textbf{if}\ k \ge \mathit{hi}\ \textbf{then return} \cr
 /// 4 & \qquad f( k , v )
 /// \end{array}}
 /// \f]
 template< class F >
 void forRange( const K & lo , const K & hi , F && f ) const;

 /// exchanges the nodes and the pools with `o`
 void swap( BTreeMap & o ) noexcept;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( key , value ) by increasing keys,
 // bidirectional, a leaf and a place in it

 class const_iterator;

 /// the pairs of constant references ( key , value )
 using value_type = std::pair< const K & , const V & >;
 using size_type = std::size_t;            ///< the type of the sizes
 using difference_type = std::ptrdiff_t;   ///< the type of the distances
 using reference = value_type;             ///< a pair, by value
 using const_reference = value_type;       ///< a pair, by value
 using iterator = const_iterator;          ///< the constant iterator

 /// the iterator on the smallest key
 [[nodiscard]] const_iterator begin( void ) const;

 /// the iterator past the largest key
 [[nodiscard]] const_iterator end( void ) const;

 /// as `begin()`
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }

 /// as `end()`
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 /// the first pair whose key is not smaller than \f$ k \f$, the place of
 /// \f$ k \f$ in the leaf reached by the descent of `lookup()`
 [[nodiscard]] const_iterator lowerBound( const K & k ) const;

 /// the first pair whose key is larger than \f$ k \f$, the place after the
 /// keys not larger than \f$ k \f$ in the leaf reached by the descent of
 /// `lookup()`
 [[nodiscard]] const_iterator upperBound( const K & k ) const;

 /// removes the pair of `it`, and gives the one after it:
 /// `remove( k )` and then `lowerBound( k )`, \f$ k \f$ the key of `it`
 const_iterator erase( const_iterator it );

 /// the pairs whose key is smaller than \f$ k \f$,
 /// \f$ O( ( L + I ) \log n ) \f$ time
 ///
 /// In pseudocode (the rank of the order-statistic trees of Cormen et al.,
 /// on a B-tree, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{rank}( k ) \cr
 /// 2 & \quad r \gets 0;\ u \gets \mathit{root} \cr
 /// 3 & \quad \textbf{while}\ u \text{ is internal}\ \textbf{do} \cr
 /// 4 & \qquad i \gets \mathrm{place}( u , k );\ r \gets r + \sum_{j < i}
 ///     u.\mathit{cnt}[ j ];\ u \gets u.c[ i ] \cr
 /// 5 & \quad \textbf{return}\ r + \mathrm{place}( u , k )
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::size_t rank( const K & k ) const requires R;

 /// the pair of rank \f$ i \f$ (from 0), `end()` if \f$ i \ge n \f$
 ///
 /// In pseudocode (the select of the order-statistic trees of Cormen et
 /// al., on a B-tree, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{select}( i ) \cr
 /// 2 & \quad \textbf{if}\ i \ge n\ \textbf{then return}\ \mathrm{end}()
 ///     \cr
 /// 3 & \quad u \gets \mathit{root} \cr
 /// 4 & \quad \textbf{while}\ u \text{ is internal}\ \textbf{do} \cr
 /// 5 & \qquad j \gets 0 \cr
 /// 6 & \qquad \textbf{while}\ i \ge u.\mathit{cnt}[ j ]\ \textbf{do}\ i
 ///     \gets i - u.\mathit{cnt}[ j ];\ j \gets j + 1 \cr
 /// 7 & \qquad u \gets u.c[ j ] \cr
 /// 8 & \quad \textbf{return}\ \text{the place } i \text{ of the leaf } u
 /// \end{array}}
 /// \f]
 [[nodiscard]] const_iterator select( std::size_t i ) const requires R;

/*--------------------------------------------------------------------------*/
 private:

 /// the keys are counted on all the places of a node, padded
 static constexpr bool FAST = std::is_arithmetic_v< K > &&
  ( std::is_same_v< Less , std::less< K > > ||
    std::is_same_v< Less , std::less<> > );

 /// the key of the unused places, larger than every key
 [[nodiscard]] static constexpr K pad( void ) {
  if constexpr( std::numeric_limits< K >::has_infinity )
   return( std::numeric_limits< K >::infinity() );
  else
   return( std::numeric_limits< K >::max() );
  }

 /// a leaf: the keys, the values and the links to its neighbors
 struct alignas( 64 ) Leaf {
  K k[ L ];                        ///< the keys
  V v[ L ];                        ///< the values
  Leaf * prev = nullptr;           ///< the leaf before
  Leaf * next = nullptr;           ///< the leaf after
  std::uint32_t n = 0;             ///< the pairs
  };

 /// nothing, in the place of the counts with no `R`
 struct None {};

 /// an internal node: the bounds, the children and the links on its level
 struct alignas( 64 ) Inner {
  K k[ I ];                        ///< the bounds of the children
  void * c[ I + 1 ];               ///< the children
  Inner * prev = nullptr;          ///< the node before, on its level
  Inner * next = nullptr;          ///< the node after, on its level
  std::uint32_t n = 0;             ///< the keys, the children - 1
  /// the pairs below each child, if R
  [[no_unique_address]] std::conditional_t< R ,
                         std::array< std::size_t , I + 1 > , None > cnt;
  };

 /// the pairs below the node \f$ x \f$ of height \f$ h \f$
 [[nodiscard]] static std::size_t total( const void * x , int h );

 /// the counts of the children of \f$ u \f$, of height \f$ h \f$, from
 /// the children
 static void recount( Inner * u , int h );

 /// the place of an internal node on a path, and the child taken
 struct Step {
  Inner * u;         ///< the node
  std::uint32_t i;   ///< the child taken
  };

 static constexpr int DEPTH = 64;  ///< the largest height


 /// a new node of the type `N`, from its pool, its places padded
 template< class N >
 [[nodiscard]] N * make( void );

 /// gives the leaf \f$ x \f$ back to its pool
 void free( Leaf * x ) noexcept;

 /// gives the internal node \f$ x \f$ back to its pool
 void free( Inner * x ) noexcept;

 /// the places \f$ [ \mathit{from} , \mathit{to} ) \f$ of the keys
 /// \f$ a \f$ padded again, if the keys are counted on all the places
 static void clean( K * a , std::size_t from , std::size_t to );

 /// prefetches the first `bytes` of the node at \f$ x \f$
 static void fetch( const void * x , std::size_t bytes );

 /// the number of the keys of \f$ a[ 0 .. n - 1 ] \f$ (of the \f$ C \f$
 /// places, padded) smaller than \f$ x \f$
 template< std::size_t C >
 [[nodiscard]] std::size_t place( const K * a , std::uint32_t n ,
                                  const K & x ) const;

 /// the leaf where \f$ x \f$ is, or would be, prefetching the values of
 /// the leaf if `Values`; the parent of the leaf and its child taken in
 /// `*up` and `*at`, if given
 template< bool Values >
 [[nodiscard]] const Leaf * descend( const K & x ,
                                     const Inner * * up = nullptr ,
                                     std::uint32_t * at = nullptr ) const;

 /// the parent of the leftmost leaf, or of the rightmost one
 [[nodiscard]] const Inner * edge( bool right ) const;

 /// the leaf where \f$ x \f$ is, or would be, the path of the descent in
 /// `path`
 [[nodiscard]] const Leaf * descend( const K & x , Step * path ) const;

 /// whether \f$ a \f$, not smaller than \f$ b \f$, is equal to it
 [[nodiscard]] bool equal( const K & a , const K & b ) const;

 /// \f$ ( k , v ) \f$ at the place \f$ p \f$ of the full leaf \f$ x \f$,
 /// which splits, the parents along `path` from the depth \f$ d \f$ up
 void split( Leaf * x , std::uint32_t p , const K & k , const V & v ,
             Step * path , int d );

 /// the bound \f$ s \f$ and the new node \f$ r \f$ into the parent at the
 /// depth \f$ d \f$ of `path`, splitting it if full, on the right edge
 /// of the tree if `edge` is positive and on the left one if negative
 void climb( K s , void * r , Step * path , int d , int edge );

 /// the leaf \f$ x \f$, less than half full, merged with a sibling or
 /// balanced with it
 void shrink( Leaf * x , Step * path , int d );

 /// the internal node \f$ x \f$, less than half full, merged with a
 /// sibling or balanced with it
 void shrink( Inner * x , Step * path , int d );

 /// frees the nodes of the subtree of \f$ x \f$, of height \f$ h \f$
 void destroy( void * x , int h ) noexcept;

 /// the invariant of the tree, \f$ O( n ) \f$
 [[nodiscard]] bool valid( void ) const;

 /// the invariant of the subtree of \f$ x \f$, of height \f$ h \f$, its
 /// keys in \f$ ( \mathit{lo} , \mathit{hi} ] \f$
 [[nodiscard]] bool valid( const void * x , int h , const K * lo ,
                           const K * hi , const Leaf * & last ,
                           std::vector< const Inner * > & left ,
                           std::size_t & count ) const;

 void * f_root = nullptr;          ///< the root, a leaf if f_height is 1
 Leaf * f_first = nullptr;         ///< the leftmost leaf
 Leaf * f_last = nullptr;          ///< the rightmost leaf
 std::size_t f_size = 0;           ///< the pairs
 int f_height = 0;                 ///< the levels
 [[no_unique_address]] Less f_less;   ///< the order of the keys
 ObjectPool< Leaf > f_leaves;      ///< the slots of the leaves
 ObjectPool< Inner > f_inners;     ///< the slots of the internal nodes

/*--------------------------------------------------------------------------*/
 public:

 /// the iterator on the pairs by increasing keys: a leaf, a place in it
 /// and the parent of the leaf
 class const_iterator {
  public:
  /// a pair of constant references, the key and its value
  using value_type = std::pair< const K & , const V & >;
  using reference = value_type;             ///< a pair, by value
  using difference_type = std::ptrdiff_t;   ///< the type of the distances
  /// the category of the iterator for the concepts of C++20, bidirectional
  using iterator_concept = std::bidirectional_iterator_tag;
  /// input for the older algorithms, since `*it` is a value
  using iterator_category = std::input_iterator_tag;

  /// an iterator on no tree
  const_iterator( void ) = default;

  /// the pair of the place
  reference operator*( void ) const {
   return( reference( f_l->k[ f_i ] , f_l->v[ f_i ] ) );
   }

  /// to the next pair, to the next leaf through the parent at the end of
  /// a leaf
  ///
  /// In pseudocode (beyond the book), \f$ ( l , i ) \f$ the leaf and the
  /// place, \f$ ( p , c ) \f$ the parent and the child that is \f$ l \f$:
  /// \f[
  /// \class{pseudocode}{\begin{array}{r@{\quad}l}
  /// 1 & \textbf{procedure}\ \mathrm{next}() \cr
  /// 2 & \quad i \gets i + 1;\ \textbf{if}\ i < l.n\ \textbf{then return}
  ///     \cr
  /// 3 & \quad i \gets 0 \cr
  /// 4 & \quad \textbf{if}\ p = \textbf{nil}\ \textbf{then}\ l \gets
  ///     l.\mathit{next};\ \textbf{return} \cr
  /// 5 & \quad \textbf{if}\ c = p.n\ \textbf{then}\ p \gets
  ///     p.\mathit{next};\ c \gets 0\ \textbf{else}\ c \gets c + 1 \cr
  /// 6 & \quad \textbf{if}\ p = \textbf{nil}\ \textbf{then}\ l \gets
  ///     \textbf{nil}\ \textbf{else}\ l \gets p.c[ c ];\ \text{prefetch }
  ///     \text{the leaf 4 children ahead}
  /// \end{array}}
  /// \f]
  const_iterator & operator++( void ) {
   if( ++f_i == f_n ) [[unlikely]]
    step();
   return( *this );
   }

  /// to the previous pair, from the end to the last pair, through the
  /// parent at the start of a leaf
  const_iterator & operator--( void ) {
   if( ! f_l ) {                            // from the end
    f_l = f_t->f_last;
    f_p = f_t->edge( true );
    f_c = f_p ? f_p->n : 0;
    f_i = f_n = f_l->n;
    }
   else if( f_i == 0 ) {
    if( f_p ) {
     if( f_c == 0 ) {
      f_p = f_p->prev;
      f_c = f_p->n;
      }
     else
      --f_c;
     f_l = static_cast< const Leaf * >( f_p->c[ f_c ] );
     }
    else
     f_l = f_l->prev;
    f_i = f_n = f_l->n;
    }
   --f_i;
   return( *this );
   }

  /// to the next pair, returning the iterator before the step
  const_iterator operator++( int ) { auto t = *this; ++*this; return( t ); }

  /// to the previous pair, returning the iterator before the step
  const_iterator operator--( int ) { auto t = *this; --*this; return( t ); }

  /// equal on the same place of the same leaf
  bool operator==( const const_iterator & o ) const {
   return( ( f_l == o.f_l ) && ( f_i == o.f_i ) );
   }

  private:
  friend class BTreeMap;

  /// the leaves this many places ahead are prefetched
  static constexpr std::uint32_t AHEAD = 4;

  /// on the place \f$ i \f$ of the leaf \f$ l \f$ of the tree \f$ t \f$,
  /// \f$ l \f$ the child \f$ c \f$ of \f$ p \f$; past a leaf, the next one
  const_iterator( const BTreeMap * t , const Leaf * l , std::uint32_t i ,
                  const Inner * p , std::uint32_t c )
   : f_t( t ) , f_l( l ) , f_p( p ) , f_i( i ) , f_n( l ? l->n : 0 ) ,
     f_c( c ) {
   if( f_l && ( f_i == f_n ) )              // past a leaf: the next one
    step();
   }

  /// to the first pair of the next leaf, through the parents, whose
  /// children give the leaves ahead to prefetch with no chase of the
  /// links between the leaves
  void step( void ) {
   f_i = 0;
   if( f_p ) {
    if( f_c == f_p->n ) {
     f_p = f_p->next;
     f_c = 0;
     if( f_p && f_p->next )
      fetch( f_p->next , sizeof( Inner ) );
     }
    else
     ++f_c;
    if( f_p ) {
     f_l = static_cast< const Leaf * >( f_p->c[ f_c ] );
     const Inner * q = f_p;
     std::uint32_t a = f_c + AHEAD;
     if( a > q->n ) {
      a -= q->n + 1;
      q = q->next;
      }
     if( q && ( a <= q->n ) )
      fetch( q->c[ a ] , sizeof( Leaf ) );
     }
    else
     f_l = nullptr;
    }
   else
    f_l = f_l->next;
   f_n = f_l ? f_l->n : 0;
   }

  const BTreeMap * f_t = nullptr;   ///< the tree
  const Leaf * f_l = nullptr;       ///< the leaf, nullptr at the end
  const Inner * f_p = nullptr;      ///< its parent, nullptr if the root
  std::uint32_t f_i = 0;            ///< the place in the leaf
  std::uint32_t f_n = 0;            ///< the pairs of the leaf
  std::uint32_t f_c = 0;            ///< the child of the parent

 };  // end( class const_iterator )

 };  // end( class BTreeMap )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/BTreeMap.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* BTreeMap.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File BTreeMap.h ----------------------------*/
/*--------------------------------------------------------------------------*/
