/*--------------------------------------------------------------------------*/
/*-------------------------- File RoaringBitmap.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The compressed bitmap of Chambi, Lemire, Kaser and Godin (Roaring),
 * beyond the book: a SET of integers of 32 bits [see ADT.h], the engineered
 * counterpart of the boolean vector of section 8.1 [see BoolSet.h] for
 * universes too large for a bit per integer and sets too dense for a sorted
 * list [see VectorIntersection.h].
 *
 * The universe is cut into chunks of \f$ 2^{16} \f$ integers by their top
 * 16 bits, and each chunk that holds some item is a container of the low
 * 16 bits of its items, kept in a vector sorted by the top bits:
 *
 * - an array container, the sorted array of the low bits, when the chunk
 *   holds at most 4096 items (2 bytes per item, at most 8 KB);
 * - a bitmap container, the \f$ 2^{16} \f$ bits of the chunk in 1024 words
 *   (8 KB), when it holds more.
 *
 * So no container takes more than 8 KB, and a set of n items takes at most
 * about 2 n bytes and at most the bits of its universe, whichever is less.
 * A search, an insertion and a removal find the container by a binary
 * search with no branch on the chunks, kept apart in an array of their
 * own, \f$ O( \log c ) \f$ for c containers, and then search an array in
 * the same way (or test a bit), and shift it (at most 4096 items, a few
 * lines of cache) or set a bit; an array that passes 4096 items becomes a
 * bitmap and a bitmap that falls to 4096 an array. The operations of two
 * sets merge the vectors of the containers by their chunks and combine the
 * containers of the same chunk:
 *
 * - two bitmaps word by word, the bits of the result counted in the same
 *   pass (4 words at a time with AVX2 [see BoolSet.h]), a result of at
 *   most 4096 items then written as an array;
 * - a bitmap and an array by testing, setting, clearing or flipping the
 *   bits of the array in a copy of the bitmap;
 * - two arrays, with SSE4.2, by blocks of 8 items: for the intersection
 *   and the difference, a block of each array compared all against all by
 *   one comparison of strings (the method of Schlegel, Willhalm and Lehner),
 *   the items of the block of the first array found, or those not found in
 *   any block of the second one, packed by a shuffle and stored, and the
 *   block that ends first moving on; for the union, the two blocks merged by
 *   the network of minima and maxima of Inoue and Taura, the 8 smallest
 *   written but those equal to the one before them, and the next block
 *   taken from the array with the smaller head; for the symmetric
 *   difference, the union of the two differences (by a merge with no branch
 *   on short arrays); a union or a symmetric difference of more than 4096
 *   items becoming a bitmap.

 * Compared with the literature: Roaring adds containers of runs (the
 * intervals of consecutive integers, for sets with long runs) and the
 * "lazy" unions of many bitmaps whose counts are computed at the end; the
 * bitmaps of the other families (WAH, Concise, EWAH) compress runs of words
 * and are slower to combine.
 *
 * rank( x ), the items smaller than x, sums the counts of the containers
 * before that of x and adds the place of x in it, \f$ O( c + 4096 ) \f$;
 * select( i ) finds the container of the rank i by the same counts and the
 * item in it; lowerBound( x ) starts an iterator at the first item not
 * smaller than x.
 *
 * The iterators visit the items in increasing order; an insertion or a
 * removal invalidates them. The contracts [see Contract.h]: after insert()
 * the item is there and after remove() it is not; build() needs strictly
 * increasing items, an audit; the invariant is that the containers are
 * sorted by their chunks, not empty, arrays of at most 4096 sorted items
 * and bitmaps of more, with their counts right, an O( n ) check at the
 * level 3 on the sets of up to 4096 items. The operations are defined in
 * RoaringBitmap.tpp.
 *
 * \see S. Chambi, D. Lemire, O. Kaser, R. Godin, Better bitmap performance
 *      with Roaring bitmaps, Software: Practice and Experience 46(5),
 *      709-719, 2016, doi:10.1002/spe.2325
 * \see D. Lemire, O. Kaser, N. Kurz, L. Deri, C. O'Hara, F. Saint-Jacques,
 *      G. Ssi-Yan-Kai, Roaring bitmaps: implementation of an optimized
 *      software library, Software: Practice and Experience 48(4), 867-895,
 *      2018, doi:10.1002/spe.2560
 * \see B. Schlegel, T. Willhalm, W. Lehner, Fast sorted-set intersection
 *      using SIMD instructions, Proceedings of ADMS 2011, 1-8, 2011
 * \see H. Inoue, K. Taura, SIMD- and cache-friendly algorithm for sorting
 *      an array of structures, Proceedings of the VLDB Endowment 8(11),
 *      1274-1285, 2015, doi:10.14778/2809974.2809988
 * \see W. Muła, N. Kurz, D. Lemire, Faster population counts using AVX2
 *      instructions, The Computer Journal 61(1), 2018,
 *      doi:10.1093/comjnl/bxx046
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

#ifndef __ADS_RoaringBitmap
 #define __ADS_RoaringBitmap
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/set/BoolSet.h"

#include <cstddef>
#include <bit>
#include <cstdint>
#include <iterator>
#include <memory>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS RoaringBitmap ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a set of integers of 32 bits as a Roaring bitmap
class RoaringBitmap {

/*--------------------------------------------------------------------------*/
 public:

 using Item = std::uint32_t;   ///< the type of the items

 /// the items of an array container at most
 static constexpr std::uint32_t ARRAY = 4096;

 RoaringBitmap( void ) = default;

 RoaringBitmap( const RoaringBitmap & o );

 RoaringBitmap( RoaringBitmap && o ) noexcept = default;

 RoaringBitmap & operator=( RoaringBitmap o ) noexcept;

 /// the set of the n strictly increasing items of a
 /// \pre a strictly increasing (an audit)
 RoaringBitmap( const std::uint32_t * a , std::size_t n );

 [[nodiscard]] bool contains( std::uint32_t x ) const;

 /// adds x; true if it was not there
 bool insert( std::uint32_t x );

 /// removes x; true if it was there
 bool remove( std::uint32_t x );

 [[nodiscard]] std::size_t size( void ) const;

 [[nodiscard]] bool empty( void ) const;

 void clear( void );

 void swap( RoaringBitmap & o ) noexcept;

 /// the containers, and those that are bitmaps
 [[nodiscard]] std::size_t containers( void ) const;
 [[nodiscard]] std::size_t bitmaps( void ) const;

 /// the bytes of the containers
 [[nodiscard]] std::size_t bytes( void ) const;

 /// f( x ) on the items in increasing order
 template< class F >
 void forEach( F && f ) const;

 /// the items in increasing order
 [[nodiscard]] std::vector< std::uint32_t > items( void ) const;

 /// the items smaller than x: the counts of the containers before that of
 /// x, O( c ), and the place of x in it
 [[nodiscard]] std::size_t rank( std::uint32_t x ) const;

 friend RoaringBitmap setUnion( const RoaringBitmap & A ,
                                const RoaringBitmap & B );
 friend RoaringBitmap setIntersection( const RoaringBitmap & A ,
                                       const RoaringBitmap & B );
 friend RoaringBitmap setDifference( const RoaringBitmap & A ,
                                     const RoaringBitmap & B );
 friend RoaringBitmap setSymmetricDifference( const RoaringBitmap & A ,
                                              const RoaringBitmap & B );

