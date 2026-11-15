/*--------------------------------------------------------------------------*/
/*---------------------- File SingleWriterFlatTable.h ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A hash table that one thread writes and many threads read at once,
 * beyond the book: the open addressing with linear probing of
 * `ConcurrentFlatTable` [see `ConcurrentFlatTable.h`] for a table that only
 * its owner fills, e.g., the CLOSED set of a worker of a parallel search
 * that the others read to prune their own states. Since one thread makes
 * every insertion, an insertion needs no read-modify-write: it reads the
 * words from the place of its key, and at the first free one stores the
 * value and then the key, the key by a store with release, which a reader
 * reads with acquire; the counters of the threads and the protocol of
 * Dekker by which `ConcurrentFlatTable` keeps its insertions apart from a
 * growth, a sequentially consistent read-modify-write per insertion, are
 * gone. On x86-64 an insertion is thus made of plain loads and stores.
 *
 * The pairs lie in the words: a key and a value trivially copyable, each
 * of at most 8 bytes, the key with a unique representation (or of floating
 * point), as the pointers and the integers. A place is one word, the key,
 * when the value is empty (a set), and two words otherwise, the key and
 * the value, on the same line; a word is 0 when its place is free, so that
 * the key made of zero bytes is kept apart, in two words of its own. A key
 * word, once written, never changes until `clear()`; a value word changes
 * by `update()`, of the writer, by a store with release.
 *
 * The place of a key with the hash \f$ h \f$ in a vector of \f$ 2^l \f$
 * places is the top \f$ l \f$ bits of \f$ h \f$, and the table grows when
 * its pairs pass half the places: the writer makes a vector twice as
 * large, copies the pairs into it, and then makes it the vector in use by
 * a store with release. The readers never wait: a reader that took the old
 * vector reads it on, since the writer no longer writes it, and a lookup
 * that ends at a free place reads the vector in use again, and starts over
 * on the new one if the table grew meanwhile (see \ref
 * theory_concurrent_single). The vectors replaced are kept until `clear()` or
 * the destruction, since a reader may still be on one; with sizes that
 * double they occupy less than the vector in use. A lookup is wait-free
 * while the table does not grow, and starts over at most once per growth.
 *
 * `clear()` writes 0 to every place, unless the policy `C` is
 * `EpochClear< S >`, by which, as in `ConcurrentFlatTable`, each group of 8
 * places has a stamp, and the group is in use only if its stamp is the
 * epoch, which `clear()` raises: the writer renews a group out of use when
 * an insertion first reaches it, writing 0 to its words and then storing
 * the epoch with release, and a reader takes a group out of use for free.
 *
 * Each operation is linearizable: an insertion at the store of its key
 * word, a lookup that finds its key at the read of the key word, and one
 * that does not at the read of the free place (or of the stamp of a group
 * out of use). A key found, and its value, were written whole before the
 * read, and so was everything that the writer wrote before inserting it,
 * e.g., the object that a pointer key points to.
 *
 * The contracts [see `Contract.h`]: one thread at a time calls the
 * operations of the writer (`findOrInsert()`, `findOrInsertKey()`,
 * `insert()`, `update()`, `reserve()` and `clear()`), any thread calls the
 * lookups (`find()`, `findKey()`, `contains()`, `size()`), with no other
 * thread during `clear()`, `forEach()` and `consistent()`; the hash passed
 * is `hash( k )` (audit); `consistent()` checks that each key is reached
 * from its place with no free place in between, and the count.
 *
 * \see T. Maier, P. Sanders, R. Dementiev, Concurrent hash tables: fast
 *      and general(?)!, ACM Transactions on Parallel Computing 5(4), 16,
 *      2019, doi:10.1145/3309206
 * \see D. E. Knuth, The Art of Computer Programming, Volume 3: Sorting and
 *      Searching, 2nd edition, Addison-Wesley, 1998, section 6.4
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

#ifndef __ADS_SingleWriterFlatTable
 #define __ADS_SingleWriterFlatTable
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Portable.h"
#include "ADS/concurrent/ConcurrentFlatTable.h"
#include "ADS/dictionary/FastHash.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS SingleWriterFlatTable -----------------------*/
/*--------------------------------------------------------------------------*/

