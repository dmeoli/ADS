/*--------------------------------------------------------------------------*/
/*------------------------------- File Pmr.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The pools of ADS as memory resources of the standard library, beyond the
 * book: `PmrResource< R >` is a `std::pmr::memory_resource` on a resource
 * \f$ R \f$ of ADS with `allocate( b , a )` and `deallocate( p , b , a )`
 * (`PoolResource` of `PoolAllocator.h` by default), so that every container
 * of `std::pmr`, and those of Boost.Container on `std::pmr`, take the size
 * classes of ADS for their nodes. Each allocation is one virtual call, the
 * price of `std::pmr`, which the class known at compile time of
 * `PoolAllocator` avoids; the adapter is for the programs already written
 * on `std::pmr`. Two adapters are equal when they are on the same resource.
 *
 * \see P. Halpern, Polymorphic memory resources, ISO/IEC JTC1 SC22 WG21
 *      N3916, 2014,
 *      https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n3916.pdf
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

#ifndef __ADS_interop_std_Pmr
 #define __ADS_interop_std_Pmr
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/memory/PoolAllocator.h"

#include <cstddef>
#include <memory_resource>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS PmrResource ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the resource \f$ r \f$ of ADS as a `std::pmr::memory_resource`
template< class R = PoolResource<> >
class PmrResource : public std::pmr::memory_resource
{

 public:
 /// the adapter of \f$ r \f$, which must outlive it and the memory it gives
 explicit PmrResource( R & r ) : f_r( &r ) { }

 /// the resource of ADS
 [[nodiscard]] R & resource( void ) const { return( *f_r ); }

 private:
 /// \f$ b \f$ bytes aligned to \f$ a \f$, `allocate( b , a )` of the resource
 void * do_allocate( std::size_t b , std::size_t a ) override {
  return( f_r->allocate( b , a ) );
  }

 /// the memory \f$ p \f$ given back, `deallocate( p , b , a )`
 void do_deallocate( void * p , std::size_t b , std::size_t a ) override {
  f_r->deallocate( p , b , a );
  }

 /// whether \f$ o \f$ is an adapter of the same resource
 bool do_is_equal(
   const std::pmr::memory_resource & o ) const noexcept override {
  const auto * q = dynamic_cast< const PmrResource * >( &o );
  return( ( q != nullptr ) && ( q->f_r == f_r ) );
  }

 R * f_r;  ///< the resource

 };  // end( class PmrResource )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif /* Pmr.h included */

/*--------------------------------------------------------------------------*/
/*------------------------------ End File Pmr.h ----------------------------*/
/*--------------------------------------------------------------------------*/
