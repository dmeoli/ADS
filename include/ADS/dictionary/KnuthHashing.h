/*--------------------------------------------------------------------------*/
/*-------------------------- File KnuthHashing.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Two hash tables of Knuth, beyond the book [see ChainedHashTable.h and
 * OpenHashTable.h for those of chapter 7], both without removal:
 *
 * - CoalescedHashTable (Algorithm 6.4C, with the cellar of Vitter): the
 *   lists of overflow live in the table itself, a key that collides being
 *   put in the empty place of largest index (the first ones taken being
 *   those of a cellar beyond the m places addressed by the hash) and linked
 *   at the end of the list of its place, so that the lists may merge
 *   (coalesce); with a cellar of about 14 % of the table the searches are
 *   nearly as fast as with separate lists, with no memory out of the table;
 * - BrentHashTable, the double hashing [see Probing] with the improvement
 *   of Brent to the insertions: the key K is placed at the j-th place of its
 *   inspection, and the key found there moved forward by s - j places of its
 *   own inspection, for the least s, and then the least j, that find an empty
 *   place; this lowers the expected number of probes of a successful search
 *   below 2.5 even in a full table, against \f$ - \ln( 1 - \alpha ) /
 *   \alpha \f$ of the plain double hashing, at no cost for the searches.
 *
 * The contracts [see Contract.h]: the table is not full on insert();
 * insert() ensures that the key is there; the invariant of
 * CoalescedHashTable is that each key is on the list of its place, an
 * audit. The operations are defined in KnuthHashing.tpp.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, 2nd ed.,
 *      Addison-Wesley, 1998, section 6.4 (Algorithm C and
 *      exercise 6.4-49 on Brent's variation)
 * \see J. S. Vitter, Implementations for coalesced hashing, Communications
 *      of the ACM 25(12), 911-926, 1982, doi:10.1145/358728.358745
 * \see R. P. Brent, Reducing the retrieval time of scatter storage
 *      techniques, Communications of the ACM 16(2), 105-109, 1973,
 *      doi:10.1145/361952.361964
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_KnuthHashing
 #define __ADS_KnuthHashing
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cstddef>
#include <functional>
#include <optional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS CoalescedHashTable ------------------------*/
/*--------------------------------------------------------------------------*/

/// a hash table with coalesced lists in the table and a cellar
template< class K , class V , class H = std::hash< K > >
class CoalescedHashTable {

/*--------------------------------------------------------------------------*/
 public:

 /// a table of m places addressed by the hash and of cellar more
 /// \pre m >= 1
 explicit CoalescedHashTable( std::size_t m , std::size_t cellar = 0 ,
                              H h = H() );

 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// associates v to k
 /// \pre k is there, or the table is not full
 /// \post k has the value v
 void insert( const K & k , const V & v );

 [[nodiscard]] std::size_t size( void ) const;

 /// the places examined by the last lookup() or insert()
 [[nodiscard]] std::size_t probes( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 struct Slot {
  bool used = false;
  K key{};
  V value{};
  long link = -1;                ///< the next place on the list, or -1
  };

 /// each key is on the list of its place, O( n^2 )
 [[nodiscard]] bool listed( void ) const;

 std::vector< Slot > f_T;        ///< the m places and the cellar
 std::size_t f_m;                ///< the places addressed by the hash
 long f_free;                    ///< the empty places are below it
 std::size_t f_size = 0;
 mutable std::size_t f_probes = 0;
 H f_h;

 };  // end( class CoalescedHashTable )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS BrentHashTable --------------------------*/
/*--------------------------------------------------------------------------*/

/// a hash table with double hashing and the insertion of Brent
template< class K , class V , class H = std::hash< K > >
class BrentHashTable {

/*--------------------------------------------------------------------------*/
 public:

 /// a table of m places
 /// \pre m >= 2 is prime
 explicit BrentHashTable( std::size_t m , H h = H() );

 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// associates v to k
 /// \pre k is there, or the table is not full
 /// \post k has the value v
 void insert( const K & k , const V & v );

 [[nodiscard]] std::size_t size( void ) const;

 /// the places examined by the last lookup()
 [[nodiscard]] std::size_t probes( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// the place i of the inspection of k
 [[nodiscard]] std::size_t at( const K & k , std::size_t i ) const;

 std::vector< std::optional< std::pair< K , V > > > f_T;
 std::size_t f_size = 0;
 mutable std::size_t f_probes = 0;
 H f_h;

 };  // end( class BrentHashTable )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/KnuthHashing.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* KnuthHashing.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File KnuthHashing.h -------------------------*/
/*--------------------------------------------------------------------------*/
