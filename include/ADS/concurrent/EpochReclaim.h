/*--------------------------------------------------------------------------*/
/*--------------------------- File EpochReclaim.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The reclamation of the memory of the lock-free structures by epochs,
 * beyond the book: the scheme of Fraser, as a policy [see `Reclaim.h`]
 * on a domain. The domain keeps an epoch
 * \f$ E \f$, from 2, and a list of records, one per thread that uses it,
 * each announcing \f$ 2 e + 1 \f$ while its thread runs an operation (a
 * guard) begun in the epoch \f$ e \f$, and 0 outside; a guard begins by
 * reading \f$ E \f$ and announcing it by a sequentially consistent
 * exchange, before its thread reads the structure, whose words it then
 * reads with sequential consistency too (`protect()`, on x86-64 the same
 * plain load as an acquire, on AArch64 the same `ldar`). A node retired
 * waits in one of three lists of its record, the <em>limbo</em> of its
 * label modulo 3, and is freed by the thread of the record once \f$ E \f$
 * has grown enough past its label. With `FraserEpochs< R >`, the label is
 * \f$ E \f$, read when the node is retired, and the node is freed by the
 * first guard of its record that reads \f$ E \ge \ell + 2 \f$ for the
 * label \f$ \ell \f$; a guard that ends after \f$ R \f$ retirements of its
 * record since the last attempt reads all the records, and raises
 * \f$ E \f$ by a compare-and-swap if none of them is inside an operation
 * of an older epoch, i.e., in \f$ O( N ) \f$ time for \f$ N \f$ records,
 * which \f$ R \f$ retirements share.
 *
 * No node is freed while a guard that may have reached it is
 * open (see \ref theory_concurrent_reclaim), and the reclamation is
 * blocking: a thread stopped inside an operation keeps \f$ E \f$ from
 * growing past its epoch plus 1, and the nodes retired since from being
 * freed, while the operations of the others go on; the nodes waiting are
 * then unbounded, which the hazard pointers [see `HazardPointer.h`] avoid.
 * A record given back by its thread keeps its limbo for the next thread
 * that takes it; the domain frees in its destructor the nodes still
 * waiting, so that none is leaked.
 *
 * The contracts [see `Contract.h`]: no record is alive when the domain is
 * destroyed; the guards of a record do not overlap, and `retire()` is
 * called inside a guard of the same record; `pending()` and `freed()` may
 * be read at any time, as sums of counters that may be read in the middle
 * of a change. The operations are defined in `EpochReclaim.tpp`.
 *
 * \see K. Fraser, Practical lock-freedom, PhD thesis, technical report
 *      UCAM-CL-TR-579, University of Cambridge, 2004, doi:10.48456/tr-579
 * \see T. E. Hart, P. E. McKenney, A. D. Brown, J. Walpole, Performance of
 *      memory reclamation for lockless synchronization, Journal of Parallel
 *      and Distributed Computing 67(12), 1270-1285, 2007,
 *      doi:10.1016/j.jpdc.2007.04.010
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

#ifndef __ADS_EpochReclaim
 #define __ADS_EpochReclaim
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/concurrent/Reclaim.h"

#include <atomic>
#include <cstddef>
#include <cstdint>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS EpochDomain ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the DOMAIN of a reclamation by epochs: the epoch and the records of the
/// threads, with the scheme of Fraser (an attempt to raise the epoch per
/// \f$ R \f$ retirements)
template< unsigned R >
class EpochDomain
{

 static_assert( R >= 1 , "an attempt per R >= 1 retirements" );

/*--------------------------------------------------------------------------*/
 public:
 /// the epochs that the label of a node must be behind \f$ E \f$ for its
 /// free
 static constexpr std::uint64_t kLag = 2;

 class Local;

 /// the domain of the epoch 2 and no records
 EpochDomain( void ) = default;
 /// no copy, since the records point to it
 EpochDomain( const EpochDomain & ) = delete;
 /// no copy, since the records point to it
 EpochDomain & operator=( const EpochDomain & ) = delete;

 /// the frees of the nodes waiting in the limbo of the records, and of the
 /// records
 /// \pre no record is alive
 ~EpochDomain();

 /// the epoch \f$ E \f$
 [[nodiscard]] std::uint64_t epoch( void ) const {
  return( f_epoch.load( std::memory_order_relaxed ) );
  }

 /// the records made so far, \f$ N \f$
 [[nodiscard]] std::size_t records( void ) const;

 /// the nodes retired and not yet freed
 [[nodiscard]] std::size_t pending( void ) const;

 /// the nodes freed so far, not counting those of the destructor
 [[nodiscard]] std::uint64_t freed( void ) const;

