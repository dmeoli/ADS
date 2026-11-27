/*--------------------------------------------------------------------------*/
/*------------------------------ File FSST.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The Fast Static Symbol Table (FSST) of Boncz, Neumann and Leis for
 * columns of short strings: a table of up to 255 symbols of 1 to 8 bytes,
 * built once from a sample of the strings, replaces each symbol of a
 * string by its code of one byte, and the code 255, the escape, is followed
 * by a byte that no symbol covers; each string is coded by itself, so that
 * any one of them is decoded alone, with no block around it to decode.
 *
 * The decoder is a loop over the codes that copies 8 bytes of the symbol of
 * each code and moves by its length, from two tables of 255 entries, with
 * no branch but the test of the escape (made on 4 codes at a time). The
 * encoder takes at each place the longest symbol it finds, by two lookups:
 * the symbols of 3 bytes or more are in a hash table of 1024 slots by
 * their first 3 bytes, one symbol per slot [see `hashWord()` of
 * `FastHash.h`], and a table of \f$ 2^{16} \f$ entries gives for the next 2
 * bytes the code of the symbol of 2 bytes, or else of the symbol of 1 byte,
 * or else the escape. The greedy choice is therefore not always the longest
 * match (two symbols of 3 bytes or more with the same first 3 bytes, or in
 * the same slot, do not both enter the table), which the construction takes
 * into account. As in the encoder of the authors, the byte least frequent
 * in the sample, the terminator, enters no symbol of 2 bytes or more, so
 * that the last 7 bytes of a string are read as one word followed by the
 * terminator, with no symbol able to run past its end, and coded by the
 * same step as the others, with no test of the bytes left.
 *
 * The table is built by 5 generations on a sample of about 16 KB of the
 * strings (pieces of 512 bytes of strings drawn at random, or all the
 * strings if they are fewer): each generation codes a fraction of the
 * sample (8/128, 38/128, 68/128, 98/128 and all of it) with the table of
 * the generation before (empty at the first), and counts each code
 * \f$ c_1 \f$ and each pair of consecutive codes \f$ c_1 c_2 \f$, a byte
 * escaped counting as a code of its own; the candidates are the symbols
 * of the codes, of gain \f$ \mathrm{count}( c_1 ) \, | c_1 | \f$ (8 times
 * as much for a byte, which saves an escape), and their concatenations of
 * up to 8 bytes, of gain \f$ \mathrm{count}( c_1 c_2 ) \, | c_1 c_2 | \f$,
 * and the next table takes the 255 of largest gain that fit in its
 * lookups. A symbol that the greedy encoder uses often thereby grows by the
 * symbol that follows it, and one that it no longer uses leaves the table.
 *
 * The format of a string is its codes and escaped bytes; that of a table
 * (`serialize()`) is the number \f$ k \f$ of its symbols in a byte, their
 * \f$ k \f$ lengths in a byte each and their bytes. `FSSTColumn` keeps the
 * codes of all the strings one after the other, with the place where each
 * one begins.
 *
 * The contracts [see `Contract.h`]: the strings of a range stay alive while
 * a table or a column is built from them, a code to decode ends with no escape
 * left open and gives a code of the table, a table to deserialize has at
 * most 255 symbols of 1 to 8 bytes, and the decoders get room for
 * \f$ 8 \f$ bytes per code. The functions are defined in `FSST.tpp`.
 *
 * \see P. Boncz, T. Neumann, V. Leis, FSST: fast random access string
 *      compression, Proceedings of the VLDB Endowment 13(11), 2649-2661,
 *      2020, doi:10.14778/3407790.3407851
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

#ifndef __ADS_FSST
 #define __ADS_FSST
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/FastHash.h"

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS FSSTTable -----------------------------*/
/*--------------------------------------------------------------------------*/

/// a static table of up to 255 symbols of 1 to 8 bytes, with its encoder
/// and its decoder
class FSSTTable
{

/*--------------------------------------------------------------------------*/
 public:
 /// the code that escapes the byte after it
 static constexpr std::uint8_t ESCAPE = 255;

 /// the table with no symbol, which escapes every byte
 FSSTTable( void );

 /// the table built from a sample of the strings of \f$ S \f$
 ///
 /// In pseudocode (FSST of Boncz, Neumann and Leis, beyond the book), a
 /// byte escaped counting as a code of its own:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{FSSTTable}( S ) \cr
 /// 2 & \quad Z \gets \text{about 16 KB of pieces of the strings of } S
 ///     ;\ T \gets \emptyset \cr
 /// 3 & \quad \textbf{for}\ g \gets 1\ \textbf{to}\ 5\ \textbf{do} \cr
 /// 4 & \qquad \textbf{foreach}\ \text{string of a fraction } ( 30 g -
 ///     22 ) / 128 \text{ of } Z\ \textbf{do} \cr
 /// 5 & \qquad\quad \text{code it greedily by } T\text{, counting each}
 ///     \text{ code } c_1 \text{ and each pair } c_1 c_2 \cr
 /// 6 & \qquad K \gets \{ ( c_1 , \mathrm{count}( c_1 ) | c_1 | ) \} \cup
 ///     \{ ( c_1 c_2 , \mathrm{count}( c_1 c_2 ) | c_1 c_2 | ) : | c_1 | <
 ///     8 \} \cr
 /// 7 & \qquad T \gets \text{the 255 symbols of } K \text{ of largest
 ///     gain that fit} \cr
 /// 8 & \quad \textbf{return}\ T
 /// \end{array}}
 /// \f]
 /// where a concatenation is cut at 8 bytes, the gains of the same symbol
 /// add up, and a symbol of one byte has 8 times its gain.
 template< std::ranges::forward_range R >
  requires std::convertible_to< std::ranges::range_reference_t< R > ,
				std::string_view >
 explicit FSSTTable( const R & S );

