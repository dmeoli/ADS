/*--------------------------------------------------------------------------*/
/*---------------------------- File Contract.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The contracts of ADS: the preconditions, the postconditions and the
 * invariants of the operations, checked at run time by three macros that
 * mirror the contracts of C++26 (pre, post and contract_assert), so that
 * they can be replaced by them when the compilers support them:
 *
 * - ADS_PRE( c ), at the start of an operation, what the caller must
 *   ensure (e.g., ! empty() before deleteMin());
 * - ADS_POST( c ), at the end, what the operation ensures, when it costs
 *   no more than the operation itself;
 * - ADS_INVARIANT( c ), at the end of the operations that change a
 *   structure, its invariant (e.g., the order of a heap), which usually
 *   costs \f$ O( n ) \f$ and is meant for the tests;
 * - ADS_AUDIT( c ), a precondition or a postcondition that costs more than
 *   the operation (e.g., a sorted vector before a binary search), checked
 *   with the invariants.
 *
 * The level ADS_CONTRACTS chooses which are checked: 0 none, 1 the
 * preconditions (the default, 0 when NDEBUG is defined, as for assert), 2
 * also the postconditions, 3 also the invariants and the audits. A violated
 * contract throws ADS::contract_violation, a std::logic_error whose message
 * names the kind of contract, the condition, the function and the place in
 * the source. The
 * conditions are compiled at every level, so that they stay correct C++, but
 * they are not evaluated above the level chosen; hence they must have no side
 * effects.
 *
 * \see J. Berne, T. Doumler, A. Krzemienski et al., Contracts for C++,
 *      P2900R14, ISO/IEC JTC1/SC22/WG21, 2025
 * \see B. Meyer, Applying "design by contract", Computer 25(10), 40-51,
 *      1992, doi:10.1109/2.161279
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

#ifndef __ADS_Contract
 #define __ADS_Contract
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <concepts>
#include <optional>
#include <stdexcept>
#include <string>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the level of the contracts checked: 0 none, 1 preconditions,
/// 2 postconditions, 3 invariants and audits; 1 by default, 0 with NDEBUG
#ifndef ADS_CONTRACTS
 #ifdef NDEBUG
  #define ADS_CONTRACTS 0
 #else
  #define ADS_CONTRACTS 1
 #endif
#endif

/// a contract violated: a bug of the caller (a precondition) or of ADS
class contract_violation : public std::logic_error {
 public:
  using std::logic_error::logic_error;
 };

namespace detail {

[[noreturn]] inline void violated( const char * kind , const char * cond ,
                                   const char * func , const char * file ,
                                   int line )
{
 throw( contract_violation( std::string( "ADS: " ) + kind + " ( " + cond +
                            " ) violated in " + func + ", " + file + ":" +
                            std::to_string( line ) ) );
 }

}  // end( namespace detail )

#define ADS_CONTRACT_CHECK( level , kind , cond )                         \
 do {                                                                     \
  if constexpr( ADS_CONTRACTS >= ( level ) )                              \
   if( !( cond ) )                                                        \
    ::ADS::detail::violated( kind , #cond , __func__ , __FILE__ ,         \
                             __LINE__ );                                  \
  } while( false )

/// a precondition, checked from level 1
#define ADS_PRE( cond ) ADS_CONTRACT_CHECK( 1 , "precondition" , cond )

/// a postcondition, checked from level 2
#define ADS_POST( cond ) ADS_CONTRACT_CHECK( 2 , "postcondition" , cond )

/// an invariant, checked from level 3
#define ADS_INVARIANT( cond ) ADS_CONTRACT_CHECK( 3 , "invariant" , cond )

/// a precondition or a postcondition dearer than the operation, checked
/// from level 3
#define ADS_AUDIT( cond ) ADS_CONTRACT_CHECK( 3 , "audit" , cond )

/*--------------------------------------------------------------------------*/
/*----------------------- HELPERS OF THE CONDITIONS ------------------------*/
/*--------------------------------------------------------------------------*/

namespace detail {

/// a == b if T has an equality, true otherwise: a postcondition that
/// compares items does not restrict the types of the items; two items
/// unequal to themselves (e.g., two NaN) are equal here, so that a NaN can
/// be stored
template< class T >
bool equal( const T & a , const T & b )
{
 if constexpr( std::equality_comparable< T > )
  return( ( a == b ) || ( !( a == a ) && !( b == b ) ) );
 else
  return( true );
 }

/// a copy of x for the audits that compare a structure with itself before
/// the operation (e.g., the frame of a write), made only at the level 3
template< class X >
std::optional< X > snapshot( const X & x )
{
 if constexpr( ADS_CONTRACTS >= 3 )
  return( x );
 else
  return( std::nullopt );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* Contract.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Contract.h ---------------------------*/
/*--------------------------------------------------------------------------*/
