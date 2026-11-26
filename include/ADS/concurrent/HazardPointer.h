/*--------------------------------------------------------------------------*/
/*-------------------------- File HazardPointer.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The hazard pointers of Michael, beyond the book: a reclamation of the
 * memory of the lock-free structures under which a thread stopped anywhere
 * keeps only the nodes that it protects from being freed, with the
 * interface of `std::hazard_pointer` of C++26 (`hazard_pointer_obj_base`,
 * `make_hazard_pointer()`, `protect()`, `try_protect()`,
 * `reset_protection()`, `retire()`) and, as a policy of reclamation [see
 * `Reclaim.h`], a domain owned by a structure.
 *
 * A domain keeps a list of <em>slots</em>, each a word that holds the node
 * protected by its thread or nil, and a list of <em>bins</em>, the lists of
 * the nodes retired by each thread; the slots and the bins are taken by a
 * thread, given back, and taken again by others, and are freed only with
 * the domain. A thread protects the node reached by an atomic \f$ s \f$ by
 * reading \f$ p \f$ from \f$ s \f$, writing \f$ p \f$ in a slot of its own
 * and reading \f$ s \f$ again, until the two reads agree (the write and the
 * second read sequentially consistent): then \f$ p \f$ was still in the
 * structure when the slot was written, and no scan that comes later frees
 * it until the slot changes. A node retired goes into the bin of its
 * thread; when the bin holds \f$ 2 H + 64 \f$ nodes, \f$ H \f$ the slots of
 * the domain, the thread reads all the slots into a sorted vector and frees
 * each node of its bin that no slot holds (the <em>scan</em>). At most
 * \f$ H \f$ nodes survive a scan, and therefore at least \f$ H + 64 \f$ are
 * freed, in \f$ O( H \log H ) \f$ time, i.e., \f$ O( \log H ) \f$ per node
 * retired, and a bin never holds more than \f$ 2 H + 63 \f$ nodes between
 * two operations of its thread, whatever the other threads do: a thread
 * stopped anywhere keeps at most its own slots and its own bin from being
 * freed (see \ref theory_concurrent_reclaim). The price, with respect to
 * the epochs [see `EpochReclaim.h`], is a sequentially consistent write per
 * node protected, and that only the nodes read by `protect()` are
 * protected: a structure whose operations walk on from a node that may
 * have been retired (e.g., the deleted prefix of `LockFreePriorityQueue`)
 * cannot take the policy.
 *
 * As in C++26, `make_hazard_pointer()` and `retire()` of an object of a
 * type derived from `hazard_pointer_obj_base` work on one domain for the
 * whole program, `HazardDomain::global()`, a static object, where each
 * thread retires into a bin of its own, kept in its thread-local storage
 * and given back, after a scan, when the thread ends; the deleter of an
 * object is moved out of it before it is called. A structure that takes
 * the policy `HazardPointers` owns a domain instead, whose records
 * (`Local< K >`, \f$ K \f$ slots and a bin) belong to its handles, and
 * whose destructor frees the nodes still retired. A bin given back keeps
 * its nodes for the next thread that takes it, after a scan.
 *
 * The contracts [see `Contract.h`]: a slot is used by one thread at a
 * time, `protect()` and `try_protect()` are called on a hazard pointer
 * that is not empty; an object is retired once, after it has been unlinked,
 * and its deleter does not retire; no slot and no bin is held when a
 * domain is destroyed, and the threads that used `global()` have ended
 * when the program exits; `pending()`, `freed()` and `slots()` may be read
 * at any time, as sums of counters that may be read in the middle of a
 * change. The operations are defined in `HazardPointer.tpp`.
 *
 * \see M. M. Michael, Hazard pointers: safe memory reclamation for
 *      lock-free objects, IEEE Transactions on Parallel and Distributed
 *      Systems 15(6), 491-504, 2004, doi:10.1109/TPDS.2004.8
 * \see M. M. Michael, M. L. Scott, Simple, fast, and practical
 *      non-blocking and blocking concurrent queue algorithms, ACM Symposium
 *      on Principles of Distributed Computing (PODC), 267-275, 1996,
 *      doi:10.1145/248052.248106
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

#ifndef __ADS_HazardPointer
 #define __ADS_HazardPointer
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Portable.h"
#include "ADS/concurrent/Reclaim.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

class hazard_pointer;

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS HazardDomain ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the DOMAIN of the hazard pointers: the slots of protection and the bins
/// of the nodes retired, of all the threads that use it
class HazardDomain
{

/*--------------------------------------------------------------------------*/
 public:
 /// the nodes that a bin holds beyond \f$ 2 H \f$ before a scan
 static constexpr std::size_t kBatch = 64;

 template< int K >
 class Local;

 /// the domain of no slots and no bins
 HazardDomain( void ) = default;
 /// no copy, since the slots and the bins point to it
 HazardDomain( const HazardDomain & ) = delete;
 /// no copy, since the slots and the bins point to it
 HazardDomain & operator=( const HazardDomain & ) = delete;

 /// the frees of the nodes still retired, of the bins and of the slots
 /// \pre no slot and no bin is held
 ~HazardDomain();

 /// the domain of `make_hazard_pointer()` and of `retire()`, for the
 /// whole program
 [[nodiscard]] static HazardDomain & global( void ) {
  static HazardDomain d;
  return( d );
  }

 /// the slots made so far, \f$ H \f$
 [[nodiscard]] std::size_t slots( void ) const {
  return( f_slots_n.load( std::memory_order_relaxed ) );
  }

 /// the nodes a bin holds that make it scan, \f$ 2 H + 64 \f$
 [[nodiscard]] std::size_t threshold( void ) const {
  return( 2 * slots() + kBatch );
  }

 /// the bins made so far
 [[nodiscard]] std::size_t bins( void ) const;

 /// the nodes retired and not yet freed
 [[nodiscard]] std::size_t pending( void ) const;

 /// the nodes freed so far, not counting those of the destructor
 [[nodiscard]] std::uint64_t freed( void ) const;

