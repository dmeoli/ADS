/*--------------------------------------------------------------------------*/
/*------------------------- File SlidingAggregate.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The aggregate of a sliding window, beyond the book: a QUEUE of values of
 * the monoid `G` [see `Algebra.h`] with `aggregate()`, the combination
 * \f$ x_1 \oplus \cdots \oplus x_n \f$ of its values from the front to the
 * back, in \f$ O( 1 ) \f$ time per operation, by the algorithm that the
 * algebra and the policy allow (see \ref theory_queue_sliding):
 *
 * - with `Amortized`, the default, and a group `G` the aggregate is kept
 *   whole, an item entering by \f$ s \gets s \oplus x \f$ and leaving by
 *   \f$ s \gets x^{-1} \oplus s \f$ (subtract-on-evict, \f$ O( 1 ) \f$),
 *   and rebuilt from the items every `period()` removals when the values
 *   are not exact (floating point), \f$ O( 1 ) \f$ amortized;
 * - with `Amortized` and a selective `G` whose values compare by `==`
 *   (the least, the largest), a deque of the candidates keeps those items
 *   that no later item absorbs, so that the front candidate is the
 *   aggregate (the monotone deque, \f$ O( 1 ) \f$ amortized);
 * - with `Amortized` and any other monoid, the items are in one deque whose
 *   first part has the aggregates of its suffixes, the rest one aggregate,
 *   and a removal from an empty first part makes all the items the first
 *   part (Two-Stacks Lite of Tangwongsan, Hirzel and Schneider,
 *   \f$ O( 1 ) \f$ amortized, one combination per item and per move);
 * - with `WorstCase`, for any monoid, the real-time queue of Hood and
 *   Melville [see `RealTimeMinQueue.h`] on stacks whose cells hold the
 *   aggregate of the cells below them, its rotation three moves per
 *   operation, so that every operation costs \f$ O( 1 ) \f$ in the worst
 *   case (the guarantee of DABA Lite by another route).
 *
 * `kind` tells which one was chosen. `MinQueue< T >` and
 * `RealTimeMinQueue< T >` [see `MinQueue.h`, `RealTimeMinQueue.h`] compute
 * what `SlidingAggregate< MinOf< T > >` and `SlidingAggregate< MinOf< T > ,
 * WorstCase >` do, and stay as they are, since they also take an order
 * that is an object rather than a type and give the least item by
 * reference.
 *
 * The contracts [see `Contract.h`]: `top()` and `dequeue()` need a window
 * that is not empty; `enqueue()` ensures one item more, `dequeue()` one
 * less. The operations are defined in `SlidingAggregate.tpp`.
 *
 * \see K. Tangwongsan, M. Hirzel, S. Schneider, In-order sliding-window
 *      aggregation in worst-case constant time, The VLDB Journal 30(6),
 *      933-957, 2021, doi:10.1007/s00778-021-00668-3
 * \see R. Hood, R. Melville, Real-time queue operations in pure LISP,
 *      Information Processing Letters 13(2), 50-54, 1981,
 *      doi:10.1016/0020-0190(81)90030-2
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

#ifndef __ADS_SlidingAggregate
 #define __ADS_SlidingAggregate
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Algebra.h"
#include "ADS/Contract.h"
#include "ADS/Portable.h"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*------------------------------ THE POLICIES ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the policy of a sliding window whose operations cost \f$ O( 1 ) \f$
/// amortized time, by the algorithm that suits the algebra
struct Amortized
{ };

/// the policy of a sliding window whose operations cost \f$ O( 1 ) \f$
/// time in the worst case
struct WorstCase
{ };

/// the algorithm of a sliding window [see `SlidingAggregate`]
enum class window_kind {
 subtract_on_evict ,
 monotone_deque ,
 two_stacks ,
 real_time
 };

/// \cond
namespace detail
{

/// the algorithm for the monoid G and the policy P
template< class G , class P >
constexpr window_kind windowKindOf( void )
{
 if constexpr( std::is_same_v< P , WorstCase > )
  return( window_kind::real_time );
 else if constexpr( Group< G > )
  return( window_kind::subtract_on_evict );
 else if constexpr( Selective< G > &&
		    std::equality_comparable< typename G::Value > )
  return( window_kind::monotone_deque );
 else
  return( window_kind::two_stacks );
 }

/// the window by subtract-on-evict: the items and their aggregate
template< class G >
class SubtractWindow
{
 public:
 using Value = typename G::Value;
 [[nodiscard]] std::size_t size( void ) const { return( f_q.size() ); }
 [[nodiscard]] const Value & front( void ) const { return( f_q.front() ); }
 [[nodiscard]] Value aggregate( void ) const { return( f_s ); }
 void push( const Value & x );
 Value pop( void );
 void clear( void ) {
  f_q.clear();
  f_s = G::identity();
  f_since = 0;
  }
 template< class F >
 void forEach( F && f ) const {
  for( const auto & x : f_q )
   f( x );
  }
 std::size_t f_period = 4096;  ///< the removals between rebuildings
 private:
 std::deque< Value > f_q;    ///< the items
 Value f_s = G::identity();  ///< their aggregate
 std::size_t f_since = 0;    ///< the removals since the rebuilding
 };

/// the window by the monotone deque of the candidates of a selective
/// monoid
template< class G >
class DequeWindow
{
 public:
 using Value = typename G::Value;
 [[nodiscard]] std::size_t size( void ) const { return( f_q.size() ); }
 [[nodiscard]] const Value & front( void ) const { return( f_q.front() ); }
 [[nodiscard]] Value aggregate( void ) const {
  return( f_c.empty() ? G::identity() : f_c.front().second );
  }
 void push( const Value & x );
 Value pop( void );
 void clear( void ) {
  f_q.clear();
  f_c.clear();
  }
 template< class F >
 void forEach( F && f ) const {
  for( const auto & x : f_q )
   f( x );
  }

 private:
 std::deque< Value > f_q;  ///< the items
 /// the candidates, by their number in the order of arrival
 std::deque< std::pair< std::uint64_t , Value > > f_c;
 std::uint64_t f_head = 0;  ///< the number of the front item
 };

/// the window of Two-Stacks Lite: the items, the aggregates of the
/// suffixes of the first part, the aggregate of the rest
template< class G >
class StacksWindow
{
 public:
 using Value = typename G::Value;
 [[nodiscard]] std::size_t size( void ) const { return( f_q.size() ); }
 [[nodiscard]] const Value & front( void ) const { return( f_q.front() ); }
 [[nodiscard]] Value aggregate( void ) const {
  return( f_f.empty() ? f_b : G::combine( f_f.back() , f_b ) );
  }
 void push( const Value & x ) {
  f_q.push_back( x );
  f_b = G::combine( f_b , x );
  }
 Value pop( void );
 void clear( void ) {
  f_q.clear();
  f_f.clear();
  f_b = G::identity();
  }
 template< class F >
 void forEach( F && f ) const {
  for( const auto & x : f_q )
   f( x );
  }

 private:
 std::deque< Value > f_q;  ///< the items
 /// the aggregates of the suffixes of the first part, the whole on top
 std::vector< Value > f_f;
 Value f_b = G::identity();  ///< the aggregate of the rest
 };

/// an array of fixed capacity whose live cells are \f$ [ lo , hi ) \f$,
/// each with an item and a summary of the cells from 0 to it (the place of
/// the least item, or their aggregate): the stacks of the real-time queue
template< class Item , class Summary >
class CellArray
{
 public:
 /// no cell
 CellArray( void ) = default;

 /// room for \f$ c \f$ cells, none built
 explicit CellArray( std::size_t c )
     : f_a( c ? std::allocator< Cell >().allocate( c ) : nullptr ) ,
       f_cap( c ) { }

 /// the cells of `o`, which is left with none
 CellArray( CellArray && o ) noexcept { swap( o ); }

 /// the cells of `o`, its own given back through it
 CellArray & operator=( CellArray && o ) noexcept {
  swap( o );
  return( *this );
  }

 /// the live cells destroyed, the room freed
 ~CellArray() {
  for( std::size_t i = f_lo ; i < f_hi ; ++i )
   std::destroy_at( f_a + i );
  if( f_a )
   std::allocator< Cell >().deallocate( f_a , f_cap );
  }

 /// exchanges the cells with `o`
 void swap( CellArray & o ) noexcept {
  std::swap( f_a , o.f_a );
  std::swap( f_cap , o.f_cap );
  std::swap( f_lo , o.f_lo );
  std::swap( f_hi , o.f_hi );
  }

 /// the live cells
 [[nodiscard]] std::size_t size( void ) const { return( f_hi - f_lo ); }

 /// the room
 [[nodiscard]] std::size_t capacity( void ) const { return( f_cap ); }

 /// the first live place
 [[nodiscard]] std::size_t lo( void ) const { return( f_lo ); }

 /// the place after the last live one
 [[nodiscard]] std::size_t hi( void ) const { return( f_hi ); }

 /// the item of the place \f$ i \f$
 [[nodiscard]] Item & at( std::size_t i ) { return( f_a[ i ].item ); }

 /// the item of the place \f$ i \f$
 [[nodiscard]] const Item & at( std::size_t i ) const {
  return( f_a[ i ].item );
  }

 /// the summary of the cells from 0 to \f$ i \f$
 [[nodiscard]] const Summary & summary( std::size_t i ) const {
  return( f_a[ i ].s );
  }

 /// \f$ x \f$ in a new cell on top, with the summary \f$ s \f$
 template< class U , class S >
 void push( U && x , S && s ) {
  ADS_PRE( f_hi < f_cap );
  std::construct_at( f_a + f_hi , Cell{ Item( std::forward< U >( x ) ) ,
				       Summary( std::forward< S >( s ) ) } );
  ++f_hi;
  }

 /// the top cell destroyed
 void pop( void ) {
  ADS_PRE( f_hi > f_lo );
  std::destroy_at( f_a + --f_hi );
  }

 /// the bottom cell destroyed
 void popBottom( void ) {
  ADS_PRE( f_hi > f_lo );
  std::destroy_at( f_a + f_lo++ );
  }

 private:
 /// a cell, an item and the summary up to it
 struct Cell
 {
  Item item;  ///< the item
  Summary s;  ///< the summary of the cells from 0 to here
  };

 Cell * f_a = nullptr;   ///< the room
 std::size_t f_cap = 0;  ///< its cells
 std::size_t f_lo = 0;   ///< the first live cell
 std::size_t f_hi = 0;   ///< after the last live cell
 };

/// the real-time queue of Hood and Melville on the stacks of cells of
/// `Link`, which gives the type of the items and of the summaries, the
/// pushes on a front (the first item on top) and on a back (the last item
/// on top), and the summary of the whole queue from its parts: one
/// rotation for the aggregates of a window and for the minima of
/// `RealTimeMinQueue`
template< class Link >
class RealTimeRotation
{
 public:
 using Item = typename Link::Item;                         ///< the items
 using Array = CellArray< Item , typename Link::Summary >;  ///< a stack

 /// the empty queue, its cells pushed by `l`
 explicit RealTimeRotation( Link l = Link() )
     : f_R( 1 ) , f_link( std::move( l ) ) { }

 /// a copy, item by item
 RealTimeRotation( const RealTimeRotation & o )
     : f_R( 1 ) , f_link( o.f_link ) {
  o.forEach( [ & ]( const Item & x ) { push( x ); } );
  }

 /// the items of `o`, which is left empty
 RealTimeRotation( RealTimeRotation && o ) noexcept
     : f_R( 1 ) , f_link( o.f_link ) {
  swap( o );
  }

 /// the items of `o`, by copy and swap
 RealTimeRotation & operator=( const RealTimeRotation & o ) {
  RealTimeRotation c( o );
  swap( c );
  return( *this );
  }

 /// the items of `o`, exchanged with these
 RealTimeRotation & operator=( RealTimeRotation && o ) noexcept {
  swap( o );
  return( *this );
  }

 /// exchanges the items with `o`
 void swap( RealTimeRotation & o ) noexcept;

 /// the number of items
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// the first item, the top of the front
 [[nodiscard]] const Item & front( void ) const {
  return( f_F.at( f_F.hi() - 1 ) );
  }

 /// the summary of the items, from those of the parts
 [[nodiscard]] decltype( auto ) aggregate( void ) const {
  return( f_link.aggregate( *this ) );
  }

 /// \f$ x \f$ on the back, and three moves of a rotation
 void push( const Item & x );

 /// the first item removed from the front, and three moves of a rotation
 Item pop( void );

 /// no item
 void clear( void ) {
  RealTimeRotation e( f_link );
  swap( e );
  }

 /// f( x ) for each item x, from the first
 template< class F >
 void forEach( F && f ) const;

 Array f_F;                          ///< the front, the first item on top
 Array f_R;                          ///< the back, the last item on top
 Array f_Rf;                         ///< the frozen back of a rotation
 Array f_N;                          ///< the new front of a rotation
 std::size_t f_r0 = 0;               ///< the items of the frozen back
 std::size_t f_i = 0;                ///< the places of the front moved
 std::size_t f_n = 0;                ///< the items
 bool f_rot = false;                 ///< a rotation is going on
 ADS_NO_UNIQUE_ADDRESS Link f_link;  ///< the pushes and the summary

 private:
 /// a rotation started if the back has one item more than the front
 void startIfDue( void );

 /// \f$ k \f$ moves of the rotation, or its end
 void steps( int k );
 };

/// the cells of a window of the monoid `G` with the aggregate of the cells
/// from 0, in the order of the window
template< class G >
struct AggLink
{
 using Item = typename G::Value;     ///< the items
 using Summary = typename G::Value;  ///< their aggregates

 /// \f$ x \f$ on top of the front \f$ a \f$, the first item on top
 template< class A >
 static void pushFront( A & a , Item x ) {
  Item s = a.size() > 0 ? G::combine( x , a.summary( a.hi() - 1 ) ) : x;
  a.push( std::move( x ) , std::move( s ) );
  }

 /// \f$ x \f$ on top of the back \f$ a \f$, the last item on top
 template< class A >
 static void pushBack( A & a , Item x ) {
  Item s = a.size() > 0 ? G::combine( a.summary( a.hi() - 1 ) , x ) : x;
  a.push( std::move( x ) , std::move( s ) );
  }

 /// the aggregate of the window \f$ q \f$
 template< class Q >
 static Item aggregate( const Q & q );
 };

/// the window of the real-time queue of Hood and Melville on stacks with
/// the aggregates [see `RealTimeMinQueue`]
template< class G >
using RealTimeWindow = RealTimeRotation< AggLink< G > >;

/// the window of the kind K
template< class G , window_kind K >
struct window_of
{
 using type = RealTimeWindow< G >;
 };

template< class G >
struct window_of< G , window_kind::subtract_on_evict >
{
 using type = SubtractWindow< G >;
 };

template< class G >
struct window_of< G , window_kind::monotone_deque >
{
 using type = DequeWindow< G >;
 };

template< class G >
struct window_of< G , window_kind::two_stacks >
{
 using type = StacksWindow< G >;
 };

}  // end( namespace detail )
/// \endcond

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS SlidingAggregate -------------------------*/
/*--------------------------------------------------------------------------*/

