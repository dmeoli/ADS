/*--------------------------------------------------------------------------*/
/*------------------------------- File ADT.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The abstract data types of the book, as C++20 concepts: SEQUENCE, SET and
 * DICTIONARY (chapter 3), STACK and QUEUE (chapter 4). A concept states the
 * syntactic specification of a type, i.e., the names of its operations and
 * their domains; the semantic one is in the documentation of each operation
 * and in the realizations, which are checked against the same tests.
 *
 * The names are those of the book, so that the pseudocode reads the same in
 * the code: a SEQUENCE has isEmpty(), finished(), head(), tail(), next(),
 * prev(), insert(), remove(), read() and write(), a position of the type
 * Pos. As in the book,
 * - the positions \f$ pos_{-1} \f$ and \f$ pos_n \f$ come before the first
 *   and after the last element of a sequence \f$ s_0 , \ldots , s_{n-1} \f$,
 *   and finished( p ) is true exactly on them;
 * - insert( p , v ) with \f$ p = pos_i \f$ puts v at index i, the old
 *   elements from index i moving one place on, and returns the position of
 *   the new element; remove( p ) with \f$ p = pos_i \f$ removes the element
 *   at index i and returns the position of the new element at index i;
 * - the preconditions of the operations (e.g., that p is a position of the
 *   sequence) are not checked, as the book explains for the realizations
 *   with pointers: checking them would cost a scan of the sequence.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 3.2 and
 *      4.3.1, https://algoritmicamente.it/materiali/libro/capitoli/cap03.pdf
 *      and https://algoritmicamente.it/materiali/libro/capitoli/cap04.pdf
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

#ifndef __ADS_ADT
 #define __ADS_ADT
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <concepts>
#include <cstddef>
#include <optional>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------------- CONCEPTS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// SEQUENCE (section 3.2.1): a dynamic linear sequence of items
template< class S >
concept Sequence = requires( S s , const S cs , typename S::Pos p ,
                             const typename S::Item & v ) {
 typename S::Item;
 typename S::Pos;
 { cs.isEmpty() } -> std::same_as< bool >;
 { cs.finished( p ) } -> std::same_as< bool >;
 { cs.head() } -> std::same_as< typename S::Pos >;
 { cs.tail() } -> std::same_as< typename S::Pos >;
 { cs.next( p ) } -> std::same_as< typename S::Pos >;
 { cs.prev( p ) } -> std::same_as< typename S::Pos >;
 { s.insert( p , v ) } -> std::same_as< typename S::Pos >;
 { s.remove( p ) } -> std::same_as< typename S::Pos >;
 { cs.read( p ) } -> std::convertible_to< typename S::Item >;
 s.write( p , v );
 };

/// SET (section 3.2.2): a finite collection of distinct items; union(),
/// intersection() and difference() are functions of two sets
template< class S >
concept Set = requires( S s , const S cs , const typename S::Item & x ) {
 typename S::Item;
 { cs.size() } -> std::convertible_to< std::size_t >;
 { cs.contains( x ) } -> std::same_as< bool >;
 s.insert( x );
 s.remove( x );
 };

/// DICTIONARY (section 3.2.3): a univocal relation from keys to values;
/// lookup() returns the value of a key, or nothing (the nil of the book)
template< class D >
concept Dictionary = requires( D d , const D cd , const typename D::Key & k ,
                               const typename D::Value & v ) {
 typename D::Key;
 typename D::Value;
 { cd.lookup( k ) } -> std::same_as< std::optional< typename D::Value > >;
 d.insert( k , v );
 d.remove( k );
 };

/// STACK (section 4.3.1): items are added and removed at the top (LIFO)
template< class S >
concept Stack = requires( S s , const S cs , const typename S::Item & v ) {
 typename S::Item;
 { cs.isEmpty() } -> std::same_as< bool >;
 s.push( v );
 { s.pop() } -> std::convertible_to< typename S::Item >;
 { cs.top() } -> std::convertible_to< typename S::Item >;
 };

/// QUEUE (section 4.3.1): items are added at the back and removed from the
/// front (FIFO); top() reads the front
template< class Q >
concept Queue = requires( Q q , const Q cq , const typename Q::Item & v ) {
 typename Q::Item;
 { cq.isEmpty() } -> std::same_as< bool >;
 q.enqueue( v );
 { q.dequeue() } -> std::convertible_to< typename Q::Item >;
 { cq.top() } -> std::convertible_to< typename Q::Item >;
 };

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* ADT.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File ADT.h ------------------------------*/
/*--------------------------------------------------------------------------*/
