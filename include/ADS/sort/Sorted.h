/*--------------------------------------------------------------------------*/
/*----------------------------- File Sorted.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The checks of the contracts of the sorts [see `Contract.h`]: a copy of the
 * input kept only when the audits are on (`ADS_CONTRACTS` at least 3), and the
 * check that the output is sorted and holds the same items as the input, which
 * sorts a copy of the input with `std::sort` and compares the two item by item
 * through the equivalence of the order, in \f$ O( n \log n ) \f$ time instead
 * of the \f$ O( n^2 ) \f$ of `std::is_permutation` with a predicate; and the
 * check that a sequence is sorted. These checks call the comparator of the
 * caller, so that `checkingContract()` tells a comparator that counts its
 * calls, as those of the tests of the book, which calls come from a contract.
 * The functions are defined in `Sorted.tpp`.
 *
 * `SortRange` is the range that the sorts of ADS take beside the
 * `std::vector` of the book and the `std::span` on which they work: a
 * random-access range of movable items, sized and writable, such as an
 * array, a `std::deque`, a `TieredVector` or the vectors of Boost and Eigen
 * (`sortRange()`). A contiguous range is sorted in place, as the span of
 * its items. A range that is not contiguous is sorted in place by the
 * sorts that read the items only by their ranks (those of the book), on
 * the view of its items, and through a vector of its items by the others,
 * whose code reads the items by their addresses (\f$ 2 n \f$ moves and
 * \f$ n \f$ items of memory beyond the sort), as the comment of each says.
 * The sorts themselves are templates on the storage of the items, taken by
 * reference (`SortStore`, a vector or a span, and `RankStore`, which adds
 * the views, for the sorts of the book), and each overload on a
 * `std::vector` is one call of the same template on the vector, inlined
 * (`ADS_SORT_FORWARD`), so that it runs the code it ran before the ranges.
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

#ifndef __ADS_Sorted
 #define __ADS_Sorted
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Portable.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <ranges>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// the overloads of the sorts on a vector, each one call of the sort on its
// storage, inlined into the caller, so that a call on a vector runs the code
// it ran before the sorts took spans and views
#define ADS_SORT_FORWARD ADS_ALWAYS_INLINE

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/// true while a contract of ADS is evaluated: a comparator that counts its
/// calls skips those made by the contracts
[[nodiscard]] bool checkingContract( void );

namespace detail
{

/// the condition `f()` evaluated with `checkingContract()` true
template< class F >
bool contract( F && f );

/// a copy of \f$ A[ \mathit{lo} .. \mathit{hi} - 1 ] \f$, \f$ A \f$ any
/// random-access range, if the audits are on, an empty vector otherwise
template< std::ranges::random_access_range S >
std::vector< std::ranges::range_value_t< S > >
auditCopy( const S & A , std::size_t lo = 0 ,
	   std::size_t hi = std::size_t( -1 ) );


/// true if \f$ A[ \mathit{lo} .. \mathit{hi} - 1 ] \f$ holds the items of
/// `before`, sorted or not, by sorting both copies and comparing them through
/// the equivalence of `less`, \f$ O( n \log n ) \f$
template< std::ranges::random_access_range S , class C >
bool samePermutation( const S & A ,
		      std::vector< std::ranges::range_value_t< S > > before ,
		      C less , std::size_t lo = 0 ,
		      std::size_t hi = std::size_t( -1 ) );


/// true if \f$ A[ \mathit{lo} .. \mathit{hi} - 1 ] \f$ is sorted by `less` and
/// holds the items of `before`, the copy of the same range made by
/// `auditCopy()` before the sort
template< std::ranges::random_access_range S , class C >
bool sortedPermutation( const S & A ,
			std::vector< std::ranges::range_value_t< S > > before ,
			C less , std::size_t lo = 0 ,
			std::size_t hi = std::size_t( -1 ) );


/// true if the items of the SEQUENCE \f$ L \f$ [see `ADT.h`] are sorted by
/// `less`
template< class S , class C >
bool sequenceSorted( const S & L , C less );

/// true if the order `C` is the natural one of `T`, i.e., `std::less<>` or
/// `std::less< T >`
template< class T , class C >
inline constexpr bool naturalOrder =
  std::is_same_v< C , std::less<> > || std::is_same_v< C , std::less< T > >;

/// whether `R` is a `std::vector` with the allocator of the standard or a
/// `std::span`, the ranges of the overloads of the sorts themselves
template< class R >
inline constexpr bool ownRange = false;

template< class T >
inline constexpr bool ownRange< std::vector< T > > = true;

template< class T , std::size_t N >
inline constexpr bool ownRange< std::span< T , N > > = true;

template< class R >
inline constexpr bool ownRange< std::ranges::ref_view< R > > = true;

/// \f$ f( s ) \f$ on the span \f$ s \f$ of the items of \f$ r \f$ if \f$ r \f$
/// is contiguous; else, if \f$ g \f$ is not `void`, \f$ g( w ) \f$ on the
/// view \f$ w \f$ of \f$ r \f$, in place; else \f$ f( v ) \f$ on the vector
/// \f$ v \f$ of the items of \f$ r \f$, moved in and moved back after the
/// call; what \f$ f \f$ (or \f$ g \f$) gives
template< class R , class F , class G = void * >
auto sortRange( R & r , F && f , G && g = nullptr );

}  // end( namespace detail )

/// a range that the sorts take beside `std::vector` and `std::span`: random
/// access, sized, its items movable in and out of it [see above]
template< class R >
concept SortRange =
  std::ranges::random_access_range< R > && std::ranges::sized_range< R > &&
  std::permutable< std::ranges::iterator_t< R > > &&
  std::movable< std::ranges::range_value_t< R > > &&
  ! detail::ownRange< std::remove_cvref_t< R > >;

/// \cond
namespace detail
{

template< class S >
inline constexpr bool spanOrVector = false;

template< class T >
inline constexpr bool spanOrVector< std::vector< T > > = true;

template< class T >
inline constexpr bool spanOrVector< std::span< T > > = true;

}  // end( namespace detail )
/// \endcond

/// the storage on which the sorts work in place, by reference: a
/// `std::vector` or a `std::span` of its items, read by their addresses
template< class S >
concept SortStore = detail::spanOrVector< std::remove_cvref_t< S > >;

/// the storage on which the sorts of the book work in place, which read the
/// items by their ranks only: a `SortStore`, or the view of a random-access
/// range that is not contiguous
template< class S >
concept RankStore =
  SortStore< S > ||
  ( std::ranges::random_access_range< std::remove_cvref_t< S > > &&
    std::ranges::sized_range< std::remove_cvref_t< S > > &&
    std::ranges::view< std::remove_cvref_t< S > > );

/// a `SortStore` of strings
template< class S >
concept StringStore =
  SortStore< S > &&
  std::same_as< std::ranges::range_value_t< std::remove_cvref_t< S > > ,
		std::string >;

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/Sorted.tpp"

/*--------------------------------------------------------------------------*/

#endif /* Sorted.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Sorted.h ----------------------------*/
/*--------------------------------------------------------------------------*/
