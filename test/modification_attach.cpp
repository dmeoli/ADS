/*--------------------------------------------------------------------------*/
/*----------------------- File modification_attach.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The wrong attachments of an observer [see Modification.h], a test of the
 * compiler, not in tests.txt: each case, chosen by ADS_ATTACH_CASE, must
 * not compile, and the error must be the one of the subject, which
 * test/CMakeLists.txt and the makefile look for in the output of the
 * compiler. Case 1 attaches an observer with an on() for none of the
 * events of the structure, case 2 one whose on() of an erasure may throw,
 * case 3 one that requires an event that the structure never emits, case 4
 * two static observers out of the order of their dependencies.
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

#include "ADS/Modification.h"

using namespace ADS;

namespace {

using Ins = mod::Inserted< int , int >;
using Era = mod::Erased< int , int >;
using Events = mod::list< Ins , Era >;

/// an observer of nothing that the structure emits
struct Nothing {
 void on( const mod::Cleared & ) noexcept {}
 };

/// an observer whose on() of an erasure may throw
struct Throwing {
 void on( const Era & ) {}
 };

/// an observer that requires an event that the structure never emits
struct Requiring {
 using requires_modifications = mod::list< Ins , mod::Rebuilt >;
 void on( const Ins & ) {}
 };

/// an observer of the insertions
struct First {
 void on( const Ins & ) {}
 };

/// one that depends on First
struct Second {
 using depends_on = mod::list< First >;
 void on( const Ins & ) {}
 };

}  // end( namespace )

int main( void )
{
#if ADS_ATTACH_CASE == 1
 Nothing o;
 Dynamic<>::bind< Events > s;
 s.attach( o );
#elif ADS_ATTACH_CASE == 2
 Throwing o;
 Static< Throwing >::bind< Events > s( o );
#elif ADS_ATTACH_CASE == 3
 Requiring o;
 Dynamic<>::bind< Events > s;
 s.attach( o );
#else
 First f;
 Second g;
 Static< Second , First >::bind< Events > s( g , f );
#endif
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- End File modification_attach.cpp -------------------*/
/*--------------------------------------------------------------------------*/
