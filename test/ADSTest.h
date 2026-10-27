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

#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

/*--------------------------------------------------------------------------*/
/*------------------------------ FRAMEWORK ---------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS::test {

/// the failure of a check
struct Failure : std::runtime_error {
 using std::runtime_error::runtime_error;
 };

/// the tests of a program, by name
inline std::map< std::string , std::function< void( void ) > > & registry()
{
 static std::map< std::string , std::function< void( void ) > > r;
 return( r );
 }

/// registers a test
struct Register {
 Register( const char * name , std::function< void( void ) > f ) {
  registry()[ name ] = std::move( f );
  }
 };

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
#define CHECK( c )                                                         \
 do {                                                                      \
  if( ! ( c ) ) {                                                          \
   std::ostringstream os;                                                  \
   os << __FILE__ << ":" << __LINE__ << ": " << #c;                        \
   throw( ADS::test::Failure( os.str() ) );                                \
   }                                                                       \
  } while( 0 )

/// checks that an expression throws an exception of type E
#define CHECK_THROWS( expr , E )                                           \
 do {                                                                      \
  bool thrown = false;                                                     \
  try { ( void )( expr ); } catch( const E & ) { thrown = true; }          \
  if( ! thrown ) {                                                         \
   std::ostringstream os;                                                  \
   os << __FILE__ << ":" << __LINE__ << ": " << #expr                      \
      << " does not throw " << #E;                                         \
   throw( ADS::test::Failure( os.str() ) );                                \
   }                                                                       \
  } while( 0 )

#define ADS_CAT2( a , b ) a##b
#define ADS_CAT( a , b ) ADS_CAT2( a , b )

/// defines and registers the test of the given name
#define ADS_TEST( name )                                                   \
 static void ADS_CAT( ads_test_ , __LINE__ )( void );                      \
 static ADS::test::Register ADS_CAT( ads_reg_ , __LINE__ )(                \
                                 name , ADS_CAT( ads_test_ , __LINE__ ) ); \
 static void ADS_CAT( ads_test_ , __LINE__ )( void )

/*--------------------------------------------------------------------------*/

#endif  /* ADSTest.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File ADSTest.h ---------------------------*/
/*--------------------------------------------------------------------------*/
