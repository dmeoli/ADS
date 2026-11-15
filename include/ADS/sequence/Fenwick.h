/*--------------------------------------------------------------------------*/
/*----------------------------- File Fenwick.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The prefix aggregates of a sequence by the tree of Fenwick, beyond the
 * book, over any algebra of `Algebra.h`, and the observer that keeps them
 * for an `ObservableArray` [see `ObservableArray.h`].
 *
 * `Fenwick< G >` holds the values \f$ a_0 , \ldots , a_{n-1} \f$ of the
 * monoid `G` and, for \f$ j = 1 , \ldots , n \f$, the node
 * \f[
 *   t_j = a_{j - \lambda( j )} \oplus \cdots \oplus a_{j - 1} , \qquad
 *   \lambda( j ) = j \mathbin{\&} ( - j ) ,
 * \f]
 * the combination of the \f$ \lambda( j ) \f$ values that end at the
 * index \f$ j - 1 \f$, \f$ \lambda( j ) \f$ the lowest bit of \f$ j \f$.
 * The prefix \f$ a_0 \oplus \cdots \oplus a_{r-1} \f$ is the combination
 * of the nodes of the binary digits of \f$ r \f$, at most \f$ \lceil \log_2
 * ( r + 1 ) \rceil \f$ of them, for any monoid; a node is the combination
 * of the nodes \f$ j - 2^k \f$, \f$ 2^k < \lambda( j ) \f$, and of
 * \f$ a_{j-1} \f$, so that the nodes are built in \f$ O( n ) \f$ and one
 * more is appended in \f$ O( \log n ) \f$. The update of a value depends
 * on the algebra: an abelian group changes the at most \f$ \lceil \log_2
 * ( n + 1 ) \rceil \f$ nodes that contain it by one combination with the
 * difference \f$ a_i^{-1} \oplus v \f$, \f$ O( \log n ) \f$; a commutative
 * monoid does the same for `add()`, the value combined with \f$ d \f$; any
 * other monoid rebuilds those nodes from their parts, \f$ O( \log^2 n ) \f$.
 * The range \f$ a_l \oplus \cdots \oplus a_{r-1} \f$ is
 * \f$ \pi_l^{-1} \oplus \pi_r \f$ in a group, \f$ \pi_r \f$ the prefix of
 * length \f$ r \f$. A group of floating point values (not `exact_v`)
 * accumulates the rounding errors of the differences, and the tree is
 * rebuilt from the values after every `period()` updates by difference,
 * \f$ O( 1 ) \f$ amortized time per update when the period is at least
 * \f$ n \f$ (see \ref theory_sequence_fenwick).
 *
 * `FenwickObserver< G >` is an observer of an array of values of `G`
 * that tells its changes, as `ObservableArray< G::Value >`: it applies
 * `Assigned` by `set()`, `Inserted` by `push_back()`, `Erased` by
 * `pop_back()` and `Cleared` by `clear()`, and on
 * `Rebuilt` it forgets its tree and builds it again from the array at its
 * next query (the events after `Rebuilt` are then ignored, since the
 * array it reads has them); `audit( a )` compares it with the array.
 *
 * The contracts [see `Contract.h`]: the indices are in \f$ [ 0 , n ) \f$,
 * the lengths of the prefixes in \f$ [ 0 , n ] \f$, `range()` needs a group
 * and \f$ l \le r \f$, `add()` a commutative monoid, `pop_back()` a tree
 * that is not empty; an observer is built from its array, or given it by
 * `rebuild()`, before the array changes. The operations are defined in
 * `Fenwick.tpp`.
 *
 * \see P. M. Fenwick, A new data structure for cumulative frequency
 *      tables, Software: Practice and Experience 24(3), 327-336, 1994,
 *      doi:10.1002/spe.4380240306
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

#ifndef __ADS_Fenwick
 #define __ADS_Fenwick
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Algebra.h"
#include "ADS/Contract.h"
#include "ADS/Modification.h"

#include <concepts>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*------------------------------ CLASS Fenwick -----------------------------*/
/*--------------------------------------------------------------------------*/

/// the prefix aggregates of \f$ n \f$ values of the monoid `G`
template< Monoid G >
class Fenwick
{

/*--------------------------------------------------------------------------*/
 public:
 using Value = typename G::Value;  ///< the values
 using Algebra = G;                ///< their algebra

 /// \f$ n \f$ values, all the identity
 explicit Fenwick( std::size_t n = 0 )
     : f_a( n , G::identity() ) , f_t( n , G::identity() ) { }

 /// the values of \f$ [ \mathit{first} , \mathit{last} ) \f$, in
 /// \f$ O( n ) \f$ time
 template< std::input_iterator I >
 Fenwick( I first , I last ) {
  assign( first , last );
  }