/*--------------------------------------------------------------------------*/
 private:

 static constexpr std::size_t WORDS = 1024;   ///< the words of a bitmap

 /// a container: the low bits of its items, sorted, or its bitmap
 struct Block {
  std::uint16_t key = 0;                 ///< the top bits of the chunk
  std::uint32_t card = 0;                ///< the items
  std::vector< std::uint16_t , detail::UninitAllocator< std::uint16_t > > a;
                                         ///< the array, if not a bitmap
  std::unique_ptr< std::uint64_t[] > w;  ///< the bitmap, if one

  [[nodiscard]] bool bits( void ) const { return( w != nullptr ); }
  };

 enum class Op { AND , OR , ANDNOT , XOR };

 template< Op O >
 static RoaringBitmap combine( const RoaringBitmap & A ,
                               const RoaringBitmap & B );

 template< Op O >
 static Block combine( const Block & a , const Block & b );

 static Block copy( const Block & b );

 static void toBitmap( Block & b );

 static void toArray( Block & b );

 static Block fromWords( std::uint16_t key ,
                         std::unique_ptr< std::uint64_t[] > w ,
                         std::uint32_t card );

 [[nodiscard]] std::size_t find( std::uint16_t key ) const;

 [[nodiscard]] bool valid( void ) const;

 /// the chunks of the containers, which a search reads
 void rekey( void );

 std::vector< Block > f_B;              ///< the containers by their chunks
 std::vector< std::uint16_t > f_K;      ///< their chunks, apart
 std::size_t f_size = 0;                ///< the items

