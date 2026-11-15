/*--------------------------------------------------------------------------*/
/*------------------------- File ObservableArray.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * An array that tells its changes, beyond the book: a vector of values of
 * `T` whose writes go through the subject policy `Policy` [see
 * `Modification.h`], `NoObservers` by default, with which every operation
 * compiles to the code of the `std::vector` it holds. The handle of a value
 * is its index, which is also the key of its insertion and erasure: `set(
 * i , v )` and `add( i , d )` send `Assigned` with the value before and the
 * value after (after the write), `push_back()` sends `Inserted` (after it),
 * `pop_back()` sends `Erased` with the last value (before it), `clear()`
 * sends `Cleared` and `assign()` sends `Rebuilt` (after them). The reads,
 * `operator[]`, `data()` and the iterators, are those of the vector and
 * never change it; a change of a value may only go through the array.
 *
 * Since the events carry the old and the new values, an observer keeps a
 * view of the array (its prefix sums [see `Fenwick.h`], a segment tree, an
 * index) without reading it, and a batch [see `batch()`] sends the net
 * effect of its writes per index, the changes of one index composed into
 * one by (1.15) of \ref theory_ADS_modifications. `apply( e )` makes the
 * change of an event, so that an `UndoLog` of the array brings it back to a
 * checkpoint.
 *
 * The contracts [see `Contract.h`]: the indices of `set()`, `add()` and
 * `operator[]` are in \f$ [ 0 , n ) \f$, `pop_back()` needs an array that
 * is not empty, `apply()` an event that starts from the present state. The
 * operations are defined in `ObservableArray.tpp`.
 *
 * \see E. Gamma, R. Helm, R. Johnson, J. Vlissides, Design Patterns:
 *      Elements of Reusable Object-Oriented Software, Addison-Wesley,
 *      1994, ISBN 0-201-63361-2, the pattern Observer
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

#ifndef __ADS_ObservableArray
 #define __ADS_ObservableArray
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Modification.h"
#include "ADS/Portable.h"

#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS ObservableArray --------------------------*/
/*--------------------------------------------------------------------------*/

/// an array of values of `T` whose changes are told through `Policy`
template< class T , class Policy = NoObservers >
class ObservableArray {

/*--------------------------------------------------------------------------*/
 public:

 using value_type = T;                          ///< the values
 using size_type = std::size_t;                 ///< the indices
 /// the reads, which never change the array
 using const_iterator = typename std::vector< T >::const_iterator;

 /// a value written: the index, the value before and after
 using Assign = mod::Assigned< std::size_t , T >;
 /// a value appended: its index (the handle and the key) and the value
 using Insert = mod::Inserted< std::size_t , std::size_t , T >;
 /// the last value removed: its index and the value
 using Erase = mod::Erased< std::size_t , std::size_t , T >;
 /// the events
 using modifications = mod::list< Assign , Insert , Erase , mod::Cleared ,
                                  mod::Rebuilt >;
 /// the subject
 using Subject = typename Policy::template bind< modifications ,
                                                 ObservableArray >;

 /// the array of \f$ n \f$ values `v`, the static observers `o...`; no
 /// event
 template< class ... O >
 explicit ObservableArray( std::size_t n = 0 , const T & v = T() ,
                           O & ... o )
  : f_a( n , v ) , f_sub( o... ) {}

 /// the array of the values of `il`, the static observers `o...`; no
 /// event
 template< class ... O >
 ObservableArray( std::initializer_list< T > il , O & ... o )
  : f_a( il ) , f_sub( o... ) {}

/*-------------------------------- THE READS -------------------------------*/

 /// the number \f$ n \f$ of values
 [[nodiscard]] std::size_t size( void ) const { return( f_a.size() ); }

 /// whether there is no value
 [[nodiscard]] bool empty( void ) const { return( f_a.empty() ); }

 /// the value of the index \f$ i \f$
 /// \pre \f$ i < n \f$
 [[nodiscard]] const T & operator[]( std::size_t i ) const {
  ADS_PRE( i < f_a.size() );
  return( f_a[ i ] );
  }

 /// the values, in a contiguous array
 [[nodiscard]] const T * data( void ) const { return( f_a.data() ); }

 /// the first value
 [[nodiscard]] const_iterator begin( void ) const { return( f_a.begin() ); }

 /// after the last value
 [[nodiscard]] const_iterator end( void ) const { return( f_a.end() ); }

 /// the vector of the values
 [[nodiscard]] const std::vector< T > & values( void ) const {
  return( f_a );
  }

/*------------------------------- THE CHANGES ------------------------------*/

