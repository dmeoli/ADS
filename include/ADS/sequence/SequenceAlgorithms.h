/*--------------------------------------------------------------------------*/
/*----------------------- File SequenceAlgorithms.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Algorithms on a SEQUENCE [see `ADT.h`], beyond the book, through its
 * positions only, so that they work on a `List` and on an `ArraySequence`:
 *
 * - `search()`, the place of an item counted from 0, or \f$ -( n + 1 ) \f$
 *   after \f$ n \f$ comparisons, and `searchOrdered()` on a sorted
 *   sequence, which stops at the first larger item and gives
 *   \f$ -c \f$, \f$ c \f$ the comparisons made, when it misses;
 * - `lowerBound()`, the position of the first item not smaller than
 *   \f$ v \f$ in a sorted sequence, i.e., where \f$ v \f$ goes to keep it
 *   sorted, and whether \f$ v \f$ is there, so that a search and an
 *   insertion share the same scan;
 * - `sublist()`, the items from a position to another one, the empty
 *   sequence being a sublist of every sequence;
 * - `order()`, which tells whether a sequence is empty, constant, strictly
 *   or weakly increasing or decreasing, or none of them;
 * - `bubbleSort()`, \f$ O( n^2 ) \f$ swaps of adjacent items by `read()`
 *   and `write()`;
 * - `naturalMergeSort()`, the natural merge sort: the maximal sorted runs
 *   are distributed alternately to two sequences and merged in pairs back,
 *   until a single run is left, in \f$ O( n \log r ) \f$ time for \f$ r \f$
 *   runs and \f$ O( n ) \f$ on a sorted sequence;
 * - `mergeSorted()`, the merge of two sorted sequences that keeps the
 *   repeated items, and `removeDuplicates()`, which keeps the first copy of
 *   each item in \f$ O( n^2 ) \f$.
 *
 * The contracts [see `Contract.h`]: `searchOrdered()` and `mergeSorted()`
 * need sorted sequences, and the sorts, the merge and `removeDuplicates()`
 * ensure a sorted result, or one without repetitions, of the right length;
 * all of them cost \f$ O( n ) \f$ or more, and are audits, whose comparisons
 * a comparator that counts its calls can skip [see `checkingContract()`].
 * The algorithms are defined in `SequenceAlgorithms.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 3.2.1 and
 *      12.2,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap12.pdf
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

#ifndef __ADS_SequenceAlgorithms
 #define __ADS_SequenceAlgorithms
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/ADT.h"
#include "ADS/Contract.h"
#include "ADS/sort/Sorted.h"

#include <cstddef>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*------------------------------- SEARCH -----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the place of \f$ v \f$ counted from 0, or \f$ -( n + 1 ) \f$
///
/// In pseudocode (section 3.2.1, the scan of a sequence):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{search}( s , v ) \cr
/// 2 & \quad p \gets s.\mathrm{head}();\ i \gets 0\quad \triangleright\ i
///     \text{ the index of } p \cr
/// 3 & \quad \textbf{while not}\ s.\mathrm{finished}( p )\ \textbf{and}
///     \ s.\mathrm{read}( p ) \ne v\ \textbf{do}\ p \gets s.\mathrm{next}( p
///     );\ i \gets i + 1 \cr
/// 4 & \quad \textbf{if}\ s.\mathrm{finished}( p )\ \textbf{then return}\ -(
///     i + 1 )\quad \triangleright\ i = n \cr
/// 5 & \quad \textbf{return}\ i
/// \end{array}}
/// \f]
template< Sequence S >
long search( const S & s , const typename S::Item & v );

/// the place of \f$ v \f$ in the sorted \f$ s \f$, or \f$ -c \f$, \f$ c \f$
/// the comparisons made, -1 on the empty \f$ s \f$, so that a miss is
/// always negative
///
/// In pseudocode (section 3.2.1), the scan stopped at the first larger
/// item:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{searchOrdered}( s , v ) \cr
/// 2 & \quad p \gets s.\mathrm{head}();\ c \gets 0\quad \triangleright\ c
///     \text{ the items read} \cr
/// 3 & \quad \textbf{while not}\ s.\mathrm{finished}( p )\ \textbf{do} \cr
/// 4 & \qquad x \gets s.\mathrm{read}( p );\ c \gets c + 1 \cr
/// 5 & \qquad \textbf{if}\ v < x\ \textbf{then return}\ -c\quad
///     \triangleright\ v \text{ would go before } x \cr
/// 6 & \qquad \textbf{if}\ v = x\ \textbf{then return}\ c - 1 \cr
/// 7 & \qquad p \gets s.\mathrm{next}( p ) \cr
/// 8 & \quad \textbf{return}\ -\max( c , 1 )
/// \end{array}}
/// \f]
/// \pre \f$ s \f$ is sorted (an audit)
template< Sequence S >
long searchOrdered( const S & s , const typename S::Item & v );

/// the position of the first item not smaller than \f$ v \f$ in the sorted
/// \f$ s \f$, or \f$ \mathit{pos}_n \f$, and whether it is \f$ v \f$:
/// `insert()` of \f$ v \f$ there keeps \f$ s \f$ sorted
///
/// In pseudocode (section 3.2.1):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{lowerBound}( s , v ) \cr
/// 2 & \quad p \gets \text{the first position of } s
///     \text{ that is finished or holds an item } \ge v \cr
/// 3 & \quad \mathit{found} \gets \textbf{not}\ s.\mathrm{finished}( p )
///     \ \textbf{and}\ s.\mathrm{read}( p ) = v \cr
/// 4 & \quad \textbf{return}\ ( p , \mathit{found} )
/// \end{array}}
/// \f]
/// \pre \f$ s \f$ is sorted (an audit)
template< Sequence S >
std::pair< typename S::Pos , bool > lowerBound( const S & s ,
					       const typename S::Item & v );

/// the sequence of the items of \f$ s \f$ from the position \f$ p \f$ to the
/// position \f$ q \f$, both included, empty if \f$ p \f$ is
/// \f$ \mathit{pos}_n \f$
///
/// In pseudocode (section 3.2.1), the items appended to \f$ r \f$ at its
/// end, whose position is kept:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{sublist}( s , p , q ) \cr
/// 2 & \quad r \gets \text{an empty sequence} \cr
/// 3 & \quad \textbf{while not}\ s.\mathrm{finished}( p )\ \textbf{do}\quad
///     \triangleright\ \text{from } p \text{ to } q \text{ included} \cr
/// 4 & \qquad \text{append } s.\mathrm{read}( p ) \text{ to } r;
///     \ \textbf{if}\ p = q\ \textbf{then}\ \textbf{break} \cr
/// 5 & \qquad p \gets s.\mathrm{next}( p ) \cr
/// 6 & \quad \textbf{return}\ r
/// \end{array}}
/// \f]
/// \pre \f$ q \f$ is \f$ p \f$ or follows it, and holds an item unless
///      \f$ p \f$ is \f$ \mathit{pos}_n \f$
template< Sequence S >
S sublist( const S & s , typename S::Pos p , typename S::Pos q );

/*--------------------------------------------------------------------------*/
/*------------------------------- ORDER ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// the order of a sequence, the most specific that holds
enum class Order {
 EMPTY ,          ///< no item
 CONSTANT ,       ///< all the items equal
 INCREASING ,     ///< each item larger than the one before
 DECREASING ,     ///< each item smaller than the one before
 NONDECREASING ,  ///< increasing, with some equal items
 NONINCREASING ,  ///< decreasing, with some equal items
 NONE            ///< neither increasing nor decreasing
 };

/// the order of \f$ s \f$, the most specific that holds
///
/// In pseudocode (section 3.2.1), one scan of the pairs of neighbors:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{order}( s ) \cr
/// 2 & \quad \textbf{if}\ s.\mathrm{empty}()\ \textbf{then return}
///     \ \text{EMPTY} \cr
/// 3 & \quad \textbf{foreach}\ \text{two neighbors } x , y \text{ of } s
///     \ \textbf{do}\ \text{note whether } x < y \text{, } y < x \text{ or }
///     x = y \cr
/// 4 & \quad \mathit{up} , \mathit{down} , \mathit{equal} \gets
///     \text{whether each of the three was seen} \cr
/// 5 & \quad \textbf{if}\ \mathit{up}\ \textbf{and}\ \mathit{down}
///     \ \textbf{then return}\ \text{NONE} \cr
/// 6 & \quad \textbf{if}\ \mathit{up}\ \textbf{then return}\ (
///     \text{NONDECREASING if } \mathit{equal} \text{, else INCREASING} ) \cr
/// 7 & \quad \textbf{if}\ \mathit{down}\ \textbf{then return}\ (
///     \text{NONINCREASING if } \mathit{equal} \text{, else DECREASING} ) \cr
/// 8 & \quad \textbf{return}\ \text{CONSTANT}\quad \triangleright
///     \ \text{one item, or all equal}
/// \end{array}}
/// \f]
template< Sequence S >
Order order( const S & s );

/*--------------------------------------------------------------------------*/
/*------------------------------- SORTING ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the bubble sort: at most \f$ n \f$ passes of swaps of adjacent items
/// out of order, stopped at the first pass without swaps
///
/// In pseudocode (section 12.2), by `read()` and `write()` only:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{bubbleSort}( s ) \cr
/// 2 & \quad \mathit{swapped} \gets \textbf{true} \cr
/// 3 & \quad \textbf{while}\ \mathit{swapped}\ \textbf{do}\quad
///     \triangleright\ \text{a pass over } s \cr
/// 4 & \qquad \mathit{swapped} \gets \textbf{false} \cr
/// 5 & \qquad \textbf{foreach}\ \text{position } p \text{ with a successor }
///     q = s.\mathrm{next}( p ) \text{, in order}\ \textbf{do} \cr
/// 6 & \qquad\quad \textbf{if}\ s.\mathrm{read}( q ) < s.\mathrm{read}( p )
///     \ \textbf{then}\ \text{exchange the two items};\ \mathit{swapped}
///     \gets \textbf{true}
/// \end{array}}
/// \f]
/// \post \f$ s \f$ is sorted, with the same length (an audit)
template< Sequence S >
void bubbleSort( S & s );

/// the merge of the sorted \f$ a \f$ and \f$ b \f$, the repeated items
/// kept, those of \f$ a \f$ first among equals
///
/// In pseudocode (section 12.2), at most \f$ |a| + |b| - 1 \f$
/// comparisons:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{mergeSorted}( a , b ) \cr
/// 2 & \quad c \gets \text{an empty sequence};\ ( p , q ) \gets (
///     a.\mathrm{head}() , b.\mathrm{head}() ) \cr
/// 3 & \quad \textbf{while}\ \text{neither } p \text{ nor } q
///     \text{ is finished}\ \textbf{do}\quad \triangleright
///     \ \text{on ties, } a \text{ first} \cr
/// 4 & \qquad \textbf{if}\ a.\mathrm{read}( p ) \le b.\mathrm{read}( q )
///     \ \textbf{then}\ \text{append } a.\mathrm{read}( p ) \text{ to } c;
///     \ p \gets a.\mathrm{next}( p ) \cr
/// 5 & \qquad \textbf{else}\ \text{append } b.\mathrm{read}( q ) \text{ to }
///     c;\ q \gets b.\mathrm{next}( q ) \cr
/// 6 & \quad \text{append to } c \text{ what is left of } a
///     \text{, then what is left of } b \cr
/// 7 & \quad \textbf{return}\ c
/// \end{array}}
/// \f]
/// \pre \f$ a \f$ and \f$ b \f$ are sorted (an audit)
/// \post the result is sorted, of length \f$ |a| + |b| \f$ (an audit)
template< Sequence S >
S mergeSorted( const S & a , const S & b );

/// the natural merge sort; the number of passes goes to `*passes`, if
/// given
///
/// In pseudocode (section 12.2, by the runs of the sequence), each pass at
/// least halving the runs:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{naturalMergeSort}( s ) \cr
/// 2 & \quad \textbf{loop} \cr
/// 3 & \qquad \text{deal the maximal nondecreasing runs of } s \text{ to } a
///     \text{ and } b \text{ in turn} \cr
/// 4 & \qquad r \gets \text{the number of runs};\ \textbf{if}\ r \le 1
///     \ \textbf{then return} \cr
/// 5 & \qquad m \gets \text{an empty sequence} \cr
/// 6 & \qquad \textbf{while}\ a \text{ or } b \text{ is not exhausted}
///     \ \textbf{do} \cr
/// 7 & \qquad\quad \text{append to } m \text{ the merge of the next run of }
///     a \text{ and the next run of } b \cr
/// 8 & \qquad s \gets m\quad \triangleright
///     \ \text{about half the runs of before}
/// \end{array}}
/// \f]
/// \post \f$ s \f$ is sorted, with the same length (an audit)
template< Sequence S >
void naturalMergeSort( S & s , long * passes = nullptr );

/// keeps the first copy of each item, in \f$ O( n^2 ) \f$ comparisons
///
/// In pseudocode (section 3.2.1), each item compared with those after it:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{removeDuplicates}( s ) \cr
/// 2 & \quad \textbf{foreach}\ \text{position } p \text{ of } s
///     \text{, in order}\ \textbf{do}\quad \triangleright
///     \ \text{the first copy stays} \cr
/// 3 & \qquad \text{remove from } s \text{ every later position } q
///     \text{ with } s.\mathrm{read}( q ) = s.\mathrm{read}( p )
/// \end{array}}
/// \f]
/// \post no item appears twice (an audit)
template< Sequence S >
void removeDuplicates( S & s );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/impl/SequenceAlgorithms.tpp"

/*--------------------------------------------------------------------------*/

#endif /* SequenceAlgorithms.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File SequenceAlgorithms.h ----------------------*/
/*--------------------------------------------------------------------------*/
