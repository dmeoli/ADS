/*--------------------------------------------------------------------------*/
/*----------------------------- File ADS.cppm ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Interface of the C++20 module ads, which exports the modules ads.core,
 * ads.base and ads.advanced, i.e., every name that the headers of include/ADS
 * declare in the namespace ADS and in those nested in it, but for the
 * namespaces detail; a translation unit can then write
 *
 *     import ads;
 *
 * in place of the #include of the headers, or import one of the three for a
 * part of the library [see cmake/ads_sets.txt]. The macros (e.g., ADS_PRE of
 * Contract.h) are not exported by a module, so a file that uses them still
 * includes the header that defines them, and those that configure the headers
 * (e.g., ADS_CONTRACTS, ADS_WITH_GMP) take the value they have where the
 * module is compiled.
 *
 * This file is written by cmake/ads_module_exports.py, which reads the names
 * from the AST of the headers and their sets from cmake/ads_sets.txt: it is
 * not to be edited by hand.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- MODULE PURVIEW -----------------------------*/
/*--------------------------------------------------------------------------*/

export module ads;

export import ads.core;
export import ads.base;
export import ads.advanced;

/*--------------------------------------------------------------------------*/
/*--------------------------- End File ADS.cppm ----------------------------*/
/*--------------------------------------------------------------------------*/
