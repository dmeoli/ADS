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
 * functions by reference are compared with zc_plain_key_ref, those of
 * ObservableArray with it too. The functions zc_heap_<op> take the heap
 * with `NoObservers`, and zc_<variant>_heap_<op> are compared with them.
 * The functions zc_<variant>_tree_<operation> call one operation of
 * JoinTree, and zerocost.py compares each, and every function of the tree
 * that the object file holds, with that of the tree with no observer, the
 * names of the policies left out of those of the relocations. With
 * `ZC_BEFORE` (the headers of an older revision) only the functions of the
 * heap and of the tree with no observer are compiled, and zerocost.py
 * compares every function of the file with that of the present revision.
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

bool zc_plain_array_weight( AG & g , int u , int v , long long w ) {
 return( g.setWeight( u , v , w ) );
 }

bool zc_observable_array_weight_ref( ADS::Observable< AG > & og , int u ,
				     int v ,
				     long long w ) {
 return( og.setWeight( u , v , w ) );
 }

#ifdef ZC_POLICIES

bool zc_none_array_weight( AG & g , int u , int v , long long w ) {
 ADS::ObservedGraph< AG > og( g );
 return( og.setWeight( u , v , w ) );
 }

bool zc_none_array_weight_ref( ADS::ObservedGraph< AG > & og , int u , int v ,
			       long long w ) {
 return( og.setWeight( u , v , w ) );
 }

bool zc_static_excl_array_weight_ref(
  ADS::ObservedGraph< AG , ADS::Static< Excluding > > & og , int u , int v ,
  long long w ) {
 return( og.setWeight( u , v , w ) );
 }

bool zc_static_count_array_weight_ref(
  ADS::ObservedGraph< AG , ADS::Static< CountDecreases > > & og , int u ,
  int v ,
  long long w ) {
 return( og.setWeight( u , v , w ) );
 }

bool zc_dynamic_array_weight_ref(
  ADS::ObservedGraph< AG , ADS::Dynamic<> > & og , int u , int v ,
  long long w ) {
 return( og.setWeight( u , v , w ) );
 }

#endif

/*--------------------------------------------------------------------------*/
/*------------------------- ListGraph::insertEdge() ------------------------*/
/*--------------------------------------------------------------------------*/

void zc_plain_list_edge( LG & g , int u , int v , long long w ) {
 g.insertEdge( u , v , w );
 }

void zc_observable_list_edge_ref( ADS::Observable< LG > & og , int u , int v ,
				  long long w ) {
 og.insertEdge( u , v , w );
 }

#ifdef ZC_POLICIES

void zc_none_list_edge( LG & g , int u , int v , long long w ) {
 ADS::ObservedGraph< LG > og( g );
 og.insertEdge( u , v , w );
 }

void zc_none_list_edge_ref( ADS::ObservedGraph< LG > & og , int u , int v ,
			    long long w ) {
 og.insertEdge( u , v , w );
 }

void zc_static_excl_list_edge_ref(
  ADS::ObservedGraph< LG , ADS::Static< Excluding > > & og , int u , int v ,
  long long w ) {
 og.insertEdge( u , v , w );
 }

void zc_static_count_list_edge_ref(
  ADS::ObservedGraph< LG , ADS::Static< CountDecreases > > & og , int u ,
  int v ,
  long long w ) {
 og.insertEdge( u , v , w );
 }

