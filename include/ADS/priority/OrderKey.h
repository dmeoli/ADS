/*--------------------------------------------------------------------------*/
/*---------------------------- File OrderKey.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The unsigned key of an item whose order is that of an unsigned word,
 * beyond the book, for the queues that compare and sort many items: a
 * number of at most 8 bytes, or a pair of two numbers of at most 8 bytes
 * in all (the pairs ( priority , item ) of the heaps), ordered by
 * std::less or std::greater. The key of a number is that of the radix sort
 * [see RadixSort.h], i.e., the bits of an unsigned integer, the sign bit
 * flipped for a signed one and, for a floating point number, all the bits
 * flipped if it is negative and the sign bit otherwise; the key of a pair
 * is the key of its first item followed by that of the second, so that
 * the order of the keys is the lexicographic order of the pairs; for
 * std::greater the key is complemented. Two items equal by the order have
 * the same key, but for the floating point zeros, -0 coming before +0,
 * which only breaks a tie; a NaN, which has no place in the order, has
 * one among the keys.
 *
 * A comparison of two keys is a comparison of two words, which the
 * compiler makes with no jump (a conditional move or a mask), where the
 * comparison of two pairs takes two, and the keys give the radix sort its
 * digits.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, 2nd ed.,
 *      Addison-Wesley, 1998, section 5.2.5
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

#ifndef __ADS_OrderKey
 #define __ADS_OrderKey
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/RadixSort.h"

#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS::detail {

/// a pair of two numbers
template< class T >
struct IsNumberPair : std::false_type {};

template< class A , class B >
struct IsNumberPair< std::pair< A , B > > : std::bool_constant<
 RadixKey< A > && RadixKey< B > && ( sizeof( A ) + sizeof( B ) <= 8 ) > {};

/// the items with an order key: numbers and pairs of numbers
template< class T >
concept OrderKeyed = RadixKey< T > || IsNumberPair< T >::value;

/// the orders of the keys: std::less and std::greater
template< class T , class C >
inline constexpr bool kAscending = std::is_same_v< C , std::less<> > ||
                                   std::is_same_v< C , std::less< T > >;

template< class T , class C >
inline constexpr bool kDescending = std::is_same_v< C , std::greater<> > ||
                                    std::is_same_v< C , std::greater< T > >;

/// the items T ordered by C have keys
template< class T , class C >
inline constexpr bool kKeyed = OrderKeyed< T > &&
                               ( kAscending< T , C > || kDescending< T , C > );

/// the key of x, ascending
template< OrderKeyed T >
[[nodiscard]] inline auto orderKey( const T & x )
{
 if constexpr( RadixKey< T > )
  return( radixKey( x ) );
 else {
  using A = typename T::first_type;
  using B = typename T::second_type;
  using U = std::conditional_t< sizeof( A ) + sizeof( B ) <= 4 ,
                                std::uint32_t , std::uint64_t >;
  return( U( ( U( radixKey( x.first ) ) << ( 8 * sizeof( B ) ) ) |
             U( radixKey( x.second ) ) ) );
  }
 }

/// the key of x in the order C, complemented for std::greater
template< class C , class T >
 requires kKeyed< T , C >
[[nodiscard]] inline auto orderKeyBy( const T & x )
{
 const auto k = orderKey( x );
 if constexpr( kDescending< T , C > )
  return( decltype( k )( ~k ) );
 else
  return( k );
 }

}  // end( namespace ADS::detail )

/*--------------------------------------------------------------------------*/

#endif  /* OrderKey.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File OrderKey.h ---------------------------*/
/*--------------------------------------------------------------------------*/