 /// the number \f$ n \f$ of values
 [[nodiscard]] std::size_t size( void ) const { return( f_a.size() ); }

 /// whether there is no value
 [[nodiscard]] bool empty( void ) const { return( f_a.empty() ); }

 /// the value \f$ a_i \f$
 /// \pre \f$ i < n \f$
 [[nodiscard]] const Value & get( std::size_t i ) const {
  ADS_PRE( i < f_a.size() );
  return( f_a[ i ] );
  }

 /// the prefix \f$ \pi_r = a_0 \oplus \cdots \oplus a_{r-1} \f$, the
 /// identity if \f$ r = 0 \f$, in \f$ O( \log n ) \f$ time
 ///
 /// In pseudocode (Fenwick, beyond the book), \f$ \lambda( j ) \f$ the
 /// lowest bit of \f$ j \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{prefix}( r ) \cr
 /// 2 & \quad x \gets e;\ j \gets r \cr
 /// 3 & \quad \textbf{while}\ j > 0\ \textbf{do}\ x \gets t_j \oplus x;
 ///     \ j \gets j - \lambda( j ) \cr
 /// 4 & \quad \textbf{return}\ x
 /// \end{array}}
 /// \f]
 /// \pre \f$ r \le n \f$
 [[nodiscard]] Value prefix( std::size_t r ) const;

 /// the range \f$ a_l \oplus \cdots \oplus a_{r-1} \f$, i.e.,
 /// \f$ \pi_l^{-1} \oplus \pi_r \f$, in \f$ O( \log n ) \f$ time
 /// \pre \f$ l \le r \le n \f$
 [[nodiscard]] Value range( std::size_t l , std::size_t r ) const
  requires Group< G >;

 /// \f$ a_i \gets v \f$: by the difference in \f$ O( \log n ) \f$ time in
 /// an abelian group, by the parts of the nodes in \f$ O( \log^2 n ) \f$
 /// time otherwise
 ///
 /// In pseudocode (beyond the book), \f$ \delta = a_i^{-1} \oplus v \f$
 /// in an abelian group:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{set}( i , v ) \cr
 /// 2 & \quad \delta \gets a_i^{-1} \oplus v;\ a_i \gets v;\ j \gets i +
 ///     1 \cr
 /// 3 & \quad \textbf{while}\ j \le n\ \textbf{do}\ t_j \gets t_j \oplus
 ///     \delta\ \text{(in a group)},\ t_j \gets \mathrm{node}( j )
 ///     \ \text{(otherwise)};\ j \gets j + \lambda( j ) \cr
 /// 4 & \textbf{function}\ \mathrm{node}( j ) \cr
 /// 5 & \quad x \gets e;\ k \gets \lambda( j ) / 2 \cr
 /// 6 & \quad \textbf{while}\ k \ge 1\ \textbf{do}\ x \gets x \oplus
 ///     t_{j - k};\ k \gets k / 2 \cr
 /// 7 & \quad \textbf{return}\ x \oplus a_{j-1}
 /// \end{array}}
 /// \f]
 /// \pre \f$ i < n \f$
 void set( std::size_t i , const Value & v );

 /// \f$ a_i \gets a_i \oplus d \f$, in \f$ O( \log n ) \f$ time
 /// \pre \f$ i < n \f$
 void add( std::size_t i , const Value & d )
  requires CommutativeMonoid< G >;

 /// \f$ v \f$ appended as \f$ a_n \f$, its node built in \f$ O( \log n ) \f$
 /// time
 void push_back( const Value & v );

 /// the last value removed, in \f$ O( 1 ) \f$ time, since no other node
 /// contains it
 /// \pre the tree is not empty
 void pop_back( void ) {
  ADS_PRE( ! f_a.empty() );
  f_a.pop_back();
  f_t.pop_back();
  }

 /// all the values removed
 void clear( void ) noexcept {
  f_a.clear();
  f_t.clear();
  f_since = 0;
  }

 /// the values of \f$ [ \mathit{first} , \mathit{last} ) \f$ in place of
 /// all the others, the nodes built in \f$ O( n ) \f$ time
 template< std::input_iterator I >
 void assign( I first , I last );

 /// the updates by difference after which a group of floating point
 /// values is rebuilt from its values
 [[nodiscard]] std::size_t period( void ) const { return( f_period ); }

 /// sets the period of the rebuilding to \f$ p \f$
 /// \pre \f$ p \ge 1 \f$
 void period( std::size_t p ) {
  ADS_PRE( p >= 1 );
  f_period = p;
  }

/*--------------------------------------------------------------------------*/
 private:
 /// the node \f$ t_j \f$ from its parts
 [[nodiscard]] Value node( std::size_t j ) const;

