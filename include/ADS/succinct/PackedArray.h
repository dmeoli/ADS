/*--------------------------------------------------------------------------*/
/*--------------------------- File PackedArray.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Arrays of small unsigned integers packed in words, beyond the book:
 *
 * - `PackedArray< B >`, \f$ n \f$ fields of \f$ B \f$ bits,
 *   \f$ B \in \{ 1 , 2 , 4 , 8 , 16 , 32 \} \f$, \f$ 64 / B \f$ to a word of
 *   64 bits, so that a field never crosses two words and `get( i )` is a
 *   shift and a mask of the word \f$ \lfloor i B / 64 \rfloor \f$; with
 *   `Saturate` the values above \f$ 2^B - 1 \f$ are stored as
 *   \f$ 2^B - 1 \f$ (and counted by `clamped()`) instead of breaking the
 *   contract, which is what a table of lower bounds wants, a value cut
 *   down being still a lower bound;
 * - `TritArray`, \f$ n \f$ values in \f$ \{ 0 , 1 , 2 \} \f$, five to a
 *   byte as the digits of \f$ \sum_{j < 5} t_j 3^j < 3^5 = 243 \f$, i.e.,
 *   \f$ 8 / 5 = 1.6 \f$ bits per value against the 2 of `PackedArray< 2 >`
 *   and \f$ 0.015 \f$ bits above the entropy \f$ \log_2 3 \f$; `get( i )`
 *   reads the digit from a table of the 243 bytes, and `set( i , v )` adds
 *   \f$ ( v - t ) 3^j \f$ to the byte.
 *
 * Both are the tables of the searches in large spaces of states, e.g., the
 * pattern databases of the heuristic search, whose entries are distances
 * of a few bits (saturated at a cap) or their residues modulo 3 (Breyer
 * and Korf), and which are filled by a scan of all the states; the
 * constructors on an executor [see `Executor.h`] fill the fields by blocks
 * of whole words (of whole bytes for `TritArray`), each block on one
 * thread, so that no two threads write the same word and no atomic is
 * needed. The widths that do not divide 64 are in `BitVector` [see
 * `get( i , len )` and `put( i , v , len )` in `BitVector.h`], whose fields
 * may cross two words (see \ref theory_succinct_packed).
 *
 * The contracts [see `Contract.h`]: the places are below \f$ n \f$, the
 * values below \f$ 2^B \f$ unless `Saturate` (below 3 for `TritArray`); the
 * invariant is that the bits past the last field are 0, and that each byte
 * of a `TritArray` is below 243. The operations are defined in
 * `PackedArray.tpp`.
 *
 * \see A. E. Felner, R. E. Korf, S. Hanan, Additive pattern database
 *      heuristics, Journal of Artificial Intelligence Research 22, 279-318,
 *      2004, doi:10.1613/jair.1480
 * \see T. M. Breyer, R. E. Korf, 1.6-bit pattern databases, Proceedings of
 *      the 24th AAAI Conference on Artificial Intelligence, 39-44, 2010,
 *      doi:10.1609/aaai.v24i1.7558
 * \see S. Gog, T. Beller, A. Moffat, M. Petri, From theory to practice:
 *      plug and play with succinct data structures, Experimental Algorithms
 *      (SEA 2014), Lecture Notes in Computer Science 8504, 326-337, 2014,
 *      doi:10.1007/978-3-319-07959-2_28
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

#ifndef __ADS_PackedArray
 #define __ADS_PackedArray
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/parallel/Executor.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS PackedArray -----------------------------*/
/*--------------------------------------------------------------------------*/

/// \f$ n \f$ unsigned fields of \f$ B \f$ bits, \f$ 64 / B \f$ to a word, the
/// values above \f$ 2^B - 1 \f$ clamped to it if `Saturate`
template< unsigned B , bool Saturate = false >
class PackedArray {

 static_assert( ( B == 1 ) || ( B == 2 ) || ( B == 4 ) || ( B == 8 ) ||
                ( B == 16 ) || ( B == 32 ) ,
                "a field of 1, 2, 4, 8, 16 or 32 bits" );

/*--------------------------------------------------------------------------*/
 public:

 using Word = std::uint64_t;   ///< the type of the words

 /// the fields in a word
 static constexpr std::size_t PER = 64 / B;

 /// the largest value of a field, \f$ 2^B - 1 \f$
 static constexpr Word MAX = ( Word( 1 ) << B ) - 1;

 /// the empty array
 PackedArray( void ) = default;

 /// \f$ n \f$ fields of the value \f$ v \f$
 /// \pre \f$ v \le 2^B - 1 \f$ unless `Saturate`
 explicit PackedArray( std::size_t n , Word v = 0 );