/// a DICTIONARY that one thread writes and many threads read at once, by
/// linear probing on atomic words, with no read-modify-write per
/// insertion, readers that read on while it grows, and no removal; cleared
/// in \f$ O( 1 ) \f$ by the policy `EpochClear`
template< class K , class V , class H = DefaultHash< K > ,
	  class Eq = std::equal_to< K > , class C = ZeroingClear >
class SingleWriterFlatTable
{

 static_assert( FlatClear< C > ,
		"the policy of clear() is ZeroingClear or EpochClear" );

 /// the bytes of a value in a word, 0 for an empty one
 static constexpr std::size_t valueBytes( void ) {
  return( std::is_empty_v< V > ? 0 : sizeof( V ) );
  }

 static_assert( std::is_trivially_copyable_v< K > &&
		  std::is_trivially_copyable_v< V > && ( sizeof( K ) <= 8 ) &&
		  ( valueBytes() <= 8 ) &&
		  ( std::has_unique_object_representations_v< K > ||
		    std::is_floating_point_v< K > ) ,
		"the key and the value are trivially copyable words, the "
		"key with a unique representation" );

/*--------------------------------------------------------------------------*/
 public:
 using Key = K;    ///< the type of the keys
 using Value = V;  ///< the type of the values
 using Clear = C;  ///< the policy of `clear()`

 /// whether `clear()` takes \f$ O( 1 ) \f$ by epochs
 static constexpr bool epochs = detail::ClearStamps< C >::epochs;

 /// the table of the least vector that holds \f$ n \f$ pairs with no
 /// growth, at least 64 places
 explicit SingleWriterFlatTable( std::size_t n = 0 , H h = H() ,
				 Eq eq = Eq() );

 SingleWriterFlatTable( const SingleWriterFlatTable & ) = delete;
 SingleWriterFlatTable & operator=( const SingleWriterFlatTable & ) = delete;

 /// the hash of 64 bits of \f$ k \f$, that the operations take
 [[nodiscard]] std::uint64_t hash( const K & k ) const;

 /// the value held for the key \f$ k \f$ of hash \f$ h \f$, which is
 /// \f$ v \f$ if absent and then inserted, and true if it was inserted; by
 /// the writer only
 ///
 /// In pseudocode (beyond the book), \f$ W \f$ the vector in use, of
 /// \f$ 2^l \f$ places, and \f$ n \f$ the pairs:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{findOrInsert}( k , h , v ) \cr
 /// 2 & \quad \textbf{if}\ 2 ( n + 1 ) > 2^l\ \textbf{then}
 ///     \ \mathrm{grow}() \cr
 /// 3 & \quad i \gets \lfloor h / 2^{64 - l} \rfloor \cr
 /// 4 & \quad \textbf{loop} \cr
 /// 5 & \qquad \textbf{if}\ \text{the group of } i \text{ is out of use}
 ///     \ \textbf{then}\ \text{write 0 to its words; store the epoch as}
 ///     \text{ its stamp}\quad \triangleright\ \text{release} \cr
 /// 6 & \qquad \textbf{if}\ W[ i ].\mathit{key} = 0\ \textbf{then} \cr
 /// 7 & \qquad\quad W[ i ].\mathit{value} \gets v;\ W[ i ].\mathit{key}
 ///     \gets k\quad \triangleright\ \text{the key with release} \cr
 /// 8 & \qquad\quad n \gets n + 1;\ \textbf{return}\ ( v ,
 ///     \textbf{true} ) \cr
 /// 9 & \qquad \textbf{if}\ W[ i ].\mathit{key} = k\ \textbf{then}
 ///     \ \textbf{return}\ ( W[ i ].\mathit{value} , \textbf{false} ) \cr
 /// 10 & \qquad i \gets ( i + 1 ) \bmod 2^l \cr
 /// 11 & \textbf{procedure}\ \mathrm{grow}() \cr
 /// 12 & \quad W' \gets \text{a vector of } 2^{l + 1} \text{ free places}
 ///      \cr
 /// 13 & \quad \textbf{foreach}\ \text{pair } ( k , v ) \text{ of } W
 ///      \ \textbf{do}\ \text{put it at the first free place of } k
 ///      \text{ in } W' \cr
 /// 14 & \quad \text{keep } W \text{ for the readers};\ W \gets W'\quad
 ///      \triangleright\ \text{release}
 /// \end{array}}
 /// \f]
 /// \pre the caller is the writer; `hash( k )` is \f$ h \f$ (audit)
 std::pair< V , bool > findOrInsert( const K & k , std::uint64_t h ,
				    const V & v );