/*--------------------------------------------------------------------------*/
 private:
 /// the record of a thread: its announce, its three limbo with their
 /// labels, its counters, and the retirements since its last attempt to
 /// raise the epoch;
 /// the records are never freed but by the destructor
 struct alignas( 64 ) Record
 {
  /// \f$ 2 e + 1 \f$ inside an operation of the epoch \f$ e \f$, 0 out
  std::atomic< std::uint64_t > local{ 0 };
  /// held by a thread
  std::atomic< bool > used{ true };
  /// the next record, fixed once the record is published
  Record * next = nullptr;
  /// the nodes retired, by their label modulo 3
  Retirable * limbo[ 3 ] = { nullptr , nullptr , nullptr };
  /// the label of the nodes of each limbo
  std::uint64_t label[ 3 ] = { 0 , 0 , 0 };
  /// the nodes retired, ever
  std::atomic< std::uint64_t > retired{ 0 };
  /// the nodes freed, ever
  std::atomic< std::uint64_t > freed{ 0 };
  /// the epoch of the open guard
  std::uint64_t now = 0;
  /// the retirements since the last attempt
  unsigned since = 0;
  };

 /// a record given back, else a new one pushed on the list
 Record * acquire( void );

 /// the nodes of the limbo whose label is kLag behind e freed
 static void collect( Record & r , std::uint64_t e ) noexcept;

 /// E raised if no record inside an operation announces an older epoch
 /// than E
 void advance( void ) noexcept;

 alignas( 64 ) std::atomic< std::uint64_t > f_epoch{ 2 };  ///< the epoch
 alignas( 64 ) std::atomic< Record * > f_records{ nullptr };
 ///< the records

 };  // end( class EpochDomain )

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS EpochDomain::Local ------------------------*/
/*--------------------------------------------------------------------------*/

/// the RECORD of one thread on an `EpochDomain`, and its guards
template< unsigned R >
class EpochDomain< R >::Local
{

/*--------------------------------------------------------------------------*/
 public:
 class Guard;

 /// a record of d, one given back by another thread or a new one
 explicit Local( EpochDomain & d ) : f_d( &d ) , f_r( d.acquire() ) { }

 /// the record moved, the other left without one
 Local( Local && o ) noexcept : f_d( o.f_d ) ,
   f_r( o.f_r ) { o.f_r = nullptr; }

 /// no copy, since a record has one owner
 Local( const Local & ) = delete;
 /// no assignment, since a record has one owner
 Local & operator=( const Local & ) = delete;
 /// no assignment, since a record has one owner
 Local & operator=( Local && ) = delete;

 /// the record given back to the domain, with its limbo
 ~Local() {
  if( f_r )
   f_r->used.store( false , std::memory_order_release );
  }

 /// the node p handed to the reclamation; inside a guard of the record
 ///
 /// In pseudocode (Fraser, beyond the book), with \f$ \ell_j \f$, \f$ L_j
 /// \f$ the label and the limbo \f$ j \f$ of the record:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{retire}( p ) \cr
 /// 2 & \quad \ell \gets E;\ j \gets \ell \bmod 3 \cr
 /// 3 & \quad \textbf{if}\ \ell_j \ne \ell\ \textbf{then}\ \text{free}
 ///     \ L_j;\ L_j \gets \emptyset;\ \ell_j \gets \ell\quad
 ///     \triangleright\ \ell_j \le \ell - 2 \cr
 /// 4 & \quad L_j \gets L_j \cup \{ p \}
 /// \end{array}}
 /// \f]
 void retire( Retirable * p ) noexcept;