/*--------------------------------------------------------------------------*/
 private:
 friend class hazard_pointer;
 template< class T , class D >
 friend class hazard_pointer_obj_base;
 friend hazard_pointer make_hazard_pointer( void );

 /// a slot of protection, alone on its line
 struct alignas( 64 ) Slot
 {
  std::atomic< const void * > ptr{ nullptr };  ///< the node protected
  std::atomic< bool > used{ true };            ///< held by a thread
  Slot * next = nullptr;                       ///< the next slot
  };

 /// a bin of the nodes retired by a thread
 struct alignas( 64 ) Bin
 {
  std::atomic< bool > used{ true };         ///< held by a thread
  Bin * next = nullptr;                     ///< the next bin
  Retirable * head = nullptr;               ///< the nodes retired
  std::atomic< std::uint64_t > count{ 0 };  ///< their number
  std::atomic< std::uint64_t > freed{ 0 };  ///< the nodes freed, ever
  bool scanning = false;                    ///< inside a scan
  std::vector< const void * > seen;         ///< the slots read by a scan
  };

 class Home;

 /// a slot given back, else a new one pushed on the list
 Slot * acquireSlot( void );

 /// the slot s given back, cleared
 static void releaseSlot( Slot * s ) noexcept;

 /// a bin given back, else a new one pushed on the list
 Bin * acquireBin( void );

 /// the bin b given back after a scan
 void releaseBin( Bin * b ) noexcept;

 /// the node p put into the bin b, and the bin scanned if full
 void retire( Bin & b , Retirable * p ) noexcept;

 /// the nodes of b that no slot holds freed
 void scan( Bin & b ) noexcept;

 /// the bin of the running thread on `global()`
 static Bin & home( void );

 alignas( 64 ) std::atomic< Slot * > f_slots{ nullptr };  ///< the slots
 std::atomic< std::size_t > f_slots_n{ 0 };  ///< their number, \f$ H \f$
 alignas( 64 ) std::atomic< Bin * > f_bins{ nullptr };  ///< the bins

 };  // end( class HazardDomain )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS hazard_pointer --------------------------*/
