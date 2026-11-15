/*--------------------------------------------------------------------------*/
/*---------------------- File MaterializedAggregate.h ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The aggregate of a structure kept by its events, beyond the book:
 * `MaterializedAggregate< G , Period >` is an observer [see
 * `Modification.h`] of a dictionary whose handles are its keys (as those of
 * `JoinTree`), which keeps the combination by the commutative monoid `G`
 * [see `Augmentation` in `JoinTree.h`] of the values \f$ \mathrm{from}( k ,
 * v ) \f$ of its pairs, from the events `Inserted`, `Erased`, `Assigned`,
 * `Cleared` and `Rebuilt`, and gives it by `value()`. How an event updates
 * the aggregate is decided by the algebra of `G` at compile time (see
 * \ref theory_tree_observed). When `G` is an abelian group (the static
 * members `commutative` and `inverse()`), an insertion combines the
 * aggregate with the new value, an erasure with the inverse of the old
 * one, an assignment with both, each in \f$ O( 1 ) \f$ combinations and no
 * memory beyond the aggregate. When `G` is selective (the static member
 * `selective`: the combination of two values is one of them, as the
 * maximum), the observer keeps the multiset of the values ordered by `G`,
 * whose first is the aggregate, in \f$ O( \log m ) \f$ per event for
 * \f$ m \f$ pairs. Otherwise an insertion combines, and an erasure or an
 * assignment marks the aggregate stale, recomputed at the next `value()`
 * from the structure given to `bind()`, in \f$ O( m ) \f$.
 *
 * `Rebuilt` marks the aggregate stale, `Cleared` makes it the identity.
 * For an abelian group of inexact values (a floating type), the
 * differences accumulate rounding errors, and the aggregate is marked
 * stale every `Period` changes (never if 0), so that the next `value()`
 * recomputes it from the structure bound, which bounds the drift; `audit()`
 * compares the aggregate with the one recomputed from a structure, by
 * equality for exact values and within a relative tolerance for inexact
 * ones, and is called by the subject at the level of the audits.
 *
 * The contracts [see `Contract.h`]: `value()` of a stale aggregate needs a
 * structure bound; the structure bound outlives the observer, or is bound
 * again; `on()` of `Erased`, `Cleared` and `Rebuilt` does not throw, which
 * needs a `combine()` and an order of `G` that do not throw.
 *
 * \see A. Frangioni, A. Manca, A computational study of cost
 *      reoptimization for min-cost flow problems, INFORMS Journal on
 *      Computing 18(1), 61-70, 2006, doi:10.1287/ijoc.1040.0081 (the
 *      audit of an incremental computation by one from scratch)
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

#ifndef __ADS_MaterializedAggregate
 #define __ADS_MaterializedAggregate
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Algebra.h"
#include "ADS/Contract.h"
#include "ADS/Modification.h"
#include "ADS/tree/JoinTree.h"

#include <cmath>
#include <concepts>
#include <cstddef>
#include <set>
#include <type_traits>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/// \cond
namespace detail
{

/// a function of a key and a value that does nothing
struct AnyPair
{
 template< class A , class B >
 void operator()( const A & , const B & ) const { }
 };

/// a structure whose pairs `forEach( f )` gives to \f$ f \f$
template< class S >
concept pair_visitable = requires( const S & s )
{ s.forEach( AnyPair() ); };

}  // end( namespace detail )
/// \endcond

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS MaterializedAggregate -----------------------*/
/*--------------------------------------------------------------------------*/

/// the combination by the commutative monoid `G` of the values of the pairs
/// of a dictionary, kept by its events; for an abelian group of inexact
/// values, recomputed from the structure bound every `Period` changes
template< class G , std::size_t Period = 1024 >
 requires CommutativeMonoid< G >
class MaterializedAggregate
{

/*--------------------------------------------------------------------------*/
 public:
 using Value = typename G::Value;  ///< the type of the aggregate

 /// how the events update the aggregate
 enum class Kind {
  Group ,      ///< by the value and its inverse
  Selective ,  ///< by the multiset of the values
  Monoid      ///< by a combination, or a recomputation
  };

 /// the kind of `G`
 static constexpr Kind kind =
   AbelianGroup< G >                                         ? Kind::Group
   : ( Selective< G > && std::equality_comparable< Value > ) ? Kind::Selective
							     : Kind::Monoid;