/*------------------------- THE STANDARD INTERFACE -------------------------*/
 public:

 // a constant view of the items in increasing order, forward

 class const_iterator {
  public:
  using value_type = std::uint32_t;
  using reference = std::uint32_t;
  using difference_type = std::ptrdiff_t;
  using iterator_concept = std::forward_iterator_tag;
  using iterator_category = std::input_iterator_tag;

  const_iterator( void ) = default;

  reference operator*( void ) const { return( f_x ); }

  const_iterator & operator++( void ) {
   if( f_word ) {                           // the next bit of the word
    f_word &= f_word - 1;
    if( f_word ) {
     f_x = f_base + std::uint32_t( std::countr_zero( f_word ) );
     return( *this );
     }
    }
   ++f_i;
   settle();
   return( *this );
   }
  const_iterator operator++( int ) { auto t = *this; ++*this; return( t ); }

  bool operator==( const const_iterator & o ) const {
   return( ( f_b == o.f_b ) && ( f_i == o.f_i ) && ( f_x == o.f_x ) );
   }

  private:
  friend class RoaringBitmap;

  const_iterator( const RoaringBitmap * s , std::size_t b );

  void settle( void );

  const RoaringBitmap * f_s = nullptr;  ///< the set
  std::size_t f_b = 0;                  ///< the container
  std::size_t f_i = 0;                  ///< the place, or the word
  std::uint64_t f_word = 0;             ///< the bits left of the word
  std::uint32_t f_base = 0;             ///< the item of its bit 0
  std::uint32_t f_x = 0;                ///< the item

 };  // end( class const_iterator )

 using value_type = std::uint32_t;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = std::uint32_t;
 using const_reference = std::uint32_t;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , 0 ) );
  }
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , f_B.size() ) );
  }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 /// the first item not smaller than x, end() if none
 [[nodiscard]] const_iterator lowerBound( std::uint32_t x ) const;

 /// the item of rank i (from 0), end() if i >= size()
 [[nodiscard]] const_iterator select( std::size_t i ) const;

 };  // end( class RoaringBitmap )

RoaringBitmap setUnion( const RoaringBitmap & A , const RoaringBitmap & B );

RoaringBitmap setIntersection( const RoaringBitmap & A ,
                               const RoaringBitmap & B );

RoaringBitmap setDifference( const RoaringBitmap & A ,
                             const RoaringBitmap & B );

RoaringBitmap setSymmetricDifference( const RoaringBitmap & A ,
                                      const RoaringBitmap & B );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/set/impl/RoaringBitmap.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* RoaringBitmap.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File RoaringBitmap.h ------------------------*/
/*--------------------------------------------------------------------------*/