/*--------------------------------------------------------------------------*/

/// a HAZARD POINTER of `HazardDomain::global()`, as `std::hazard_pointer`
/// of C++26: one slot of protection, or none if empty
class hazard_pointer
{

/*--------------------------------------------------------------------------*/
 public:
 /// the empty hazard pointer, with no slot
 hazard_pointer( void ) noexcept = default;

 /// the slot moved, the other left empty
 hazard_pointer( hazard_pointer && o ) noexcept
     : f_s( std::exchange( o.f_s , nullptr ) ) { }

 /// the slot of this given back, and the one of o moved
 hazard_pointer & operator=( hazard_pointer && o ) noexcept {
  if( this != &o ) {
   if( f_s )
    HazardDomain::releaseSlot( f_s );
   f_s = std::exchange( o.f_s , nullptr );
   }
  return( *this );
  }

 /// no copy, since a slot has one owner
 hazard_pointer( const hazard_pointer & ) = delete;
 /// no copy, since a slot has one owner
 hazard_pointer & operator=( const hazard_pointer & ) = delete;

 /// the slot given back, cleared
 ~hazard_pointer() {
  if( f_s )
   HazardDomain::releaseSlot( f_s );
  }

 /// whether the hazard pointer has no slot
 [[nodiscard]] bool empty( void ) const noexcept { return( ! f_s ); }

 /// the pointer of src, protected: written in the slot and read again from
 /// src until the two reads agree
 /// \pre not `empty()`
 ///
 /// In pseudocode (Michael, beyond the book), with \f$ h \f$ the slot:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{protect}( s ) \cr
 /// 2 & \quad p \gets s \cr
 /// 3 & \quad \textbf{loop}\ h \gets p;\ q \gets s\quad \triangleright
 ///     \ \text{both sequentially consistent} \cr
 /// 4 & \qquad \textbf{if}\ q = p\ \textbf{then return}\ p \cr
 /// 5 & \qquad p \gets q
 /// \end{array}}
 /// \f]
 template< class T >
 T * protect( const std::atomic< T * > & src ) noexcept {
  T * p = src.load( std::memory_order_relaxed );
  while( ! try_protect( p , src ) ) {
   }
  return( p );
  }

 /// whether ptr, still the pointer of src after it was written in the
 /// slot, is protected; otherwise the slot is cleared and ptr is the
 /// pointer read again
 /// \pre not `empty()`
 template< class T >
 bool try_protect( T *& ptr , const std::atomic< T * > & src ) noexcept {
  ADS_PRE( f_s );
  f_s->ptr.store( ptr , std::memory_order_seq_cst );
  T * q = src.load( std::memory_order_seq_cst );
  if( q == ptr )
   return( true );
  f_s->ptr.store( nullptr , std::memory_order_release );
  ptr = q;
  return( false );
  }

 /// ptr protected with no check, for a node that the caller knows is not
 /// yet retired (e.g., protected by another slot)
 /// \pre not `empty()`
 template< class T >
 void reset_protection( const T * ptr ) noexcept {
  ADS_PRE( f_s );
  f_s->ptr.store( ptr , std::memory_order_seq_cst );
  }

 /// no node protected
 /// \pre not `empty()`
 void reset_protection( std::nullptr_t = nullptr ) noexcept {
  ADS_PRE( f_s );
  f_s->ptr.store( nullptr , std::memory_order_release );
  }

 /// the slots of this and of o swapped
 void swap( hazard_pointer & o ) noexcept { std::swap( f_s , o.f_s ); }

/*--------------------------------------------------------------------------*/
 private:
 friend hazard_pointer make_hazard_pointer( void );

 /// the hazard pointer of the slot s
 explicit hazard_pointer( HazardDomain::Slot * s ) noexcept : f_s( s ) { }

