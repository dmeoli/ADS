/*--------------------------------------------------------------------------*/
/*--------------------------- File abi_units.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The test of the configurations of the layouts [see `Abi.h`], beyond the
 * book: one program linked from this file and from two units of
 * `abi_unit.cpp` built with different flags, the unit A with the broadword
 * matching of the hash tables and without the compare-and-swap of 16 bytes
 * (`-DADS_FLAT_SIMD=0 -DADS_FLAT_CAS16=0`), the unit B with the defaults of
 * the target and, where the compiler has them, `-mcx16` or
 * `-march=x86-64-v3`. The tables of each unit must hold what it put in
 * them, which they would not if the linker kept the inline code of one unit
 * for both; and the names of the types must differ exactly when the
 * configurations do, and carry the name of the configuration.
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

#include <iostream>
#include <string>

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

bool abiWorkA( void );
bool abiWorkB( void );
std::string abiNameA( std::string & type );
std::string abiNameB( std::string & type );

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( void )
{
 std::string typeA , typeB;
 const std::string a = abiNameA( typeA ) , b = abiNameB( typeB );
 std::cout << "unit A: " << a << "\nunit B: " << b << std::endl;
 const bool work = abiWorkA() && abiWorkB();
 const bool names = ( ( a != b ) == ( typeA != typeB ) ) &&
		    ( typeA.find( a ) != std::string::npos ) &&
		    ( typeB.find( b ) != std::string::npos );
 std::cout << ( work && names ? "PASSED" : "FAILED" ) << " abi-units"
	   << std::endl;
 return( work && names ? 0 : 1 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- End File abi_units.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