/// a QUEUE of values of the monoid `G` with the aggregate of its values,
/// each operation in \f$ O( 1 ) \f$ time, amortized or in the worst case by
/// `Policy`
template< Monoid G , class Policy = Amortized >
class SlidingAggregate
{

 static_assert( std::is_same_v< Policy , Amortized > ||
		  std::is_same_v< Policy , WorstCase > ,
		"ADS: the policy of a window is Amortized or WorstCase" );

/*--------------------------------------------------------------------------*/
 public:
 using Value = typename G::Value;  ///< the values
 using Item = Value;               ///< the items of the QUEUE
 using Algebra = G;                ///< their algebra

 /// the algorithm chosen by the algebra and the policy
 static constexpr window_kind kind = detail::windowKindOf< G , Policy >();

 /// true if there is no item
 [[nodiscard]] bool empty( void ) const { return( f_w.size() == 0 ); }

 /// the number \f$ n \f$ of items
 [[nodiscard]] std::size_t size( void ) const { return( f_w.size() ); }

 /// \f$ x \f$ at the back
 ///
 /// In pseudocode (beyond the book), the monotone deque \f$ C \f$ of the
 /// candidates of a selective monoid:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{enqueue}( x ) \cr
 /// 2 & \quad \text{append } x \text{ to the items} \cr
 /// 3 & \quad \textbf{while}\ C \ne \emptyset\ \textbf{and}
 ///     \ \mathrm{last}( C ) \oplus x = x\ \textbf{do}
 ///     \ \text{remove } \mathrm{last}( C ) \cr
 /// 4 & \quad \text{append } x \text{ to } C
 /// \end{array}}
 /// \f]
 /// \post one item more
 void enqueue( const Value & x ) {
  [[maybe_unused]] const std::size_t n = size();
  f_w.push( x );
  ADS_POST( size() == n + 1 );
  }

