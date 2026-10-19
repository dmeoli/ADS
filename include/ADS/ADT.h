/*--------------------------------------------------------------------------*/
/*------------------------------- File ADT.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The abstract data types of the book, as C++20 concepts: SEQUENCE, SET and
 * DICTIONARY (chapter 3), STACK and QUEUE (chapter 4).
 *
 * A specification of an abstract data type is a triple ( D , F , A ): D is
 * a set of domains, one of which is the designated domain (the type being
 * defined, e.g., the stacks) and the others are the auxiliary ones (e.g.,
 * the items and the booleans); F is a set of operations on D; A is a set
 * of axioms, i.e., the semantics of F. The pair ( D , F ) is the syntactic
 * specification, which a concept states as the names of the operations and
 * their domains. The semantic one may be given in two ways: (i) algebraic,
 * i.e., by equations among the operations, those that observe or remove
 * defined on those that construct (e.g., top( push( S , v ) ) = v below);
 * (ii) constructive, i.e., by the preconditions and the postconditions of
 * each operation, expressed in terms of a type already known (e.g., the
 * sequences of items). The library follows (ii) in the code, through the
 * ADS_PRE and ADS_POST of Contract.h, and states (i) here; the tests check
 * both on every realization. The operations are of four kinds:
 * constructors, which make a value of the type (create(), insert(),
 * push()); modifiers, which change it; observers, which return a property
 * of it (empty(), top(), read()); iterators, which visit its items (the
 * positions of a SEQUENCE, the forEach() of the sets).
 *
 * A realization meets the requirement of abstraction, i.e., a program that
 * uses the type does not depend on how it is realized, and the one of
 * protection, i.e., only the operations of the type act on it: hence each
 * realization is a class whose data are private, and a realization can
 * replace another one under the same concept.
 *
 * The names are those of the book, so that the pseudocode reads the same in
 * the code: a SEQUENCE has empty(), finished(), head(), tail(), next(),
 * prev(), insert(), remove(), read() and write(), a position of the type
 * Pos. As in the book,
 * - the positions \f$ pos_{-1} \f$ and \f$ pos_n \f$ come before the first
 *   and after the last element of a sequence \f$ s_0 , \ldots , s_{n-1} \f$,
 *   and finished( p ) is true exactly on them;
 * - insert( p , v ) with \f$ p = pos_i \f$ puts v at index i, the old
 *   elements from index i moving one place on, and returns the position of
 *   the new element; remove( p ) with \f$ p = pos_i \f$ removes the element
 *   at index i and returns the position of the new element at index i;
 * - a precondition that costs a scan of the sequence (e.g., that p is a
 *   position of a list, which the book does not check in the realizations
 *   with pointers) is an audit of Contract.h, checked only at its level 3.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 3.2 and
 *      4.3.1, https://algoritmicamente.it/materiali/libro/capitoli/cap03.pdf
 *      and https://algoritmicamente.it/materiali/libro/capitoli/cap04.pdf
 * \see B. Liskov, S. Zilles, Programming with abstract data types, ACM
 *      SIGPLAN Notices 9(4), 50-59, 1974
 * \see D. L. Parnas, On the criteria to be used in decomposing systems into
 *      modules, Communications of the ACM 15(12), 1053-1058, 1972
 * \see J. V. Guttag, J. J. Horning, The algebraic specification of abstract
 *      data types, Acta Informatica 10(1), 27-52, 1978
 * \see M. Cadoli, M. Lenzerini, P. Naggar, A. Schaerf, Fondamenti della
 *      progettazione dei programmi: principi, tecniche e loro applicazioni
 *      in C++, CittaStudi, 1997
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

/// SEQUENCE (section 3.2.1): a dynamic linear sequence of items; as
/// axioms, \f$ empty( s ) = finished( head( s ) ) \f$ (empty() is
/// redundant), \f$ read( insert( p , v ) ) = v \f$,
/// \f$ remove( insert( p , v ) ) = s \f$, and
/// \f$ read( write( p , v ) ) = v \f$ with the other items unchanged
template< class S >
concept Sequence = requires( S s , const S cs , typename S::Pos p ,
                             const typename S::Item & v ) {
 typename S::Item;
 typename S::Pos;
 { cs.empty() } -> std::same_as< bool >;
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
/// intersection() and difference() are functions of two sets. As axioms,
/// \f$ contains( x , create ) = false \f$,
/// \f$ contains( x , insert( y , A ) ) = ( x = y ) \lor contains( x , A ) \f$,
/// \f$ contains( x , remove( y , A ) ) = ( x \neq y ) \land
/// contains( x , A ) \f$; inserting an item already there, and removing
/// one that is not, leave the set as it is (a relaxation of the
/// preconditions \f$ x \notin A \f$ and \f$ x \in A \f$ that some
/// specifications state)
template< class S >
concept Set = requires( S s , const S cs , const typename S::Item & x ) {
 typename S::Item;
 { cs.size() } -> std::convertible_to< std::size_t >;
 { cs.contains( x ) } -> std::same_as< bool >;
 s.insert( x );
 s.remove( x );
 };

/// DICTIONARY (section 3.2.3): a univocal relation from keys to values;
/// lookup() returns the value of a key, or nothing (the nil of the book).
/// As axioms, \f$ lookup( k , create ) = nil \f$,
/// \f$ lookup( k , insert( k' , v , D ) ) \f$ is v if \f$ k = k' \f$ and
/// \f$ lookup( k , D ) \f$ otherwise (inserting a key already there
/// replaces its value), \f$ lookup( k , remove( k' , D ) ) \f$ is nil if
/// \f$ k = k' \f$ and \f$ lookup( k , D ) \f$ otherwise; lookup() and
/// remove() are total, i.e., they do not need the key to be there
template< class D >
concept Dictionary = requires( D d , const D cd , const typename D::Key & k ,
                               const typename D::Value & v ) {
 typename D::Key;
 typename D::Value;
 { cd.lookup( k ) } -> std::same_as< std::optional< typename D::Value > >;
 d.insert( k , v );
 d.remove( k );
 };

/// STACK (section 4.3.1): items are added and removed at the top (LIFO).
/// As axioms, \f$ empty( create ) = true \f$,
/// \f$ empty( push( S , v ) ) = false \f$,
/// \f$ top( push( S , v ) ) = v \f$, \f$ pop( push( S , v ) ) = S \f$,
/// while \f$ top( create ) \f$ and \f$ pop( create ) \f$ are undefined,
/// i.e., top() and pop() are partial, with the precondition that the stack
/// is not empty; pop() also returns the item it removes, and the axiom is
/// about the stack it leaves
template< class S >
concept Stack = requires( S s , const S cs , const typename S::Item & v ) {
 typename S::Item;
 { cs.empty() } -> std::same_as< bool >;
 s.push( v );
 { s.pop() } -> std::convertible_to< typename S::Item >;
 { cs.top() } -> std::convertible_to< typename S::Item >;
 };

/// QUEUE (section 4.3.1): items are added at the back and removed from the
/// front (FIFO); top() reads the front. As axioms,
/// \f$ empty( create ) = true \f$,
/// \f$ top( enqueue( create , v ) ) = v \f$, and for Q not empty
/// \f$ top( enqueue( Q , v ) ) = top( Q ) \f$ and
/// \f$ dequeue( enqueue( Q , v ) ) = enqueue( dequeue( Q ) , v ) \f$, while
/// top() and dequeue() of the empty queue are undefined
template< class Q >
concept Queue = requires( Q q , const Q cq , const typename Q::Item & v ) {
 typename Q::Item;
 { cq.empty() } -> std::same_as< bool >;
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