void zc_dynamic_list_edge_ref( ADS::ObservedGraph< LG , ADS::Dynamic<> > & og ,
			       int u , int v , long long w ) {
 og.insertEdge( u , v , w );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- THE ARRAY OF KEYS ----------------------------*/
/*--------------------------------------------------------------------------*/

void zc_plain_key_ref( std::vector< int > & a , std::size_t h , int k ) {
 a[ h ] = k;
 }

void zc_none_key_ref( KeyArray< ADS::NoObservers > & a , std::size_t h ,
		      int k ) {
 a.set( h , k );
 }

void zc_static_excl_key_ref( KeyArray< ADS::Static< KeyExcluding > > & a ,
			     std::size_t h , int k ) {
 a.set( h , k );
 }

void zc_static_count_key_ref( KeyArray< ADS::Static< KeyCountDecreases > > & a ,
			      std::size_t h , int k ) {
 a.set( h , k );
 }

void zc_dynamic_key_ref( KeyArray< ADS::Dynamic<> > & a , std::size_t h ,
			 int k ) {
 a.set( h , k );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- THE GRID --------------------------------*/
/*--------------------------------------------------------------------------*/

void zc_none_grid_cost_ref( Grid< ADS::NoObservers > & g , int u , int c ) {
 g.setCost( u , c );
 }

void zc_static_count_grid_cost_ref(
  Grid< ADS::Static< GridCountDecreases > > & g , int u , int c ) {
 g.setCost( u , c );
 }

void zc_dynamic_grid_cost_ref( Grid< ADS::Dynamic<> > & g , int u , int c ) {
 g.setCost( u , c );
 }

#endif

/*--------------------------------------------------------------------------*/
/*---------------------------- IndexedDaryHeap -----------------------------*/
/*--------------------------------------------------------------------------*/

void zc_heap_push( Heap & h , int id , int k ) {
 h.push( id , k );
 }

void zc_heap_decrease( Heap & h , int id , int k ) {
 h.decrease( id , k );
 }

void zc_heap_increase( Heap & h , int id , int k ) {
 h.increase( id , k );
 }

void zc_heap_erase( Heap & h , int id ) {
 h.erase( id );
 }

int zc_heap_pop( Heap & h ) {
 return( h.pop().id );
 }

#ifdef ZC_EMITTERS

void zc_static_excl_heap_decrease(
  ADS::IndexedDaryHeap< int , 4 , ADS::Static< HeapExcluding > > & h , int id ,
  int k ) {
 h.decrease( id , k );
 }

int zc_static_excl_heap_pop(
  ADS::IndexedDaryHeap< int , 4 , ADS::Static< HeapExcluding > > & h ) {
 return( h.pop().id );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- ObservableArray -----------------------------*/
/*--------------------------------------------------------------------------*/

void zc_none_oarray_ref( OArray< ADS::NoObservers > & a , std::size_t h ,
			 int k ) {
 a.set( h , k );
 }

void zc_static_excl_oarray_ref( OArray< ADS::Static< ArrayExcluding > > & a ,
				std::size_t h , int k ) {
 a.set( h , k );
 }

void zc_static_count_oarray_ref(
  OArray< ADS::Static< ArrayCountDecreases > > & a , std::size_t h , int k ) {
 a.set( h , k );
 }

void zc_dynamic_oarray_ref( OArray< ADS::Dynamic<> > & a , std::size_t h ,
			    int k ) {
 a.set( h , k );
 }

#endif

 }  // end( extern "C" )

/*--------------------------------------------------------------------------*/
/*------------------------------- JoinTree ---------------------------------*/
/*--------------------------------------------------------------------------*/

#ifdef ZC_POLICIES

/// the operations of the tree T, by functions zc_<tag>_tree_<operation>
#define ZC_TREE( tag, T )                                                     \
 void zc_##tag##_tree_insert( T & t, int k, long long v ) {                   \
  t.insert( k, v );                                                           \
 }                                                                            \
 void zc_##tag##_tree_remove( T & t, int k ) {                                \
  t.remove( k );                                                              \
 }                                                                            \
 bool zc_##tag##_tree_lookup( const T & t, int k ) {                          \
  return( t.lookup( k ).has_value() );                                        \
 }                                                                            \
 int zc_##tag##_tree_lower_bound( const T & t, int k ) {                      \
  return( t.lowerBound( k ) ? t.lowerBound( k )->first : -1 );                \
 }                                                                            \
 void zc_##tag##_tree_clear( T & t ) {                                        \
  t.clear();                                                                  \
 }                                                                            \
 long long zc_##tag##_tree_range( const T & t, int lo, int hi ) {             \
  return( t.augRange( lo, hi ) + t.augPrefix( hi ) );                         \
 }                                                                            \
 void zc_##tag##_tree_union( T & t, T & o ) {                                 \
  t.unionWith( std::move( o ) );                                              \
 }                                                                            \
 void zc_##tag##_tree_multi_insert(                                           \
   T & t, std::span< const std::pair< int, long long > > B ) {                \
  t.multiInsert( B );                                                         \
 }                                                                            \
 void zc_##tag##_tree_multi_remove( T & t, std::span< const int > B ) {       \
  t.multiRemove( B );                                                         \
 }

#define ZC_TREE_COPY( tag, T )                                                \
 void zc_##tag##_tree_copy( T & t, const T & o ) {                            \
  t = o;                                                                      \
 }                                                                            \
 void zc_##tag##_tree_swap( T & t, T & o ) {                                  \
  t.swap( o );                                                                \
 }

extern "C" {

ZC_TREE( none , JT )
ZC_TREE_COPY( none , JT )

#ifndef ZC_BEFORE

void zc_none_tree_assign( JT & t , int k , long long v ) {
 t.assign( k , v );
 }

ZC_TREE( static_excl , JTP< ADS::Static< TreeExcluding > > )
ZC_TREE( static_count , JTP< ADS::Static< TreeCountDecreases > > )
ZC_TREE( dynamic , JTP< ADS::Dynamic<> > )
ZC_TREE_COPY( dynamic , JTP< ADS::Dynamic<> > )

void zc_static_excl_tree_assign( JTP< ADS::Static< TreeExcluding > > & t ,
				 int k , long long v ) {
 t.assign( k , v );
 }

void zc_static_count_tree_assign( JTP< ADS::Static< TreeCountDecreases > > & t ,
				  int k , long long v ) {
 t.assign( k , v );
 }

void zc_dynamic_tree_assign( JTP< ADS::Dynamic<> > & t , int k ,
			     long long v ) {
 t.assign( k , v );
 }

#endif

 }  // end( extern "C" )

#endif

/*--------------------------------------------------------------------------*/
/*------------------------ End File zerocost_asm.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