 /// removes the item at the front and returns it
 ///
 /// In pseudocode (Two-Stacks Lite, beyond the book), \f$ q \f$ the
 /// items, \f$ m \f$ the length of the first part, \f$ S \f$ the
 /// aggregates of its suffixes and \f$ b \f$ that of the rest:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{dequeue}() \cr
 /// 2 & \quad \textbf{if}\ m = 0\ \textbf{then} \cr
 /// 3 & \qquad s \gets e;\ \textbf{for}\ k \gets |q| - 1\ \textbf{downto}
 ///     \ 0\ \textbf{do}\ s \gets q_k \oplus s;\ \mathrm{push}( S , s )
 ///     \cr
 /// 4 & \qquad m \gets |q|;\ b \gets e \cr
 /// 5 & \quad \mathrm{pop}( S );\ m \gets m - 1 \cr
 /// 6 & \quad \textbf{return}\ \text{the first item of } q,
 ///     \text{ removed}
 /// \end{array}}
 /// \f]
 /// and the aggregate is \f$ \mathrm{top}( S ) \oplus b \f$.
 /// \pre the window is not empty
 /// \post one item less
 Value dequeue( void ) {
  ADS_PRE( ! empty() );
  [[maybe_unused]] const std::size_t n = size();
  Value x = f_w.pop();
  ADS_POST( size() == n - 1 );
  return( x );
  }

