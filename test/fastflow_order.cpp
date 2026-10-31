/*--------------------------------------------------------------------------*/
/*------------------------- File fastflow_order.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The order of the includes of FastFlowExecutor.h [see FastFlowExecutor.h],
 * a test of the compiler, not in tests.txt: a program that includes a
 * header of FastFlow before FastFlowExecutor.h must not compile, and the
 * error must be the one of the header, which test/CMakeLists.txt and the
 * makefile look for in the output of the compiler. Without the sources of
 * FastFlow, the guard of its ff/config.hpp is defined by hand, as its
 * inclusion would define it. With ADS_FASTFLOW_DEFINED_FIRST the program
 * defines the three macros itself before FastFlow, and compiles (on the
 * sources of FastFlow only, since the header includes them).
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#if ! defined( ADS_WITH_FASTFLOW )
 #define ADS_WITH_FASTFLOW
#endif

#if defined( ADS_FASTFLOW_DEFINED_FIRST )
 #define NUM_CORES 4
 #define NUM_REAL_CORES 4
 #define NO_DEFAULT_MAPPING
#endif

#if __has_include( <ff/config.hpp> )
 #include <ff/config.hpp>
#else
 #define FF_CONFIG_HPP
#endif

#include "ADS/parallel/FastFlowExecutor.h"

/*--------------------------------------------------------------------------*/
/*--------------------------------- MAIN -----------------------------------*/
/*--------------------------------------------------------------------------*/

int main( void )
{
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- End File fastflow_order.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