 HazardDomain::Slot * f_s = nullptr;  ///< the slot, nil if empty

 };  // end( class hazard_pointer )

/// a hazard pointer of `HazardDomain::global()`, on a slot given back by
/// another or a new one
[[nodiscard]] inline hazard_pointer make_hazard_pointer( void )
{
 return( hazard_pointer( HazardDomain::global().acquireSlot() ) );
 }

/// the slots of a and b swapped
inline void swap( hazard_pointer & a , hazard_pointer & b ) noexcept
{
 a.swap( b );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- CLASS hazard_pointer_obj_base ----------------------*/
/*--------------------------------------------------------------------------*/

/// the base of a type \f$ T \f$ whose objects are retired to
/// `HazardDomain::global()`, as `std::hazard_pointer_obj_base` of C++26,
/// freed by the deleter \f$ D \f$
template< class T , class D = std::default_delete< T > >
class hazard_pointer_obj_base : private Retirable
{

 static_assert( std::is_nothrow_move_constructible_v< D > ,
		"the deleter is moved without exceptions" );

/*--------------------------------------------------------------------------*/
 public:
 /// the object handed to the reclamation, to be freed by d once no hazard
 /// pointer protects it
 void retire( D d = D() ) noexcept {
  f_d = std::move( d );
  Retirable::reclaim = &hazard_pointer_obj_base::reclaimer;
  HazardDomain::global().retire( HazardDomain::home() ,
				 static_cast< Retirable * >( this ) );
  }

/*--------------------------------------------------------------------------*/
 protected:
 /// the base of an object not retired
 hazard_pointer_obj_base( void ) = default;
 /// the base of a copy, not retired
 hazard_pointer_obj_base( const hazard_pointer_obj_base & ) noexcept
     : Retirable() , f_d() { }
 /// the base of a move, not retired
 hazard_pointer_obj_base( hazard_pointer_obj_base && ) noexcept
     : Retirable() , f_d() { }
 /// nothing assigned, the object being the same
 hazard_pointer_obj_base &
 operator=( const hazard_pointer_obj_base & ) noexcept {
  return( *this );
  }
 /// nothing assigned, the object being the same
 hazard_pointer_obj_base & operator=( hazard_pointer_obj_base && ) noexcept {
  return( *this );
  }
 /// the base of an object freed
 ~hazard_pointer_obj_base() = default;

/*--------------------------------------------------------------------------*/
 private:
 friend class HazardDomain;

 /// the address of the object, or its free by its deleter, moved out
 static void * reclaimer( Retirable * r , Retirable::Op op ) noexcept {
  auto * b = static_cast< hazard_pointer_obj_base * >( r );
  T * t = static_cast< T * >( b );
  if( op == Retirable::Op::address )
   return( static_cast< void * >( t ) );
  D d( std::move( b->f_d ) );
  d( t );
  return( nullptr );
  }

 ADS_NO_UNIQUE_ADDRESS D f_d{};  ///< the deleter

 };  // end( class hazard_pointer_obj_base )

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS HazardDomain::Local -----------------------*/
/*--------------------------------------------------------------------------*/

/// the RECORD of one thread on a `HazardDomain`, its \f$ K \f$ slots and its
/// bin, and its guards
template< int K >
class HazardDomain::Local
{

 static_assert( K >= 0 , "K slots, none or more" );

/*--------------------------------------------------------------------------*/
 public:
 class Guard;

 /// K slots and a bin of d, given back by other threads or new
 explicit Local( HazardDomain & d );

 /// the record moved, the other left without one
 Local( Local && o ) noexcept
     : f_d( o.f_d ) , f_s( o.f_s ) ,
       f_b( std::exchange( o.f_b , nullptr ) ) { }

 /// no copy, since a record has one owner
 Local( const Local & ) = delete;
 /// no assignment, since a record has one owner
 Local & operator=( const Local & ) = delete;
 /// no assignment, since a record has one owner
 Local & operator=( Local && ) = delete;

 /// the slots given back, and the bin after a scan
 ~Local();

 /// the node p handed to the reclamation, its bin scanned if full
 ///
 /// In pseudocode (Michael, beyond the book), with \f$ B \f$ the bin of the
 /// thread and \f$ H \f$ the slots of the domain:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{retire}( p ) \cr
 /// 2 & \quad B \gets B \cup \{ p \} \cr
 /// 3 & \quad \textbf{if}\ | B | \ge 2 H + 64\ \textbf{then} \cr
 /// 4 & \qquad P \gets \text{the nodes held by the slots, sorted}\quad
 ///     \triangleright\ \text{sequentially consistent reads} \cr
 /// 5 & \qquad \textbf{for each}\ q \in B\ \textbf{do}\ \textbf{if}\ q
 ///     \notin P\ \textbf{then}\ B \gets B \setminus \{ q \};\ \text{free}
 ///     \ q
 /// \end{array}}
 /// \f]
 void retire( Retirable * p ) noexcept { f_d->retire( *f_b , p ); }

/*--------------------------------------------------------------------------*/
 private:
 HazardDomain * f_d;             ///< the domain
 std::array< Slot * , K > f_s{};  ///< the slots
 Bin * f_b = nullptr;            ///< the bin, nil once moved

 };  // end( class HazardDomain::Local )

/*--------------------------------------------------------------------------*/
/*-------------------- CLASS HazardDomain::Local::Guard --------------------*/
/*--------------------------------------------------------------------------*/

/// the span of an OPERATION of a thread under the hazard pointers: only
/// the nodes read by `protect()` are protected, each until its slot
/// protects another or the guard ends
template< int K >
class HazardDomain::Local< K >::Guard
{

/*--------------------------------------------------------------------------*/
 public:
 /// the span of an operation on the slots of l
 explicit Guard( Local & l ) noexcept : f_l( l ) { }

 /// no copy, since a guard is one span of an operation
 Guard( const Guard & ) = delete;
 /// no assignment, since a guard is one span of an operation
 Guard & operator=( const Guard & ) = delete;

 /// the slots cleared
 ~Guard() {
  for( Slot * s : f_l.f_s )
   s->ptr.store( nullptr , std::memory_order_release );
  }

 /// the pointer of src, protected by the slot i [see
 /// `hazard_pointer::protect()`]
 /// \pre \f$ 0 \le i < K \f$
 template< class T >
 [[nodiscard]] T * protect( int i , const std::atomic< T * > & src ) noexcept {
  ADS_PRE( ( i >= 0 ) && ( i < K ) );
  std::atomic< const void * > & h = f_l.f_s[ std::size_t( i ) ]->ptr;
  T * p = src.load( std::memory_order_relaxed );
  for( ; ; ) {
   h.store( p , std::memory_order_seq_cst );
   T * q = src.load( std::memory_order_seq_cst );
   if( q == p )
    return( p );
   p = q;
   }
  }

 /// the slot i cleared
 /// \pre \f$ 0 \le i < K \f$
 void reset( int i ) noexcept {
  ADS_PRE( ( i >= 0 ) && ( i < K ) );
  f_l.f_s[ std::size_t( i ) ]->ptr.store( nullptr ,
					  std::memory_order_release );
  }

/*--------------------------------------------------------------------------*/
 private:
 Local & f_l;  ///< the record

 };  // end( class HazardDomain::Local::Guard )

/*--------------------------------------------------------------------------*/
/*---------------------------- THE POLICY ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the POLICY of the hazard pointers: a domain per structure, \f$ K \f$
/// slots per record
struct HazardPointers
{
 using Domain = HazardDomain;  ///< the domain
 /// the record of a thread, with K slots
 template< int K >
 using Local = HazardDomain::Local< K >;
 /// a guard protects only the nodes read by protect()
 static constexpr bool kRegions = false;
 };

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/concurrent/impl/HazardPointer.tpp"

/*--------------------------------------------------------------------------*/

#endif /* HazardPointer.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File HazardPointer.h ------------------------*/
/*--------------------------------------------------------------------------*/
