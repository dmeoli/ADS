/*--------------------------------------------------------------------------*/
/*---------------------------- File YFastTrie.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The y-fast trie of Willard, beyond the book, in the form of section 13.3
 * of Morin's book: a SORTED SET of the integers of \f$ w \f$ bits, each
 * with a value, in \f$ O( n ) \f$ space, whose `find( x )` (the smallest
 * key \f$ \ge x \f$), `insert()` and `remove()` take
 * \f$ O( \log w ) \f$ expected time, the last two amortized.
 *
 * The keys are cut into groups of consecutive keys, each kept in a
 * `Treap` [see `Treap.h`], and the largest key of each group, its
 * representative, is in an `XFastTrie` [see `XFastTrie.h`] with its group
 * as the value; the key \f$ 2^w - 1 \f$ is always a representative, of the
 * last group, so that every key has a group, the one of the smallest
 * representative \f$ \ge \f$ it. A new key goes into its group and, with
 * probability \f$ 1 / w \f$, becomes a representative, the group being
 * split at it; a removed representative merges its group with the next
 * one. The representatives are thus about \f$ n / w \f$, and the x-fast
 * trie takes \f$ O( n ) \f$ space; a group has \f$ O( w ) \f$ keys in
 * expectation (Lemma 13.3 of Morin's book), so that the operations on the
 * treaps take \f$ O( \log w ) \f$ expected time, as the search in the
 * x-fast trie does, while its updates, \f$ O( w ) \f$, happen once every
 * \f$ w \f$ operations on average.
 *
 * Where it is meant to be faster than the code of Morin's book: the
 * x-fast trie of the representatives is the one of `XFastTrie.h`, with
 * its faster tables; the search in a group descends its treap with no
 * copy of a key, and the coin of an insertion is one word of SplitMix64
 * reduced modulo \f$ w \f$ instead of a call to `rand()`.
 *
 * The coins come from SplitMix64 [see `RandomHashing.h`], seeded, so that a
 * run is reproducible. The contracts [see `Contract.h`]: the keys are below
 * \f$ 2^w \f$, `insert()` ensures that the key is there and `remove()` that
 * it is not; the invariant (each group between two consecutive
 * representatives, each representative in its group but the last, and
 * the count of the keys) is checked in \f$ O( n ) \f$ time at the level 3
 * while \f$ n \le 256 \f$. The operations are defined in `YFastTrie.tpp`.
 *
 * \see P. Morin, Open Data Structures (in C++), Athabasca University Press,
 *      2013, doi:10.15215/aupress/9781927356388.01, section 13.3
 * \see D. E. Willard, Log-logarithmic worst-case range queries are possible
 *      in space Theta(N), Information Processing Letters 17(2), 81-84,
 *      1983, doi:10.1016/0020-0190(83)90075-3
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

#ifndef __ADS_YFastTrie
 #define __ADS_YFastTrie
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/RandomHashing.h"
#include "ADS/dictionary/XFastTrie.h"
#include "ADS/tree/Treap.h"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <variant>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS YFastTrie ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the y-fast trie: an x-fast trie of representatives over treaps of
/// \f$ O( w ) \f$ keys
template< std::unsigned_integral U , class V = std::monostate ,
          int W = std::numeric_limits< U >::digits >
 requires TrieWidth< U , W >
class YFastTrie {

/*--------------------------------------------------------------------------*/
 public:

 using Key = U;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 /// the empty trie, its coins drawn from the seed
 explicit YFastTrie( std::uint64_t seed = 1 );

 /// the trie of `o`, which is left empty
 YFastTrie( YFastTrie && o );

 /// the trie of `o`, which gets the keys of this one
 YFastTrie & operator=( YFastTrie && o ) noexcept;

 YFastTrie( const YFastTrie & ) = delete;
 YFastTrie & operator=( const YFastTrie & ) = delete;

 /// frees the groups
 ~YFastTrie();