 /// the number of the symbols
 [[nodiscard]] std::size_t symbols( void ) const { return( f_n ); }

 /// the symbol of the code \f$ c \f$
 /// \pre \f$ c < \f$ `symbols()`
 [[nodiscard]] std::string symbol( std::size_t c ) const;

 /// the codes of \f$ s \f$ appended to `out`
 ///
 /// In pseudocode (FSST, beyond the book), \f$ H \f$ the hash table of the
 /// symbols of 3 bytes or more and \f$ Q \f$ the table of the pairs of
 /// bytes:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{encode}( s ) \cr
 /// 2 & \quad i \gets 0 \cr
 /// 3 & \quad \textbf{while}\ i < | s |\ \textbf{do} \cr
 /// 4 & \qquad w \gets \text{the next 8 bytes of } s \text{ (fewer at its}
 ///     \text{ end)};\ ( c , \ell ) \gets H[ h( w_{0..2} ) ] \cr
 /// 5 & \qquad \textbf{if}\ \ell < 3\ \textbf{or}\ \text{the symbol of } c
 ///     \text{ is not a prefix of } w\ \textbf{then}\ ( c , \ell ) \gets Q[
 ///     w_{0..1} ] \cr
 /// 6 & \qquad \text{append } c;\ \textbf{if}\ c = 255\ \textbf{then}
 ///     \ \text{append } w_0 \cr
 /// 7 & \qquad i \gets i + \ell
 /// \end{array}}
 /// \f]
 /// \post `decode()` of the codes appended is \f$ s \f$ (audit)
 void encode( std::string_view s , std::vector< std::uint8_t > & out ) const;

 /// the codes of \f$ s \f$, by `encode()` above
 [[nodiscard]] std::vector< std::uint8_t > encode( std::string_view s ) const;

 /// the bytes of the codes `code` written at `out`, which has room for
 /// \f$ 8 \f$ bytes per code; their number
 ///
 /// In pseudocode (FSST, beyond the book), \f$ \sigma_c \f$ and
 /// \f$ \ell_c \f$ the 8 bytes of the symbol of \f$ c \f$, padded, and its
 /// length:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{decode}( y , \mathit{out} ) \cr
 /// 2 & \quad j \gets 0 \cr
 /// 3 & \quad \textbf{foreach}\ \text{code } c \text{ of } y\ \textbf{do}
 ///     \cr
 /// 4 & \qquad \textbf{if}\ c \ne 255\ \textbf{then}\ \mathit{out}[ j .. j
 ///     + 7 ] \gets \sigma_c;\ j \gets j + \ell_c \cr
 /// 5 & \qquad \textbf{else}\ \mathit{out}[ j ] \gets \text{the next byte}
 ///     \text{ of } y;\ j \gets j + 1 \cr
 /// 6 & \quad \textbf{return}\ j
 /// \end{array}}
 /// \f]
 /// \pre `code` is a code of the table, with no escape at its end
 std::size_t decode( std::span< const std::uint8_t > code , char * out ) const;

 /// the string of the codes `code`, by `decode()` above
 /// \pre `code` is a code of the table, with no escape at its end
 [[nodiscard]] std::string
 decode( std::span< const std::uint8_t > code ) const;

 /// the table as bytes: the number of the symbols, their lengths and their
 /// bytes
 [[nodiscard]] std::vector< std::uint8_t > serialize( void ) const;

 /// the table of the bytes of `serialize()`
 /// \pre `bytes` is the serialization of a table
 [[nodiscard]] static FSSTTable
 deserialize( std::span< const std::uint8_t > bytes );

/*--------------------------------------------------------------------------*/
 private:
 friend class FSSTColumn;

 /// the slots of the hash table
 static constexpr std::size_t kHash = 1024;

 /// a symbol of 3 bytes or more in the hash table, its bytes kept by the
 /// shift of the mask of a word; len 0 if none, with a symbol that no word
 /// masked by the shift equals
 struct Slot
 {
  std::uint64_t sym = 2;
  std::uint8_t len = 0 , code = 0 , shift = 63;
  };

 /// the slot of the first 3 bytes of the word w
 static std::size_t slot( std::uint64_t w ) {
  return( std::size_t( hashWord( w & 0xFFFFFF ) ) & ( kHash - 1 ) );
  }

