/*--------------------------------------------------------------------------*/
/*----------------------------- File heaps.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The indexed heaps of the module `ads`, which `heapq` has not: the
 * \f$ d \f$-ary heap of the ids \f$ [ 0 , n ) \f$ by a key of type
 * `double`, with the decrease, the increase and the deletion of any id by
 * its place, and the pair of heaps that also knows the least of a second
 * key, the bound, among the ids it holds. The ids and the keys are
 * checked before they reach the heap, so that a wrong one raises an
 * exception of Python instead of breaking an invariant.
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

#include "ADS/priority/IndexedDaryHeap.h"
#include "ADS/priority/IndexedHeapPair.h"

#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>

#include <cstddef>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ads_python
{

using namespace nb::literals;

namespace
{

/// the id \f$ i \f$ of the heap \f$ h \f$ checked: in its range, and in
/// the heap if \f$ in \f$, out of it if not \f$ in \f$
template< class H >
void checkId( const H & h , int i , int in )
{
 if( ( i < 0 ) || ( i >= h.ids() ) )
  throw( nb::index_error( "not an id of the heap" ) );
 if( ( in == 1 ) && ! h.contains( i ) )
  throw( nb::key_error( "the id is not in the heap" ) );
 if( ( in == 0 ) && h.contains( i ) )
  throw( nb::key_error( "the id is already in the heap" ) );
 }

/// the heap \f$ h \f$ checked not empty
template< class H >
void checkFull( const H & h )
{
 if( h.empty() )
  throw( nb::index_error( "the heap is empty" ) );
 }

/// the indexed heap of D children, named \f$ s \f$
template< unsigned D >
void bindHeap( nb::module_ & m , const char * s )
{
 using H = ADS::IndexedDaryHeap< double , D >;
 nb::class_< H > c( m , s );
 c.doc() = "A d-ary heap of the ids [0, n) by keys of type float, with "
	   "decrease, increase, update and erase of any id in "
	   "O(d log_d n) time.";
 c.def( nb::init< int >() , "n"_a = 0 , "An empty heap of the ids [0, n)." )
   .def_prop_ro( "ids" , &H::ids , "The number of the ids." )
   .def( "__len__" , &H::size )
   .def( "__bool__" , []( const H & h ) { return( ! h.empty() ); } )
   .def(
     "__contains__" ,
     []( const H & h , int i ) {
      return( ( i >= 0 ) && ( i < h.ids() ) && h.contains( i ) );
      } ,
     "id"_a )
   .def(
     "key" ,
     []( const H & h , int i ) {
      checkId( h , i , 1 );
      return( h.key( i ) );
      } ,
     "id"_a , "The key of the id." )
   .def(
     "push" ,
     []( H & h , int i , double k ) {
      checkId( h , i , 0 );
      h.push( i , k );
      } ,
     "id"_a , "key"_a , "Inserts the id, not in the heap, with the key." )
   .def(
     "push_many" ,
     []( H & h , const Input< int > & ids , const Input< double > & keys ) {
      const auto i = items( ids );
      const auto k = items( keys );
      if( i.size() != k.size() )
       throw( std::invalid_argument( "one key for each id" ) );
      for( std::size_t x = 0 ; x < i.size() ; ++x ) {
       checkId( h , i[ x ] , 0 );
       h.push( i[ x ] , k[ x ] );
       }
      } ,
     "ids"_a , "keys"_a , "Inserts each id with its key." )
   .def(
     "decrease" ,
     []( H & h , int i , double k ) {
      checkId( h , i , 1 );
      if( k > h.key( i ) )
       throw( std::invalid_argument( "a key above the one of the id" ) );
      h.decrease( i , k );
      } ,
     "id"_a , "key"_a , "Lowers the key of the id to key." )
   .def(
     "increase" ,
     []( H & h , int i , double k ) {
      checkId( h , i , 1 );
      if( k < h.key( i ) )
       throw( std::invalid_argument( "a key below the one of the id" ) );
      h.increase( i , k );
      } ,
     "id"_a , "key"_a , "Raises the key of the id to key." )
   .def(
     "update" ,
     []( H & h , int i , double k ) {
      checkId( h , i , 1 );
      h.update( i , k );
      } ,
     "id"_a , "key"_a , "Changes the key of the id to key." )
   .def(
     "push_or_decrease" ,
     []( H & h , int i , double k ) {
      checkId( h , i , -1 );
      return( h.pushOrDecrease( i , k ) );
      } ,
     "id"_a , "key"_a ,
     "Inserts the id with the key, or lowers its key to it if it is "
     "lower (the relaxation of Dijkstra); whether the heap changed." )
   .def(
     "erase" ,
     []( H & h , int i ) {
      checkId( h , i , 1 );
      h.erase( i );
      } ,
     "id"_a , "Removes the id." )
   .def(
     "top" ,
     []( const H & h ) {
      checkFull( h );
      return( std::make_pair( h.top().id , h.top().key ) );
      } ,
     "The id of least key and its key." )
   .def(
     "pop" ,
     []( H & h ) {
      checkFull( h );
      const auto e = h.pop();
      return( std::make_pair( e.id , e.key ) );
      } ,
     "Removes the id of least key; it and its key." )
   .def( "clear" , &H::clear , "Removes every id." )
   .def(
     "reset" ,
     []( H & h , int n ) {
      if( n < 0 )
       throw( std::invalid_argument( "a negative number of ids" ) );
      h.reset( n );
      } ,
     "n"_a , "Empties the heap, of the ids [0, n) from now on." )
   .def(
     "grow" ,
     []( H & h , int n ) {
      if( n < h.ids() )
       throw( std::invalid_argument( "fewer ids than the heap has" ) );
      h.grow( n );
      } ,
     "n"_a , "The ids [0, n), the heap kept." );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*--------------------------------- THE AREA -------------------------------*/
