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

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------------- SEARCH -----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the place of \f$ v \f$ counted from 0, or \f$ -( n + 1 ) \f$
///
/// In pseudocode (section 3.2.1, the scan of a sequence):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{search}( s , v ) \cr
/// 2 & \quad i \gets 0;\ p \gets s.\mathrm{head}() \cr
/// 3 & \quad \textbf{while not}\ s.\mathrm{finished}( p )\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ s.\mathrm{read}( p ) = v\ \textbf{then return}\ i
///     \cr
/// 5 & \qquad p \gets s.\mathrm{next}( p );\ i \gets i + 1 \cr
/// 6 & \quad \textbf{return}\ -( i + 1 )
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
/// 2 & \quad c \gets 0;\ p \gets s.\mathrm{head}() \cr
/// 3 & \quad \textbf{while not}\ s.\mathrm{finished}( p )\ \textbf{do} \cr
/// 4 & \qquad c \gets c + 1 \cr
/// 5 & \qquad \textbf{if}\ s.\mathrm{read}( p ) = v\ \textbf{then return}\ c
///     - 1 \cr
/// 6 & \qquad \textbf{if}\ v < s.\mathrm{read}( p )\ \textbf{then}
///     \ \textbf{return}\ -c \cr
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
/// 2 & \quad p \gets s.\mathrm{head}() \cr
/// 3 & \quad \textbf{while not}\ s.\mathrm{finished}( p )\ \textbf{and}
///     \ s.\mathrm{read}( p ) < v\ \textbf{do}\ p \gets s.\mathrm{next}( p )
///     \cr
/// 4 & \quad \textbf{return}\ ( p , \textbf{not}\ s.\mathrm{finished}( p )
///     \ \textbf{and}\ s.\mathrm{read}( p ) = v )
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
/// 3 & \quad \textbf{if}\ s.\mathrm{finished}( p )\ \textbf{then return}\ r
///     \cr
/// 4 & \quad \textbf{loop} \cr
/// 5 & \qquad \text{append } s.\mathrm{read}( p ) \text{ to } r \cr
/// 6 & \qquad \textbf{if}\ p = q\ \textbf{then return}\ r \cr
/// 7 & \qquad p \gets s.\mathrm{next}( p )
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
enum class Order { EMPTY ,           ///< no item
                   CONSTANT ,        ///< all the items equal
                   INCREASING ,      ///< each item larger than the one before
                   DECREASING ,      ///< each item smaller than the one before
                   NONDECREASING ,   ///< increasing, with some equal items
                   NONINCREASING ,   ///< decreasing, with some equal items
                   NONE              ///< neither increasing nor decreasing
                   };

/// the order of \f$ s \f$, the most specific that holds
///
/// In pseudocode (section 3.2.1), one scan of the pairs of neighbors:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{order}( s ) \cr
/// 2 & \quad \textbf{if}\ s.\mathrm{empty}()\ \textbf{then return}
///     \ \text{EMPTY} \cr
/// 3 & \quad \mathit{up} \gets \mathit{down} \gets \mathit{equal} \gets
///     \textbf{false} \cr
/// 4 & \quad \textbf{foreach}\ \text{pair } ( x , y ) \text{ of }
///     \text{neighbors of } s\ \textbf{do} \cr
/// 5 & \qquad \textbf{if}\ x < y\ \textbf{then}\ \mathit{up} \gets
///     \textbf{true}\ \textbf{else if}\ y < x\ \textbf{then}
///     \ \mathit{down} \gets \textbf{true}\ \textbf{else}\ \mathit{equal}
///     \gets \textbf{true} \cr
/// 6 & \quad \textbf{if}\ \mathit{up}\ \textbf{and}\ \mathit{down}
///     \ \textbf{then return}\ \text{NONE} \cr
/// 7 & \quad \textbf{if not}\ \mathit{up}\ \textbf{and not}
///     \ \mathit{down}\ \textbf{then return}\ \text{CONSTANT} \cr
/// 8 & \quad \textbf{if}\ \mathit{up}\ \textbf{then return}\ (
///     \text{NONDECREASING if } \mathit{equal} \text{, else INCREASING} ) \cr
/// 9 & \quad \textbf{return}\ ( \text{NONINCREASING if } \mathit{equal}
///     \text{, else DECREASING} )
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
/// 2 & \quad \textbf{repeat} \cr
/// 3 & \qquad \mathit{swapped} \gets \textbf{false} \cr
/// 4 & \qquad \textbf{foreach}\ \text{pair of positions } ( p , q ) ,\ q =
///     s.\mathrm{next}( p ) ,\ \text{in order}\ \textbf{do} \cr
/// 5 & \qquad\quad \textbf{if}\ s.\mathrm{read}( q ) < s.\mathrm{read}( p )
///     \ \textbf{then} \cr
/// 6 & \qquad\qquad \text{swap the items in } p \text{ and } q;
///     \ \mathit{swapped} \gets \textbf{true} \cr
/// 7 & \quad \textbf{until not}\ \mathit{swapped}
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
/// 2 & \quad c \gets \text{an empty sequence};\ p \gets a.\mathrm{head}();
///     \ q \gets b.\mathrm{head}() \cr
/// 3 & \quad \textbf{while not}\ a.\mathrm{finished}( p )\ \textbf{and not}
///     \ b.\mathrm{finished}( q )\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ b.\mathrm{read}( q ) < a.\mathrm{read}( p )
///     \ \textbf{then}\ \text{append } b.\mathrm{read}( q ) \text{ to } c;\ q
///     \gets b.\mathrm{next}( q ) \cr
/// 5 & \qquad \textbf{else}\ \text{append } a.\mathrm{read}( p ) \text{ to }
///     c;\ p \gets a.\mathrm{next}( p ) \cr
/// 6 & \quad \text{append the items left in } a \text{, then those left in }
///     b \text{, to } c \cr
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
/// 3 & \qquad a \gets b \gets \text{empty sequences};\ r \gets 0 \cr
/// 4 & \qquad \textbf{foreach}\ \text{run of } s \text{, in order}
///     \ \textbf{do} \cr
/// 5 & \qquad\quad \text{append it to } a \text{ if } r \text{ is even, }
///     \text{to } b \text{ otherwise};\ r \gets r + 1 \cr
/// 6 & \qquad \textbf{if}\ r \le 1\ \textbf{then return} \cr
/// 7 & \qquad m \gets \text{an empty sequence} \cr
/// 8 & \qquad \textbf{while}\ a \text{ or } b \text{ has items left}
///     \ \textbf{do} \cr
/// 9 & \qquad\quad \text{append to } m \text{ the merge of the next run }
///     \text{of } a \text{ with the next run of } b \cr
/// 10 & \qquad s \gets m
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
/// 2 & \quad \textbf{foreach}\ \text{position } p \text{ of } s \text{, in }
///     \text{order}\ \textbf{do} \cr
/// 3 & \qquad q \gets s.\mathrm{next}( p ) \cr
/// 4 & \qquad \textbf{while not}\ s.\mathrm{finished}( q )\ \textbf{do} \cr
/// 5 & \qquad\quad \textbf{if}\ s.\mathrm{read}( q ) = s.\mathrm{read}( p
///     )\ \textbf{then}\ q \gets s.\mathrm{remove}( q )\ \textbf{else}\ q
///     \gets s.\mathrm{next}( q )
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

#endif  /* SequenceAlgorithms.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File SequenceAlgorithms.h ----------------------*/
/*--------------------------------------------------------------------------*/
