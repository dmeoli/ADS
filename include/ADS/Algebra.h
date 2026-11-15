/*--------------------------------------------------------------------------*/
/*---------------------------- File Algebra.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The algebra of the aggregates, beyond the book: the concepts by which a
 * structure that keeps a combination of values (a prefix sum, the
 * aggregate of a window, the augmentation of a tree) chooses its update
 * rule at compile time (see \ref theory_ADS_algebra). An algebra `G` is a
 * type with static members, as the augmentations of `JoinTree` [see
 * `JoinTree.h`], of which these are refinements by opt-in members:
 *
 * - `Monoid< G >`: `G::identity()` and the associative `G::combine( a , b )`
 *   on `G::Value`; a structure updates an aggregate by recombining the
 *   parts that a change touched;
 * - `CommutativeMonoid< G >`: also `G::commutative`, true, so that a value
 *   may be combined into an aggregate wherever it stands;
 * - `Group< G >`: also `G::inverse( a )`, so that a value leaves an
 *   aggregate by one combination with its inverse (subtract-on-evict, the
 *   point update of a prefix sum by its difference);
 * - `AbelianGroup< G >`: both;
 * - `Selective< G >`: also `G::selective`, true, when `combine( a , b )` is
 *   always \f$ a \f$ or \f$ b \f$ (the least, the largest, the first), so
 *   that an aggregate of a window is kept by a monotone deque;
 * - `Action< A , G >`: a monoid of maps `A::Map` on `G::Value`, of identity
 *   `A::id()`, composition `A::compose( f , g )` (first \f$ g \f$, then
 *   \f$ f \f$) and `A::apply( f , a , n )`, the map applied to the
 *   aggregate \f$ a \f$ of \f$ n \f$ values (the lazy updates of a range);
 * - `exact_v< G >`: whether the values are exact, false for the floating
 *   point types, whose groups accumulate rounding errors and need a
 *   periodic recomputation.
 *
 * `SumOf< T >` (the abelian group of the sums), `MinOf< T >` and
 * `MaxOf< T >` (the selective commutative monoids of the least and of the
 * largest value, of identity the largest and the least value of `T`) are
 * the algebras that the structures of ADS use most. A type with the members
 * `from()` of an augmentation of `JoinTree` and these is both.
 *
 * \see P. M. Fenwick, A new data structure for cumulative frequency
 *      tables, Software: Practice and Experience 24(3), 327-336, 1994,
 *      doi:10.1002/spe.4380240306
 * \see K. Tangwongsan, M. Hirzel, S. Schneider, In-order sliding-window
 *      aggregation in worst-case constant time, The VLDB Journal 30(6),
 *      933-957, 2021, doi:10.1007/s00778-021-00668-3
 * \see M. Budiu, T. Chajed, F. McSherry, L. Ryzhyk, V. Tannen, DBSP:
 *      automatic incremental view maintenance for rich query languages,
 *      Proceedings of the VLDB Endowment 16(7), 1601-1614, 2023,
 *      doi:10.14778/3587136.3587137
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

#ifndef __ADS_Algebra
 #define __ADS_Algebra
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <concepts>
#include <cstddef>
#include <limits>
#include <type_traits>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*------------------------------ THE CONCEPTS ------------------------------*/
/*--------------------------------------------------------------------------*/

/// \cond
namespace detail
{

/// whether `G::commutative` is declared and true
template< class G >
constexpr bool commutativeFlag( void )
{
 if constexpr( requires {
		{ G::commutative } -> std::convertible_to< bool >;
	        } )
  return( bool( G::commutative ) );
 else
  return( false );
 }

/// whether `G::selective` is declared and true
template< class G >
constexpr bool selectiveFlag( void )
{
 if constexpr( requires {
		{ G::selective } -> std::convertible_to< bool >;
	        } )
  return( bool( G::selective ) );
 else
  return( false );
 }

}  // end( namespace detail )
/// \endcond

/// a monoid on `G::Value`: the identity `G::identity()` and the associative
/// operation `G::combine( a , b )`
template< class G >
concept Monoid = requires( const typename G::Value & a )
{
 { G::identity() } -> std::convertible_to< typename G::Value >;
 { G::combine( a , a ) } -> std::convertible_to< typename G::Value >;
 };

