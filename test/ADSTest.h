/*--------------------------------------------------------------------------*/
/*---------------------------- File ADSTest.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A minimal test framework: each test is a function registered with a name
 * by ADS_TEST, CHECK( c ) fails the test with the file and the line of c,
 * and the main() of a test program runs the test named on the command line,
 * or all of them, so that ctest can run each one as a test of its own.
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

#ifndef __ADS_ADSTest
 #define __ADS_ADSTest
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"  // ADS_CONTRACTS and contract_violation

#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

/*--------------------------------------------------------------------------*/
/*------------------------------ FRAMEWORK ---------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS::test
{

/// the failure of a check
struct Failure : std::runtime_error
{
 using std::runtime_error::runtime_error;
 };

/// the tests of a program, by name
inline std::map< std::string , std::function< void( void ) > > & registry()
{
 static std::map< std::string , std::function< void( void ) > > r;
 return( r );
 }

/// registers a test
struct Register
{
 Register( const char * name , std::function< void( void ) > f ) {
  registry()[ name ] = std::move( f );
  }
 };

/// the size of a case that is there for its scale alone: \f$ n \f$, or
/// small when ADS_TEST_SMALL is defined, as in the build of the coverage,
/// where a case whose audits take \f$ \Theta( n^2 ) \f$ runs for an hour
/// at -O0 with the counters of gcov; the same steps, fewer of them, never
/// a path left out
template< class T >
constexpr T scaled( [[maybe_unused]] T n , [[maybe_unused]] T small )
{
#ifdef ADS_TEST_SMALL
 return( small );
#else
 return( n );
#endif
 }

/// true at the level 3 of the contracts, where every contract is checked
/// (that of the tests by default), false below it, where the steps of a
/// test that rest on the contracts themselves (a violation, an audit, the
/// counts kept for the audits) are left out
inline constexpr bool kAllContracts = ADS_CONTRACTS >= 3;

/// true if CHECK_THROWS( expr , E ) runs `expr`: always at the level 3,
/// and below it only when `E` cannot be a `contract_violation`, since the
/// contract that `expr` violates may then be unchecked, and `expr`, run,
/// would break it; the tests are thus built and passed at every level, the
/// checks of the contracts themselves only at the level 3
template< class E >
inline constexpr bool kRunsThrow =
  kAllContracts || ! std::is_base_of_v< E , ::ADS::contract_violation >;

/// runs the test named argv[ 1 ], or all of them; -l lists them
inline int main( int argc , char ** argv )
{
 auto & r = registry();
 if( ( argc > 1 ) && ( std::string( argv[ 1 ] ) == "-l" ) ) {
  for( const auto & t : r )
   std::cout << t.first << std::endl;
  return( 0 );
  }

 int failed = 0;
 auto run = [ & ]( const std::string & name ,
		   const std::function< void( void ) > & f ) {
  try {
   f();
   std::cout << "PASSED " << name << std::endl;
   }
  catch( const std::exception & e ) {
   std::cout << "FAILED " << name << ": " << e.what() << std::endl;
   ++failed;
   }
  };

 if( argc > 1 ) {
  const auto it = r.find( argv[ 1 ] );
  if( it == r.end() ) {
   std::cout << "no test " << argv[ 1 ] << std::endl;
   return( 1 );
   }
  run( it->first , it->second );
  }
 else
  for( const auto & t : r )
   run( t.first , t.second );

 return( failed ? 1 : 0 );
 }

}  // end( namespace ADS::test )

/// checks a condition, failing the test with its file and line
#define CHECK( c )                                                            \
 do {                                                                         \
  if( ! ( c ) ) {                                                             \
   std::ostringstream os;                                                     \
   os << __FILE__ << ":" << __LINE__ << ": " << #c;                           \
   throw( ADS::test::Failure( os.str() ) );                                   \
  }                                                                           \
 } while( 0 )

/// checks that an expression throws an exception of type E, if it runs
/// [see kRunsThrow]; it is compiled at every level
#define CHECK_THROWS( expr, E )                                               \
 do {                                                                         \
  if constexpr( ADS::test::kRunsThrow< E > ) {                                \
   bool thrown = false;                                                       \
   try {                                                                      \
    ( void )( expr );                                                         \
   }                                                                          \
   catch( const E & ) {                                                       \
    thrown = true;                                                            \
   }                                                                          \
   if( ! thrown ) {                                                           \
    std::ostringstream os;                                                    \
    os << __FILE__ << ":" << __LINE__ << ": " << #expr << " does not throw "  \
       << #E;                                                                 \
    throw( ADS::test::Failure( os.str() ) );                                  \
   }                                                                          \
  }                                                                           \
 } while( 0 )

#define ADS_CAT2( a, b ) a##b
#define ADS_CAT( a, b ) ADS_CAT2( a, b )

/// defines and registers the test of the given name
#define ADS_TEST( name )                                                      \
 static void ADS_CAT( ads_test_, __LINE__ )( void );                          \
 static ADS::test::Register ADS_CAT( ads_reg_, __LINE__ )(                    \
   name, ADS_CAT( ads_test_, __LINE__ ) );                                    \
 static void ADS_CAT( ads_test_, __LINE__ )( void )

/*--------------------------------------------------------------------------*/

#endif /* ADSTest.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File ADSTest.h ---------------------------*/
/*--------------------------------------------------------------------------*/