 /// the record, for the tests
 [[nodiscard]] const void * record( void ) const { return( f_r ); }

/*--------------------------------------------------------------------------*/
 private:
 EpochDomain * f_d;  ///< the domain
 Record * f_r;       ///< the record, nil once moved

 };  // end( class EpochDomain::Local )

/*--------------------------------------------------------------------------*/
/*-------------------- CLASS EpochDomain::Local::Guard ---------------------*/
/*--------------------------------------------------------------------------*/

/// the span of an OPERATION of a thread under the epochs: every node that
/// it reaches is protected until it ends
template< unsigned R >
class EpochDomain< R >::Local::Guard
{

/*--------------------------------------------------------------------------*/
 public:
 /// the epoch announced on the record of l, and its limbo collected
 ///
 /// In pseudocode (Fraser, beyond the book), with \f$ a \f$ the announce
 /// of the record:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{enter}() \cr
 /// 2 & \quad e \gets E;\ a \gets 2 e + 1\quad
 ///     \triangleright\ \text{a sequentially consistent exchange} \cr
 /// 3 & \quad \text{free the limbo } L_j \text{ whose label is}
 ///     \text{ at most } e - 2 \cr
 /// 4 & \textbf{procedure}\ \mathrm{leave}() \cr
 /// 5 & \quad \textbf{if}\ R \text{ retirements since the last}
 ///     \text{ attempt}\ \textbf{then} \cr
 /// 6 & \qquad \textbf{if}\ \text{no record has } a \text{ odd and}
 ///     \ a \ne 2 e + 1\ \textbf{then}\ \mathrm{CAS}( E , e , e + 1 ) \cr
 /// 7 & \quad a \gets 0
 /// \end{array}}
 /// \f]
 explicit Guard( Local & l ) noexcept;

 /// no copy, since a guard is one span of an operation
 Guard( const Guard & ) = delete;
 /// no assignment, since a guard is one span of an operation
 Guard & operator=( const Guard & ) = delete;

 /// out of the operation, after an attempt to raise the epoch
 ~Guard();

 /// the pointer of src, read with sequential consistency; the slot is
 /// ignored, since the guard protects every node it reaches
 template< class T >
 [[nodiscard]] T * protect( int ,
			    const std::atomic< T * > & src ) const noexcept {
  return( src.load( std::memory_order_seq_cst ) );
  }

 /// nothing, since the guard protects every node it reaches
 void reset( int ) noexcept { }

 /// the epoch announced
 [[nodiscard]] std::uint64_t epoch( void ) const { return( f_l.f_r->now ); }

/*--------------------------------------------------------------------------*/
 private:
 Local & f_l;  ///< the record

 };  // end( class EpochDomain::Local::Guard )

/*--------------------------------------------------------------------------*/
/*-------------------------- THE POLICIES ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the POLICY of the epochs of Fraser: an attempt to raise the epoch, a
/// read of all the records, at the end of an operation after \f$ R \f$
/// retirements since the last attempt
template< unsigned R = 1 >
struct FraserEpochs
{
 using Domain = EpochDomain< R >;  ///< the domain
 /// the record of a thread, whatever its slots
 template< int K >
 using Local = typename Domain::Local;
 /// a guard protects all that its operation reaches
 static constexpr bool kRegions = true;
 };

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/concurrent/impl/EpochReclaim.tpp"

/*--------------------------------------------------------------------------*/

#endif /* EpochReclaim.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File EpochReclaim.h ------------------------*/
/*--------------------------------------------------------------------------*/