 /// `findOrInsert()` above, with the hash computed
 std::pair< V , bool > findOrInsert( const K & k , const V & v );

 /// the key held equal to \f$ k \f$ by `Eq`, of hash \f$ h \f$, which is
 /// \f$ k \f$ itself if absent and then inserted; true if it was inserted,
 /// for a set (an empty value); by the writer only
 /// \pre the caller is the writer; `hash( k )` is \f$ h \f$ (audit)
 std::pair< K , bool > findOrInsertKey( const K & k , std::uint64_t h )
  requires( valueBytes() == 0 );

 /// true if the key \f$ k \f$ was absent and \f$ ( k , v ) \f$ inserted;
 /// by the writer only
 /// \pre the caller is the writer
 bool insert( const K & k , const V & v );

 /// the value of the key \f$ k \f$ replaced by `f( value )`, true if the
 /// key is there, by a store with release that a reader sees whole; by the
 /// writer only
 /// \pre the caller is the writer
 template< class F >
 bool update( const K & k , F && f )
  requires( valueBytes() > 0 );

 /// a copy of the value of the key \f$ k \f$ of hash \f$ h \f$, or nothing;
 /// by any thread, also while the writer inserts and grows the table
 ///
 /// In pseudocode (beyond the book), with the notation of
 /// `findOrInsert()` above:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{find}( k , h ) \cr
 /// 2 & \quad \textbf{loop} \cr
 /// 3 & \qquad T \gets \text{the vector in use } W\quad \triangleright
 ///     \ \text{acquire} \cr
 /// 4 & \qquad i \gets \lfloor h / 2^{64 - l} \rfloor \cr
 /// 5 & \qquad \textbf{while}\ \text{the group of } i \text{ is in use}
 ///     \ \textbf{and}\ T[ i ].\mathit{key} \ne 0\ \textbf{do}\quad
 ///     \triangleright\ \text{acquire} \cr
 /// 6 & \qquad\quad \textbf{if}\ T[ i ].\mathit{key} = k\ \textbf{then}
 ///     \ \textbf{return}\ T[ i ].\mathit{value} \cr
 /// 7 & \qquad\quad i \gets ( i + 1 ) \bmod 2^l \cr
 /// 8 & \qquad \textbf{if}\ T \text{ is still the vector in use}
 ///     \ \textbf{then return nil}
 /// \end{array}}
 /// \f]
 /// \pre `hash( k )` is \f$ h \f$ (audit)
 [[nodiscard]] std::optional< V > find( const K & k , std::uint64_t h ) const;

 /// `find()` above, with the hash computed
 [[nodiscard]] std::optional< V > find( const K & k ) const;

 /// a copy of the key held equal to \f$ k \f$ by `Eq`, of hash \f$ h \f$, or
 /// nothing, by the lookup of `find()`; by any thread
 /// \pre `hash( k )` is \f$ h \f$ (audit)
 [[nodiscard]] std::optional< K > findKey( const K & k ,
					   std::uint64_t h ) const;

 /// `findKey()` above, with the hash computed
 [[nodiscard]] std::optional< K > findKey( const K & k ) const;

 /// whether the key \f$ k \f$ of hash \f$ h \f$ is there; by any thread
 /// \pre `hash( k )` is \f$ h \f$ (audit)
 [[nodiscard]] bool contains( const K & k , std::uint64_t h ) const;

 /// `contains()` above, with the hash computed
 [[nodiscard]] bool contains( const K & k ) const;

 /// the pairs: exact for the writer, and for another thread a number of
 /// pairs that the table held at some time since its last `clear()`
 [[nodiscard]] std::size_t size( void ) const {
  return( f_size.load( std::memory_order_relaxed ) );
  }

 /// whether there is no pair
 [[nodiscard]] bool empty( void ) const { return( size() == 0 ); }

 /// the places of the vector in use
 [[nodiscard]] std::size_t capacity( void ) const {
  return( f_table.load( std::memory_order_acquire )->places() );
  }

