/*--------------------------------------------------------------------------*/
/*----------------------- File PartitionedEliasFano.h ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The partitioned Elias-Fano representation of Ottaviano and Venturini: a
 * strictly increasing sequence \f$ x_0 < \cdots < x_{n-1} \f$ of integers
 * of \f$ [ 0 , u ) \f$ cut into blocks of \f$ B = 128 \f$ items, each one
 * coded in the cheapest of three ways relative to the end of the block
 * before, so that a sequence with clusters (the lists of an inverted index)
 * takes far fewer bits than the \f$ 2 + \lceil \log_2( u / n ) \rceil \f$
 * per item of Elias-Fano [see `EliasFano.h`], which ignores its
 * regularities, with the same `access()` and `nextGEQ()`.
 *
 * The block \f$ j \f$ holds the items of places
 * \f$ [ j B , ( j + 1 ) B ) \f$, \f$ m \f$ of them, relative to
 * \f$ b_j = x_{j B - 1} + 1 \f$ (0 for the first block): the values
 * \f$ v = x - b_j \f$ are in \f$ [ 0 , u_j ) \f$ with
 * \f$ u_j = x_{\mathrm{last}} - b_j + 1 \f$, and the block is
 *
 * - full, with no bit, if \f$ u_j = m \f$ (every value is there);
 * - a vector of \f$ u_j \f$ bits, the bit \f$ v \f$ for the value
 *   \f$ v \f$, if \f$ u_j \f$ is at most the bits of Elias-Fano;
 * - else Elias-Fano with \f$ \ell = \lfloor \log_2( u_j / m ) \rfloor \f$,
 *   in \f$ m \ell + m + \lfloor ( u_j - 1 ) / 2^\ell \rfloor + 1 \f$ bits;
 *
 * the kind is a function of \f$ ( m , u_j ) \f$. For each block a record of
 * fixed width holds the last item, the place of the code of the block and
 * its kind with \f$ \ell \f$ (8 bits, so that a query computes no
 * logarithm), and the last item of every 8-th block is kept apart for the
 * searches; so
 *
 * - `access( i )` reads the records of the block of \f$ i \f$ and of the one
 *   before (next to each other) and selects the item in the code of the
 *   block, a scan of its few words (at most \f$ 2 B \f$ bits of high part,
 *   or a vector of \f$ u_j \le B ( \ell + 3 ) \f$ bits, since it is chosen
 *   only when not longer than Elias-Fano);
 * - `nextGEQ( x )` finds the first block whose last item is at least
 *   \f$ x \f$ by a search of the items apart in the layout of Eytzinger (a
 *   complete binary tree by levels, the node \f$ t \f$ with the children
 *   \f$ 2 t \f$ and \f$ 2 t + 1 \f$, whose descent fetches ahead the nodes
 *   three levels down) and then by at most 8 records, and the item in that
 *   block by the high part (the 0 of the bucket of \f$ x \f$) or by the
 *   sideways sums of the vector up to \f$ x \f$.
 *
 * Ottaviano and Venturini also choose the ends of the blocks by an
 * approximate dynamic program on the cost of the codes, which gains a
 * further 10% or so on the lists of an index; the blocks here have the
 * fixed size of their uniform partition.
 *
 * The contracts [see `Contract.h`]: the sequence is strictly increasing (an
 * audit) and below \f$ u \f$, and `access()` needs \f$ i < n \f$; the audit
 * of the construction is that `access()` gives back each item, and the
 * postcondition of `nextGEQ()` is that the items before the place are
 * \f$ < x \f$ and the one at it is not. The functions are defined in
 * `PartitionedEliasFano.tpp`.
 *
 * \see G. Ottaviano, R. Venturini, Partitioned Elias-Fano indexes, Proc.
 *      37th SIGIR, 273-282, 2014, doi:10.1145/2600428.2609615
 * \see S. Vigna, Quasi-succinct indices, Proc. 6th WSDM, 83-92, 2013,
 *      doi:10.1145/2433396.2433409
 * \see P.-V. Khuong, P. Morin, Array layouts for comparison-based
 *      searching, ACM Journal of Experimental Algorithmics 22, 1-39, 2017,
 *      doi:10.1145/3053370
 * \see G. E. Pibiri, R. Venturini, Techniques for inverted index
 *      compression, ACM Computing Surveys 53(6), 125, 2020,
 *      doi:10.1145/3415148
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

#ifndef __ADS_PartitionedEliasFano
 #define __ADS_PartitionedEliasFano
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/succinct/BitVector.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS PartitionedEliasFano -----------------------*/
/*--------------------------------------------------------------------------*/

/// a static strictly increasing sequence of integers in blocks, each coded
/// as full, as a vector of bits or by Elias-Fano, with `access()` and
/// `nextGEQ()`
class PartitionedEliasFano {

/*--------------------------------------------------------------------------*/
 public:

 /// the empty sequence
 PartitionedEliasFano( void ) = default;

