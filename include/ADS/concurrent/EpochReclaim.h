/*--------------------------------------------------------------------------*/
/*--------------------------- File EpochReclaim.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The reclamation of the memory of the lock-free structures by epochs,
 * beyond the book: the scheme of Fraser, and the one of DEBRA of Brown,
 * which amortizes the advance of the epoch over the operations, as two
 * policies [see `Reclaim.h`] of one domain. The domain keeps an epoch
 * \f$ E \f$, from 2, and a list of records, one per thread that uses it,
 * each announcing \f$ 2 e + 1 \f$ while its thread runs an operation (a
 * guard) begun in the epoch \f$ e \f$, and 0 outside; a guard begins by
 * reading \f$ E \f$ and announcing it by a sequentially consistent
 * exchange, before its thread reads the structure, whose words it then
 * reads with sequential consistency too (`protect()`, on x86-64 the same
 * plain load as an acquire, on AArch64 the same `ldar`). A node retired
 * waits in one of three lists of its record, the <em>limbo</em> of its
 * label modulo 3, and is freed by the thread of the record once \f$ E \f$
 * has grown enough past its label:
 * - with `FraserEpochs< R >`, the label is \f$ E \f$, read when the node
 *   is retired, and the node is freed by the first guard of its record
 *   that reads \f$ E \ge \ell + 2 \f$ for the label \f$ \ell \f$; a guard
 *   that ends after \f$ R \f$ retirements of its record since the last
 *   attempt reads all the records, and raises \f$ E \f$ by a
 *   compare-and-swap if none of them is inside an operation of an older
 *   epoch, i.e., in \f$ O( N ) \f$ time for \f$ N \f$ records (Fraser; the
 *   reclamation of `LockFreePriorityQueue`, of which \f$ R = 1 \f$ is the
 *   default);
 * - with `DebraEpochs`, a guard reads \f$ E \f$ again after its announce,
 *   and announces again until the two reads agree, so that a thread inside
 *   an operation that announces \f$ e \f$ sees \f$ E \in \{ e , e + 1 \}
 *   \f$ throughout; the label of a node is the epoch that its guard
 *   announced, and the node is freed by the first guard that announces an
 *   epoch \f$ \ge \ell + 3 \f$. Each guard checks one more record of the
 *   list, kept by a cursor of its own (Brown): a record outside or in the
 *   current epoch lets the cursor go on, and a guard whose cursor has
 *   passed all the records raises \f$ E \f$, so that an operation costs
 *   \f$ O( 1 ) \f$ time for the reclamation, and \f$ E \f$ grows after
 *   about \f$ N \f$ operations of each thread.
 *
 * Under both, no node is freed while a guard that may have reached it is
 * open (see \ref theory_concurrent_reclaim), and the reclamation is
 * blocking: a thread stopped inside an operation keeps \f$ E \f$ from
 * growing past its epoch plus 1, and the nodes retired since from being
 * freed, while the operations of the others go on; the nodes waiting are
 * then unbounded, which the hazard pointers [see `HazardPointer.h`] avoid.
 * A record given back by its thread keeps its limbo for the next thread
 * that takes it; the domain frees in its destructor the nodes still
 * waiting, so that none is leaked. The neutralization of the threads by
 * signals of DEBRA+ is left out, since signals are not portable.
 *
 * The contracts [see `Contract.h`]: no record is alive when the domain is
 * destroyed; the guards of a record do not overlap, and `retire()` is
 * called inside a guard of the same record; `pending()` and `freed()` may
 * be read at any time, as sums of counters that may be read in the middle
 * of a change. The operations are defined in `EpochReclaim.tpp`.
 *
 * \see K. Fraser, Practical lock-freedom, PhD thesis, technical report
 *      UCAM-CL-TR-579, University of Cambridge, 2004, doi:10.48456/tr-579
 * \see T. A. Brown, Reclaiming memory for lock-free data structures: there
 *      has to be a better way, ACM Symposium on Principles of Distributed
 *      Computing (PODC), 261-270, 2015, doi:10.1145/2767386.2767436
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
/// threads, with the scheme of Fraser (`Debra` false, an attempt to raise
/// the epoch per \f$ R \f$ retirements) or of DEBRA (`Debra` true)
template< bool Debra = false , unsigned R = 1 >
class EpochDomain
{

 static_assert( R >= 1 , "an attempt per R >= 1 retirements" );

/*--------------------------------------------------------------------------*/
 public:
 /// the epochs that the label of a node must be behind \f$ E \f$ for its
 /// free: 2 with the labels of Fraser, 3 with those of DEBRA
 static constexpr std::uint64_t kLag = Debra ? 3 : 2;

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
 /// labels, its counters, and the state of its attempts to raise the epoch;
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
  /// Fraser: the retirements since the last attempt
  unsigned since = 0;
  /// DEBRA: the epoch of the pass of the cursor, and the cursor
  std::uint64_t pass = 0;
  /// DEBRA: the next record to check, nil when all are
  Record * cursor = nullptr;
  };

 /// a record given back, else a new one pushed on the list
 Record * acquire( void );

 /// the nodes of the limbo whose label is kLag behind e freed
 static void collect( Record & r , std::uint64_t e ) noexcept;

 /// Fraser: E raised if no record inside an operation announces an older
 /// epoch than E
 void advance( void ) noexcept;

 /// DEBRA: one record checked by the cursor of r in the epoch e, and E
 /// raised once all are
 void step( Record & r , std::uint64_t e ) noexcept;

 alignas( 64 ) std::atomic< std::uint64_t > f_epoch{ 2 };  ///< the epoch
 alignas( 64 ) std::atomic< Record * > f_records{ nullptr };
 ///< the records

 };  // end( class EpochDomain )

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS EpochDomain::Local ------------------------*/
/*--------------------------------------------------------------------------*/