/*--------------------------------------------------------------------------*/

void bindHeaps( nb::module_ & m )
{
 bindHeap< 2 >( m , "IndexedBinaryHeap" );
 bindHeap< 4 >( m , "IndexedHeap" );
 bindHeap< 8 >( m , "IndexedHeap8" );
 using P = ADS::IndexedHeapPair< double , double , 4 >;
 nb::class_< P >(
   m , "IndexedHeapPair" ,
   "A heap of the ids [0, n) by a key of selection that also knows the "
   "least of a second key, the bound, among the ids it holds: the "
   "frontier of the focal searches and of the branch and bound by best "
   "estimate." )
   .def( nb::init< int >() , "n"_a = 0 , "An empty heap of the ids [0, n)." )
   .def_prop_ro( "ids" , &P::ids , "The number of the ids." )
   .def( "__len__" , &P::size )
   .def( "__bool__" , []( const P & h ) { return( ! h.empty() ); } )
   .def(
     "__contains__" ,
     []( const P & h , int i ) {
      return( ( i >= 0 ) && ( i < h.ids() ) && h.contains( i ) );
      } ,
     "id"_a )
   .def(
     "key" ,
     []( const P & h , int i ) {
      checkId( h , i , 1 );
      return( h.key( i ) );
      } ,
     "id"_a , "The key of the id." )
   .def(
     "bound" ,
     []( const P & h , int i ) {
      checkId( h , i , 1 );
      return( h.bound( i ) );
      } ,
     "id"_a , "The bound of the id." )
   .def(
     "push" ,
     []( P & h , int i , double k , double b ) {
      checkId( h , i , 0 );
      h.push( i , k , b );
      } ,
     "id"_a , "key"_a , "bound"_a ,
     "Inserts the id, not in the heap, with the key and the bound." )
   .def(
     "update" ,
     []( P & h , int i , double k , double b ) {
      checkId( h , i , 1 );
      h.update( i , k , b );
      } ,
     "id"_a , "key"_a , "bound"_a ,
     "Changes the key and the bound of the id." )
   .def(
     "erase" ,
     []( P & h , int i ) {
      checkId( h , i , 1 );
      h.erase( i );
      } ,
     "id"_a , "Removes the id." )
   .def(
     "top" ,
     []( const P & h ) {
      checkFull( h );
      return( std::make_pair( h.top() , h.topKey() ) );
      } ,
     "The id of least key and its key." )
   .def(
     "min_bound" ,
     []( const P & h ) {
      checkFull( h );
      return( std::make_pair( h.minBoundId() , h.minBound() ) );
      } ,
     "The id of least bound and its bound." )
   .def(
     "pop" ,
     []( P & h ) {
      checkFull( h );
      return( h.pop() );
      } ,
     "Removes the id of least key, and returns it." )
   .def(
     "pop_min_bound" ,
     []( P & h ) {
      checkFull( h );
      return( h.popMinBound() );
      } ,
     "Removes the id of least bound, and returns it." )
   .def( "clear" , &P::clear , "Removes every id." );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ads_python )

/*--------------------------------------------------------------------------*/
/*--------------------------- End File heaps.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
