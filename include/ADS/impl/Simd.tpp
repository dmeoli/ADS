/*--------------------------------------------------------------------------*/
/*------------------------------ File Simd.tpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The kernels of Simd.h, one file per area, which Highway includes once
 * per set of vector instructions [see hwy/foreach_target.h]: each opens
 * the namespace `ADS::detail::simd::HWY_NAMESPACE` of the set it is
 * compiled for, and is guarded by `HWY_TARGET_TOGGLE` instead of a guard
 * of its own, so that it is read again for every set. Only Simd.h
 * includes this file.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/impl/simd/Bitwise.tpp"
#include "ADS/impl/simd/Compression.tpp"
#include "ADS/impl/simd/Hash.tpp"
#include "ADS/impl/simd/Sets.tpp"

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Simd.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