/// a monoid whose operation commutes, by the opt-in `G::commutative`
template< class G >
concept CommutativeMonoid = Monoid< G > && detail::commutativeFlag< G >();

/// a monoid with the inverse `G::inverse( a )`, i.e., `combine( inverse( a
/// ) , a )` and `combine( a , inverse( a ) )` are the identity
template< class G >
concept Group = Monoid< G > && requires( const typename G::Value & a )
{
 { G::inverse( a ) } -> std::convertible_to< typename G::Value >;
 };

/// a group whose operation commutes
template< class G >
concept AbelianGroup = Group< G > && CommutativeMonoid< G >;

/// a monoid whose `combine( a , b )` is always \f$ a \f$ or \f$ b \f$, by
/// the opt-in `G::selective`
template< class G >
concept Selective = Monoid< G > && detail::selectiveFlag< G >();

/// a monoid of the maps `A::Map` acting on the values of the monoid `G`:
/// the identity map `A::id()`, the composition `A::compose( f , g )` (first
/// \f$ g \f$, then \f$ f \f$), and `A::apply( f , a , n )`, the map applied
/// to the aggregate \f$ a \f$ of \f$ n \f$ values
template< class A , class G >
concept Action =
  Monoid< G > && requires( const typename A::Map & f ,
			   const typename G::Value & a , std::size_t n )
{
   { A::id() } -> std::convertible_to< typename A::Map >;
   { A::compose( f , f ) } -> std::convertible_to< typename A::Map >;
   { A::apply( f , a , n ) } -> std::convertible_to< typename G::Value >;
   };

/// whether the values of `G` are exact, false for the floating point types,
/// whose groups drift by rounding when a value leaves by its inverse
template< class G >
inline constexpr bool exact_v =
  ! std::is_floating_point_v< typename G::Value >;

/*--------------------------------------------------------------------------*/
/*------------------------------ THE ALGEBRAS ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the abelian group of the sums of `T`, of identity `T()`
template< class T >
struct SumOf
{
 using Value = T;                           ///< the sums
 static constexpr bool commutative = true;  ///< \f$ a + b = b + a \f$
 /// 0, the value made by `T()`
 static constexpr Value identity( void ) { return( T() ); }
 /// \f$ a + b \f$
 static constexpr Value combine( const Value & a , const Value & b ) {
  return( a + b );
  }
 /// \f$ -a \f$
 static constexpr Value inverse( const Value & a ) { return( -a ); }
 };

/// the selective commutative monoid of the least value of `T`, of identity
/// \f$ +\infty \f$ if `T` has it, its largest value otherwise
template< class T >
struct MinOf
{
 using Value = T;                           ///< the minima
 static constexpr bool commutative = true;  ///< it commutes
 static constexpr bool selective = true;    ///< one of the two
 /// the largest value of the type
 static constexpr Value identity( void ) {
  if constexpr( std::numeric_limits< T >::has_infinity )
   return( std::numeric_limits< T >::infinity() );
  else
   return( std::numeric_limits< T >::max() );
  }
 /// \f$ \min( a , b ) \f$, \f$ a \f$ if they are equivalent
 static constexpr Value combine( const Value & a , const Value & b ) {
  return( b < a ? b : a );
  }
 };

/// the selective commutative monoid of the largest value of `T`, of
/// identity \f$ -\infty \f$ if `T` has it, its least value otherwise
template< class T >
struct MaxOf
{
 using Value = T;                           ///< the maxima
 static constexpr bool commutative = true;  ///< it commutes
 static constexpr bool selective = true;    ///< one of the two
 /// the least value of the type
 static constexpr Value identity( void ) {
  if constexpr( std::numeric_limits< T >::has_infinity )
   return( -std::numeric_limits< T >::infinity() );
  else
   return( std::numeric_limits< T >::lowest() );
  }
 /// \f$ \max( a , b ) \f$, \f$ a \f$ if they are equivalent
 static constexpr Value combine( const Value & a , const Value & b ) {
  return( a < b ? b : a );
  }
 };

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif /* Algebra.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Algebra.h ---------------------------*/
/*--------------------------------------------------------------------------*/