 /// the identity, with no structure bound
 MaterializedAggregate( void ) = default;

 /// the aggregate of \f$ s \f$, bound
 template< detail::pair_visitable S >
 explicit MaterializedAggregate( const S & s ) {
  bind( s );
  }

 /// \f$ s \f$ bound, and the aggregate recomputed from it, in
 /// \f$ O( m ) \f$ (\f$ O( m \log m ) \f$ if selective)
 template< detail::pair_visitable S >
 void bind( const S & s );

/*--------------------------------------------------------------------------*/
/*------------------------------ THE EVENTS --------------------------------*/
/*--------------------------------------------------------------------------*/

 /// the pair \f$ ( k , v ) \f$ inserted: \f$ a \gets a \otimes
 /// \mathrm{from}( k , v ) \f$, or the value added to the multiset
 template< class H , class K , class V >
 void on( const mod::Inserted< H , K , V > & e );

 /// the pair \f$ ( k , v ) \f$ erased: \f$ a \gets a \otimes
 /// \mathrm{from}( k , v )^{-1} \f$, or the value taken out of the
 /// multiset, or the aggregate stale
 template< class H , class K , class V >
 void on( const mod::Erased< H , K , V > & e ) noexcept;

 /// the value of the key \f$ h \f$ changed from \f$ v \f$ to \f$ w \f$:
 /// \f$ a \gets a \otimes \mathrm{from}( h , w ) \otimes \mathrm{from}( h ,
 /// v )^{-1} \f$, or the two values exchanged in the multiset, or the
 /// aggregate stale
 template< class H , class V >
 void on( const mod::Assigned< H , V > & e );

 /// the structure emptied: the identity
 void on( const mod::Cleared & ) noexcept;

 /// anything may have changed: the aggregate stale
 void on( const mod::Rebuilt & ) noexcept { f_stale = true; }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE QUERIES -------------------------------*/
/*--------------------------------------------------------------------------*/

 /// the aggregate, recomputed from the structure bound if stale
 /// \pre a structure is bound, if stale
 [[nodiscard]] Value value( void ) const;

 /// whether the next `value()` recomputes
 [[nodiscard]] bool stale( void ) const { return( f_stale ); }

 /// whether a structure is bound
 [[nodiscard]] bool bound( void ) const { return( f_src != nullptr ); }

 /// the recomputations made so far
 [[nodiscard]] std::size_t recomputations( void ) const {
  return( f_recomputed );
  }

 /// whether the aggregate (if not stale) is that of \f$ s \f$: equal for
 /// exact values, within a relative tolerance of \f$ 10^{-9} \f$ for those
 /// of a floating type
 template< detail::pair_visitable S >
 [[nodiscard]] bool audit( const S & s ) const;

/*--------------------------------------------------------------------------*/
 private:
 /// the order of the values of a selective `G`: \f$ x \f$ before
 /// \f$ y \f$ if \f$ x \ne y \f$ and \f$ x \otimes y = x \f$
 struct Before
 {
  bool operator()( const Value & x , const Value & y ) const {
   return( ! ( x == y ) && ( G::combine( x , y ) == x ) );
   }
  };

 /// the multiset of a selective `G`, nothing otherwise
 struct NoSet
 { };
 using Set = std::conditional_t< kind == Kind::Selective ,
				 std::multiset< Value , Before > , NoSet >;

 /// the value counted for the drift
 void changed( void ) noexcept;

 /// the aggregate recomputed from the structure bound
 void recompute( void ) const;

 /// the aggregate of \f$ s \f$, from scratch
 template< class S >
 static Value fold( const S & s );

 mutable Value f_a = G::identity();        ///< the aggregate
 ADS_NO_UNIQUE_ADDRESS mutable Set f_set;  ///< the values, if selective
 mutable bool f_stale = false;             ///< whether to recompute
 mutable std::size_t f_drift = 0;          ///< the changes since then
 mutable std::size_t f_recomputed = 0;     ///< the recomputations
 const void * f_src = nullptr;             ///< the structure bound
 /// the recomputation from the structure bound
 void ( *f_redo )( const void * , const MaterializedAggregate & ) = nullptr;

 };  // end( class MaterializedAggregate )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/MaterializedAggregate.tpp"

/*--------------------------------------------------------------------------*/

#endif /* MaterializedAggregate.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File MaterializedAggregate.h ------------------*/
/*--------------------------------------------------------------------------*/