/// the RECORD of one thread on an `EpochDomain`, and its guards
template< bool Debra , unsigned R >
class EpochDomain< Debra , R >::Local
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
 /// In pseudocode (Fraser and Brown, beyond the book), with \f$ \lambda \f$
 /// = 2 for Fraser and 3 for DEBRA, and \f$ \ell_j \f$, \f$ L_j \f$ the
 /// label and the limbo \f$ j \f$ of the record:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{retire}( p ) \cr
 /// 2 & \quad \ell \gets E \text{ (Fraser), or the epoch announced by the}
 ///     \text{ guard (DEBRA)};\ j \gets \ell \bmod 3 \cr
 /// 3 & \quad \textbf{if}\ \ell_j \ne \ell\ \textbf{then}\ \text{free}
 ///     \ L_j;\ L_j \gets \emptyset;\ \ell_j \gets \ell\quad
 ///     \triangleright\ \ell_j \le \ell - \lambda \cr
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
template< bool Debra , unsigned R >
class EpochDomain< Debra , R >::Local::Guard
{

/*--------------------------------------------------------------------------*/
 public:
 /// the epoch announced on the record of l, and its limbo collected
 ///
 /// In pseudocode (Fraser and Brown, beyond the book), with \f$ a \f$ the
 /// announce of the record, \f$ c \f$ its cursor and \f$ \pi \f$ the epoch
 /// of the pass of the cursor:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{enter}() \cr
 /// 2 & \quad \textbf{repeat}\ e \gets E;\ a \gets 2 e + 1\quad
 ///     \triangleright\ \text{a sequentially consistent exchange} \cr
 /// 3 & \quad \textbf{until}\ \text{Fraser, or}\ E = e \cr
 /// 4 & \quad \text{free the limbo } L_j \text{ whose label is}
 ///     \text{ at most } e - \lambda \cr
 /// 5 & \quad \textbf{if}\ \text{DEBRA}\ \textbf{then} \cr
 /// 6 & \qquad \textbf{if}\ \pi \ne e\ \textbf{then}\ \pi \gets e;\ c \gets
 ///     \text{the first record} \cr
 /// 7 & \qquad \textbf{if}\ c \ne \textbf{nil}\ \textbf{and}\ c.a \in \{ 0
 ///     , 2 e + 1 \}\ \textbf{then}\ c \gets c.\mathit{next} \cr
 /// 8 & \qquad \textbf{if}\ c = \textbf{nil}\ \textbf{then}\ \mathrm{CAS}( E
 ///     , e , e + 1 );\ \pi \gets 0 \cr
 /// 9 & \textbf{procedure}\ \mathrm{leave}() \cr
 /// 10 & \quad \textbf{if}\ \text{Fraser and } R \text{ retirements since}
 ///     \text{ the last attempt}\ \textbf{then} \cr
 /// 11 & \qquad \textbf{if}\ \text{no record has } a \text{ odd and}
 ///     \ a \ne 2 e + 1\ \textbf{then}\ \mathrm{CAS}( E , e , e + 1 ) \cr
 /// 12 & \quad a \gets 0
 /// \end{array}}
 /// \f]
 explicit Guard( Local & l ) noexcept;

 /// no copy, since a guard is one span of an operation
 Guard( const Guard & ) = delete;
 /// no assignment, since a guard is one span of an operation
 Guard & operator=( const Guard & ) = delete;

 /// out of the operation, after an attempt to raise the epoch (Fraser)
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
 using Domain = EpochDomain< false , R >;  ///< the domain
 /// the record of a thread, whatever its slots
 template< int K >
 using Local = typename Domain::Local;
 /// a guard protects all that its operation reaches
 static constexpr bool kRegions = true;
 };

/// the POLICY of the epochs of DEBRA: one record checked per operation
struct DebraEpochs
{
 using Domain = EpochDomain< true , 1 >;  ///< the domain
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
