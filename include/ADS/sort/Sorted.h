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
 * `std::vector` of the book: a random-access range of movable items, sized
 * and writable, such as a `std::span`, a `std::deque`, an array, a
 * `TieredVector` or the vectors of Boost and Eigen. Each sort on such a
 * range moves its items into a vector, sorts them there and moves them
 * back (`throughVector()`): \f$ 2 n \f$ moves and \f$ n \f$ items of
 * memory beyond the sort, the buffer that the radix and the learned sorts
 * need anyway, the same code whatever the range and the same result as on
 * a vector of the same items.
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

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>

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

/// a copy of \f$ A[ \mathit{lo} .. \mathit{hi} - 1 ] \f$ if the audits are on,
/// an empty vector otherwise
template< class T >
std::vector< T > auditCopy( const std::vector< T > & A , std::size_t lo = 0 ,
			    std::size_t hi = std::size_t( -1 ) );

/// true if \f$ A[ \mathit{lo} .. \mathit{hi} - 1 ] \f$ is sorted by `less` and
/// holds the items of `before`, the copy of the same range made by
/// `auditCopy()` before the sort
template< class T , class C >
bool sortedPermutation( const std::vector< T > & A , std::vector< T > before ,
			C less , std::size_t lo = 0 ,
			std::size_t hi = std::size_t( -1 ) );

/// true if \f$ A[ \mathit{lo} .. \mathit{hi} - 1 ] \f$ holds the items of
/// `before`, sorted or not, by sorting both copies and comparing them through
/// the equivalence of `less`, \f$ O( n \log n ) \f$
template< class T , class C >
bool samePermutation( const std::vector< T > & A , std::vector< T > before ,
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

/// whether `R` is a `std::vector` with the allocator of the standard, the
/// range of the sorts of the book
template< class R >
inline constexpr bool stdVector = false;

template< class T >
inline constexpr bool stdVector< std::vector< T > > = true;

/// \f$ f( v ) \f$ on the vector \f$ v \f$ of the items of \f$ r \f$, moved
/// in and moved back after the call; what \f$ f \f$ gives
template< class R , class F >
auto throughVector( R & r , F && f );

}  // end( namespace detail )

/// a range that the sorts take beside `std::vector`: random access, sized,
/// its items movable in and out of it, and not a `std::vector` itself, which
/// takes the overloads of the book
template< class R >
concept SortRange =
  std::ranges::random_access_range< R > && std::ranges::sized_range< R > &&
  std::permutable< std::ranges::iterator_t< R > > &&
  std::movable< std::ranges::range_value_t< R > > &&
  ! detail::stdVector< std::remove_cvref_t< R > >;

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