 /// the sequence \f$ x \f$, with the universe \f$ x_{n-1} + 1 \f$
 /// \pre \f$ x \f$ is strictly increasing (an audit)
 explicit PartitionedEliasFano( std::span< const std::uint64_t > x );

 /// the sequence \f$ x \f$ in \f$ [ 0 , u ) \f$
 ///
 /// In pseudocode (Ottaviano and Venturini with the uniform partition,
 /// beyond the book), each block coded relative to the end of the one
 /// before:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{build}( x , u ) \cr
 /// 2 & \quad \textbf{foreach}\ \text{block } j \text{ of } m \le 128
 ///     \text{ items}\ \textbf{do} \cr
 /// 3 & \qquad b_j \gets x_{128 j - 1} + 1\ \text{(0 for } j = 0
 ///     \text{)};\ u_j \gets x_{\mathrm{last}} - b_j + 1 \cr
 /// 4 & \qquad \ell \gets \lfloor \log_2( u_j / m ) \rfloor;\ e \gets m \ell
 ///     + m + \lfloor ( u_j - 1 ) / 2^\ell \rfloor + 1 \cr
 /// 5 & \qquad \textbf{if}\ u_j = m\ \textbf{then}\ \text{no code (full)} \cr
 /// 6 & \qquad \textbf{else if}\ u_j \le e\ \textbf{then}\ \text{the }
 ///     u_j \text{ bits with a 1 at each } x - b_j \cr
 /// 7 & \qquad \textbf{else}\ \text{the Elias-Fano code of the values } x -
 ///     b_j \text{ with } \ell \text{ low bits} \cr
 /// 8 & \qquad \text{the record of } j \text{: its last item, the place of }
 ///     \text{its code, its kind and } \ell \cr
 /// 9 & \quad \text{the last items of the blocks } 0 , 8 , 16 , \ldots
 ///     \text{ in the layout of Eytzinger}
 /// \end{array}}
 /// \f]
 /// \pre \f$ x \f$ is strictly increasing (an audit),
 ///      \f$ x_{n-1} < u \f$
 PartitionedEliasFano( std::span< const std::uint64_t > x , std::uint64_t u );