 /// the fields `f( i )` for \f$ i \in [ 0 , n ) \f$, filled by blocks of
 /// whole words on the threads of `ex`, \f$ O( n / p + \log n ) \f$ time on
 /// \f$ p \f$ threads
 ///
 /// In pseudocode (beyond the book), the words of \f$ W \f$ split in
 /// blocks by `parallelForBlocks()`:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{build}( \mathit{ex} , n , f ) \cr
 /// 2 & \quad \textbf{foreach}\ \text{block } [ a , b ) \text{ of the }
 ///     \text{words, in parallel}\ \textbf{do} \cr
 /// 3 & \qquad \textbf{for}\ w \gets a\ \textbf{to}\ b - 1\ \textbf{do} \cr
 /// 4 & \qquad\quad x \gets 0 \cr
 /// 5 & \qquad\quad \textbf{for}\ j \gets 0\ \textbf{to}\ \min\{ 64 / B ,
 ///     n - 64 w / B \} - 1\ \textbf{do} \cr
 /// 6 & \qquad\qquad x \gets x \vee ( \min\{ f( 64 w / B + j ) , 2^B - 1
 ///     \} \ll j B ) \cr
 /// 7 & \qquad\quad W[ w ] \gets x
 /// \end{array}}
 /// \f]
 /// \pre `f( i )` is at most \f$ 2^B - 1 \f$ unless `Saturate`, and it may
 ///      be called on any thread
 template< Executor E , class F >
 PackedArray( E & ex , std::size_t n , F && f );

 /// the number of fields, \f$ n \f$
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// whether there is no field
 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }

 /// the value of the field \f$ i \f$, the bits \f$ ( i \bmod 64 / B ) B \f$
 /// on of the word \f$ \lfloor i B / 64 \rfloor \f$
 /// \pre \f$ i < n \f$
 [[nodiscard]] Word get( std::size_t i ) const;

 /// as `get( i )`
 /// \pre \f$ i < n \f$
 [[nodiscard]] Word operator[]( std::size_t i ) const { return( get( i ) ); }

 /// the field \f$ i \f$ set to \f$ v \f$, or to \f$ 2^B - 1 \f$ if
 /// \f$ v \f$ is larger and `Saturate`; true if \f$ v \f$ was clamped
 ///
 /// In pseudocode (beyond the book), \f$ s = ( i \bmod 64 / B ) B \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{set}( i , v ) \cr
 /// 2 & \quad c \gets v > 2^B - 1;\ v \gets \min\{ v , 2^B - 1 \} \cr
 /// 3 & \quad W[ \lfloor i B / 64 \rfloor ] \gets ( W[ \lfloor i B / 64
 ///     \rfloor ] \wedge \neg( ( 2^B - 1 ) \ll s ) ) \vee ( v \ll s ) \cr
 /// 4 & \quad \textbf{return}\ c
 /// \end{array}}
 /// \f]
 /// \pre \f$ i < n \f$, and \f$ v \le 2^B - 1 \f$ unless `Saturate`
 /// \post `get( i )` is \f$ \min\{ v , 2^B - 1 \} \f$
 bool set( std::size_t i , Word v );

 /// appends a field of the value \f$ v \f$, clamped if `Saturate`
 /// \pre \f$ v \le 2^B - 1 \f$ unless `Saturate`
 void push_back( Word v );

 /// the values clamped by the constructors, by `set()` and by
 /// `push_back()`, 0 unless `Saturate`
 [[nodiscard]] std::size_t clamped( void ) const { return( f_clamped ); }

 /// the words, the field \f$ i \f$ in the word \f$ \lfloor i B / 64 \rfloor
 /// \f$, the bits past the last field 0
 [[nodiscard]] const std::vector< Word > & words( void ) const {
  return( f_w );
  }

 /// the memory of the fields, in bytes
 [[nodiscard]] std::size_t bytes( void ) const {
  return( f_w.size() * sizeof( Word ) );
  }

 /// whether the two arrays have the same fields
 friend bool operator==( const PackedArray & a , const PackedArray & b ) {
  return( ( a.f_n == b.f_n ) && ( a.f_w == b.f_w ) );
  }

/*--------------------------------------------------------------------------*/
 private:

 /// the bits past the last field are 0
 [[nodiscard]] bool clean( void ) const;

 std::vector< Word > f_w;       ///< the words
 std::size_t f_n = 0;           ///< the fields
 std::size_t f_clamped = 0;     ///< the values clamped

 };  // end( class PackedArray )

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS TritArray ------------------------------*/
/*--------------------------------------------------------------------------*/