 /// the item at the front
 /// \pre the window is not empty
 [[nodiscard]] const Value & top( void ) const {
  ADS_PRE( ! empty() );
  return( f_w.front() );
  }

 /// the aggregate \f$ x_1 \oplus \cdots \oplus x_n \f$ of the items from
 /// the front to the back, the identity if there is none
 [[nodiscard]] Value aggregate( void ) const { return( f_w.aggregate() ); }

 /// `f( x )` on the items from the front to the back, \f$ O( n ) \f$
 template< class F >
 void forEach( F && f ) const {
  f_w.forEach( std::forward< F >( f ) );
  }

 /// removes all the items
 void clear( void ) { f_w.clear(); }

 /// the removals after which a group of floating point values is rebuilt
 /// from its items (subtract-on-evict only)
 [[nodiscard]] std::size_t period( void ) const
  requires( kind == window_kind::subtract_on_evict )
 {
  return( f_w.f_period );
  }

 /// sets the period of the rebuilding to \f$ p \f$
 /// \pre \f$ p \ge 1 \f$
 void period( std::size_t p )
  requires( kind == window_kind::subtract_on_evict )
 {
  ADS_PRE( p >= 1 );
  f_w.f_period = p;
  }

/*--------------------------------------------------------------------------*/
 private:
 typename detail::window_of< G , kind >::type f_w;  ///< the window

 };  // end( class SlidingAggregate )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/queue/impl/SlidingAggregate.tpp"

/*--------------------------------------------------------------------------*/

#endif /* SlidingAggregate.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File SlidingAggregate.h -----------------------*/
/*--------------------------------------------------------------------------*/