 /// the number of items, \f$ n \f$
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// whether there is no item
 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }

 /// the bound \f$ u \f$ of the items
 [[nodiscard]] std::uint64_t universe( void ) const { return( f_u ); }

 /// the item \f$ i \f$
 ///
 /// In pseudocode (Ottaviano and Venturini, beyond the book), the block
 /// \f$ j \f$ of the item \f$ i \f$, its \f$ m \f$ items, its code from the
 /// place \f$ p \f$ and \f$ k \f$ the rank of \f$ i \f$ in it:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{access}( i ) \cr
 /// 2 & \quad j \gets \lfloor i / 128 \rfloor;\ k \gets i \bmod 128;\ b
 ///     \gets \text{the last item of } j - 1 \text{, plus 1} \cr
 /// 3 & \quad \textbf{if}\ j \text{ is full }\ \textbf{then return}\ b + k \cr
 /// 4 & \quad \textbf{if}\ j \text{ is a vector }\ \textbf{then return}\ b +
 ///     ( \mathrm{select}_1( p , k ) - p ) \cr
 /// 5 & \quad h \gets p + m \ell\quad \triangleright\ \text{the start of the }
 ///     \text{high part} \cr
 /// 6 & \quad \textbf{return}\ b + ( \mathrm{select}_1( h , k ) - h - k )
 ///     \cdot 2^\ell + \text{the low bits of } k
 /// \end{array}}
 /// \f]
 /// \pre \f$ i < \f$ `size()`
 [[nodiscard]] std::uint64_t access( std::size_t i ) const;

 /// the item \f$ i \f$, as `access()`
 /// \pre \f$ i < \f$ `size()`
 [[nodiscard]] std::uint64_t operator[]( std::size_t i ) const {
  return( access( i ) );
  }

 /// the place of the first item \f$ \ge x \f$, `size()` if none
 ///
 /// In pseudocode (Ottaviano and Venturini, with the search of Khuong and
 /// Morin, beyond the book), \f$ E \f$ the items apart in the layout of
 /// Eytzinger, from 1, and \f$ \mathrm{last}( j ) \f$ the last item of the
 /// block \f$ j \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{nextGEQ}( x ) \cr
 /// 2 & \quad \textbf{if}\ n = 0\ \textbf{or}\ x > \text{the last item }
 ///     \ \textbf{then return}\ n \cr
 /// 3 & \quad t \gets 1;\ \textbf{while}\ t \le | E |\ \textbf{do}\ t \gets 2
 ///     t + [ E[ t ] < x ] \cr
 /// 4 & \quad a \gets \text{the rank of the first item apart} \ge x \text{,}
 ///     \text{ from the trailing 1s of } t \cr
 /// 5 & \quad j \gets 8 ( a - 1 ) + 1\ \text{(0 if } a = 0 \text{)};
 ///     \ \textbf{while}\ \mathrm{last}( j ) < x\ \textbf{do}\ j \gets j + 1
 ///     \cr
 /// 6 & \quad v \gets \max\{ 0 , x - b_j \} \cr
 /// 7 & \quad \textbf{if}\ j \text{ is full }\ \textbf{then}\ k \gets v \cr
 /// 8 & \quad \textbf{else if}\ j \text{ is a vector }\ \textbf{then}\ k
 ///     \gets \text{the 1s of its first } v \text{ bits} \cr
 /// 9 & \quad \textbf{else}\ k \gets \text{the place of } v \text{ in its }
 ///     \text{code, as in } \mathrm{nextGEQ} \text{ of Elias-Fano} \cr
 /// 10 & \quad \textbf{return}\ 128 j + k
 /// \end{array}}
 /// \f]
 /// \post the items before it are \f$ < x \f$, the one at it is
 ///       \f$ \ge x \f$
 [[nodiscard]] std::size_t nextGEQ( std::uint64_t x ) const;

 /// whether \f$ x \f$ is in the sequence, by `nextGEQ()`
 [[nodiscard]] bool contains( std::uint64_t x ) const;

 /// the numbers of the full blocks, of those coded as vectors and of
 /// those coded by Elias-Fano
 [[nodiscard]] std::array< std::size_t , 3 > kinds( void ) const;

 /// the bits of the codes, of the records and of the records apart
 [[nodiscard]] std::size_t sizeInBits( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 /// the type of an item
 using value_type = std::uint64_t;
 /// the type of a size
 using size_type = std::size_t;
 /// the type of the distance of two places
 using difference_type = std::ptrdiff_t;
 /// an item, returned by value
 using reference = std::uint64_t;
 /// an item, returned by value
 using const_reference = std::uint64_t;
 /// the constant iterator, random access
 using const_iterator = SubscriptIterator< PartitionedEliasFano ,
                                           std::uint64_t >;
 /// the same as `const_iterator`
 using iterator = const_iterator;

 /// the iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , 0 ) );
  }
 /// the iterator past the last item
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , f_n ) );
  }
 /// the same as `begin()`
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// the same as `end()`
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 static constexpr std::size_t kB = 128;     ///< the items of a block
 static constexpr std::size_t kApart = 8;   ///< blocks between records apart

 /// the encoding of a block
 enum Kind {
  FULL = 0 ,     ///< all the values of the range, no bits
  VECTOR = 1 ,   ///< a vector of bits over the range
  ELIAS = 2      ///< the code of Elias and Fano
  };

 /// the code of a block of m values in [ 0 , u ): its kind, its bits and
 /// the low bits of Elias-Fano
 struct Code {
  Kind kind;            ///< the encoding chosen
  std::uint64_t bits;   ///< the bits of the code
  int ell;              ///< \f$ \ell \f$, the low bits of Elias-Fano
  };
 [[nodiscard]] static Code code( std::uint64_t m , std::uint64_t u );

 /// the last item of the block j, the place of its code, and its kind
 /// with its low bits ( ell << 2 | kind )
 [[nodiscard]] std::uint64_t last( std::size_t j ) const {
  return( f_rec.get( j * std::size_t( f_wr ) , f_wl ) );
  }
 [[nodiscard]] std::size_t where( std::size_t j ) const {
  return( std::size_t( f_rec.get( j * std::size_t( f_wr ) +
                                  std::size_t( f_wl ) , f_wo ) ) );
  }
 [[nodiscard]] unsigned form( std::size_t j ) const {
  return( unsigned( f_rec.get( j * std::size_t( f_wr ) +
                               std::size_t( f_wl + f_wo ) , 8 ) ) );
  }

 /// the place of the k-th 1 (from 0) in the bits [ p , p + len ) of the
 /// codes, and the place of the k-th 0
 [[nodiscard]] std::size_t select1( std::size_t p , std::size_t k ) const;
 [[nodiscard]] std::size_t select0( std::size_t p , std::size_t k ) const;

 /// the 1s in the bits [ p , q ) of the codes
 [[nodiscard]] std::size_t ones( std::size_t p , std::size_t q ) const;

 std::size_t f_n = 0;
 std::uint64_t f_u = 0;
 BitVector f_codes;                     ///< the codes of the blocks
 BitVector f_rec;                       ///< the records of the blocks
 int f_wl = 1 , f_wo = 1 , f_wr = 10;   ///< a last, a place, a record
 std::vector< std::uint64_t > f_apart;  ///< the last of every 8-th block
 std::vector< std::uint64_t > f_eyt;    ///< them by Eytzinger, from 1
 std::vector< std::uint32_t > f_rank;   ///< the rank of each one there

 };  // end( class PartitionedEliasFano )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/succinct/impl/PartitionedEliasFano.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* PartitionedEliasFano.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File PartitionedEliasFano.h --------------------*/
/*--------------------------------------------------------------------------*/