/*--------------------------------------------------------------------------*/

 /// the smallest key \f$ \ge x \f$, or nothing, in \f$ O( \log w ) \f$
 /// expected time
 ///
 /// In pseudocode (section 13.3 of Morin's book), \f$ X \f$ the x-fast
 /// trie of the representatives:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{find}( x ) \cr
 /// 2 & \quad ( r , T ) \gets \text{the pair of } X.\mathrm{find}( x ) \cr
 /// 3 & \quad \textbf{return}\ \text{the smallest key} \ge x \text{ of }
 ///     T \text{, or } \textbf{nil}
 /// \end{array}}
 /// \f]
 /// \pre \f$ x < 2^w \f$
 [[nodiscard]] std::optional< U > find( U x ) const;

 /// the value of \f$ x \f$, or nothing
 /// \pre \f$ x < 2^w \f$
 [[nodiscard]] std::optional< V > lookup( U x ) const;

 /// true if \f$ x \f$ is there
 /// \pre \f$ x < 2^w \f$
 [[nodiscard]] bool contains( U x ) const;

 /// associates \f$ v \f$ to \f$ x \f$; true if \f$ x \f$ was not there, in
 /// \f$ O( \log w ) \f$ expected amortized time
 ///
 /// In pseudocode (section 13.3 of Morin's book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insert}( x , v ) \cr
 /// 2 & \quad ( r , T ) \gets \text{the pair of } X.\mathrm{find}( x ) \cr
 /// 3 & \quad \textbf{if}\ x \in T\ \textbf{then}\ \text{give it the value }
 ///     v;\ \textbf{return false} \cr
 /// 4 & \quad T.\mathrm{insert}( x , v ) \cr
 /// 5 & \quad \textbf{if}\ x \ne r \text{ and a coin of bias } 1 / w
 ///     \text{ comes up}\ \textbf{then} \cr
 /// 6 & \qquad ( T' , T ) \gets \text{the keys of } T \text{ up to } x
 ///     \text{ and the others, by a split} \cr
 /// 7 & \qquad X.\mathrm{insert}( x , T' ) \cr
 /// 8 & \quad \textbf{return true}
 /// \end{array}}
 /// \f]
 /// \pre \f$ x < 2^w \f$
 /// \post \f$ x \f$ is there
 bool insert( U x , const V & v = V() );

 /// removes \f$ x \f$; true if it was there, in \f$ O( \log w ) \f$
 /// expected amortized time
 ///
 /// In pseudocode (section 13.3 of Morin's book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{remove}( x ) \cr
 /// 2 & \quad ( r , T ) \gets \text{the pair of } X.\mathrm{find}( x ) \cr
 /// 3 & \quad \textbf{if}\ x \notin T\ \textbf{then return false} \cr
 /// 4 & \quad T.\mathrm{remove}( x ) \cr
 /// 5 & \quad \textbf{if}\ x = r \ne 2^w - 1\ \textbf{then} \cr
 /// 6 & \qquad ( r' , T' ) \gets \text{the pair after } ( r , T ) \text{ in }
 ///     X \cr
 /// 7 & \qquad T' \gets \mathrm{join}( T , T' );\ X.\mathrm{remove}( x ) \cr
 /// 8 & \quad \textbf{return true}
 /// \end{array}}
 /// \f]
 /// \pre \f$ x < 2^w \f$
 /// \post \f$ x \f$ is not there
 bool remove( U x );

 /// the number \f$ n \f$ of keys
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// true if there are no keys
 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }

 /// the number of groups, the last one included
 [[nodiscard]] std::size_t groups( void ) const { return( f_x.size() ); }

 /// `f( x , v )` on the pairs by increasing keys
 template< class F >
 void forEach( F && f ) const;

/*--------------------------------------------------------------------------*/
 private:

 using Group = Treap< U , V >;   ///< a group of consecutive keys

 /// the largest key, \f$ 2^w - 1 \f$
 static constexpr U TOP = ( W == std::numeric_limits< U >::digits ) ?
                          std::numeric_limits< U >::max() :
                          U( ( U( 1 ) << W ) - 1 );

 /// the smallest key \f$ \ge x \f$ of \f$ T \f$, or nil, by the descent of
 /// a binary search tree
 [[nodiscard]] static const U * ceiling( const Group & T , U x );

 /// the groups between their representatives, each representative in its
 /// group but the last, and `size()` their total; for \f$ n \le 256 \f$
 [[nodiscard]] bool consistent( void ) const;

 XFastTrie< U , Group * , W > f_x;   ///< the representatives
 SplitMix64 f_g;                     ///< the coins and the seeds
 std::size_t f_n = 0;                ///< the number of keys

 };  // end( class YFastTrie )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/YFastTrie.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* YFastTrie.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File YFastTrie.h ---------------------------*/
/*--------------------------------------------------------------------------*/
