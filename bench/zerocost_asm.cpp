/*--------------------------------------------------------------------------*/
/*-------------------------- File zerocost_asm.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The translation unit that zerocost.py compiles at -O2 and disassembles:
 * one function of C linkage per change and variant [see zerocost.h], whose
 * code the script compares with that of its reference, byte by byte with
 * the relocations, which the object file leaves unresolved. A function
 * zc_<variant>_<workload> takes the plain structure and builds the wrapper
 * in it, and is compared with zc_plain_<workload>; a function
 * zc_<variant>_<workload>_ref takes the wrapper by reference, and is
 * compared with zc_none_<workload>_ref, i.e., with the wrapper that nobody
 * listens to, since the wrapper holds a reference to the graph that the
 * plain graph does not; the array of keys holds its vector, so that its
 * functions by reference are compared with zc_plain_key_ref.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */

#include "zerocost.h"

using namespace zc;

extern "C" {

/*--------------------------------------------------------------------------*/
/*------------------------ ArrayGraph::setWeight() -------------------------*/
/*--------------------------------------------------------------------------*/

bool zc_plain_array_weight( AG & g , int u , int v , long long w )
{
 return( g.setWeight( u , v , w ) );
 }

bool zc_observable_array_weight_ref( ADS::Observable< AG > & og , int u ,
                                     int v , long long w )
{
 return( og.setWeight( u , v , w ) );
 }

#ifdef ZC_POLICIES

bool zc_none_array_weight( AG & g , int u , int v , long long w )
{
 ADS::ObservedGraph< AG > og( g );
 return( og.setWeight( u , v , w ) );
 }

bool zc_none_array_weight_ref( ADS::ObservedGraph< AG > & og , int u ,
                               int v , long long w )
{
 return( og.setWeight( u , v , w ) );
 }

bool zc_static_excl_array_weight_ref(
      ADS::ObservedGraph< AG , ADS::Static< Excluding > > & og , int u ,
      int v , long long w )
{
 return( og.setWeight( u , v , w ) );
 }

bool zc_static_count_array_weight_ref(
      ADS::ObservedGraph< AG , ADS::Static< CountDecreases > > & og , int u ,
      int v , long long w )
{
 return( og.setWeight( u , v , w ) );
 }

bool zc_dynamic_array_weight_ref(
      ADS::ObservedGraph< AG , ADS::Dynamic<> > & og , int u , int v ,
      long long w )
{
 return( og.setWeight( u , v , w ) );
 }

#endif

/*--------------------------------------------------------------------------*/
/*------------------------- ListGraph::insertEdge() ------------------------*/
/*--------------------------------------------------------------------------*/

void zc_plain_list_edge( LG & g , int u , int v , long long w )
{
 g.insertEdge( u , v , w );
 }

void zc_observable_list_edge_ref( ADS::Observable< LG > & og , int u , int v ,
                                  long long w )
{
 og.insertEdge( u , v , w );
 }

#ifdef ZC_POLICIES

void zc_none_list_edge( LG & g , int u , int v , long long w )
{
 ADS::ObservedGraph< LG > og( g );
 og.insertEdge( u , v , w );
 }

void zc_none_list_edge_ref( ADS::ObservedGraph< LG > & og , int u , int v ,
                            long long w )
{
 og.insertEdge( u , v , w );
 }

void zc_static_excl_list_edge_ref(
      ADS::ObservedGraph< LG , ADS::Static< Excluding > > & og , int u ,
      int v , long long w )
{
 og.insertEdge( u , v , w );
 }

void zc_static_count_list_edge_ref(
      ADS::ObservedGraph< LG , ADS::Static< CountDecreases > > & og , int u ,
      int v , long long w )
{
 og.insertEdge( u , v , w );
 }

void zc_dynamic_list_edge_ref( ADS::ObservedGraph< LG , ADS::Dynamic<> > & og ,
                               int u , int v , long long w )
{
 og.insertEdge( u , v , w );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- THE ARRAY OF KEYS ----------------------------*/
/*--------------------------------------------------------------------------*/

void zc_plain_key_ref( std::vector< int > & a , std::size_t h , int k )
{
 a[ h ] = k;
 }

void zc_none_key_ref( KeyArray< ADS::NoObservers > & a , std::size_t h ,
                      int k )
{
 a.set( h , k );
 }

void zc_static_excl_key_ref( KeyArray< ADS::Static< KeyExcluding > > & a ,
                             std::size_t h , int k )
{
 a.set( h , k );
 }

void zc_static_count_key_ref(
      KeyArray< ADS::Static< KeyCountDecreases > > & a , std::size_t h ,
      int k )
{
 a.set( h , k );
 }

void zc_dynamic_key_ref( KeyArray< ADS::Dynamic<> > & a , std::size_t h ,
                         int k )
{
 a.set( h , k );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- THE GRID --------------------------------*/
/*--------------------------------------------------------------------------*/

void zc_none_grid_cost_ref( Grid< ADS::NoObservers > & g , int u , int c )
{
 g.setCost( u , c );
 }

void zc_static_count_grid_cost_ref(
      Grid< ADS::Static< GridCountDecreases > > & g , int u , int c )
{
 g.setCost( u , c );
 }

void zc_dynamic_grid_cost_ref( Grid< ADS::Dynamic<> > & g , int u , int c )
{
 g.setCost( u , c );
 }

#endif

}  // end( extern "C" )

/*--------------------------------------------------------------------------*/
/*------------------------ End File zerocost_asm.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