 /// the value of the index \f$ i \f$ set to \f$ v \f$, and `Assigned`
 ///
 /// In pseudocode (beyond the book), the pattern of every change, with
 /// \f$ \ell \f$ whether someone may want `Assigned` (a constant of the
 /// type) and \f$ \lambda \f$ whether someone wants it now:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{set}( i , v ) \cr
 /// 2 & \quad \textbf{if}\ \ell\ \textbf{and}\ \lambda\ \textbf{then}
 ///     \ u \gets a[ i ];\ a[ i ] \gets v;
 ///     \ \mathrm{emit}( \mathrm{Assigned}( i , u , v ) ) \cr
 /// 3 & \quad \textbf{else}\ a[ i ] \gets v
 /// \end{array}}
 /// \f]
 /// \pre \f$ i < n \f$
 void set( std::size_t i , const T & v ) {
  ADS_PRE( i < f_a.size() );
  if constexpr( Subject::template listened< Assign > )
   if( f_sub.template live< Assign >() ) [[unlikely]] {
    if constexpr( Subject::dynamic )
     setCold( i , v );
    else
     setTold( i , v );
    return;
    }
  f_a[ i ] = v;
  }

 /// the value of the index \f$ i \f$ raised by \f$ d \f$ (`+=`), and
 /// `Assigned`
 /// \pre \f$ i < n \f$
 void add( std::size_t i , const T & d ) {
  ADS_PRE( i < f_a.size() );
  if constexpr( Subject::template listened< Assign > )
   if( f_sub.template live< Assign >() ) [[unlikely]] {
    if constexpr( Subject::dynamic )
     setCold( i , f_a[ i ] + d );
    else
     setTold( i , f_a[ i ] + d );
    return;
    }
  f_a[ i ] += d;
  }

 /// \f$ v \f$ appended, and `Inserted`
 void push_back( const T & v ) {
  if constexpr( Subject::template listened< Insert > )
   if( f_sub.template live< Insert >() ) [[unlikely]] {
    if constexpr( Subject::dynamic )
     pushCold( v );
    else
     pushTold( v );
    return;
    }
  f_a.push_back( v );
  }

 /// the last value removed, and `Erased` before
 /// \pre the array is not empty
 void pop_back( void ) {
  ADS_PRE( ! f_a.empty() );
  if constexpr( Subject::template listened< Erase > )
   if( f_sub.template live< Erase >() ) [[unlikely]] {
    if constexpr( Subject::dynamic )
     popCold();
    else
     popTold();
    return;
    }
  f_a.pop_back();
  }

 /// all the values removed, and `Cleared`
 void clear( void );

 /// the values of \f$ [ \mathit{first} , \mathit{last} ) \f$ in place of
 /// all those of the array, and `Rebuilt`
 template< std::input_iterator I >
 void assign( I first , I last );

 /// the change of the event \f$ e \f$, told as such (for `UndoLog`)
 /// \pre \f$ e \f$ starts from the present state
 void apply( const Assign & e ) { set( e.h , e.to ); }
 /// the insertion of \f$ e \f$
 /// \pre \f$ e \f$ is at the index \f$ n \f$
 void apply( const Insert & e ) {
  ADS_PRE( e.h == f_a.size() );
  push_back( e.value );
  }
 /// the erasure of \f$ e \f$
 /// \pre \f$ e \f$ is at the index \f$ n - 1 \f$
 void apply( const Erase & e ) {
  ADS_PRE( e.h + 1 == f_a.size() );
  pop_back();
  }

/*----------------------------- THE OBSERVERS ------------------------------*/

 /// the subject, for `ADS::batch()` and `ADS::quiet()`
 [[nodiscard]] Subject & subject( void ) { return( f_sub ); }
 /// the subject
 [[nodiscard]] const Subject & subject( void ) const { return( f_sub ); }

 /// attaches the observer \f$ o \f$ and gives its number
 /// \pre no batch is open
 template< class O >
 int attach( O & o ) requires( Subject::dynamic ) {
  return( f_sub.attach( o ) );
  }

 /// detaches the observer number \f$ \mathit{id} \f$
 /// \pre it is attached, no batch is open
 void detach( int id ) requires( Subject::dynamic ) { f_sub.detach( id ); }

/*--------------------------------------------------------------------------*/
 private:

 /// set() when someone listens
 void setTold( std::size_t i , const T & v );
 /// set() when someone listens, out of the hot code
 ADS_COLD_PATH void setCold( std::size_t i , const T & v ) {
  setTold( i , v );
  }

 /// push_back() when someone listens
 void pushTold( const T & v );
 /// push_back() when someone listens, out of the hot code
 ADS_COLD_PATH void pushCold( const T & v ) { pushTold( v ); }

 /// pop_back() when someone listens
 void popTold( void );
 /// pop_back() when someone listens, out of the hot code
 ADS_COLD_PATH void popCold( void ) { popTold(); }

 std::vector< T > f_a;                     ///< the values
 ADS_NO_UNIQUE_ADDRESS Subject f_sub;      ///< the observers

 };  // end( class ObservableArray )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/impl/ObservableArray.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ObservableArray.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File ObservableArray.h -----------------------*/
/*--------------------------------------------------------------------------*/