/// \f$ n \f$ values in \f$ \{ 0 , 1 , 2 \} \f$, five to a byte in base 3
class TritArray {

/*--------------------------------------------------------------------------*/
 public:

 /// the empty array
 TritArray( void ) = default;

 /// \f$ n \f$ values \f$ v \f$
 /// \pre \f$ v \le 2 \f$
 explicit TritArray( std::size_t n , unsigned v = 0 );

 /// the values `f( i )` for \f$ i \in [ 0 , n ) \f$, filled by blocks of
 /// whole bytes on the threads of `ex`
 ///
 /// In pseudocode (Breyer and Korf, beyond the book), by blocks of bytes
 /// as `PackedArray` does by blocks of words:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{build}( \mathit{ex} , n , f ) \cr
 /// 2 & \quad \textbf{foreach}\ \text{block } [ a , b ) \text{ of the }
 ///     \text{bytes, in parallel}\ \textbf{do} \cr
 /// 3 & \qquad \textbf{for}\ y \gets a\ \textbf{to}\ b - 1\ \textbf{do} \cr
 /// 4 & \qquad\quad x \gets 0 \cr
 /// 5 & \qquad\quad \textbf{for}\ j \gets \min\{ 5 , n - 5 y \} - 1
 ///     \ \textbf{downto}\ 0\ \textbf{do}\ x \gets 3 x + f( 5 y + j ) \cr
 /// 6 & \qquad\quad Y[ y ] \gets x
 /// \end{array}}
 /// \f]
 /// \pre `f( i )` is at most 2, and it may be called on any thread
 template< Executor E , class F >
 TritArray( E & ex , std::size_t n , F && f );

 /// the number of values, \f$ n \f$
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// whether there is no value
 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }

 /// the value \f$ i \f$, the digit \f$ i \bmod 5 \f$ of the byte
 /// \f$ \lfloor i / 5 \rfloor \f$, from the table of the digits of the 243
 /// bytes
 /// \pre \f$ i < n \f$
 [[nodiscard]] unsigned get( std::size_t i ) const;

 /// as `get( i )`
 /// \pre \f$ i < n \f$
 [[nodiscard]] unsigned operator[]( std::size_t i ) const {
  return( get( i ) );
  }

 /// the value \f$ i \f$ set to \f$ v \f$
 ///
 /// In pseudocode (Breyer and Korf, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{set}( i , v ) \cr
 /// 2 & \quad t \gets \mathrm{get}( i ) \cr
 /// 3 & \quad Y[ \lfloor i / 5 \rfloor ] \gets Y[ \lfloor i / 5 \rfloor ]
 ///     + ( v - t ) \, 3^{i \bmod 5}
 /// \end{array}}
 /// \f]
 /// \pre \f$ i < n \f$ and \f$ v \le 2 \f$
 /// \post `get( i )` is \f$ v \f$
 void set( std::size_t i , unsigned v );

 /// appends the value \f$ v \f$
 /// \pre \f$ v \le 2 \f$
 void push_back( unsigned v );

 /// the bytes, five values each, those past the last value 0
 [[nodiscard]] const std::vector< std::uint8_t > & data( void ) const {
  return( f_y );
  }

 /// the memory of the values, in bytes
 [[nodiscard]] std::size_t bytes( void ) const { return( f_y.size() ); }

 /// whether the two arrays have the same values
 friend bool operator==( const TritArray & a , const TritArray & b ) {
  return( ( a.f_n == b.f_n ) && ( a.f_y == b.f_y ) );
  }

/*--------------------------------------------------------------------------*/
 private:

 /// the five digits of each byte below 243, 1215 bytes
 static constexpr std::array< std::array< std::uint8_t , 5 > , 243 >
  DIGITS = [] {
   std::array< std::array< std::uint8_t , 5 > , 243 > d{};
   for( int b = 0 ; b < 243 ; ++b )
    for( int j = 0 , x = b ; j < 5 ; ++j , x /= 3 )
     d[ std::size_t( b ) ][ std::size_t( j ) ] = std::uint8_t( x % 3 );
   return( d );
   }();

 /// \f$ 3^j \f$, \f$ j < 5 \f$
 static constexpr std::uint8_t POW3[ 5 ] = { 1 , 3 , 9 , 27 , 81 };

 /// each byte below 243 (the last one alone past 1024 bytes), the digits
 /// past the last value 0
 [[nodiscard]] bool clean( void ) const;

 std::vector< std::uint8_t > f_y;   ///< the bytes
 std::size_t f_n = 0;               ///< the values

 };  // end( class TritArray )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/succinct/impl/PackedArray.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* PackedArray.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File PackedArray.h -------------------------*/
/*--------------------------------------------------------------------------*/
