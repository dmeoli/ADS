/*--------------------------------------------------------------------------*/
/*------------------------------- File ADS.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * All the headers of ADS, i.e., those of the two sets and of their core [see
 * `core.h`, `base.h` and `advanced.h`], as the module ads exports all their
 * names, but for those of the interoperation with other libraries
 * (`ADS/interop/`), which a program includes one by one next to the headers
 * of the other library.
 *
 * This file is written by `cmake/ads_sets.py` from `cmake/ads_sets.txt`: it
 * is not to be edited by hand.
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

#ifndef __ADS_ADS
 #define __ADS_ADS
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/advanced.h"
#include "ADS/base.h"
#include "ADS/core.h"

/*--------------------------------------------------------------------------*/

#endif /* ADS.h included */

/*--------------------------------------------------------------------------*/
/*----------------------------- End File ADS.h -----------------------------*/
/*--------------------------------------------------------------------------*/
