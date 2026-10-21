/*--------------------------------------------------------------------------*/
/*---------------------------- File EliasFano.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The Elias-Fano representation of a non-decreasing sequence
 * \f$ x_0 \le \cdots \le x_{n-1} \f$ of integers in \f$ [ 0 , u ) \f$
 * (section 11.6 and 15.1.2 of the notes): each item is split into its
 * \f$ \ell = \lceil \log_2 ( u / n ) \rceil \f$ low bits, written one after
 * the other in L, and its high bits, the bucket \f$ x_i \gg \ell \f$,
 * written in H as the unary counts of the buckets (\f$ 1^c 0 \f$ for a
 * bucket of c items), so that the i-th 1 of H is at the place
 * \f$ ( x_i \gg \ell ) + i \f$. H has n 1s and at most n 0s, and the
 * sequence takes less than \f$ 2 n + n \lceil \log_2 ( u / n ) \rceil \f$
 * bits whatever its distribution (Theorem 11.1), within 2 bits per item of
 * the \f$ \log_2 \binom{u}{n} \f$ bits needed to tell n items of
 * \f$ [ 0 , u ) \f$ apart.
 *
 * With rank and select on H [see RankSelect.h] the sequence is not only
 * decoded but searched in place:
 *
 * - access( i ), \f$ x_i \f$ in O( 1 ) time, as \f$ ( select_1( i + 1 ) - i )
 *   \f$ followed by the i-th field of L;
 * - nextGEQ( x ), the place of the first item \f$ \ge x \f$, which is the
 *   rank of x (the items \f$ < x \f$) and gives the predecessor and the
 *   successor: \f$ select_0 \f$ of the bucket of x gives the first item of
 *   that bucket, and the scan stops at its end, in O( 1 + c ) time for a
 *   bucket of c items, expected O( 1 ) since \f$ 2^\ell \approx u / n \f$;
 *   past 8 items (the clusters of a sequence fill some buckets) the end of
 *   the bucket is found by \f$ select_0 \f$ too and its low bits, sorted,
 *   by a binary search, \f$ O( \log c ) \f$ time;
 * - viewed as the places of the 1s of a vector of bits B of u bits with n
 *   1s (section 15.1.2), access( i ) is \f$ select_1( i + 1 ) \f$ of B and
 *   nextGEQ( i ) is \f$ rank_1( i ) \f$ of B, in \f$ n ( 2 + \log ( u / n ) )
 *   + o( n ) \f$ bits rather than u + o( u ) (Theorem 15.5).
 *
 * The notes compute the rank of B by a binary search on select (Algorithm
 * 15.1), in O( log n ) time; the bucket found by \f$ select_0 \f$ makes it
 * O( 1 ) expected, the design of Vigna's quasi-succinct indices and of
 * Okanohara and Sadakane's sdarray. H is written with the 1s for the items
 * and the 0s for the buckets, as in chapter 11 (section 15.1.2 swaps them,
 * which only swaps \f$ select_0 \f$ and \f$ select_1 \f$). The
 * representation ignores the regularities of the sequence: the partitioned
 * Elias-Fano of Ottaviano and Venturini splits it into chunks, each one
 * coded at its own best, and comes within a few percent of the
 * interpolative code [see IntegerCodes.h] keeping the random access.
 *
 * The contracts [see Contract.h]: the sequence is non-decreasing (an audit)
 * and below u, and access() needs i < n; the audit of the construction is
 * that access() gives back each item, and the postcondition of nextGEQ()
 * is that the items before the place are \f$ < x \f$ and the one at it is
 * not. The functions are defined in EliasFano.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapters 11 and 15
 * \see P. Elias, Efficient storage and retrieval by content and address of
 *      static files, Journal of the ACM 21(2), 246-260, 1974,
 *      doi:10.1145/321812.321820
 * \see R. M. Fano, On the number of bits required to implement an
 *      associative memory, Memorandum 61, Computation Structures Group,
 *      MIT, 1971
 * \see D. Okanohara, K. Sadakane, Practical entropy-compressed rank/select
 *      dictionary, Proc. 9th ALENEX, 60-70, 2007,
 *      doi:10.1137/1.9781611972870.6
 * \see S. Vigna, Quasi-succinct indices, Proc. 6th WSDM, 83-92, 2013,
 *      doi:10.1145/2433396.2433409
 * \see G. Ottaviano, R. Venturini, Partitioned Elias-Fano indexes, Proc.
 *      37th SIGIR, 273-282, 2014, doi:10.1145/2600428.2609615
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

#ifndef __ADS_EliasFano
 #define __ADS_EliasFano
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/combinatorial/Bitwise.h"
#include "ADS/succinct/BitVector.h"
#include "ADS/succinct/RankSelect.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS EliasFano -----------------------------*/
/*--------------------------------------------------------------------------*/

/// a non-decreasing sequence of integers in the Elias-Fano representation
class EliasFano {

/*--------------------------------------------------------------------------*/
 public:

 EliasFano( void ) = default;

 /// the sequence x in the universe [ 0 , x.back() + 1 )
 /// \pre x is non-decreasing (an audit) and x.back() < 2^64 - 1, so that
 ///      the universe fits in 64 bits
 explicit EliasFano( const std::vector< std::uint64_t > & x );

 /// the sequence x in the universe [ 0 , u )
 /// \pre x is non-decreasing (an audit) and its items are below u
 EliasFano( const std::vector< std::uint64_t > & x , std::uint64_t u );

 [[nodiscard]] std::size_t size( void ) const;

 [[nodiscard]] bool empty( void ) const;

 /// the bound u of the items
 [[nodiscard]] std::uint64_t universe( void ) const;

 /// the item i
 /// \pre i < size()
 [[nodiscard]] std::uint64_t access( std::size_t i ) const;

 /// the item i
 /// \pre i < size()
 [[nodiscard]] std::uint64_t operator[]( std::size_t i ) const;

 /// the place of the first item >= x, size() if there is none; the number
 /// of items < x
 /// \post the items before it are < x, the one at it is >= x
 [[nodiscard]] std::size_t nextGEQ( std::uint64_t x ) const;

 /// whether x is an item
 [[nodiscard]] bool contains( std::uint64_t x ) const;

 /// the number of low bits of each item, ell
 [[nodiscard]] int lowWidth( void ) const;

 /// the low bits, L
 [[nodiscard]] const BitVector & low( void ) const;

 /// the high bits, H, with their directory
 [[nodiscard]] const RankSelect & high( void ) const;

 /// the bits of L and H, without the directory of H
 [[nodiscard]] std::size_t codeBits( void ) const;

 /// all the bits, with the directory of H
 [[nodiscard]] std::size_t sizeInBits( void ) const;

 /// the items, decoded in O( n ) time
 [[nodiscard]] std::vector< std::uint64_t > decode( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 using value_type = std::uint64_t;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = std::uint64_t;
 using const_reference = std::uint64_t;
 using const_iterator = SubscriptIterator< EliasFano , std::uint64_t >;
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

 /// the bucket of x, x >> ell
 [[nodiscard]] std::uint64_t bucket( std::uint64_t x ) const;

 std::size_t f_n = 0;
 std::uint64_t f_u = 0;
 int f_l = 0;                ///< the low bits of an item
 BitVector f_low;            ///< L
 RankSelect f_high;          ///< H

 };  // end( class EliasFano )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/succinct/impl/EliasFano.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* EliasFano.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File EliasFano.h --------------------------*/
/*--------------------------------------------------------------------------*/
