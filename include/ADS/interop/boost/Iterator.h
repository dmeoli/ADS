/*--------------------------------------------------------------------------*/
/*----------------------------- File Iterator.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The traversal that Boost reads from the iterators of ADS whose reference
 * is built, beyond the book, compiled only with `ADS_WITH_BOOST`. The
 * iterators of the dictionaries, of the search trees and of the succinct
 * structures make their item when they are read (a pair of references, a
 * bit, an integer), so that they declare the legacy category
 * `std::input_iterator_tag` while they model `std::forward_iterator` or
 * more; Boost.Range, Boost.Graph and the rest of Boost read the traversal
 * from the legacy category, and would take them as single pass, rejecting
 * them where a forward range is needed (`boost::max_element()`). The
 * specialization of `boost::iterators::iterator_traversal` of this header
 * gives such an iterator the traversal of its concept of C++20
 * (forward, bidirectional or random access), and only to the iterators of
 * ADS (those whose associated namespaces include `ADS`, found by a probe
 * that argument-dependent lookup sees only there), so that the iterators of
 * the other libraries in the same program keep the traversal they declare.
 * The header must be included before the headers of Boost that read the
 * traversal.
 *
 * \see D. Abrahams, J. Siek, T. Witt, New iterator concepts, ISO/IEC JTC1
 *      SC22 WG21 N1640, 2004,
 *      https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2004/n1640.html
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

#ifndef __ADS_interop_boost_Iterator
 #define __ADS_interop_boost_Iterator
/* self-identification: #endif constant */

#if defined( ADS_WITH_BOOST )

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <boost/iterator/iterator_categories.hpp>

#include <iterator>
#include <type_traits>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// \cond
namespace ADS
{

/// the probe of the namespace of ADS, declared and never defined: a call
/// of it by argument-dependent lookup is valid only on a type of ADS
template< class I >
void adsIteratorProbe( const I & );

namespace detail
{

template< class I >
void adsIteratorProbe( const I & );

}  // end( namespace detail )

namespace bgl::probe
{

void adsIteratorProbe( void ) = delete;  // only by ADL

/// whether `I` is a type of ADS
template< class I >
concept of_ads = requires( const I & i )
{ adsIteratorProbe( i ); };

}  // end( namespace bgl::probe )

namespace bgl
{

/// an iterator of ADS that declares the legacy category of input while it
/// models a forward iterator of C++20, i.e., one whose reference is built
template< class I >
concept built_reference_iterator =
  probe::of_ads< I > && std::forward_iterator< I > &&
  std::is_same_v< typename std::iterator_traits< I >::iterator_category ,
		  std::input_iterator_tag >;

}  // end( namespace bgl )

}  // end( namespace ADS )

namespace boost::iterators
{

/// the traversal of an iterator of ADS whose reference is built: that of
/// its concept of C++20
template< class I >
 requires ADS::bgl::built_reference_iterator< I >
struct iterator_traversal< I >
{
 using type = std::conditional_t<
   std::random_access_iterator< I > , random_access_traversal_tag ,
   std::conditional_t< std::bidirectional_iterator< I > ,
		       bidirectional_traversal_tag , forward_traversal_tag > >;
 };

}  // end( namespace boost::iterators )
/// \endcond

#endif /* ADS_WITH_BOOST */

/*--------------------------------------------------------------------------*/

#endif /* Iterator.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Iterator.h --------------------------*/
/*--------------------------------------------------------------------------*/