 /// the growths made so far
 [[nodiscard]] std::size_t rehashes( void ) const {
  return( f_rehashes.load( std::memory_order_relaxed ) );
  }

 /// the table grown so that it holds \f$ n \f$ pairs with no growth; by
 /// the writer only
 /// \pre the caller is the writer
 void reserve( std::size_t n );

 /// no pair, in time linear in the places, or \f$ O( 1 ) \f$ with
 /// `EpochClear` but once per \f$ 2^{|S|} - 2 \f$ calls; the vectors
 /// replaced are freed
 /// \pre no other thread accesses the table
 void clear( void );

 /// the epoch of the policy `EpochClear`, 1 for `ZeroingClear`
 [[nodiscard]] std::uint64_t epoch( void ) const { return( now() ); }

 /// `f( k , v )` on each pair, in no order
 /// \pre no other thread accesses the table
 template< class F >
 void forEach( F && f ) const;

 /// whether each key is reached from its place with no free place in
 /// between, once, and the pairs are those counted
 /// \pre no other thread accesses the table
 [[nodiscard]] bool consistent( void ) const;

/*--------------------------------------------------------------------------*/
 private:
 static constexpr int kLeast = 6;  ///< log2 of the least vector

 /// the words of a place, the key and the value if any
 static constexpr std::size_t kWords = valueBytes() == 0 ? 1 : 2;

 using Stamp = typename detail::ClearStamps< C >::Stamp;  ///< of a group

 /// the largest stamp, which the epoch never reaches
 static constexpr Stamp kLast = std::numeric_limits< Stamp >::max();

 /// a vector of places, with the stamps of its groups if epochs
 struct Table
 {
  int lg;                                               ///< log2 of the places
  std::unique_ptr< std::atomic< std::uint64_t >[] > W;  ///< the words
  std::unique_ptr< std::atomic< Stamp >[] > E;  ///< the stamps, if epochs
  Table( int l , Stamp e );
  [[nodiscard]] std::size_t places( void ) const {
   return( std::size_t( 1 ) << lg );
   }
  [[nodiscard]] std::size_t groups( void ) const {
   return( ( places() + 7 ) >> 3 );
   }
  };

 [[nodiscard]] Stamp now( void ) const {
  if constexpr( epochs )
   return( f_epoch );
  else
   return( 1 );
  }

 template< class X >
 [[nodiscard]] static std::uint64_t bits( const X & x );

 [[nodiscard]] static K keyOf( std::uint64_t w );

 [[nodiscard]] static V valueOf( std::uint64_t w );

 [[nodiscard]] static bool zeroKey( const K & k );

 [[nodiscard]] static std::size_t home( std::uint64_t h , const Table & T );

 [[nodiscard]] bool live( const Table & T , std::size_t g ) const;

 [[nodiscard]] std::uint64_t wordAt( const Table & T , std::size_t i ) const;

 // the place of the key k in the vector in use, or nullptr; by any thread
 [[nodiscard]] const std::atomic< std::uint64_t > *
 locate( const K & k , std::uint64_t h ) const;

 // the place of the key k in the vector in use for the writer, its key
 // word 0 if it is absent, the group renewed if out of use
 std::atomic< std::uint64_t > * place( Table & T , const K & k ,
				       std::uint64_t h );

 void grow( int lg );

 std::atomic< Table * > f_table{ nullptr };      ///< the vector in use
 std::unique_ptr< Table > f_cur;                 ///< the vector in use, owned
 std::vector< std::unique_ptr< Table > > f_old;  ///< the vectors replaced
 std::atomic< std::uint64_t > f_zero[ 2 ];       ///< the key 0: 1, its value
 std::atomic< std::size_t > f_size{ 0 };         ///< the pairs
 std::atomic< std::size_t > f_rehashes{ 0 };     ///< the growths
 ADS_NO_UNIQUE_ADDRESS std::conditional_t< epochs , Stamp , std::monostate >
   f_epoch{};
 H f_h;
 Eq f_eq;

 };  // end( class SingleWriterFlatTable )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/concurrent/impl/SingleWriterFlatTable.tpp"

/*--------------------------------------------------------------------------*/

#endif /* SingleWriterFlatTable.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File SingleWriterFlatTable.h -------------------*/
/*--------------------------------------------------------------------------*/
