/*--------------------------------------------------------------------------*/
/*----------------------- File PartitionedEliasFano.h ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The partitioned Elias-Fano representation of Ottaviano and Venturini: a
 * strictly increasing sequence \f$ x_0 < \cdots < x_{n-1} \f$ of integers
 * of \f$ [ 0 , u ) \f$ cut into blocks of B = 128 items, each one coded
 * in the cheapest of three ways relative to the end of the block before,
 * so that a sequence with clusters (the lists of an inverted index) takes
 * far fewer bits than the \f$ 2 + \lceil \log_2 ( u / n ) \rceil \f$ per
 * item of Elias-Fano [see EliasFano.h], which ignores its regularities,
 * with the same access and nextGEQ.
 *
 * The block j holds the items of places \f$ [ j B , ( j + 1 ) B ) \f$,
 * m of them, relative to \f$ b_j = x_{j B - 1} + 1 \f$ (0 for the first
 * block): the values \f$ v = x - b_j \f$ are in \f$ [ 0 , u_j ) \f$ with
 * \f$ u_j = x_{\mathrm{last}} - b_j + 1 \f$, and the block is
 *
 * - full, with no bit, if \f$ u_j = m \f$ (every value is there);
 * - a vector of \f$ u_j \f$ bits, the bit v for the value v, if
 *   \f$ u_j \f$ is at most the bits of Elias-Fano;
 * - else Elias-Fano with \f$ \ell = \lfloor \log_2 ( u_j / m ) \rfloor \f$,
 *   in \f$ m \ell + m + \lfloor ( u_j - 1 ) / 2^\ell \rfloor + 1 \f$ bits;
 *
 * the kind is a function of \f$ ( m , u_j ) \f$. For each block a record of
 * fixed width holds the last item, the place of the code of the block and
 * its kind with \f$ \ell \f$ (8 bits, so that a query computes no
 * logarithm), and the last item of every 8-th block is kept apart for the
 * searches; so
 *
 * - access( i ) reads the records of the block of i and of the one before
 *   (next to each other) and selects the item in the code of the block, a
 *   scan of its few words (at most \f$ 2 B \f$ bits of high part, or a
 *   vector of \f$ u_j \le B ( \ell + 3 ) \f$ bits, since it is chosen
 *   only when not longer than Elias-Fano);
 * - nextGEQ( x ) finds the first block whose last item is at least x by a
 *   search of the items apart in the layout of Eytzinger (a complete
 *   binary tree by levels, the node t with the children 2t and 2t + 1,
 *   whose descent fetches ahead the nodes three levels down) and then by
 *   at most 8 records,
 *   and the item in that block by the high part (the 0 of the bucket of x)
 *   or by the sideways sums of the vector up to x.
 *
 * Ottaviano and Venturini also choose the ends of the blocks by an
 * approximate dynamic program on the cost of the codes, which gains a
 * further 10% or so on the lists of an index; the blocks here have the
 * fixed size of their uniform partition.
 *
 * The contracts [see Contract.h]: the sequence is strictly increasing (an
 * audit) and below u, and access() needs i < n; the audit of the
 * construction is that access() gives back each item, and the
 * postcondition of nextGEQ() is that the items before the place are
 * \f$ < x \f$ and the one at it is not. The functions are defined in
 * PartitionedEliasFano.tpp.
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
 *         Universita' di Pisa \n
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
/// as full, as a vector of bits or by Elias-Fano, with access and nextGEQ
class PartitionedEliasFano {

/*--------------------------------------------------------------------------*/
 public:

 PartitionedEliasFano( void ) = default;

 /// the sequence x, with the universe x.back() + 1
 explicit PartitionedEliasFano( std::span< const std::uint64_t > x );

 /// the sequence x in [ 0 , u )
 /// \pre x is strictly increasing, x.back() < u
 PartitionedEliasFano( std::span< const std::uint64_t > x , std::uint64_t u );

 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }

 [[nodiscard]] std::uint64_t universe( void ) const { return( f_u ); }

 /// the item i
 /// \pre i < size()
 [[nodiscard]] std::uint64_t access( std::size_t i ) const;

 [[nodiscard]] std::uint64_t operator[]( std::size_t i ) const {
  return( access( i ) );
  }

 /// the place of the first item >= x, size() if none
 [[nodiscard]] std::size_t nextGEQ( std::uint64_t x ) const;

 /// whether x is in the sequence
 [[nodiscard]] bool contains( std::uint64_t x ) const;

 /// the numbers of the full blocks, of those coded as vectors and of
 /// those coded by Elias-Fano
 [[nodiscard]] std::array< std::size_t , 3 > kinds( void ) const;

 /// the bits of the codes, of the records and of the records apart
 [[nodiscard]] std::size_t sizeInBits( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 using value_type = std::uint64_t;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = std::uint64_t;
 using const_reference = std::uint64_t;
 using const_iterator = SubscriptIterator< PartitionedEliasFano ,
                                           std::uint64_t >;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , 0 ) );
  }
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , f_n ) );
  }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 static constexpr std::size_t kB = 128;     ///< the items of a block
 static constexpr std::size_t kApart = 8;   ///< blocks between records apart

 enum Kind { FULL = 0 , VECTOR = 1 , ELIAS = 2 };

 /// the code of a block of m values in [ 0 , u ): its kind, its bits and
 /// the low bits of Elias-Fano
 struct Code {
  Kind kind;
  std::uint64_t bits;
  int ell;
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