 /// the table with no symbol and no lookup of the pairs of bytes yet
 explicit FSSTTable( std::nullptr_t );

 /// the code and the length of the symbol that the encoder takes at the
 /// word w of the r bytes left, r > 0, as ( len << 8 ) | code, from the
 /// table of the pairs of bytes if Done, else from the symbols of 2 bytes
 /// in it and those of 1 byte (while the table is built)
 template< bool Done = true >
 std::uint16_t match( std::uint64_t w , std::size_t r ) const;

 /// the codes of the n bytes from p written at out, which has room for
 /// 2 n bytes; the end of the codes
 std::uint8_t * encode( const char * p , std::size_t n ,
			std::uint8_t * out ) const;

 /// whether the symbol of the bytes sym, len bytes, enters the lookups,
 /// and enters it with the next code if it does
 bool add( std::uint64_t sym , unsigned len );

 /// the table of the pairs of bytes from the symbols of 1 and 2 bytes,
 /// and the terminator
 void finish( void );

 /// the table of the strings of the sample Z
 void build( const std::vector< std::string_view > & Z );

 std::size_t f_n = 0;                        ///< the symbols
 std::array< std::uint64_t , 255 > f_sym{};   ///< their bytes, padded with 0
 std::array< std::uint8_t , 255 > f_len{};    ///< their lengths
 std::vector< Slot > f_hash;                 ///< the symbols of 3+ bytes
 std::vector< std::uint16_t > f_pair;        ///< the codes of 2 bytes
 std::array< std::uint16_t , 256 > f_byte{};  ///< the codes of 1 byte
 std::uint16_t f_term = 256;  ///< a byte of no symbol of 2+ bytes, or 256

 };  // end( class FSSTTable )

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS FSSTColumn ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a static column of strings, each coded by itself by an `FSSTTable`
/// built from them, with the access to any one of them
class FSSTColumn
{

/*--------------------------------------------------------------------------*/
 public:
 /// the empty column
 FSSTColumn( void ) = default;

 /// the column of the strings of \f$ S \f$, by the table built from them
 /// \post each string is the one of \f$ S \f$ at its place (audit)
 template< std::ranges::forward_range R >
  requires std::convertible_to< std::ranges::range_reference_t< R > ,
				std::string_view >
 explicit FSSTColumn( const R & S );

 /// the column of the strings of \f$ S \f$ in place of the strings it
 /// held, in the room that it already has when it suffices (that of the
 /// codes is kept at 2 bytes per byte of the strings, the most that they
 /// take)
 /// \post each string is the one of \f$ S \f$ at its place (audit)
 template< std::ranges::forward_range R >
  requires std::convertible_to< std::ranges::range_reference_t< R > ,
				std::string_view >
 void assign( const R & S );

 /// the number of the strings
 [[nodiscard]] std::size_t size( void ) const {
  return( f_at.empty() ? 0 : f_at.size() - 1 );
  }

 /// whether there is no string
 [[nodiscard]] bool empty( void ) const { return( size() == 0 ); }

 /// the codes of the string \f$ i \f$
 /// \pre \f$ i < \f$ `size()`
 [[nodiscard]] std::span< const std::uint8_t > code( std::size_t i ) const;

 /// the string \f$ i \f$ written at `out`, which has room for
 /// `bound( i )` bytes; its length
 /// \pre \f$ i < \f$ `size()`
 std::size_t decode( std::size_t i , char * out ) const {
  return( f_table.decode( code( i ) , out ) );
  }

 /// the room that `decode( i , out )` needs, 8 bytes per code
 /// \pre \f$ i < \f$ `size()`
 [[nodiscard]] std::size_t bound( std::size_t i ) const {
  return( 8 * code( i ).size() );
  }

 /// the string \f$ i \f$
 /// \pre \f$ i < \f$ `size()`
 [[nodiscard]] std::string operator[]( std::size_t i ) const {
  return( f_table.decode( code( i ) ) );
  }

 /// the table of the column
 [[nodiscard]] const FSSTTable & table( void ) const { return( f_table ); }

 /// the bytes of the codes of all the strings
 [[nodiscard]] std::size_t codeBytes( void ) const {
  return( f_code.size() );
  }

 /// the bytes of the codes, of the places of the strings and of the table
 [[nodiscard]] std::size_t sizeInBytes( void ) const;

/*--------------------------------------------------------------------------*/
 private:
 /// the codes of the strings of S written from o, and their places in
 /// f_at, which has room for them; the bytes of the codes
 template< std::ranges::forward_range R >
 std::size_t encodeAll( const R & S , std::uint8_t * o );

 /// whether the strings of the column are those of S
 template< std::ranges::forward_range R >
 bool holds( const R & S ) const;

 FSSTTable f_table;                   ///< the table
 std::vector< std::uint8_t > f_code;  ///< the codes, one string after another
 std::vector< std::size_t > f_at;  ///< where each string begins, and the end

 };  // end( class FSSTColumn )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/compression/impl/FSST.tpp"

/*--------------------------------------------------------------------------*/

#endif /* FSST.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File FSST.h -----------------------------*/
/*--------------------------------------------------------------------------*/
