/*--------------------------------------------------------------------------*/
/*----------------------------- File module.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The extension module `ads._core`, on nanobind: the exceptions of ADS as
 * exceptions of Python (`PersistError` for a file that cannot be read,
 * `ContractViolation` for a broken precondition, both subclasses of
 * `ValueError`), and the classes and the functions of each area.
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

#include "Bindings.h"

#include "ADS/Contract.h"

/*--------------------------------------------------------------------------*/
/*------------------------------- THE MODULE -------------------------------*/
/*--------------------------------------------------------------------------*/

NB_MODULE( _core , m )
{
 namespace nb = nanobind;
 m.doc() = "The structures and the algorithms of ADS beyond the book that "
	   "NumPy, SciPy and NetworkX lack.";
 nb::exception< ADS::PersistError >( m , "PersistError" , PyExc_ValueError );
 nb::exception< ADS::contract_violation >( m , "ContractViolation" ,
					   PyExc_ValueError );
 ads_python::bindGraphs( m );
 ads_python::bindDynamic( m );
 ads_python::bindSuccinct( m );
 ads_python::bindFilters( m );
 ads_python::bindHeaps( m );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- End File module.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