 /// the nodes built from the values, in \f$ O( n ) \f$ time
 void build( void );

 /// one more update by difference, the nodes rebuilt at the period
 void counted( void );

 /// the lowest bit of \f$ j \f$
 static std::size_t low( std::size_t j ) { return( j & ( ~j + 1 ) ); }

 std::vector< Value > f_a;     ///< the values
 std::vector< Value > f_t;     ///< the nodes, \f$ t_j \f$ at j - 1
 std::size_t f_since = 0;      ///< the updates since the rebuilding
 std::size_t f_period = 4096;  ///< the period of the rebuilding

 };  // end( class Fenwick )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS FenwickObserver -------------------------*/
/*--------------------------------------------------------------------------*/

/// the prefix aggregates of the values of an array of values of `G` that
/// tells its changes [see `ObservableArray`], a `Fenwick< G >` kept by its
/// events
template< Monoid G >
class FenwickObserver
{

 using Value = typename G::Value;

/*--------------------------------------------------------------------------*/
 public:
 /// a value written
 using Assign = mod::Assigned< std::size_t , Value >;
 /// a value appended
 using Insert = mod::Inserted< std::size_t , std::size_t , Value >;
 /// the last value removed
 using Erase = mod::Erased< std::size_t , std::size_t , Value >;

 /// an observer of no array, given one later by `rebuild()`
 FenwickObserver( void ) = default;

 /// the observer of the array \f$ a \f$, its tree built from it
 template< class A >
  requires requires( const A & a ) {
   { a.values() } -> std::same_as< const std::vector< Value > & >;
   }
 explicit FenwickObserver( const A & a ) {
  rebuild( a );
  }

 /// the tree built again from the array \f$ a \f$, whose vector of values
 /// `a.values()` it reads from now on when `Rebuilt` comes
 template< class A >
  requires requires( const A & a ) {
   { a.values() } -> std::same_as< const std::vector< Value > & >;
   }
 void rebuild( const A & a ) {
  f_src = &a.values();
  f_f.assign( f_src->begin() , f_src->end() );
  f_stale = false;
  }

 /// \f$ a_h \gets \mathit{to} \f$
 void on( const Assign & e ) {
  if( ! f_stale )
   f_f.set( e.h , e.to );
  }

 /// the value appended
 void on( const Insert & e ) {
  if( ! f_stale ) {
   ADS_PRE( e.h == f_f.size() );
   f_f.push_back( e.value );
   }
  }

 /// the last value removed
 void on( const Erase & ) noexcept {
  if( ! f_stale && ! f_f.empty() )
   f_f.pop_back();
  }

 /// no value
 void on( const mod::Cleared & ) noexcept {
  f_f.clear();
  f_stale = false;
  }

 /// the tree forgotten, built again at the next query
 void on( const mod::Rebuilt & ) noexcept { f_stale = true; }

 /// the tree built again if `Rebuilt` came
 void flush( void ) const {
  if( f_stale ) {
   ADS_PRE( f_src != nullptr );
   f_f.assign( f_src->begin() , f_src->end() );
   f_stale = false;
   }
  }

 /// the number of values
 [[nodiscard]] std::size_t size( void ) const {
  flush();
  return( f_f.size() );
  }

 /// the prefix \f$ \pi_r \f$ [see `Fenwick::prefix()`]
 /// \pre \f$ r \le n \f$
 [[nodiscard]] Value prefix( std::size_t r ) const {
  flush();
  return( f_f.prefix( r ) );
  }

 /// the range \f$ a_l \oplus \cdots \oplus a_{r-1} \f$
 /// \pre \f$ l \le r \le n \f$
 [[nodiscard]] Value range( std::size_t l , std::size_t r ) const
  requires Group< G >
 {
  flush();
  return( f_f.range( l , r ) );
  }

 /// the tree
 [[nodiscard]] const Fenwick< G > & tree( void ) const {
  flush();
  return( f_f );
  }

 /// whether the tree has the values of the array \f$ a \f$ and, when
 /// they are exact, its prefixes are those of a scan of \f$ a \f$
 template< class A >
 [[nodiscard]] bool audit( const A & a ) const;

/*--------------------------------------------------------------------------*/
 private:
 mutable Fenwick< G > f_f;                      ///< the tree
 const std::vector< Value > * f_src = nullptr;  ///< the values observed
 mutable bool f_stale = false;                  ///< whether `Rebuilt` came

 };  // end( class FenwickObserver )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/impl/Fenwick.tpp"

/*--------------------------------------------------------------------------*/

#endif /* Fenwick.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Fenwick.h ---------------------------*/
/*--------------------------------------------------------------------------*/
