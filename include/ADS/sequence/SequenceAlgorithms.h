/*--------------------------------------------------------------------------*/
/*----------------------- File SequenceAlgorithms.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Algorithms on a SEQUENCE [see ADT.h], beyond the book, through its
 * positions only, so that they work on a List and on an ArraySequence:
 *
 * - search(), the place of an item counted from 0, or -( n + 1 ) after n
 *   comparisons, and searchOrdered() on a sorted sequence, which stops at
 *   the first larger item and gives -( comparisons ) when it misses;
 * - lowerBound(), the position of the first item not smaller than v in a
 *   sorted sequence, i.e., where v goes to keep it sorted, and whether v
 *   is there, so that a search and an insertion share the same scan;
 * - sublist(), the items from a position to another one, the empty
 *   sequence being a sublist of every sequence;
 * - order(), which tells whether a sequence is empty, constant, strictly
 *   or weakly increasing or decreasing, or none of them;
 * - bubbleSort(), \f$ O( n^2 ) \f$ swaps of adjacent items by read() and
 *   write();
 * - naturalMergeSort(), the natural merge sort: the maximal sorted runs
 *   are distributed alternately to two sequences and merged in pairs back,
 *   until a single run is left, in \f$ O( n \log r ) \f$ time for r runs
 *   and O( n ) on a sorted sequence;
 * - mergeSorted(), the merge of two sorted sequences that keeps the
 *   repeated items, and removeDuplicates(), which keeps the first copy of
 *   each item in \f$ O( n^2 ) \f$.
 *
 * The contracts [see Contract.h]: searchOrdered() and mergeSorted() need
 * sorted sequences, and the sorts, the merge and removeDuplicates() ensure
 * a sorted result, or one without repetitions, of the right length; all
 * of them cost O( n ) or more, and are audits, whose comparisons a
 * comparator that counts its calls can skip [see checkingContract()]. The
 * algorithms are defined in SequenceAlgorithms.tpp.
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

/// the place of v counted from 0, or -( n + 1 )
template< Sequence S >
long search( const S & s , const typename S::Item & v );

/// the place of v in the sorted s, or -( the comparisons made ), -1 on
/// the empty s, so that a miss is always negative
/// \pre s is sorted (an audit)
template< Sequence S >
long searchOrdered( const S & s , const typename S::Item & v );

/// the position of the first item not smaller than v in the sorted s, or
/// \f$ pos_n \f$, and whether it is v: insert() of v there keeps s sorted
/// \pre s is sorted (an audit)
template< Sequence S >
std::pair< typename S::Pos , bool > lowerBound( const S & s ,
                                                const typename S::Item & v );

/// the sequence of the items of s from the position p to the position q,
/// both included, empty if p is \f$ pos_n \f$
/// \pre q is p or follows it, and holds an item unless p is \f$ pos_n \f$
template< Sequence S >
S sublist( const S & s , typename S::Pos p , typename S::Pos q );

/*--------------------------------------------------------------------------*/
/*------------------------------- ORDER ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// the order of a sequence, the most specific that holds
enum class Order { EMPTY , CONSTANT , INCREASING , DECREASING ,
                   NONDECREASING , NONINCREASING , NONE };

template< Sequence S >
Order order( const S & s );

/*--------------------------------------------------------------------------*/
/*------------------------------- SORTING ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// bubble sort: n passes of swaps of adjacent items out of order, stopped
/// at the first pass without swaps
/// \post s is sorted, with the same length (an audit)
template< Sequence S >
void bubbleSort( S & s );

/// the merge of the sorted a and b, the repeated items kept, those of a
/// first among equals
/// \pre a and b are sorted (an audit)
/// \post the result is sorted, of length | a | + | b | (an audit)
template< Sequence S >
S mergeSorted( const S & a , const S & b );

/// the natural merge sort; the number of passes goes to *passes, if given
/// \post s is sorted, with the same length (an audit)
template< Sequence S >
void naturalMergeSort( S & s , long * passes = nullptr );

/// keeps the first copy of each item, in O( n^2 ) comparisons
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
